# OpenBMS [WIP]
![Build Status](https://github.com/SNURhythm/OpenBMS/actions/workflows/ios-testflight.yml/badge.svg)
![Build Status](https://github.com/SNURhythm/OpenBMS/actions/workflows/macos-build.yml/badge.svg)

OpenBMS is a crossplatform BMS player which depends on open source libraries only

## Download
You can download OpenBMS from [Releases](https://github.com/SNURhythm/OpenBMS/releases/latest)

## Development
### Setup for macOS/iOS

```bash
git clone https://github.com/SNURhythm/OpenBMS.git
cd OpenBMS
git submodule update --init --recursive
./scripts/init_bgfx_ios.sh # initialize bgfx xcodeproj for iOS
./scripts/macos_init.sh
```

### Setup for Windows
```bash
git clone https://github.com/SNURhythm/OpenBMS.git
cd OpenBMS
git submodule update --init --recursive
./scripts/windows_init.sh
```

### Current Progress 
- [x] Implement basic chart select screen 
- [x] Implement audio output
- [x] Implement BGA playback
- [ ] Implement gameplay
  - [x] Implement basic judgement system
- [ ] Support skin

## Dependency

- SDL2 + bgfx
- FFmpeg (for BGA rendering)
- SQLite3
- PortAudio (for desktop) + miniaudio (for mobile)
- libsndfile
- [bms-parser-cpp](https://github.com/SNURhythm/bms-parser-cpp) for fast BMS parsing

