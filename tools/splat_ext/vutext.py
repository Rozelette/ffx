#!/usr/bin/env python3

from pathlib import Path
from typing import Optional

from splat.util import options
from splat.segtypes.common.textbin import CommonSegTextbin

#TODO .dsm?
class PS2SegVutext(CommonSegTextbin):
    def get_linker_section(self) -> str:
        return ".vutext"

    def get_section_flags(self):
        return "a" # TODO ax?

    def out_path(self) -> Optional[Path]:
        if self.use_src_path:
            return options.opts.src_path / self.dir / f"{self.name}.s"

        return options.opts.data_path / self.dir / f"{self.name}.s"