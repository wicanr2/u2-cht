# U2-CHT PoC 建置/執行環境 (headless,離屏渲染存 PNG)
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake pkg-config \
        libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev libsdl2-mixer-dev \
        fonts-wqy-zenhei \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# WQY 字型路徑 (CJK 來源):/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc
WORKDIR /work
