# Kaong Wine System — Project Rules for Claude

## Build

```bash
~/.platformio/penv/bin/pio run          # compile
~/.platformio/penv/bin/pio run -t upload # flash
```

Host macOS clang shows false-positive errors (TFT_WHITE, Arduino.h). Ignore them. The ESP32 toolchain via PlatformIO is the real compiler.

---

## Display Design System

TFT_eSPI, ILI9488, 320×480 px. Every system-check screen must follow these rules exactly.

### Layout zones

| Zone | Y range | Notes |
|------|---------|-------|
| Header | 0–50 | Green bar |
| Content | 52–432 | Tiles, panels, data |
| Footer | 434–480 | White background, 1–2 hint lines |

### Color palette

| Name | Hex | Use |
|------|-----|-----|
| Header green | `0x03E0` | All screen headers (`fillRect 0,0,320,50`) |
| Selected/active tile | `0x3566` | Highlighted menu item, active button |
| Unselected tile | `0xD6BA` | Default tile background |
| Running/OK | `0x0400` | Heater/pump/fan on, test passing |
| Stopped/idle | `0x4208` | Off state (neutral, not an error) |
| Error/fail | `0xF800` | Sensor fail, test fail, danger |
| Warning | `0xFFE0` | Notice/caution bar |
| Dark panel | `0x2124` | Live data panels, dark backgrounds |
| Active panel header | `0x0320` | Active section strip in multi-panel screens |

### Header (every screen)

```cpp
tft.fillRect(0, 0, 320, 50, 0x03E0);
tft.setTextColor(TFT_WHITE);
tft.drawCentreString("SCREEN TITLE", CENTER_X, 15, 4);
```

### Tile (selection highlight)

- Selected: `0x3566` fill, white text
- Unselected: `0xD6BA` fill, black text
- **Always call `drawRect` AFTER `fillRect`** — border must go on top of fill

```cpp
uint16_t bg  = selected ? 0x3566 : 0xD6BA;
uint16_t fg  = selected ? TFT_WHITE : TFT_BLACK;
tft.fillRect(x, y, w, h, bg);
tft.setTextColor(fg, bg);
tft.drawCentreString(label, cx, ty, font);
tft.drawRect(x, y, w, h, TFT_DARKGREY);   // border last
```

### Footer (every screen)

Single-line footer:
```cpp
tft.fillRect(0, 434, 320, 46, TFT_WHITE);
tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
tft.drawCentreString("UP/DOWN: NAVIGATE   SELECT: ACTIVATE   RETURN: BACK", CENTER_X, 458, 1);
```

Two-line footer:
```cpp
tft.fillRect(0, 434, 320, 46, TFT_WHITE);
tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
tft.drawCentreString("Line 1 hints", CENTER_X, 447, 1);
tft.drawCentreString("Line 2 hints   RETURN: BACK", CENTER_X, 462, 1);
```

Footer text format: `"KEY: ACTION   KEY: ACTION   RETURN: BACK"` — always include `RETURN: BACK`.

---

## Code Patterns

### New screen checklist

1. Add `AppState` enum value in `config.h`
2. Add `bool xxxNeedsFullRedraw` + selection/state vars to `config.h` (extern) and `main.cpp` (define)
3. Add draw function declaration to `display.h`
4. Implement draw function in `display.cpp` following the layout/color rules above
5. Add state transitions in `main.cpp` (SELECT to enter, LEFT to exit, set `xxxNeedsFullRedraw = true` on entry)
6. Set `needsFullRedraw = true` in the ESTOP resume block if appropriate

### No `fillScreen` — draw header color first

Never use `tft.fillScreen(TFT_WHITE)` in any draw function. It writes all 153,600 pixels before any content appears, causing a white flash on every screen transition. Instead:

```cpp
// In the full-redraw block:
tft.fillRect(0, 0, 320, 50, 0x03E0);      // header first — user sees color immediately
tft.fillRect(0, 50, 320, 384, TFT_WHITE); // body
```

This applies everywhere: full-redraw guards, unconditional redraws, and entry points.

### Preventing selection flash (`static prevSel` pattern)

For any screen where only the selection highlight changes on UP/DOWN (no live data):

```cpp
void drawMyPickScreen() {
  static int prevSel = -1;

  const char *options[] = {"OPTION A", "OPTION B", "OPTION C"};

  auto drawTile = [&](int i, bool sel) {
    uint16_t bg  = sel ? 0x3566 : 0xD6BA;
    uint16_t fg  = sel ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(20, 80 + (i * 110), 280, 80, bg);
    tft.setTextColor(fg, bg);
    tft.drawCentreString(options[i], CENTER_X, 105 + (i * 110), 4);
    tft.drawRect(20, 80 + (i * 110), 280, 80, TFT_DARKGREY);
  };

  if (myScreenNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 384, TFT_WHITE);
    // draw header title, tiles, footer
    for (int i = 0; i < 3; i++) drawTile(i, mySelection == i);
    myScreenNeedsFullRedraw = false;
    prevSel = mySelection;
  } else if (prevSel != mySelection) {
    if (prevSel >= 0) drawTile(prevSel, false);
    drawTile(mySelection, true);
    prevSel = mySelection;
  }
}
```

This replaces redrawing all N tiles on every call with only 2 tile redraws per selection change, eliminating flicker.

### Per-tile cursor model (`xxxSelection` + `xxxEditing`)

For screens with navigable rows that each hold an editable value (like heater test, flow cal):

- `xxxSelection` (int): which row cursor is on (0, 1, 2, ...)
- `xxxEditing` (bool): whether that row is currently being edited

Button handling:
- UP/DOWN when not editing: move cursor
- UP/DOWN when editing: adjust the selected row's value
- SELECT when not editing on a value row: enter editing (`xxxEditing = true`)
- SELECT when editing: exit editing (`xxxEditing = false`)
- SELECT on an action row (e.g. START/STOP): toggle directly, no editing needed
- LEFT when editing: exit editing only (`xxxEditing = false; drawMenu();`)
- LEFT when not editing: clean up and exit screen

```cpp
// LEFT handler example
} else if (currentAppState == MY_MENU) {
  if (myEditing) {
    myEditing = false;
    drawMyMenu();
  } else {
    myRunning = false;
    mySelection = 0;
    currentAppState = SYSTEM_CHECK_MENU;
    systemCheckNeedsFullRedraw = true;
    drawSystemCheckMenu();
  }
}
```

Footer should change dynamically:
- Editing: `"UP/DN: ADJUST VALUE   SELECT: DONE"`
- Not editing: `"UP/DN: NAVIGATE   SELECT: EDIT / ACTION"`

### Live-data refresh (`valuesOnly` pattern)

For screens that update sensor/timer data every second:

```cpp
void drawMyDataScreen(bool valuesOnly) {
  if (!valuesOnly) {
    // full redraw: header, static labels, tiles, footer
  }
  // always: update live value text in place using setTextPadding
  tft.setTextPadding(120);
  tft.drawRightString(liveValue, 300, y + 13, 2);
  tft.setTextPadding(0);
}
```

Call `drawMyDataScreen(true)` from the 1-second ticker in `main.cpp`.

### `needsFullRedraw` flag

- Set `true` immediately before changing `currentAppState` to enter the screen
- The draw function clears it to `false` after the full redraw is complete
- Setting it `true` also resets `static prevSel` automatically (the full redraw path writes `prevSel = currentSel`)

---

### Ferm fan hardware quirks

The fermentation fan relay is **normally-open (NO)**, same as the pre-heat fan. Standard behavior:

| Intent | `mcp.digitalWrite` value | Why |
|--------|--------------------------|-----|
| Fan ON | `RELAY_ON` (LOW) | Energize relay, NO contact closes, fan runs |
| Fan OFF | `RELAY_OFF` (HIGH) | De-energize relay, NO contact opens, fan stops |

The ferm fan PWM controller is **active-high** (opposite of the pre-heat fan):

| Fan | Direction | Arduino mapping |
|-----|-----------|-----------------|
| Pre-heat | Active-low (0 = max) | `map(percent, 0, 100, 255, 0)` |
| Ferm | Active-high (255 = max) | `map(percent, 0, 100, 0, 255)` |

`setFanSpeed` in `main.cpp` auto-selects the mapping based on `isFermFanOn` or `fanTestFanChoice == 1`.

---

## Documentation rule

When a screen is added, renamed, or its controls change, update `SYSTEM_GUIDE.md` in the same commit. Keep `PINOUT.md` current whenever pin assignments in `config.h` change.
