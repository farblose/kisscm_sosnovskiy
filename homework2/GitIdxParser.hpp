#include <arpa/inet.h>
#include <string>
#include <vector>

#ifndef GITIDXPARSER_HPP
#define GITIDXPARSER_HPP

class GitIdxParser {
    private:
        static const uint32_t IDX_V2_MAGIC = 0xFF744F63;

        struct IndexEntry {
            std::string sha1;
            uint32_t crc32;
            uint32_t offset;
        };

        std::vector<IndexEntry> entries;

        std::string bytesToHex(const unsigned char* bytes, size_t length);

        bool readExactly(std::ifstream& file, char* buffer, size_t size);

    public:
        bool parseFile(const std::string& filename);

        void printEntries(bool verbose = false) const;
};

#endif