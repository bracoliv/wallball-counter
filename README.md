# WallBall Counter

Hyrox-style Wall Ball rep counter.

- **ESP32**: Impact detection via piezo + SoftAP hotspot + `/status` API
- **PWA**: Phone local MoveNet Lightning squat validation + full state machine (squat → stand up → hit)

## Usage

1. Flash `esp32/WallBall.ino` to ESP32
2. Connect phone to hotspot `WallBall-1` (password: `12345678`)
3. Open `pwa/index.html` in mobile browser (or host it)
4. Allow camera, perform squat → stand up → throw to count

## Repo

https://github.com/bracoliv/wallball-counter
