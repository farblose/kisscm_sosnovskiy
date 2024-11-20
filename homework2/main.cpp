#include <iostream>
#include <filesystem>
#include "GitIdxParser.hpp"
#include "inicpp.hpp"

int main()
{
    std::string IdxFilePath, PackFilePath;

    if (!std::filesystem::exists("config.ini"))
    {
        std::cerr << "Отсутствует конфигурационный файл!\n";
        return -1;
    }

    inicpp::IniManager ini("config.ini");

    if (!ini["options"].isKeyExist("plantuml_jar_path") || !ini["options"].isKeyExist("repo_path") || !ini["options"].isKeyExist("output_path") || !ini["options"].isKeyExist("date"))
    {
        std::cerr << "Ошибка в конфигурационном файле!\n";
        return -1;
    }

    for (const auto & entry : std::filesystem::directory_iterator(ini["options"]["repo_path"] + ".git/objects/pack"))
    {
        if (entry.path().extension() == ".idx")
            IdxFilePath = std::filesystem::absolute(entry.path());

        else if (entry.path().extension() == ".pack")
            PackFilePath = std::filesystem::absolute(entry.path());
    }

    try {
        GitIdxParser parser;
        if (parser.parseFile(IdxFilePath)) {
            parser.extractCommitsToPuml(PackFilePath, ini["options"].toInt("date"), ini["options"]["output_path"]);
            std::string outputFile = parser.convertPumlToPng(ini["options"]["plantuml_jar_path"]);
            std::cout << "PNG файл успешно создан: " << outputFile << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
