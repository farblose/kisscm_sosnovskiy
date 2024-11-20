#include <fstream>
#include <zlib.h>
#include "PackedObject.hpp"

#ifndef GITPACKPARSER_HPP
#define GITPACKPARSER_HPP

class GitPackParser {
private:
    static const uint32_t PACK_SIGNATURE = 0x5041434B;  // "PACK"
    static const size_t CHUNK_SIZE = 4096;

    std::ifstream packFile;
    std::string packPath;

    bool readExactly(char* buffer, size_t size);

    // Чтение переменной длины числа
    uint64_t readVariableLengthNumber(int& shift);

    // Декодирование сжатых данных zlib
    std::vector<uint8_t> inflateData(z_stream& zs, size_t expectedSize);

    // Чтение объекта по смещению
    PackedObject readObjectAtOffset(uint32_t offset);

    std::vector<uint8_t> applyDelta(const std::vector<uint8_t>& baseData, const std::vector<uint8_t>& deltaData);

public:
    GitPackParser(const std::string& packFilePath);

    ~GitPackParser();

    std::pair<GitObjectType, std::vector<uint8_t>> getObjectContent(uint32_t offset);

    static std::string objectTypeToString(GitObjectType type);
};

#endif
