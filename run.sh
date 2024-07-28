#!/bin/bash

# Создаем папку для штрих-кодов, если она еще не существует
mkdir -p barcodes

# Вызываем Python скрипт для генерации штрих-кодов
cd QR-Noize
./build.sh && ./run.sh generate.config test test_mod noize.config
mkdir -p ../barcodes
cp -r test_mod/* ../barcodes/
cd ..

# Переходим к директории C++ проекта (измените на ваш путь)
cd .

mkdir build
cd build

# Собираем C++ проект
cmake ..
make

# Переходим обратно в директорию со скриптом
cd -

# Путь к исполняемому файлу C++ программы (измените на ваш путь)
executable="./build/Tutorial_Step6"

# Путь к папке с фонами
backgrounds_path="photos/"

# Цикл по всем файлам штрих-кодов в папке barcodes
for barcode in barcodes/*.jpg; do
    # Предполагаем, что фон для каждого штрих-кода один и тот же, замените на правильный путь
    for background in "$backgrounds_path"*.jpg; do

        combined_image="combined_${barcode##*/}"
        python3 save_combined_image.py "$barcode" "$combined_image" > coords.txt

        # Extract the coordinates from the file
        read -r TOP_LEFT_X TOP_LEFT_Y BOTTOM_RIGHT_X BOTTOM_RIGHT_Y < coords.txt
        rm coords.txt
        # Вызываем C++ программу с аргументами штрих-кода и фона
        $executable $combined_image $background $TOP_LEFT_X $TOP_LEFT_Y $BOTTOM_RIGHT_X $BOTTOM_RIGHT_Y

    done

    # Здесь можно добавить логику сохранения или обработки результатов
done

# Удаляем папку с штрих-кодами
rm -rf barcodes
rm -rf build

echo "Обработка завершена."
