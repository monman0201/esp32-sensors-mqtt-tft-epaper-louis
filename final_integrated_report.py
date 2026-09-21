# -*- coding: utf-8 -*-
"""Create the final report from the untouched 0810 document plus integrated 0914 content."""
from pathlib import Path
from docx import Document
from docx.shared import Inches, Pt, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_CELL_VERTICAL_ALIGNMENT
from docx.oxml import OxmlElement
from docx.oxml.ns import qn

ROOT = Path(r"D:\monman\esp32\ESP32實習")
SOURCE = ROOT / "0810物聯網成果報告.docx"
OUTPUT = ROOT / "0914物聯網成果報告.docx"
DIAGRAMS = ROOT / "_report_diagrams"
PHOTOS = ROOT / "成果照片"
FONT = "Microsoft JhengHei"
NAVY, TEAL, GRAY = "17324D", "168AAD", "5B6770"
TARGET = None


def fmt(run, size=11, color="222222", bold=False):
    run.font.name = FONT
    rpr = run._element.get_or_add_rPr()
    for key in ("eastAsia", "ascii", "hAnsi"):
        rpr.rFonts.set(qn("w:" + key), FONT)
    run.font.size = Pt(size)
    run.font.color.rgb = RGBColor.from_string(color)
    run.bold = bold


def make_para(text="", size=11, color="222222", bold=False, align=None):
    d = Document()
    p = d.add_paragraph()
    p.paragraph_format.space_after = Pt(6)
    p.paragraph_format.line_spacing = 1.2
    if align is not None:
        p.alignment = align
    fmt(p.add_run(text), size, color, bold)
    return p


def make_heading(text, level=2):
    d = Document()
    p = d.add_paragraph(style=f"Heading {level}")
    fmt(p.add_run(text), 12.5 if level == 2 else 11.5, NAVY, True)
    return p


def make_bullet(text):
    d = Document()
    p = d.add_paragraph(style="List Bullet")
    p.paragraph_format.space_after = Pt(3)
    fmt(p.add_run(text), 10.5)
    return p


def make_picture(path, caption, width=6.2):
    TARGET.add_picture(str(path), width=Inches(width))
    pic = TARGET.paragraphs[-1]
    cap = TARGET.add_paragraph()
    cap.alignment = WD_ALIGN_PARAGRAPH.CENTER
    fmt(cap.add_run(caption), 9, GRAY)
    return [pic._p, cap._p]


def make_table(rows, widths):
    d = Document()
    t = d.add_table(rows=len(rows), cols=len(widths))
    t.alignment = WD_TABLE_ALIGNMENT.CENTER
    t.autofit = False
    for i, row in enumerate(rows):
        for j, value in enumerate(row):
            cell = t.cell(i, j)
            cell.width = Inches(widths[j])
            cell.vertical_alignment = WD_CELL_VERTICAL_ALIGNMENT.CENTER
            if i == 0:
                shd = OxmlElement("w:shd")
                shd.set(qn("w:fill"), NAVY)
                cell._tc.get_or_add_tcPr().append(shd)
            p = cell.paragraphs[0]
            p.paragraph_format.space_after = Pt(2)
            fmt(p.add_run(str(value)), 9.2, "FFFFFF" if i == 0 else "222222", i == 0)
    return [t._tbl]


def find_para(doc, text):
    for p in doc.paragraphs:
        if text in p.text:
            return p
    raise ValueError(text)


def insert_after(anchor, elements):
    current = anchor._p
    for element in elements:
        if hasattr(element, "_p"):
            element = element._p
        current.addnext(element)
        current = element


def insert_before(anchor, elements):
    current = anchor._p
    for element in reversed(elements):
        if hasattr(element, "_p"):
            element = element._p
        current.addprevious(element)
        current = element


doc = Document(str(SOURCE))
TARGET = doc

# Add today's integration date without altering any original paragraph.
cover_anchor = find_para(doc, "本專案以 ESP32 為核心")
insert_after(cover_anchor, [make_para("本次整合報告日期：2026 年 9 月 14 日", 11, GRAY, True, WD_ALIGN_PARAGRAPH.CENTER)])

# The original architecture graphic remains in place. Add the integrated view
# immediately after it, so both diagrams are visible in the same section.
arch_caption = find_para(doc, "圖 1　系統架構圖")
insert_after(arch_caption, [
    make_para("整合後的學習架構將感測、現場顯示、雲端資料、通知與控制視為同一套 ESP32 物聯網系統：DHT11 與光敏電阻輸入 ESP32，資料可由 OLED／TFT 呈現，並透過 ThingSpeak、Google Sheets、LINE 與 MQTT 完成不同形式的資料應用。"),
    *make_picture(DIAGRAMS / "architecture_0914_imagegen.png", "圖 9　整合後 ESP32 物聯網系統架構圖（imagegen 製作）"),
    *make_table([
        ["系統層", "整合內容"],
        ["感測層", "DHT11 溫度／濕度與光敏電阻亮度"],
        ["顯示層", "OLED 即時資訊、ILI9225 TFT 四頁儀表板與趨勢圖"],
        ["資料層", "ThingSpeak、Google Sheets 與 MQTT JSON 資料"],
        ["應用層", "LINE 異常通知、MQTT 三色 LED 控制與 GPIO0 換頁"],
    ], [2.0, 4.7]),
])

# Make the cloud-learning path explicit without removing the original text.
gs_anchor = find_para(doc, "5.2 Google Sheets 上傳")
insert_before(gs_anchor, [
    make_heading("ThingSpeak 雲端資料上傳學習成果", 2),
    make_para("系統先以 ThingSpeak 建立雲端資料通道，ESP32 透過 Wi-Fi 使用 HTTP GET 上傳溫度、濕度與亮度，讓感測數值能以歷史圖表方式在瀏覽器查看。這個資料流程與現場 OLED 顯示互相配合，形成即時觀察與歷史分析的完整學習內容。"),
    *make_table([
        ["ThingSpeak 欄位", "資料內容", "學習用途"],
        ["field1", "溫度（°C）", "建立溫度歷史曲線"],
        ["field2", "濕度（%）", "建立濕度歷史曲線"],
        ["field3", "亮度（%）", "建立亮度歷史曲線"],
    ], [1.6, 2.0, 3.1]),
    make_para("ThingSpeak 測試上傳週期為 15 秒，曾取得 HTTP code 200 與 entry id 7；報告中的 ThingSpeak／雲端圖表照片依此學習進度呈現。接著同一組感測資料再串接 Google Sheets、LINE 與 MQTT，分別完成資料整理、異常通知與即時雙向控制。"),
])

# Add the integrated program flow after the original flowchart.
flow_caption = find_para(doc, "圖 2　系統運作與異常通知流程圖")
insert_after(flow_caption, [
    make_para("整合後的程式流程由感測讀取開始，經過 OLED／TFT 顯示與雲端傳輸，再依需求執行 LINE 通知或 MQTT 控制。0914 版本加入 Wi-Fi、NTP、MQTT、四頁 TFT、歷史資料陣列與 GPIO0 換頁，但各項功能仍在同一個感測資料循環中運作。"),
    *make_picture(DIAGRAMS / "flow_0914_imagegen.png", "圖 10　整合後 ESP32 程式運作流程圖（imagegen 製作）"),
    make_bullet("感測資料：DHT11 讀取溫度／濕度，GPIO33 讀取亮度；每 10 秒發布 MQTT JSON，並記錄最多 60 筆資料。"),
    make_bullet("顯示與分析：OLED 呈現即時數值，ILI9225 TFT 以首頁、溫度、濕度、亮度四頁呈現 Gauge 與 10 分鐘趨勢圖。"),
    make_bullet("控制與通知：LINE 處理異常提示，MQTT 三個主題分別控制綠、黃、紅 LED；GPIO0 按鍵循環切換頁面。"),
])

# Present photos in chronological learning order, while all original photos
# and their original positions remain untouched in the source document.
results_anchor = find_para(doc, "六、測試與成果")
insert_after(results_anchor, [
    make_heading("學習進度成果照片", 2),
    make_para("以下依專案資料夾中的拍攝日期排列，呈現由感測與雲端資料，到 MQTT 與 TFT 控制整合的實作進度。"),
])

# Place the dated progress photos after the original 0810 photo sequence.
last_original_photo = find_para(doc, "圖 8　雲端資料圖表呈現畫面")
insert_after(last_original_photo, [
    *make_picture(PHOTOS / "20260831_153358.jpg", "圖 14　2026/08/31：感測器、ESP32 與顯示器實體整合測試", 5.7),
    *make_picture(PHOTOS / "20260831_154454.jpg", "圖 15　2026/08/31：環境監測裝置接線與顯示測試", 5.7),
    *make_picture(PHOTOS / "20260831_154459.jpg", "圖 16　2026/08/31：感測資料與顯示介面測試", 5.7),
    *make_picture(PHOTOS / "20260914_115835.jpg", "圖 17　2026/09/14：MQTTGO Broker 接收 louis/class305/data 感測資料", 5.7),
    *make_picture(PHOTOS / "20260914_134740.jpg", "圖 18　2026/09/14：ESP32 環境監測裝置與 TFT 顯示運作", 5.7),
    *make_picture(PHOTOS / "20260914_135704.jpg", "圖 19　2026/09/14：ILI9225 TFT、感測器與三色 LED 整合成果", 5.7),
])

doc.core_properties.title = "0914物聯網成果報告"
doc.core_properties.subject = "ESP32 物聯網完整學習成果整合報告"
doc.core_properties.comments = "完整保留 0810 報告內容，並以 UTF-8 整合 0914 學習進度。"
doc.save(str(OUTPUT))
print(OUTPUT)
