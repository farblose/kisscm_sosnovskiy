#include <vector>
#include <cstdint>
#include <string>
#include <ostream>

#ifndef PACKEDOBJECT_HPP
#define PACKETOBJECT_HPP

enum class GitObjectType {
    COMMIT = 1,
    TREE = 2,
    BLOB = 3,
    TAG = 4,
    OFS_DELTA = 6,
    REF_DELTA = 7
};

inline std::ostream& operator<<(std::ostream& os, GitObjectType type) {
    switch (type) {
        case GitObjectType::COMMIT: os << "COMMIT"; break;
        case GitObjectType::TREE: os << "TREE"; break;
        case GitObjectType::BLOB: os << "BLOB"; break;
        case GitObjectType::TAG: os << "TAG"; break;
        case GitObjectType::OFS_DELTA: os << "OFS_DELTA"; break;
        case GitObjectType::REF_DELTA: os << "REF_DELTA"; break;
        default: os << "UNKNOWN"; break;
    }
    return os;
}

class PackedObject {
public:
    GitObjectType type;
    size_t size;
    std::vector<uint8_t> data;
    uint64_t baseOffset;  // Для OFS_DELTA
    std::string baseHash; // Для REF_DELTA
};

#endif
