# -*- coding: utf-8 -*-
"""
Generate an SVG sequence diagram for the current PTT data flow.

The script intentionally uses only Python's standard library so the generated
diagram can be rebuilt on a clean Windows machine without installing Mermaid,
Graphviz, Pillow, or matplotlib.
"""

from __future__ import print_function

import io
import os
from xml.sax.saxutils import escape


ACTORS = [
    "发送端 DSP",
    "本地 Art-Pi SPI",
    "本地 Art-Pi UDP",
    "对端 Art-Pi UDP",
    "对端 Art-Pi SPI",
    "接收端 DSP",
]

EVENTS = [
    {"kind": "section", "text": "阶段 1：发送端 DSP 通过三线握手和 SPI block 上传整段 AMR"},
    {"kind": "note", "actor": 0, "text": "APP_STATE_ENCODE：录音完成后整段 AMR 编码"},
    {"kind": "msg", "src": 0, "dst": 1, "text": "DSP_REQ = 1，请求一次 SPI transaction"},
    {"kind": "msg", "src": 1, "dst": 0, "text": "SPI_READY = 1，Art-Pi slave DMA 已 arm"},
    {"kind": "msg", "src": 0, "dst": 1, "text": "DSP_UPLOAD_BLOCK：session、block_id、block_count、payload_bytes、AMR payload"},
    {"kind": "note", "actor": 1, "text": "解包 16-bit word payload，追加到 ptt_uplink_amr_buffer，并保存 last_ack"},
    {"kind": "msg", "src": 1, "dst": 0, "text": "SPI_READY = 0，本次 SPI transaction 完成"},
    {"kind": "msg", "src": 0, "dst": 1, "text": "DSP_STATUS_POLL：读取上一块 upload block 的延迟 ACK"},
    {"kind": "msg", "src": 1, "dst": 0, "text": "ARTPI_STATUS：ack_block_id、status_code"},
    {"kind": "note", "actor": 1, "text": "最后一块 ACK 被 DSP 取走后，唤醒 UDP TX 线程发送整段 AMR"},
    {"kind": "section", "text": "阶段 2：本地 Art-Pi 到对端 Art-Pi 的 UDP session 停等传输"},
    {"kind": "msg", "src": 1, "dst": 2, "text": "ptt_udp_request_uplink_tx：提交完整 uplink AMR session"},
    {"kind": "note", "actor": 2, "text": "UDP TX 线程按 chunk 发送；每个 chunk 等 ACK，超时最多重传 3 次"},
    {"kind": "msg", "src": 2, "dst": 3, "text": "SESSION_DATA chunk 0：magic、version、session_id、chunk_id、total_bytes、payload"},
    {"kind": "msg", "src": 3, "dst": 2, "text": "SESSION_ACK chunk 0：status OK"},
    {"kind": "msg", "src": 2, "dst": 3, "text": "SESSION_DATA chunk N：重复直到整段 AMR 发送完成"},
    {"kind": "msg", "src": 3, "dst": 2, "text": "SESSION_ACK chunk N：status OK"},
    {"kind": "note", "actor": 3, "text": "按顺序缓存到 ptt_downlink_amr_buffer，收齐后检查 AMR 头 #!AMR"},
    {"kind": "msg", "src": 3, "dst": 4, "text": "设置 PTT_STATE_DOWNLINK_READY_FOR_DSP"},
    {"kind": "msg", "src": 4, "dst": 5, "text": "DATA_READY = 1，通知 DSP 有完整 session 可下载"},
    {"kind": "section", "text": "阶段 3：接收端 DSP 空闲时主动 SPI 下载、解码并播放"},
    {"kind": "note", "actor": 5, "text": "APP_STATE_IDLE 轮询 DATA_READY，高电平后进入 APP_STATE_PTT_DOWNLOAD"},
    {"kind": "msg", "src": 5, "dst": 4, "text": "DSP_REQ = 1，请求下载下一块下行 AMR"},
    {"kind": "msg", "src": 4, "dst": 5, "text": "SPI_READY = 1，预装 ARTPI_DOWNLOAD_BLOCK；无数据则预装 ARTPI_NO_SESSION"},
    {"kind": "msg", "src": 5, "dst": 4, "text": "DSP_DOWNLOAD_REQ：session、block_id"},
    {"kind": "msg", "src": 4, "dst": 5, "text": "ARTPI_DOWNLOAD_BLOCK：block_id、block_count、payload_bytes、AMR payload"},
    {"kind": "note", "actor": 4, "text": "SPI 完成后推进下载游标；最后一块被取走后 DATA_READY = 0，状态回 IDLE"},
    {"kind": "note", "actor": 5, "text": "解包 payload 到 spi_receive_buffer[1...]，收齐后设置 received_amr_len"},
    {"kind": "note", "actor": 5, "text": "codec_service_decode_received：逐帧 AMR 解码到 PCM16 play_buf"},
    {"kind": "note", "actor": 5, "text": "codec_service_get_play_sample：直接取 PCM16 sample 播放"},
]


STYLE = {
    "width": 1520,
    "left": 80,
    "top": 72,
    "actor_y": 42,
    "actor_w": 160,
    "actor_h": 34,
    "col_gap": 238,
    "row_h": 74,
    "section_h": 46,
    "note_w": 210,
    "font": "Microsoft YaHei, SimSun, Arial, sans-serif",
}


def text_units(text):
    """Return a rough visual width where CJK characters count wider."""
    total = 0
    for char in text:
        total += 2 if ord(char) > 127 else 1
    return total


def wrap_text(text, max_units):
    """Wrap text into short lines suitable for SVG tspan rendering."""
    words = text.replace("，", "， ").replace("；", "； ").replace("。", "。 ").split()
    lines = []
    current = ""

    for word in words:
        candidate = word if not current else current + " " + word
        if text_units(candidate) <= max_units:
            current = candidate
            continue

        if current:
            lines.append(current)
        current = word

    if current:
        lines.append(current)

    return lines or [text]


def actor_x(index):
    """Return the center x coordinate of an actor lifeline."""
    return STYLE["left"] + index * STYLE["col_gap"]


def svg_text(x, y, text, size=14, anchor="middle", weight="400", fill="#1f2937", max_units=None):
    """Create an SVG text element with optional wrapping.

    @brief 生成一段 SVG text，可根据 max_units 自动拆成多行 tspan。
    @param x 文本锚点 x 坐标。
    @param y 文本第一行 y 坐标。
    @param text 要显示的文本。
    @param size 字体大小。
    @param anchor SVG text-anchor 属性。
    @param weight 字重。
    @param fill 文本颜色。
    @param max_units 单行最大近似宽度；为 None 时不换行。
    @return str SVG text 字符串。
    """
    lines = wrap_text(text, max_units) if max_units else [text]
    out = [
        '<text x="{:.1f}" y="{:.1f}" font-family="{}" font-size="{}" '
        'font-weight="{}" fill="{}" text-anchor="{}">'.format(
            x, y, STYLE["font"], size, weight, fill, anchor
        )
    ]
    for i, line in enumerate(lines):
        dy = 0 if i == 0 else size + 4
        out.append('<tspan x="{:.1f}" dy="{}">{}</tspan>'.format(x, dy, escape(line)))
    out.append("</text>")
    return "".join(out)


def draw_actor(index):
    """Draw one actor header and its lifeline."""
    x = actor_x(index)
    y = STYLE["actor_y"]
    w = STYLE["actor_w"]
    h = STYLE["actor_h"]
    return [
        '<rect x="{:.1f}" y="{:.1f}" width="{}" height="{}" rx="8" fill="#eef2ff" stroke="#4f46e5" />'.format(
            x - w / 2, y, w, h
        ),
        svg_text(x, y + 22, ACTORS[index], size=14, weight="700"),
    ]


def draw_lifeline(index, height):
    """Draw one vertical lifeline."""
    x = actor_x(index)
    return '<line x1="{:.1f}" y1="{}" x2="{:.1f}" y2="{}" stroke="#cbd5e1" stroke-dasharray="6 6" />'.format(
        x, STYLE["actor_y"] + STYLE["actor_h"] + 14, x, height - 38
    )


def draw_arrow(event, y):
    """Draw one message arrow between two actors."""
    src = actor_x(event["src"])
    dst = actor_x(event["dst"])
    direction = 1 if dst >= src else -1
    line_start = src + direction * 26
    line_end = dst - direction * 26
    label_x = (src + dst) / 2

    return [
        '<line x1="{:.1f}" y1="{:.1f}" x2="{:.1f}" y2="{:.1f}" stroke="#2563eb" stroke-width="2" marker-end="url(#arrow)" />'.format(
            line_start, y, line_end, y
        ),
        svg_text(label_x, y - 12, event["text"], size=13, fill="#0f172a", max_units=54),
    ]


def draw_note(event, y):
    """Draw one note box attached to an actor."""
    x = actor_x(event["actor"])
    w = STYLE["note_w"]
    h = 48
    lines = wrap_text(event["text"], 28)
    if len(lines) > 2:
        h = 30 + len(lines) * 18
    left = x - w / 2

    out = [
        '<rect x="{:.1f}" y="{:.1f}" width="{}" height="{}" rx="6" fill="#fff7ed" stroke="#fb923c" />'.format(
            left, y - 24, w, h
        )
    ]
    out.append(svg_text(x, y - 4, event["text"], size=12, fill="#7c2d12", max_units=28))
    return out


def draw_section(text, y):
    """Draw a section title band."""
    return [
        '<rect x="40" y="{:.1f}" width="1440" height="36" rx="8" fill="#ecfeff" stroke="#06b6d4" />'.format(
            y - 26
        ),
        svg_text(760, y - 4, text, size=15, weight="700", fill="#155e75"),
    ]


def build_svg():
    """Build the complete SVG document.

    @brief 根据固定事件表生成完整 PTT 时序图 SVG。
    @return str SVG 文件内容。
    """
    event_rows = 0
    for event in EVENTS:
        event_rows += 1.0 if event["kind"] != "note" else 1.15

    height = int(STYLE["top"] + STYLE["actor_y"] + STYLE["actor_h"] + event_rows * STYLE["row_h"] + 80)
    out = io.StringIO()
    out.write('<?xml version="1.0" encoding="UTF-8"?>\n')
    out.write(
        '<svg xmlns="http://www.w3.org/2000/svg" width="{}" height="{}" viewBox="0 0 {} {}">\n'.format(
            STYLE["width"], height, STYLE["width"], height
        )
    )
    out.write("<defs>\n")
    out.write(
        '<marker id="arrow" markerWidth="10" markerHeight="10" refX="9" refY="3" orient="auto" markerUnits="strokeWidth">'
        '<path d="M0,0 L0,6 L9,3 z" fill="#2563eb" /></marker>\n'
    )
    out.write("</defs>\n")
    out.write('<rect width="100%" height="100%" fill="#f8fafc" />\n')
    out.write(svg_text(760, 28, "当前 PTT 数据流时序图", size=22, weight="700", fill="#0f172a"))
    out.write(svg_text(760, 54, "DSP SPI 传输、Art-Pi SPI block、Art-Pi UDP session、接收端下载播放", size=13, fill="#475569"))

    for index in range(len(ACTORS)):
        for item in draw_actor(index):
            out.write(item + "\n")
    for index in range(len(ACTORS)):
        out.write(draw_lifeline(index, height) + "\n")

    y = STYLE["top"] + STYLE["actor_y"] + STYLE["actor_h"] + 44
    for event in EVENTS:
        if event["kind"] == "section":
            for item in draw_section(event["text"], y):
                out.write(item + "\n")
            y += STYLE["section_h"]
        elif event["kind"] == "msg":
            for item in draw_arrow(event, y):
                out.write(item + "\n")
            y += STYLE["row_h"]
        elif event["kind"] == "note":
            for item in draw_note(event, y):
                out.write(item + "\n")
            y += int(STYLE["row_h"] * 1.15)

    out.write(svg_text(760, height - 22, "由 tools/generate_ptt_sequence_diagram.py 生成", size=12, fill="#64748b"))
    out.write("</svg>\n")
    return out.getvalue()


def main():
    """Generate docs/ptt_sequence_diagram.svg.

    @brief 生成当前 PTT 数据流 SVG 时序图。
    @return int 0 表示生成成功。
    """
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir))
    output_dir = os.path.join(repo_root, "docs")
    output_path = os.path.join(output_dir, "ptt_sequence_diagram.svg")

    if not os.path.isdir(output_dir):
        os.makedirs(output_dir)

    with io.open(output_path, "w", encoding="utf-8", newline="\n") as svg_file:
        svg_file.write(build_svg())

    print("Generated {}".format(output_path))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
