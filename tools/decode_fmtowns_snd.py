#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""FM Towns .SND(RF5C68 PCM 音效)→ WAV 解碼器。

格式(逆向自《Ultima Trilogy》CD /SOUND/*.SND,2026-06-13):
  +0x00  名稱(8 byte ASCII,null pad)
  +0x0c  dword = PCM 資料長度(= 檔案大小 - 0x20)
  +0x20  PCM 資料起點,長度見 +0x0c
編碼:**8-bit sign-magnitude**(RF5C68 慣例:bit7 = 正負號,bits0-6 = 振幅;
  0x00 = +0、0x80 = -0,兩者皆靜音)。轉成標準 signed 16-bit。

⚠ 取樣率不在標頭明確欄位(RF5C68 由 pitch 暫存器設);預設 8000 Hz,需以耳朵/原版比對校正
  (--rate 調整)。版權音效,輸出 WAV 不入 repo。

用法: decode_fmtowns_snd.py <in.SND ...> --out <dir> [--rate 8000]
"""
import sys, os, struct, wave, argparse

def decode_snd(path, rate):
    d = open(path, "rb").read()
    if len(d) < 0x20:
        raise ValueError("too small")
    dlen = struct.unpack("<I", d[0x0c:0x10])[0]
    if dlen <= 0 or 0x20 + dlen > len(d):
        dlen = len(d) - 0x20            # 後備:用檔尾
    pcm = d[0x20:0x20 + dlen]
    out = bytearray()
    for b in pcm:
        mag = b & 0x7f
        val = -mag if (b & 0x80) else mag    # sign-magnitude → signed
        out += struct.pack("<h", val * 256)  # 7-bit → 16-bit
    return bytes(out)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("inputs", nargs="+")
    ap.add_argument("--out", required=True)
    ap.add_argument("--rate", type=int, default=8000)
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    for p in a.inputs:
        pcm16 = decode_snd(p, a.rate)
        name = os.path.splitext(os.path.basename(p))[0].lower()
        outp = os.path.join(a.out, name + ".wav")
        w = wave.open(outp, "wb")
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(a.rate)
        w.writeframes(pcm16); w.close()
        # 振幅統計(無耳驗證時的健全性檢查)
        import statistics
        smp = struct.unpack("<%dh" % (len(pcm16)//2), pcm16)
        peak = max(abs(x) for x in smp) if smp else 0
        print("%-16s -> %s  (%d 樣本, %.2fs @%dHz, peak %d)" %
              (os.path.basename(p), outp, len(smp), len(smp)/a.rate, a.rate, peak))

if __name__ == "__main__":
    main()
