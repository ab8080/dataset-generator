import sys
from PIL import Image

def add_barcode_to_white_image(barcode_path, output_path):
    # Открываем изображение штрих-кода
    barcode = Image.open(barcode_path)

    # Уменьшаем изображение штрих-кода в 3 раза
    new_size = (barcode.width, barcode.height)
    barcode = barcode.resize(new_size, Image.Resampling.LANCZOS)

    # Создаем белое изображение размером 256x256
    width = 484
    height = 884
    white_image = Image.new('RGB', (width, height), (255, 255, 255))

    # Вычисляем позиции для вставки штрих-кода
    positions = [
        ((white_image.width - barcode.width) // 2 + 20, 40),  #changable
    ]

    # Вставляем штрих-код в каждую позицию и выводим координаты рамки
    for pos in positions:
        white_image.paste(barcode, pos)
        print(pos[0] / width, pos[1] / height, (pos[0] + barcode.width) / width, (pos[1] + barcode.height) / height)

    # Сохраняем полученное изображение
    white_image.save(output_path)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python save_combined_image.py <barcode_path> <output_path>")
        sys.exit(1)

    barcode_path = sys.argv[1]
    output_path = sys.argv[2]

    add_barcode_to_white_image(barcode_path, output_path)
