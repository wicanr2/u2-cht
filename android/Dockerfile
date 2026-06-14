# Ultima II 繁中 — Android 建置環境(NDK + SDK + Gradle)。
# 用途:在 Docker 內把 C/SDL2 引擎編成 Android APK(Linux 無法直接驗實機,僅保證可編/可組裝)。
FROM eclipse-temurin:17-jdk-jammy

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        wget unzip git python3 file ca-certificates && \
    rm -rf /var/lib/apt/lists/*

# ---- Android SDK cmdline-tools ----
ENV ANDROID_HOME=/opt/android-sdk
ENV ANDROID_SDK_ROOT=/opt/android-sdk
ENV PATH=$PATH:$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools

RUN mkdir -p $ANDROID_HOME/cmdline-tools && cd /tmp && \
    wget -q https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip -O cmdtools.zip && \
    unzip -q cmdtools.zip -d $ANDROID_HOME/cmdline-tools && \
    mv $ANDROID_HOME/cmdline-tools/cmdline-tools $ANDROID_HOME/cmdline-tools/latest && \
    rm cmdtools.zip

# ---- SDK 套件:platform 34 + build-tools + NDK r26 ----
RUN yes | sdkmanager --licenses >/dev/null 2>&1 || true && \
    sdkmanager --install \
        "platform-tools" \
        "platforms;android-34" \
        "build-tools;34.0.0" \
        "ndk;26.1.10909125" \
        "cmake;3.22.1" >/dev/null

ENV ANDROID_NDK_HOME=$ANDROID_HOME/ndk/26.1.10909125
ENV ANDROID_NDK_ROOT=$ANDROID_HOME/ndk/26.1.10909125

WORKDIR /work
