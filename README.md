# stutter

Stutter is an AI assistant built directly into Cutter and deeply integrated with it, able to carry out any task for you across the reverse engineering and binary analysis process. Have it work on a target alongside you, ask it for help or even have it teach you, all without leaving Cutter.

![Stutter in Cutter](assets/screenshot.png)

> [!WARNING]
> Stutter is under active development and is not ready for general use. Features are incomplete and may change or break without notice.

## Requirements

- Cutter
- Rizin
- Qt 6 (or Qt 5)
- CMake 3.16+

## Building

```bash
cmake -S . -B build
cmake --build build
cmake --install build
```

## License

Stutter is licensed under the GNU General Public License v3.0.
