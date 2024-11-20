#include <iostream>
#include <filesystem>
#include "GitIdxParser.hpp"

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Использование: " << argv[0]
                  << " <путь к репозиторию>\n";
        return 1;
    }

    std::string IdxFilePath;

    for (const auto & entry : std::filesystem::directory_iterator(std::string(argv[1]) + "/.git/objects/pack"))
    {
        if (entry.path().extension() == ".idx")
            IdxFilePath = std::filesystem::absolute(entry.path());
    }

    try {
        GitIdxParser parser;
        if (parser.parseFile(IdxFilePath)) {
            parser.printEntries(true);
        }
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
