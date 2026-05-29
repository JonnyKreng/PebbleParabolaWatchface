# AGENTS.md — Pebble Watchface (Parabola)

## Commands

```bash
npm run build                   # Build for all platforms (tsc + pebble build)
npm run start                   # Build, and install on emery emulator
npm run screenshot              # Render screenshot to screenshot.png
pebble build -t release         # Release build
```

## Architecture

- **`src/c/main.c`** — entry point. Creates window, time/date text layers, battery layer, weather layer, and two custom parabola decorative layers. Subscribes to tick (minute), battery state, and app message inbox. Renamed from MyWatchface.c.
- **`src/c/parabola_draw.[ch]`** — custom `Layer` implementations for upper-left and lower-right parabola decorative lines.
- **`src/pkjs/index.ts`** — PebbleKitJS side. Fetches weather from Open-Meteo API using geolocation, sends temperature to C via `Pebble.sendAppMessage`. Compiled to JS by the Pebble toolchain — `src/pkjs/**/*.js` is gitignored.
- **`package.json`** — Pebble app manifest (watchface: true, platforms: aplite/basalt/chalk/diorite/emery/flint).
- **`tsconfig.json`** — TS target ES5, types from `pebblekitjs`.

## Gotchas

- Weather data is sent C → JS via `app_message` with **64-byte send/receive buffers** (`app_message_open(64, 64)` in `main.c:146`). This is very small — messages must fit.
- The weather layer currently shows a hardcoded `"20°"` placeholder when no message arrives.
- Screenshot: `npm run screenshot` (may fail if QEMU is broken on this host).
- No test framework — verify visually via emulator screenshots.
