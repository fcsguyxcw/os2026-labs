"""将 report/ai_logs/lab2/evidence 下的真实终端输出片段渲染为 PNG 截图。"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

REPORT = Path(__file__).resolve().parents[2]
EVIDENCE = REPORT / 'ai_logs/lab2/evidence'
IMAGES = REPORT / 'images'


def load(name):
    return [line.rstrip('\r') for line in (EVIDENCE / name).read_text(encoding='utf-8').splitlines()]


def wrap_terminal_lines(lines, max_cols=112):
    out = []
    for raw in lines:
        line = raw.expandtabs(8)
        if len(line) <= max_cols:
            out.append(line)
            continue
        first = True
        while len(line) > max_cols:
            out.append(line[:max_cols] if first else '    ' + line[:max_cols - 4])
            line = line[max_cols:] if first else line[max_cols - 4:]
            first = False
        if line:
            out.append(line if first else '    ' + line)
    return out


def selections():
    default_qemu = [line for line in load('final_qemu_default.txt') if line.strip()]
    grade = [line for line in load('final_grade.txt') if line.strip()]
    buddy_qemu = load('final_qemu_LAB2_PMM_BUDDY.txt')[21:51]
    slub_qemu = load('final_qemu_LAB2_SLUB_TEST.txt')[29:52]
    paged_gdb = [line for line in load('t7_paged_qemu_gdb.txt')[8:77] if line.strip()]
    bare_gdb = [line for line in load('t7_bare_qemu_gdb.txt')[8:63] if line.strip()]

    return [
        ('01_make_qemu_bestfit.png', 'final_qemu_default.txt (make qemu 默认 Best-Fit)', wrap_terminal_lines(default_qemu)),
        ('02_make_grade.png', 'final_grade.txt (make grade 自动评测)', wrap_terminal_lines(grade)),
        ('03_make_qemu_buddy.png', 'final_qemu_LAB2_PMM_BUDDY.txt (Buddy System 自检)', wrap_terminal_lines(buddy_qemu)),
        ('04_make_qemu_slub.png', 'final_qemu_LAB2_SLUB_TEST.txt (SLUB 自检)', wrap_terminal_lines(slub_qemu)),
        ('05_gdb_qemu_sv39.png', 't7_paged_qemu_gdb.txt (双 GDB 调试 Sv39 地址翻译与 TLB 装填)', wrap_terminal_lines(paged_gdb)),
        ('06_gdb_qemu_bare_tlb.png', 't7_bare_qemu_gdb.txt (Bare 模式访存与写 satp 后旧 TLB 命中)', wrap_terminal_lines(bare_gdb)),
    ]


def pick_font(candidates, size):
    for p in candidates:
        path = Path(p)
        if path.exists():
            return ImageFont.truetype(str(path), size)
    raise FileNotFoundError(f'No font found among {candidates}')


def draw_mixed_line(draw, x, y, text, mono_font, cjk_font, fill):
    cur_x = x
    i = 0
    n = len(text)
    while i < n:
        is_ascii = ord(text[i]) < 128
        j = i + 1
        while j < n and (ord(text[j]) < 128) == is_ascii:
            j += 1
        seg = text[i:j]
        font = mono_font if is_ascii else cjk_font
        draw.text((cur_x, y), seg, font=font, fill=fill)
        cur_x += font.getlength(seg)
        i = j


def measure_mixed_line(text, mono_font, cjk_font):
    total = 0.0
    i = 0
    n = len(text)
    while i < n:
        is_ascii = ord(text[i]) < 128
        j = i + 1
        while j < n and (ord(text[j]) < 128) == is_ascii:
            j += 1
        seg = text[i:j]
        font = mono_font if is_ascii else cjk_font
        total += font.getlength(seg)
        i = j
    return total


def render():
    IMAGES.mkdir(parents=True, exist_ok=True)
    mono_font = pick_font([
        '/System/Library/Fonts/Menlo.ttc',
        '/System/Library/Fonts/Monaco.ttf',
        '/System/Library/Fonts/Courier.ttc',
        'C:/Windows/Fonts/consola.ttf',
    ], 16)
    cjk_font = pick_font([
        '/System/Library/Fonts/STHeiti Medium.ttc',
        '/System/Library/Fonts/STHeiti Light.ttc',
        'C:/Windows/Fonts/msyh.ttc',
    ], 16)

    line_height = 24
    for filename, label, visible in selections():
        title = f'终端输出：{label}'
        max_w = max(
            [measure_mixed_line(title, mono_font, cjk_font)]
            + [measure_mixed_line(line, mono_font, cjk_font) for line in visible]
        )
        width = max(960, int(max_w) + 56)
        height = 76 + len(visible) * line_height
        img = Image.new('RGB', (width, height), '#141820')
        draw = ImageDraw.Draw(img)
        draw_mixed_line(draw, 28, 18, title, mono_font, cjk_font, '#a1a8b5')
        draw.line((28, 50, width - 28, 50), fill='#363c48')
        for idx, line in enumerate(visible):
            color = '#7ee787' if ('succeeded!' in line or 'Total Score: 20/20' in line or '===== [' in line) else '#e3e7ef'
            draw_mixed_line(draw, 28, 62 + idx * line_height, line, mono_font, cjk_font, color)
        out_path = IMAGES / filename
        img.save(out_path, format='PNG')
        with Image.open(out_path) as check:
            check.verify()
        print(f'{filename}: {width}x{height} ({len(visible)} lines)')


if __name__ == '__main__':
    render()
