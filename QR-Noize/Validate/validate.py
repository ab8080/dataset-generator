import zxingcpp
import cv2
import sys
import os
import json

JSON_FILENAME = "validation.json"

mod_img_path = sys.argv[1]

decoded_dict = {}

for img in os.listdir(mod_img_path):
    img_mat = cv2.imread(os.path.join(mod_img_path, img))
    if not (img.endswith('.jpg') or img.endswith('.jpeg')):
        print("Warning: ", img, " is not image!")
        continue
    result = zxingcpp.read_barcodes(img_mat)
    if len(result) > 0:
        decoded_dict[img] = result[0].text
    else:
        decoded_dict[img] = "unknown"

with open(os.path.join(mod_img_path, JSON_FILENAME), '+w') as f:
    json.dump(decoded_dict, f)
