from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from docx import Document
from docx.shared import Inches, Pt, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.section import WD_SECTION
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_CELL_VERTICAL_ALIGNMENT
from docx.oxml import OxmlElement
from docx.oxml.ns import qn

ROOT = Path(r'D:\monman\esp32\ESP32實習')
OUT = ROOT / '0810物聯網成果報告_新版.docx'
ASSET = ROOT / '成果照片'
DIAGRAM = ROOT / '_report_diagrams'
DIAGRAM.mkdir(exist_ok=True)

NAVY = '17324D'; TEAL = '168AAD'; LIGHT = 'EAF4F4'; GOLD = 'F4A261'; GRAY = '5B6770'; BORDER = 'CBD5E1'
FONT = 'Microsoft JhengHei'

def font(run, size=11, color='222222', bold=False, italic=False):
    run.font.name = FONT; run._element.rPr.rFonts.set(qn('w:eastAsia'), FONT); run._element.rPr.rFonts.set(qn('w:ascii'), FONT); run._element.rPr.rFonts.set(qn('w:hAnsi'), FONT)
    run.font.size = Pt(size); run.font.color.rgb = RGBColor.from_string(color); run.bold = bold; run.italic = italic

def shade(cell, fill):
    tcPr = cell._tc.get_or_add_tcPr(); shd = tcPr.find(qn('w:shd'))
    if shd is None: shd = OxmlElement('w:shd'); tcPr.append(shd)
    shd.set(qn('w:fill'), fill)

def cell_margins(cell, top=100, start=120, bottom=100, end=120):
    tc = cell._tc; tcPr = tc.get_or_add_tcPr(); mar = tcPr.first_child_found_in('w:tcMar')
    if mar is None: mar = OxmlElement('w:tcMar'); tcPr.append(mar)
    for side, val in [('top',top),('start',start),('bottom',bottom),('end',end)]:
        node = mar.find(qn('w:'+side))
        if node is None: node = OxmlElement('w:'+side); mar.append(node)
        node.set(qn('w:w'), str(val)); node.set(qn('w:type'), 'dxa')

def set_width(cell, width):
    tcPr = cell._tc.get_or_add_tcPr(); tcW = tcPr.find(qn('w:tcW'))
    if tcW is None: tcW = OxmlElement('w:tcW'); tcPr.append(tcW)
    tcW.set(qn('w:w'), str(width)); tcW.set(qn('w:type'), 'dxa')

def table(doc, rows, widths, header=True):
    t = doc.add_table(rows=len(rows), cols=len(widths)); t.alignment = WD_TABLE_ALIGNMENT.CENTER; t.autofit = False
    for i,row in enumerate(rows):
        for j,val in enumerate(row):
            c=t.cell(i,j); set_width(c,widths[j]); cell_margins(c); c.vertical_alignment=WD_CELL_VERTICAL_ALIGNMENT.CENTER
            if i==0 and header: shade(c,NAVY)
            p=c.paragraphs[0]; p.paragraph_format.space_after=Pt(2); p.paragraph_format.space_before=Pt(2)
            r=p.add_run(str(val)); font(r, 9.5 if i else 9.5, 'FFFFFF' if i==0 and header else '222222', bold=(i==0 and header))
            if i>0 and j==0: font(r, 9.5, TEAL, bold=True)
    return t

def heading(doc, text, level=1):
    p=doc.add_paragraph(style=f'Heading {level}'); r=p.add_run(text); font(r, 16 if level==1 else 12.5, TEAL if level==1 else NAVY, bold=True); return p

def para(doc, text='', size=11, color='222222', bold=False, align=None, after=6):
    p=doc.add_paragraph(); p.paragraph_format.space_after=Pt(after); p.paragraph_format.line_spacing=1.25
    if align is not None: p.alignment=align
    r=p.add_run(text); font(r,size,color,bold); return p

def bullet(doc, text):
    p=doc.add_paragraph(style='List Bullet'); p.paragraph_format.space_after=Pt(3); p.paragraph_format.line_spacing=1.15; r=p.add_run(text); font(r,10.5); return p

def add_photo(doc, filename, caption, width=5.9, page_break=False):
    p = ASSET / filename
    if not p.exists():
        return
    if page_break:
        doc.add_page_break()
    doc.add_picture(str(p), width=Inches(width))
    para(doc, caption, 9, GRAY, False, WD_ALIGN_PARAGRAPH.CENTER, 10)

def make_diagram(path, mode):
    w,h=1400,650; im=Image.new('RGB',(w,h),'white'); d=ImageDraw.Draw(im)
    fp = r'C:\Windows\Fonts\msjh.ttc'
    try: f=ImageFont.truetype(fp,28); fs=ImageFont.truetype(fp,22); ft=ImageFont.truetype(fp,34)
    except: f=fs=ft=ImageFont.load_default()
    def box(x,y,bw,bh,title,detail,fill):
        d.rounded_rectangle((x,y,x+bw,y+bh),radius=20,fill=fill,outline='#17324D',width=3)
        d.text((x+18,y+16),title,font=f,fill='#17324D'); d.text((x+18,y+62),detail,font=fs,fill='#334155')
    def arrow(x1,y1,x2,y2):
        d.line((x1,y1,x2,y2),fill='#168AAD',width=6); d.polygon([(x2,y2),(x2-18,y2-10),(x2-18,y2+10)],fill='#168AAD')
    if mode=='arch':
        d.text((40,25),'ESP32 物聯網環境監測系統架構',font=ft,fill='#17324D')
        box(45,190,270,150,'感測層','DHT11 溫溼度\n光敏電阻亮度','#DFF3F3')
        box(390,160,300,210,'控制與顯示層','ESP32\n資料判斷與 Wi‑Fi\nOLED 即時顯示','#E8F1FA')
        box(785,80,260,140,'雲端資料層','Google Sheets\n每 10 秒上傳','#FFF1D6')
        box(785,290,260,140,'通知層','LINE Messaging API\n異常每 30 秒通知','#FDE2E4')
        box(1120,180,230,150,'使用者','手機 LINE\n瀏覽資料表','#E9DDFB')
        arrow(315,265,390,265); arrow(690,230,785,150); arrow(690,300,785,360); arrow(1045,150,1120,230); arrow(1045,360,1120,275)
    else:
        d.text((40,25),'系統運作與異常通知流程',font=ft,fill='#17324D')
        box(45,180,230,120,'開始／連線','Wi‑Fi、OLED 初始化','#E8F1FA')
        box(350,180,230,120,'讀取感測值','溫度、濕度、亮度','#DFF3F3')
        box(655,180,230,120,'顯示與上傳','OLED 顯示\nGoogle Sheets 每 10 秒','#FFF1D6')
        box(960,100,250,120,'判斷異常','溫度 > 28°C\n或濕度 > 70%','#FDE2E4')
        box(960,300,250,120,'LINE 通知','顯示 TX，發送成功顯示 OK\n間隔 30 秒','#FDE2E4')
        arrow(275,240,350,240); arrow(580,240,655,240); arrow(885,220,960,160); arrow(1085,220,1085,300); arrow(960,360,885,270); arrow(1208,160,1310,160); d.text((1225,130),'正常：持續監測',font=fs,fill='#168AAD')
    im.save(path)

make_diagram(DIAGRAM/'architecture.png','arch'); make_diagram(DIAGRAM/'flow.png','flow')

doc=Document(); sec=doc.sections[0]; sec.top_margin=Inches(.75); sec.bottom_margin=Inches(.7); sec.left_margin=Inches(.85); sec.right_margin=Inches(.85)
styles=doc.styles; normal=styles['Normal']; normal.font.name=FONT; normal._element.rPr.rFonts.set(qn('w:eastAsia'),FONT); normal.font.size=Pt(11); normal.paragraph_format.space_after=Pt(6); normal.paragraph_format.line_spacing=1.25
for sname,size,col in [('Heading 1',16,TEAL),('Heading 2',12.5,NAVY),('Heading 3',11.5,GRAY)]:
    s=styles[sname]; s.font.name=FONT; s._element.rPr.rFonts.set(qn('w:eastAsia'),FONT); s.font.size=Pt(size); s.font.bold=True; s.font.color.rgb=RGBColor.from_string(col); s.paragraph_format.space_before=Pt(12); s.paragraph_format.space_after=Pt(6)
header=sec.header.paragraphs[0]; header.alignment=WD_ALIGN_PARAGRAPH.RIGHT; font(header.add_run('ESP32 物聯網環境監測專題｜結案報告'),9,GRAY)
footer=sec.footer.paragraphs[0]; footer.alignment=WD_ALIGN_PARAGRAPH.CENTER; font(footer.add_run('0810物聯網成果報告'),9,GRAY)

# cover
para(doc,'物聯網環境監測與異常通知系統',29,NAVY,True,WD_ALIGN_PARAGRAPH.CENTER,10)
para(doc,'ESP32 × DHT11 × 光敏電阻 × OLED × Google Sheets × LINE',14,TEAL,False,WD_ALIGN_PARAGRAPH.CENTER,35)
para(doc,'結案報告',22,GOLD,True,WD_ALIGN_PARAGRAPH.CENTER,70)
para(doc,'專案檔案：25_dht_line\n完成日期：2026 年 8 月 10 日\n使用語言：Arduino C++／繁體中文',11,GRAY,False,WD_ALIGN_PARAGRAPH.CENTER,16)
para(doc,'本專案以 ESP32 為核心，整合環境感測、OLED 即時顯示、雲端資料紀錄與 LINE 異常通知，完成一套可持續運作的物聯網監測原型。',12,'333333',False,WD_ALIGN_PARAGRAPH.CENTER,0)
doc.add_page_break()

heading(doc,'摘要',1)
para(doc,'本專案完成一套以 ESP32 為核心的環境監測系統。系統使用 DHT11 讀取溫度與濕度，使用光敏電阻讀取環境亮度，並透過 I²C OLED 顯示「溫度、濕度、亮度」數值。資料可經 Wi‑Fi 上傳至 Google Sheets；當溫度超過 28°C 或濕度超過 70% 時，系統會透過 LINE Messaging API 推送異常訊息，且每 30 秒最多通知一次，以避免短時間重複洗版。')
table(doc,[['項目','成果摘要'],['核心控制器','ESP32；負責感測、顯示、網路與判斷'],['感測設備','DHT11 溫溼度感測器、光敏電阻'],['顯示介面','128×64 OLED；保留中文欄位名稱與數值'],['雲端紀錄','Google Sheets；溫度、濕度、亮度每 10 秒傳送一筆'],['異常通知','LINE；溫度 > 28°C 或濕度 > 70%，30 秒通知一次']], [2600,6760])
heading(doc,'一、研究動機與目的',1)
para(doc,'傳統環境監測常需要人工查看儀表，無法即時掌握異常狀態。本專案以低成本、易擴充的 ESP32 為基礎，將感測資料轉化為「現場可看、雲端可查、異常可通知」的完整流程，作為物聯網實作與後續智慧環境應用的基礎。')
bullet(doc,'建立溫度、濕度與亮度的即時量測功能。'); bullet(doc,'以 OLED 保持現場資訊可讀性，並顯示網路／傳送狀態。'); bullet(doc,'將資料上傳 Google Sheets，形成可追蹤的歷史紀錄。'); bullet(doc,'建立 LINE 異常通知，降低人工巡檢負擔。')
doc.add_page_break()

heading(doc,'二、系統架構',1); doc.add_picture(str(DIAGRAM/'architecture_ai.png'),width=Inches(6.55)); para(doc,'圖 1　系統架構圖（影像工具製作）',9,GRAY,False,WD_ALIGN_PARAGRAPH.CENTER,8)
heading(doc,'三、硬體架構與接線',1)
table(doc,[['元件','ESP32 腳位／介面','功能'],['DHT11','GPIO 14','量測溫度與濕度'],['光敏電阻','GPIO 33（ADC）','量測類比亮度並換算為百分比'],['OLED','SDA GPIO 21、SCL GPIO 22','顯示感測值與傳送狀態'],['Wi‑Fi','ESP32 內建無線網路','連線 Google Sheets 與 LINE API']], [2200,3000,4160])
para(doc,'系統採用分層設計：感測器負責取得資料，ESP32 負責資料處理與網路通訊，OLED 提供現場回饋，Google Sheets 與 LINE 分別承擔歷史紀錄與即時通知。')
add_photo(doc, '20260810_091121.jpg', '圖 3　ESP32、OLED、DHT11 與光敏電阻實體接線成果', 5.2)
heading(doc,'四、軟體流程與資料傳遞',1); doc.add_picture(str(DIAGRAM/'flow_ai.png'),width=Inches(6.55)); para(doc,'圖 2　系統運作與異常通知流程圖（影像工具製作）',9,GRAY,False,WD_ALIGN_PARAGRAPH.CENTER,8)
heading(doc,'資料流程說明',2)
for s in ['開機後初始化 I²C OLED、DHT11、ADC 與 Wi‑Fi。','每秒讀取一次 DHT11 與光敏電阻，並更新 OLED。','Google Sheets 版本以 10 秒為週期上傳溫度、濕度、亮度。','最新 LINE 版本檢查溫度與濕度是否超過門檻；異常時 OLED 顯示 TX，傳送完成顯示 OK。','同一異常狀態以 30 秒為通知間隔，避免重複通知過於頻繁。'] : bullet(doc,s)
doc.add_page_break()

heading(doc,'五、功能實作',1)
heading(doc,'5.1 OLED 顯示設計',2)
para(doc,'OLED 以三個資訊區塊呈現資料：上方左右兩個區塊顯示溫度與濕度，下方寬區塊顯示亮度。中文欄位名稱保留呈現，單位分別使用 C、% 與 %；右側以 UP／OK 或 TX／OK 表示資料傳送狀態，方便確認系統是否正常運作。')
heading(doc,'5.2 Google Sheets 上傳',2)
para(doc,'Google Sheets 版本使用 HTTPS 連線至 Google Apps Script Web App，以 URL 參數傳遞 CSV 格式資料，欄位順序規劃為：時間戳記、溫度（°C）、濕度（%）、亮度（%）。每 10 秒傳送一筆，並在 OLED 顯示 UP（上傳中）及 OK（成功）。')
add_photo(doc, 'thingspeak上傳成果(305教室).png', '圖 4　雲端資料上傳與歷程呈現成果', 5.6)
add_photo(doc, '螢幕擷取畫面 2026-08-10 094915.png', '圖 5　感測資料與顯示成果畫面', 4.5)
heading(doc,'5.3 LINE 異常通知',2)
para(doc,'LINE 版本使用 WiFiClientSecure 連線至 LINE Messaging API，將異常狀態組成文字訊息推送至指定使用者。異常條件為溫度 > 28°C 或濕度 > 70%；通知間隔設定為 30 秒。通知內容包含異常提示、溫度與濕度數值，便於使用者判斷現場狀況。')
add_photo(doc, 'Screenshot_20260810_160929_LINE.jpg', '圖 6　LINE 異常通知實際接收畫面', 3.7)
heading(doc,'5.4 程式模組',2)
table(doc,[['模組','說明'],['connectWiFi()','建立 Wi‑Fi 連線並將連線狀態顯示於 OLED'],['drawDashboard()','統一繪製溫度、濕度、亮度與傳送狀態'],['uploadToGoogleSheets()','以 HTTPS GET 上傳 CSV 感測資料'],['sendLineMessage()','以 HTTPS POST 推送 LINE 文字訊息'],['urlEncode()','將雲端傳送參數進行網址編碼']], [2600,6760])
heading(doc,'六、測試與成果',1)
table(doc,[['測試項目','測試條件／觀察','結果'],['感測讀值','DHT11 與光敏電阻持續讀取','通過'],['OLED 顯示','中文欄位、數值、單位與狀態顯示','通過'],['Google Sheets','每 10 秒寫入溫度、濕度、亮度','通過／依網路狀態而定'],['LINE 通知','溫度 > 28°C 或濕度 > 70%','通過；每 30 秒一次'],['網路中斷處理','斷線時重新嘗試連線','已納入程式邏輯']], [2600,4700,2060])
para(doc,'測試結果顯示，本系統已完成從感測、顯示、雲端紀錄到異常通知的整合。成果照片包含實體裝置、ThingSpeak 歷程畫面、雲端資料畫面與 LINE 通知畫面，作為本專案的實作佐證。')

add_photo(doc, '螢幕擷取畫面 2026-08-10 113831.png', '圖 7　資料紀錄成果畫面', 4.4)
add_photo(doc, '螢幕擷取畫面 2026-08-10 113849.png', '圖 8　雲端資料圖表呈現畫面', 5.6)

doc.add_page_break(); heading(doc,'七、問題、改善與後續建議',1)
table(doc,[['觀察／限制','改善方向'],['DHT11 量測解析度與反應速度有限','後續可改用 DHT22、AHT20 等精度較高的感測器'],['OLED 中文字型需配合 U8g2 字型資源','可依顯示器尺寸進一步調整字型與排版'],['HTTPS 使用 setInsecure()','正式部署時應導入憑證驗證，提升通訊安全'],['Wi‑Fi 或雲端服務中斷會影響上傳','可加入離線暫存、重試與失敗佇列機制'],['LINE Token 與 Wi‑Fi 密碼寫在程式中','建議改用環境設定、加密儲存或避免公開程式碼']], [3600,5760])
heading(doc,'八、結論',1)
para(doc,'本專案以 ESP32 完成環境監測物聯網原型，整合 DHT11、光敏電阻、OLED、Google Sheets 與 LINE 通知。系統具備即時顯示、週期上傳與異常推播三項核心能力，達成「現場可視化、雲端可追蹤、異常可即時掌握」的專案目標。透過本次實作，也建立了感測器讀取、網路連線、HTTPS API、資料格式化與嵌入式介面設計的整合經驗，具備延伸至智慧教室、居家環境與設備監控的基礎。')
heading(doc,'附錄：專案檔案與資料欄位',1)
table(doc,[['項目','內容'],['主要程式','25_dht_line/25_dht_line.ino'],['Google Sheets 程式','24_google_dht_light_oled/24_google_dht_light_oled.ino'],['感測資料欄位','時間戳記、溫度、濕度、亮度'],['單位','溫度：°C；濕度：%；亮度：%'],['重要門檻','溫度 > 28°C 或濕度 > 70%；通知間隔 30 秒']], [2800,6560])

doc.core_properties.title='0810物聯網成果報告'; doc.core_properties.subject='ESP32 物聯網環境監測與異常通知系統'; doc.core_properties.author=''
doc.save(OUT)
print(OUT)
