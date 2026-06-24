# Firmware Binaries

Place your compiled PlatformIO firmware binaries here for GitHub Pages hosting.

## How to Build and Export Binaries

After running `pio run -e ssd1306`, find the binaries at:
```
.pio/build/ssd1306/
├── bootloader.bin      → copy to firmware/bootloader.bin
├── partitions.bin      → copy to firmware/partitions.bin
└── firmware.bin        → copy to firmware/BOBO_ssd1306.bin
```

The `boot_app0.bin` is from:
```
%USERPROFILE%\.platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin
```
Copy it to `firmware/boot_app0.bin`.

## For SH110X variant
Build with `pio run -e sh110x` and copy firmware.bin to `firmware/BOBO_sh110x.bin`
Update manifest.json to point to the correct binary.

## Placeholder files
The `.gitkeep` file keeps this folder tracked by git even when empty.
