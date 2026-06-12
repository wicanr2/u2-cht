#!/usr/bin/env python3
"""從 FM Towns《Ultima Trilogy》CD 映像(.cue + .img/.bin)抽 CDDA 音軌 → WAV。

FM Towns 版的 BGM 是 CDDA(紅皮書音軌):TRACK 02+ AUDIO,44100Hz/16-bit/stereo,
2352 byte/sector。本工具依 cue 的 INDEX 01 MSF 算 byte offset,切出每軌 raw PCM
包成 WAV(CD 與 WAV 同為 little-endian,直接複製不需 byte-swap)。

WAV → ogg 交給 ffmpeg(`ffmpeg -i tNN.wav tNN.ogg`),ogg 供引擎 SDL_mixer 播放。
※ 音訊為版權內容,抽出的檔不入 repo;使用者自備 CD 映像、本地抽取。

用法:extract_fmtowns_cdda.py <game.cue> <輸出目錄>
"""
import sys
import os
import re
import wave


def main():
    cue_path, outdir = sys.argv[1], sys.argv[2]
    cue = open(cue_path).read()
    m = re.search(r'FILE "([^"]+)"', cue)
    img = os.path.join(os.path.dirname(cue_path), m.group(1))
    size = os.path.getsize(img)
    os.makedirs(outdir, exist_ok=True)

    # 解析每軌 INDEX 01 的 MSF → byte offset(sector = 2352 byte,75 sector/sec)
    tracks = []
    for t in re.finditer(r"TRACK (\d+) (\w+).*?INDEX 01 (\d+):(\d+):(\d+)", cue, re.S):
        n, mode = int(t.group(1)), t.group(2)
        M, S, F = int(t.group(3)), int(t.group(4)), int(t.group(5))
        tracks.append((n, mode, ((M*60+S)*75 + F) * 2352))

    data = open(img, "rb").read()
    audio = [t for t in tracks if t[1] == "AUDIO"]
    for i, (n, _mode, start) in enumerate(audio):
        end = audio[i+1][2] if i+1 < len(audio) else size
        pcm = data[start:end]
        out = os.path.join(outdir, f"track{n:02d}.wav")
        w = wave.open(out, "wb")
        w.setnchannels(2); w.setsampwidth(2); w.setframerate(44100)
        w.writeframes(pcm); w.close()
        print(f"track{n:02d}.wav  {len(pcm)/2352/75:.1f}s")


if __name__ == "__main__":
    main()
