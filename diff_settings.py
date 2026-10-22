#!/usr/bin/env python3

import os

# Pick the version the same way the Makefile does: VERSION=us_alpha ./diff.py -mwo <func>
VERSIONS = {
    "intl": "SLPS_250.88",
    "us_alpha": "SLUS_203.12",
}


def apply(config, args):
    version = os.environ.get("VERSION", "intl")
    game_code = VERSIONS[version]
    build_dir = f"build/{version}"

    config["arch"] = "mipsee"
    config['mapfile'] = f'{build_dir}/{game_code}.map'
    config['expected_mapfile'] = f'expected/{build_dir}/{game_code}.map'
    config['build_dir'] = f"{build_dir}/" # only needed for mw map format
    config["expected_dir"] = f"expected/"
    config['expected_build_dir'] = f'expected/{build_dir}/'
    config['myimg'] = f'{build_dir}/{game_code}.rom'
    config['baseimg'] = f'expected/{build_dir}/{game_code}.rom'
    config['makeflags'] = [f"VERSION={version}"]
    config['source_directories'] = ['src', f'asm/{version}', 'include']
    config["objdump_flags"] = ["-Mreg-names=n32"]
    config['objdump_executable'] = 'mips-linux-gnu-objdump'
