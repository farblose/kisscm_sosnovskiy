#include "GitPackParser.hpp"
#include <arpa/inet.h>

GitPackParser::GitPackParser(const std::string& packFilePath) : packPath(packFilePath) {
    packFile.open(packPath, std::ios::binary);
    if (!packFile.is_open()) {
        throw std::runtime_error("Не удалось открыть pack файл");
    }

    // Проверяем сигнатуру pack файла
    uint32_t signature;
    readExactly(reinterpret_cast<char*>(&signature), 4);
    signature = ntohl(signature);
    if (signature != PACK_SIGNATURE) {
        throw std::runtime_error("Неверная сигнатура pack файла");
    }

    // Проверяем версию (должна быть 2 или 3)
    uint32_t version;
    readExactly(reinterpret_cast<char*>(&version), 4);
    version = ntohl(version);
    if (version != 2 && version != 3) {
        throw std::runtime_error("Неподдерживаемая версия pack файла");
    }
}

GitPackParser::~GitPackParser() {
    if (packFile.is_open()) {
        packFile.close();
    }
}

std::pair<GitObjectType, std::vector<uint8_t>> GitPackParser::getObjectContent(uint32_t offset) {
    PackedObject obj = readObjectAtOffset(offset);

    if (obj.type == GitObjectType::OFS_DELTA) {
        // Рекурсивно получаем базовый объект
        auto [baseType, baseContent] = getObjectContent(obj.baseOffset);
        obj.type = baseType;
        obj.data = applyDelta(baseContent, obj.data);
    } else if (obj.type == GitObjectType::REF_DELTA) {
        // TODO: Реализовать поиск базового объекта по хешу
        throw std::runtime_error("REF_DELTA пока не поддерживается");
    }

    return {obj.type, obj.data};
}

std::string GitPackParser::objectTypeToString(GitObjectType type) {
    switch (type) {
        case GitObjectType::COMMIT: return "commit";
        case GitObjectType::TREE: return "tree";
        case GitObjectType::BLOB: return "blob";
        case GitObjectType::TAG: return "tag";
        case GitObjectType::OFS_DELTA: return "ofs-delta";
        case GitObjectType::REF_DELTA: return "ref-delta";
        default: return "unknown";
    }
}

PackedObject GitPackParser::readObjectAtOffset(uint32_t offset) {
    if (packFile.is_open()) {
        packFile.close();
    }
    packFile.open(packPath, std::ios::binary);

    packFile.seekg(offset, std::ios::beg);
    if (!packFile.good()) {
        throw std::runtime_error("Не удалось установить позицию в файле на смещение " + std::to_string(offset));
    }

    uint8_t byte;
    packFile.read(reinterpret_cast<char*>(&byte), 1);

    // Получаем тип объекта из первого байта
    GitObjectType type = static_cast<GitObjectType>((byte >> 4) & 0x7);

    // Получаем размер объекта
    uint64_t size = byte & 0x0F;
    int shift = 4;
    if (byte & 0x80) {
        size = readVariableLengthNumber(shift);
    }

    z_stream zs = {0};
    if (inflateInit(&zs) != Z_OK) {
        throw std::runtime_error("Ошибка инициализации zlib");
    }

    PackedObject obj;
    obj.type = type;
    obj.size = size;

    try {
        obj.data = inflateData(zs, size);
        inflateEnd(&zs);
    } catch (...) {
        inflateEnd(&zs);
        throw;
    }

    // Обработка дельта-объектов
    if (type == GitObjectType::OFS_DELTA) {
        uint64_t negativeOffset = readVariableLengthNumber(shift);
        obj.baseOffset = offset - negativeOffset;
    } else if (type == GitObjectType::REF_DELTA) {
        char baseHash[20];
        readExactly(baseHash, 20);
        obj.baseHash = std::string(baseHash, 20);
    }

    return obj;
}

std::vector<uint8_t> GitPackParser::inflateData(z_stream& zs, size_t expectedSize) {
    std::vector<uint8_t> output;
    std::vector<uint8_t> buffer(CHUNK_SIZE);
    output.reserve(expectedSize);

    do {
        char input[CHUNK_SIZE];
        packFile.read(input, CHUNK_SIZE);
        size_t bytesRead = packFile.gcount();

        if (bytesRead == 0) break;

        zs.avail_in = bytesRead;
        zs.next_in = reinterpret_cast<Bytef*>(input);

        do {
            zs.avail_out = buffer.size();
            zs.next_out = buffer.data();

            int ret = inflate(&zs, Z_NO_FLUSH);
            if (ret != Z_OK && ret != Z_STREAM_END) {
                throw std::runtime_error("Ошибка декомпрессии иди нахуй");
            }

            size_t inflatedSize = buffer.size() - zs.avail_out;
            output.insert(output.end(), buffer.begin(), buffer.begin() + inflatedSize);

        } while (zs.avail_out == 0);

    } while (packFile.good());

    return output;
}

uint64_t GitPackParser::readVariableLengthNumber(int& shift) {
    uint64_t result = 0;
    uint8_t byte;
    shift = 0;

    do {
        packFile.read(reinterpret_cast<char*>(&byte), 1);
        result |= (static_cast<uint64_t>(byte & 0x7F) << shift);
        shift += 7;
    } while (byte & 0x80);

    return result;
}

bool GitPackParser::readExactly(char* buffer, size_t size) {
    packFile.read(buffer, size);
    return packFile.gcount() == static_cast<std::streamsize>(size);
}

std::vector<uint8_t> GitPackParser::applyDelta(const std::vector<uint8_t>& baseData,
                               const std::vector<uint8_t>& deltaData) {
    std::vector<uint8_t> result;
    size_t pos = 0;

    // Пропускаем размеры из дельты
    while (pos < deltaData.size() && (deltaData[pos] & 0x80)) pos++;
    pos++;
    while (pos < deltaData.size() && (deltaData[pos] & 0x80)) pos++;
    pos++;

    while (pos < deltaData.size()) {
        uint8_t cmd = deltaData[pos++];
        if (cmd & 0x80) {  // Copy команда
            uint64_t offset = 0, size = 0;
            int shift = 0;
            for (int i = 0; i < 7; i++) {
                if (cmd & (1 << i)) {
                    offset |= static_cast<uint64_t>(deltaData[pos++]) << shift;
                    shift += 8;
                }
            }
            shift = 0;
            for (int i = 0; i < 4; i++) {
                if (cmd & (1 << (i + 3))) {
                    size |= static_cast<uint64_t>(deltaData[pos++]) << shift;
                    shift += 8;
                }
            }
            result.insert(result.end(),
                        baseData.begin() + offset,
                        baseData.begin() + offset + size);
        } else if (cmd) {  // Insert команда
            result.insert(result.end(),
                        deltaData.begin() + pos,
                        deltaData.begin() + pos + cmd);
            pos += cmd;
        }
    }
    return result;
}
