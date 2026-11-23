# FFX-True-Break-Damage-Limit

Configurable Break Damage Limit for FFX HD Remaster (Steam, PC).

**A cheat.** Break Damage Limit lets a hit go past 9,999, up to 99,999. This module raises that
second ceiling to 999,999, or any value you choose from 10,000 to 999,999. Characters without
Break Damage Limit keep the normal 9,999 limit. It is a small module: no Cheat Engine, nothing to
launch.

## Requirements

- FFX HD Remaster on Steam, current build (the 2026.09.30 update or later).
- ffgriever's FF10 Module Loader. It comes with the "FFX/X-2 HD External File Loader" on Nexus
  Mods; use version 1.2.0 or later. UnX works alongside it.

## Install

1. Build `ffx_true_bdl.dll` (see Building), or take it from a release. Then copy it and
   `modules/ffx_true_bdl.ini` into `<game folder>\modules\`. That is the folder next to FFX.exe
   that holds the loader's own `ff10-file-loader.dll`.
2. Start the game. Back up your save before trying any mod.

To uninstall, delete `ffx_true_bdl.dll` from the modules folder.

## Settings

The setting is in `modules\ffx_true_bdl.ini`. Restart the game after changing it.

```
BreakDamageLimit=999999   ; 10000..999999; anything else falls back to 999999
```

## If something goes wrong

The module writes `ffx_true_bdl.log` next to FFX.exe each time the game starts. A good run says:
```
hook found at FFX.exe+XXXXXX
PATCHED - Break Damage Limit cap is now 999999 (normal limit stays 9999)
```
`NOT FOUND` means it could not locate the code (for example after a game update) and changed
nothing. It never half-patches.

## Notes

- The limit depends on who is attacking. If an enemy attack already breaks the limit, it would be
  affected too (not tested).
- It can be installed next to "FFX Tweaks Collection: Module Edition": they patch different parts
  of the game. Do not combine it with a Cheat Engine table that patches the same spot.

## How it works

For every hit, the game works out a damage limit: 9,999, plus 90,000 if the attacker has Break
Damage Limit. The module finds that calculation by scanning the game's code for its byte pattern,
then changes two numbers in it. Nothing else is touched. Because it finds the code by pattern,
not by a fixed address, small game updates should not break it.

## Building

- `source/build_msvc.bat` needs Visual Studio's C++ tools. The DLL must be 32-bit: FFX.exe is a
  32-bit process.
- `source/build_mingw.bat` needs MinGW-w64 targeting i686.
- `source/tests/run_tests.py` runs the game's own limit-building instructions, patched with the
  module's values, against every case. It needs gcc, as and ld.
- GitHub Actions builds the DLL on every push (`.github/workflows/build.yml`).

## Credits

- **Beelzepog:** the idea and request.
- **Shro:** research, testing and release.
- **Claude (Anthropic):** guidance and code. The code was written with the help of an AI
  assistant.
- **ffgriever:** the Module Loader and External File Loader.

## Licence

**GNU General Public License v3.0**: see `LICENSE`. Final Fantasy X is owned by Square Enix; no
game files are included.
