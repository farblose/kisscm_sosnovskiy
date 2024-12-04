#include "GitIdxParser.hpp"
#include "GitPackParser.hpp"
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <filesystem>

int GitIdxParser::find_unix_timestamp(const std::string& data)
{
    std::string unixTimestamp;
    for (const char& sym : data)
    {
        if (48 <= sym && sym <= 57)
            unixTimestamp += sym;
        else
        {
            if (unixTimestamp.length() == 10)
                return std::stoi(unixTimestamp);
            else
                unixTimestamp = "";
        }
    }
    if (unixTimestamp != "")
    {
        return std::stoi(unixTimestamp);
    }
    return -1;
}

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

void GitIdxParser::extractCommitsToPuml(const std::string& packFilePath, const int& from, const std::string& outputDir) {
    try {
        GitPackParser packParser(packFilePath);
        pumlFile = outputDir + "commits.puml";
        std::ofstream output(outputDir + "commits.puml");
        output << "@startuml\ndigraph dependencies {\n";
        for (const auto& entry : entries) {
            try {
                auto [type, content] = packParser.getObjectContent(entry.offset);
                if (GitPackParser::objectTypeToString(type) == "commit") {
                    std::string textContent(content.begin(), content.end());
                    int time = find_unix_timestamp(textContent);
                    if (!time)
                    {
                        std::cerr << "no unix timestamp\n";
                    }
                    if (time >= from)
                    {
                        std::string parent = textContent.substr(textContent.find("parent") + 7, 40);
                        if (parent.back() == '\n')
                            parent.pop_back();
                        std::cout << "{\"hash\": \"" << entry.sha1 << "\", \"parent\": \"" << parent << "\"}\n";
                        //plantuml_code += f'  "{parent}" -> "{commit["hash"]}";\n'
                        output << "  \"" << entry.sha1 << "\";\n  \"" << parent << "\" -> \"" << entry.sha1 << "\";\n";
                    }
                }
            } catch (const std::exception& e) {

            }
        }
        output << "}\n@enduml";
        output.close();
    } catch (const std::exception& e) {
        std::cerr << "Ошибка при работе с pack файлом: " << e.what() << std::endl;
    }
}

std::string GitIdxParser::convertPumlToPng(const std::string& plantUmlJarPath)
{
    if (!std::filesystem::exists(pumlFile)) {
        throw std::runtime_error("Файл " + pumlFile + " не найден.");
    }

    if (!std::filesystem::exists(plantUmlJarPath)) {
        throw std::runtime_error("Файл " + plantUmlJarPath + " не найден.");
    }

    std::string command = "java -jar " + plantUmlJarPath + " -tpng " + pumlFile;
    int result = std::system(command.c_str());
    if (result != 0) {
        throw std::runtime_error("Ошибка при выполнении PlantUML: команда завершилась с кодом " + std::to_string(result));
    }

    std::string output_file = std::filesystem::path(pumlFile).parent_path().string() + "/" + std::filesystem::path(pumlFile).stem().string() + ".png";
    if (!std::filesystem::exists(output_file)) {
        throw std::runtime_error("Не удалось создать PNG файл.");
    }

    return output_file;
}
