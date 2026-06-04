#!/usr/bin/env python3
"""解碼 Ultima II 對話檔 (tlkxNN) 為翻譯表 TSV。

格式 (見 docs/DATA_FORMATS.md):
  - high-bit-set ASCII:每 byte OR 0x80,清 bit7 還原。
  - \\x00 分段,每段一句;\\r (0x0d) 為段內換行。
  - 檔尾常見組譯殘渣 (PRBYTE / PRINT / $00 ...),過濾。

用法:
  python3 decode_talk.py <ultima2_dir> > translations/talk_dialogue.tsv

注意:本工具不散布原版資料檔;請自備合法 Ultima II。
"""
import glob
import os
import re
import sys


def decode_file(path):
    data = open(path, "rb").read()
    dec = bytes(b & 0x7F for b in data)
    out = []
    for seg in dec.split(b"\x00"):
        s = seg.decode("ascii", "replace")
        if len(s.strip()) < 4:
            continue
        if "PRBYTE" in s or "PRINT" in s or "$00" in s:
            continue
        if not re.search(r"[A-Z]{3}", s):
            continue
        out.append(s.replace("\r", "\\r"))
    return out


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    files = sorted(glob.glob(os.path.join(sys.argv[1], "tlkx*")))
    print("file\tindex\toriginal_text\tzh_hant")
    for fp in files:
        for idx, line in enumerate(decode_file(fp)):
            print(f"{os.path.basename(fp)}\t{idx}\t{line}\t")


if __name__ == "__main__":
    main()
