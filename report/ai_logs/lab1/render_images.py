"""在 Windows/Pillow 中将既有证据的原文片段渲染为 PNG，不运行实验。"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

REPORT = Path(__file__).resolve().parents[2]
EVIDENCE = REPORT / 'ai_logs/lab1/evidence'
IMAGES = REPORT / 'images'


def load(name):
    return (EVIDENCE / name).read_text(encoding='utf-8').splitlines()


def locate(lines, prefix):
    return next(i for i, line in enumerate(lines) if line.startswith(prefix))


def selections():
    qemu = load('t2_qemu.txt')
    address = locate(qemu, 'Domain0 Next Address')
    message = locate(qemu, '(THU.CST) os is loading ...')
    reset = load('t3_a_reset.txt')
    end_reset = locate(reset, '[Inferior')
    final_load = locate(reset, '=> 0x1010:')
    kernel = load('t3_d_kernel.txt')
    putchar = locate(kernel, 'Breakpoint 2 at')
    return [
        ('01_make_qemu.png', 't2_qemu.txt', qemu[1:2] + qemu[address-3:address+6] + qemu[message:message+1]),
        ('02_reset_mrom.png', 't3_a_reset.txt', reset[:16] + reset[final_load:end_reset] + reset[-3:]),
        ('03_opensbi_entry.png', 't3_b_opensbi.txt', load('t3_b_opensbi.txt')),
        ('04_watch_vs_break.png', 't3_c_watch.txt', load('t3_c_watch.txt')),
        ('05_kernel_entry.png', 't3_d_kernel.txt', kernel[:putchar]),
        ('06_putchar_ecall.png', 't3_d_kernel.txt', kernel[putchar:]),
    ]


def render():
    IMAGES.mkdir(parents=True, exist_ok=True)
    mono_path = next(p for p in [Path('C:/Windows/Fonts/consola.ttf'), Path('C:/Windows/Fonts/cour.ttf')] if p.exists())
    chinese_path = next(p for p in [Path('C:/Windows/Fonts/msyh.ttc'), Path('C:/Windows/Fonts/simsun.ttc')] if p.exists())
    body_font = ImageFont.truetype(str(mono_path), 19)
    title_font = ImageFont.truetype(str(chinese_path), 19)
    for filename, source, lines in selections():
        original = load(source)
        assert all(line in original for line in lines), 'Selected line is not source text'
        # Tabs are displayed as terminal columns; text is neither wrapped nor rewritten.
        visible = [line.expandtabs(8) for line in lines]
        title = f'来源：evidence/{source}（终端输出渲染）'
        width = max(1100, int(max([title_font.getlength(title)] + [body_font.getlength(line) for line in visible])) + 56)
        height = 82 + len(visible) * 27
        img = Image.new('RGB', (width, height), '#141820')
        draw = ImageDraw.Draw(img)
        draw.text((28, 20), title, font=title_font, fill='#a1a8b5')
        draw.line((28, 55, width-28, 55), fill='#363c48')
        for i, line in enumerate(visible):
            draw.text((28, 69 + i*27), line, font=body_font, fill='#e3e7ef')
        path = IMAGES / filename
        img.save(path, format='PNG')
        with Image.open(path) as check:
            check.verify()
        print(f'{filename}: {width}x{height}; {len(lines)} source lines')


if __name__ == '__main__':
    render()
