# Final Fantasy X Decompilation
This is a work-in-progress decompilation of Final Fantasy X. This targets the US edition of FFX, `SLUS-20312`.

---

### Dependencies

Requires at least Python version 3.9. Additional python dependencies can be obtained by running `pip install -U -r requirements.txt`.
A `mips-linux-gnu` binutils toolchain is also required.

---

### Setup

1. Extract the ELF file (`SLUS_203.12`) from the ISO of the game and place it in the root of the repo.
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

After creating a new file in the YAML, run `make extract`.
To build the game, run `make all`.
To set up diffing, run `make diff-init`.
After setting up diffing, functions can be diff'd with `./diff.py -mwo <func_name>`.