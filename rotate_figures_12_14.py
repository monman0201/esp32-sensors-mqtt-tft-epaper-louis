# -*- coding: utf-8 -*-
from io import BytesIO
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
from PIL import Image
from docx import Document

ROOT = Path(r"D:\monman\esp32\ESP32實習")
SOURCE = ROOT / "0914物聯網成果報告.docx"
INTERMEDIATE = ROOT / "0914物聯網成果報告_rotated_shapes.docx"
OUTPUT = ROOT / "0914物聯網成果報告_旋轉完成.docx"

# Inline shapes 10, 11 and 12 are figures 12, 13 and 14 in the report.
doc = Document(str(SOURCE))
for index in (9, 10, 11):
    shape = doc.inline_shapes[index]
    width, height = shape.width, shape.height
    shape.width = height
    shape.height = width
doc.save(str(INTERMEDIATE))

targets = {"word/media/image10.jpg", "word/media/image11.jpg", "word/media/image12.jpg"}
with ZipFile(str(INTERMEDIATE), "r") as zin, ZipFile(str(OUTPUT), "w", ZIP_DEFLATED) as zout:
    for item in zin.infolist():
        data = zin.read(item.filename)
        if item.filename in targets:
            image = Image.open(BytesIO(data)).convert("RGB")
            rotated = image.rotate(-90, expand=True)
            buf = BytesIO()
            rotated.save(buf, format="JPEG", quality=95, optimize=True)
            data = buf.getvalue()
        zout.writestr(item, data)
print(OUTPUT)
