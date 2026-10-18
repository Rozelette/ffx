# Final Fantasy X Decompilation
This is a work-in-progress decompilation of Final Fantasy X International . This targets the JPN edition of FFX International, `SLPS-25088`.

---

### Dependencies

Requires at least Python version 3.9. Additional python dependencies can be obtained by running `pip install -U -r requirements.txt`.
A `mips-linux-gnu` binutils toolchain is also required.

---

### Setup

1. Extract the ELF file (`SLPS_250.88`) from the ISO of the game and place it in the root of the repo.
2. Run `make init`. This does, as follows:
    - Cleans the existing directory
    - Builds any tool dependencies
    - Converts the ELF into a workable ROM
    - Splits the ROM into individual files
    - Builds the ROM
    - Sets up diff comparison tools

---

### Notes

This is still very much a work-in-progress. Many conveniences, such as file splitting, have not been done. For general documentation for how to contribute, see [Splat](https://github.com/ethteck/splat/wiki/General-Workflow).

Two versions are supported. Pick one with `VERSION` (default `intl`) on every make command:

| `VERSION` | Game | ELF to place in the repo root |
| --- | --- | --- |
| `intl` | Final Fantasy X International (JPN), `SLPS-25088` | `SLPS_250.88` |
| `us_alpha` | US alpha, `SLUS-20312` | `SLUS_203.12` |

For example `make VERSION=us_alpha init`. Each version has its own `asm/<version>`, `build/<version>` and
`expected/build/<version>`. The C in `src/` is shared, with `#if defined(VERSION_US_ALPHA)` where they differ.
For diffing, set it the same way: `VERSION=us_alpha ./diff.py -mwo <func_name>`.

After creating a new file in the YAML, run `make extract`.
To build the game, run `make all`.
To set up diffing, run `make diff-init`.
After setting up diffing, functions can be diff'd with `./diff.py -mwo <func_name>`.
