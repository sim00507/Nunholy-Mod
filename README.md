# Nunholy Mod (Memory Editor)

This is a C++ memory editing tool for the game **Nunholy**.  
This tool enables real-time editing of essential in-game values, such as character stats and currencies, through memory manipulation using the Windows API.

## Features

- ✅ Read and modify:
  - Max Health
  - Current Health
  - Shield
  - Movement Speed
  - Silver
  - Ruby
  - BloodStone
- ✅ Detect dynamic addresses using pointer chains
- ✅ Debug mode for detailed memory access tracking

## How It Works

The tool uses the following techniques:

- Retrieves the target process ID using the window title (`FindWindowW`)
- Fetches base module addresses with `CreateToolhelp32Snapshot` and `Module32First/Next`
- Resolves multi-level pointer chains for target values
- Reads and writes to process memory using `ReadProcessMemory` and `WriteProcessMemory`

All pointer paths were manually found using Cheat Engine and are accurate as of `2025-03-23`.
Functionality may break if the game receives updates or patches.

## Requirements

- Windows OS
- A running instance of **Nunholy** (the game window must be open and titled `"Nunholy"`)
- Visual Studio or compatible C++ compiler
- Administrator privileges (for `OpenProcess` with `PROCESS_ALL_ACCESS`)

## Pointer Paths (Examples)

### BloodStone Pointer Path (as of latest version):
```
"UnityPlayer.dll" + 0x1CB26A8 └── 0xF08 └── 0xCB0 └── 0x30 └── 0x18 └── 0x28 └── 0xB8 └── 0x270 → BloodStone Value (4 bytes)
```

### Silver & Ruby Pointer Path:
```
"mono-2.0-bdwgc.dll" + 0x774518 └── 0x400 └── 0x640 └── 0x640 └── 0x648 └── 0x18 → Silver Silver - 0x20 → Ruby
```

### Dungeon Stats (Character) Pointer Path:
```
"UnityPlayer.dll" + 0x1D29E48 └── 0x58 └── 0x88 └── 0x08 └── 0x18 └── 0x10 └── 0x28 └── 0x90 → MaxHealth +0x04 → Health +0x08 → Shield +0x0C → Speed
```

## Disclaimer

This project is intended for **educational and research** purposes only.  
Do not use it to violate any game's terms of service or EULA.  
The author is not responsible for any misuse.