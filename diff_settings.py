#!/usr/bin/env python3

import os

def apply(config, args):
    config["arch"] = "mipsee"
    config['mapfile'] = 'build/SLUS_203.12.map'
    config['expected_mapfile'] = 'expected/build/SLUS_203.12.map'
    config['build_dir'] = "build/" # only needed for mw map format
    config["expected_dir"] = f"expected/"
    config['expected_build_dir'] = 'expected/build/'
    config['myimg'] = 'build/SLUS_203.12.rom'
    config['baseimg'] = 'expected/build/SLUS_203.12.rom'
    config['makeflags'] = []
    config['source_directories'] = ['src', 'asm', 'include']
    config["objdump_flags"] = ["-Mreg-names=n32"]
    config['objdump_executable'] = 'mips-linux-gnu-objdump'

