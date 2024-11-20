#include <iostream>
#include <filesystem>

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

    return 0;
}
