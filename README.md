# stutter

AI-assisted reverse engineering for Cutter.

Stutter adds an AI chat panel to Cutter for asking questions about the binary you are analyzing. Responses stream in as they are generated and support Markdown.

![Stutter in Cutter](assets/screenshot.png)

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
