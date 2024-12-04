# Программа для построения графика зависимостей коммитов в PlantUML.
## Общее описание.
Программа написанна на языке программирования C++, не использует никаких сторонних инструментов.
Конфигурационный файл программы называется config.ini и должен лежать в одной директории с исполняемым файлом.
## Конфигурационный файл.
```
[options]
    plantuml_jar_path = путь к программе для визуализации графа
    repo_path = путь к обрабатываемому репозиторию
    output_path = путь к файлу-результату в виде png
    date = дата для фильтрации комитов (unixtimestamp)
```
## Сборка проекта
```bash
git clone https://github.com/farblose/kisscm_sosnovskiy.git && \
cd kisscm_sosnovskiy/homework2
```
Далее меняем файл config.ini
```
clang++ GitIdxParser.cpp GitPackParser.cpp main.cpp -lz -o graphviz && \
./graphviz
```
## Запуск тестов
```bash
clang++ GitIdxParser.cpp GitPackParser.cpp test.cpp -lz -o test && \
./test
```
