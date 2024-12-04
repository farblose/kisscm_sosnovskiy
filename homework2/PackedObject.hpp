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

class PackedObject {
public:
    GitObjectType type;
    size_t size;
    std::vector<uint8_t> data;
    uint64_t baseOffset;  // Для OFS_DELTA
    std::string baseHash; // Для REF_DELTA
};

#endif
