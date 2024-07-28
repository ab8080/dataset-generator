import subprocess
import os
import uuid
import treepoem
from pylibdmtx.pylibdmtx import encode
from PIL import Image
import random
import string

output_dir = "barcodes"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

def generate_random_string(length):
    # Строка, содержащая все буквы и цифры
    characters = string.ascii_letters + string.digits
    # Выбираем случайные символы из строки characters
    random_string = ''.join(random.choice(characters) for i in range(length))
    return random_string
def generate_barcode_with_treepoem(data, barcode_type):
    img = treepoem.generate_barcode(
        barcode_type=barcode_type,  # Например, 'qrcode' или 'azteccode'
        data=data
    )
    filename = os.path.join(output_dir, f"{uuid.uuid4()}.jpg")  # Изменено здесь
    img.save(filename)
    return filename

def generate_datamatrix(data):
    encoded = encode(data.encode('utf-8'))
    img = Image.frombytes('RGB', (encoded.width, encoded.height), encoded.pixels)
    filename = os.path.join(output_dir, f"{uuid.uuid4()}.png")  # Изменено здесь
    img.save(filename)
    return filename

def generate_random_barcode(data):
    barcode_types = ['qrcode', 'azteccode', 'maxicode', 'datamatrix']
    selected_type = random.choice(barcode_types)
    # print(f"Selected barcode type: {selected_type}")
    if selected_type == 'datamatrix':
        return generate_datamatrix(data)
    else:
        return generate_barcode_with_treepoem(data, selected_type)

def main():
    # Генерация штрих-кода
    for i in range(10):
        barcode_file = generate_random_barcode(generate_random_string(10))


if __name__ == "__main__":
    main()

