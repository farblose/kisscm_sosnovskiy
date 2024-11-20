#include "GitIdxParser.hpp"
#include "GitPackParser.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>

std::string GitIdxParser::bytesToHex(const unsigned char* bytes, size_t length) {
    std::stringstream ss;
    for (size_t i = 0; i < length; i++) {
        ss << std::hex << std::setfill('0') << std::setw(2)
           << static_cast<int>(bytes[i]);
    }
    return ss.str();
}

bool GitIdxParser::readExactly(std::ifstream& file, char* buffer, size_t size) {
    file.read(buffer, size);
    return file.gcount() == static_cast<std::streamsize>(size);
}

bool GitIdxParser::parseFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Не удалось открыть файл: " << filename << std::endl;
        return false;
    }

    // Читаем и проверяем заголовок
    uint32_t header;
    if (!readExactly(file, reinterpret_cast<char*>(&header), sizeof(header))) {
        std::cerr << "Ошибка чтения заголовка" << std::endl;
        return false;
    }
    header = ntohl(header);

    if (header != IDX_V2_MAGIC) {
        std::cerr << "Неверный формат файла. Прочитано: 0x"
                 << std::hex << header << std::dec << std::endl;
        return false;
    }

    // Читаем и проверяем версию
    uint32_t version;
    if (!readExactly(file, reinterpret_cast<char*>(&version), sizeof(version))) {
        std::cerr << "Ошибка чтения версии" << std::endl;
        return false;
    }
    version = ntohl(version);

    if (version != 2) {
        std::cerr << "Неподдерживаемая версия: " << version << std::endl;
        return false;
    }

    // Читаем fanout таблицу
    uint32_t fanout[256];
    for (int i = 0; i < 256; i++) {
        if (!readExactly(file, reinterpret_cast<char*>(&fanout[i]), sizeof(uint32_t))) {
            std::cerr << "Ошибка чтения fanout таблицы" << std::endl;
            return false;
        }
        fanout[i] = ntohl(fanout[i]);
    }

    uint32_t numObjects = fanout[255];
    entries.reserve(numObjects);

    // Читаем SHA-1 хеши
    for (uint32_t i = 0; i < numObjects; i++) {
        IndexEntry entry;
        unsigned char sha1Bytes[20];

        if (!readExactly(file, reinterpret_cast<char*>(sha1Bytes), 20)) {
            std::cerr << "Ошибка чтения SHA-1 хеша для объекта " << i << std::endl;
            return false;
        }

        entry.sha1 = bytesToHex(sha1Bytes, 20);
        entries.push_back(entry);
    }

    // Читаем CRC32
    for (uint32_t i = 0; i < numObjects; i++) {
        uint32_t crc;
        if (!readExactly(file, reinterpret_cast<char*>(&crc), sizeof(crc))) {
            std::cerr << "Ошибка чтения CRC32 для объекта " << i << std::endl;
            return false;
        }
        entries[i].crc32 = ntohl(crc);
    }

    // Читаем смещения
    for (uint32_t i = 0; i < numObjects; i++) {
        uint32_t offset;
        if (!readExactly(file, reinterpret_cast<char*>(&offset), sizeof(offset))) {
            std::cerr << "Ошибка чтения смещения для объекта " << i << std::endl;
            return false;
        }
        entries[i].offset = ntohl(offset);
    }

    file.close();
    return true;
}

void GitIdxParser::printEntries(bool verbose) const {
    for (size_t i = 0; i < entries.size(); i++) {
        const auto& entry = entries[i];
        if (verbose) {
            std::cout << "Объект " << i + 1 << "/" << entries.size() << ":\n"
                     << "  SHA-1: " << entry.sha1 << "\n"
                     << "  Смещение: 0x" << std::hex << entry.offset << std::dec << "\n"
                     << "  CRC32: 0x" << std::hex << entry.crc32 << std::dec << "\n"
                     << "-------------------\n";
        } else {
            std::cout << entry.sha1 << " @ " << entry.offset << "\n";
        }
    }

    std::cout << "Всего объектов: " << entries.size() << std::endl;
}

void GitIdxParser::extractObjects(const std::string& packFilePath) {
    try {
        GitPackParser packParser(packFilePath);

        for (const auto& entry : entries) {
            try {
                auto [type, content] = packParser.getObjectContent(entry.offset);
                if (GitPackParser::objectTypeToString(type) == "commit") {
                    std::cout << "Обработка объекта " << entry.sha1 << std::endl;
                    std::cout << "  Тип: " << GitPackParser::objectTypeToString(type)
                             << "\n  Размер: " << content.size() << " байт\n";

                // Для текстовых объектов (commit, tag) выводим содержимое
                if (type == GitObjectType::COMMIT || type == GitObjectType::TAG) {
                    std::string textContent(content.begin(), content.end());
                    std::cout << "  Содержимое:\n" << textContent << std::endl;
                    std::cout << textContent.substr(textContent.find_last_of('>')+2, 10) << "\n";
                }

                std::cout << "-------------------\n";
                }
            } catch (const std::exception& e) {
                std::cerr << "Ошибка при обработке объекта: " << e.what() << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Ошибка при работе с pack файлом: " << e.what() << std::endl;
    }
}
