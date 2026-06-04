#!/usr/bin/env python3
"""從 Win 版 Ultima2.exe 抽出內嵌可中文化 UI 字串。

流程 (見 docs/EXTRACTION_PROCESS.md Phase D1):
  1. 用 Ghidra 反編產出的具名字串符號 (s_TEXT_<vaddr>) 取得位址清單,
     或直接掃 .rdata/.data 找 null-terminated 可列印字串。
  2. 用 PE section 表把 vaddr 映射回 file offset,讀原始 bytes (cp1252)。
  3. 過濾 format string / registry key / MFC class,輸出翻譯表 TSV。

用法:
  python3 extract_exe_strings.py Ultima2.exe [s_syms.txt] > out.tsv
  s_syms.txt 可選:每行一個 Ghidra 字串符號 (grep -oE 's_[A-Za-z0-9_]+_[0-9a-f]{8}')。
  不給則自掃 .rdata/.data。

注意:本工具不散布原版 binary;請自備合法 Ultima II / Alderson Win port。
"""
import re
import sys

# PE section 表 (Win Ultima2.exe v1.01):(name, VMA, file_off, vsize)
SECTIONS = [
    (".text", 0x401000, 0x00400, 0x34600),
    (".rdata", 0x436000, 0x34A00, 0x07868),
    (".data", 0x43E000, 0x3C400, 0x06400),
]

NOISE = re.compile(
    r"^(%[-0-9.]*[ds]|ColorMap\d|Settings|FirstTime|CMD_|[A-Za-z]:\\|"
    r"Software\\|\.exe$|\.txt$|player$|Aldersoft|windows|roman|kanji|"
    r"hangeul|kanjimenu|hangeulmenu)",
    re.I,
)


def vaddr_to_offset(va):
    for _name, vma, fo, vsize in SECTIONS:
        if vma <= va < vma + vsize:
            return fo + (va - vma)
    return None


def read_cstr(exe, va, maxlen=300):
    off = vaddr_to_offset(va)
    if off is None:
        return None
    end = exe.find(b"\x00", off)
    if end < 0 or end - off > maxlen:
        return None
    return exe[off:end].decode("cp1252", "replace")


def scan_data_strings(exe):
    """無符號清單時:掃 .rdata/.data 找可列印 ASCII 字串的 vaddr。"""
    addrs = []
    for _name, vma, fo, vsize in SECTIONS:
        if _name == ".text":
            continue
        blob = exe[fo : fo + vsize]
        for m in re.finditer(rb"[\x20-\x7e]{4,}", blob):
            addrs.append(vma + m.start())
    return addrs


def is_noise(text):
    return (
        NOISE.match(text)
        or re.match(r"^[%\d \-_.]+$", text)
        or text.startswith("CUltima2")
        or text.startswith("CPalette")
    )


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    exe = open(sys.argv[1], "rb").read()
    if len(sys.argv) >= 3:
        syms = open(sys.argv[2]).read().splitlines()
        addrs = []
        for s in syms:
            m = re.search(r"_([0-9a-f]{8})$", s.strip())
            if m:
                addrs.append(int(m.group(1), 16))
    else:
        addrs = scan_data_strings(exe)

    seen = {}
    for va in addrs:
        s = read_cstr(exe, va)
        if s and len(s.strip()) >= 2:
            seen[va] = s

    print("vaddr\tcategory\toriginal_text\tzh_hant")
    for va, s in sorted(seen.items()):
        cat = "SKIP" if is_noise(s) else "GAME"
        safe = s.replace("\t", " ").replace("\n", "\\n")
        print(f"0x{va:08x}\t{cat}\t{safe}\t")


if __name__ == "__main__":
    main()
