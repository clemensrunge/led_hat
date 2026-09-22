#!/usr/bin/env python3
"""Generate forward declarations for the functions in an Arduino sketch.

The Arduino IDE does this automatically before compiling a .ino, which is why
the sketch can call functions that are defined further down.
Usage: gen_prototypes.py <sketch.ino> <output.h>
"""
import re
import sys

KEYWORDS = {"if", "else", "while", "for", "switch", "return", "do", "case"}


def strip(src):
    """Remove comments and string/char literals, keeping line structure."""
    pattern = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)
    return pattern.sub(lambda m: re.sub(r"[^\n]", " ", m.group(0)), src)


def main():
    src = strip(open(sys.argv[1], encoding="utf-8").read())
    func = re.compile(r"^\s*([A-Za-z_][\w\s\*]*?[\s\*])([A-Za-z_]\w*)\s*\(([^;{}()]*)\)\s*\{?\s*$")

    protos = []
    depth = 0
    for line in src.splitlines():
        if depth == 0:
            m = func.match(line)
            if m and not ({m.group(2)} | set(m.group(1).split())) & KEYWORDS:
                ret = " ".join(m.group(1).split()).replace(" *", "*")
                if m.group(2) not in ("setup", "loop"):
                    protos.append(f"{ret} {m.group(2)}({' '.join(m.group(3).split())});")
        depth += line.count("{") - line.count("}")

    with open(sys.argv[2], "w", encoding="utf-8") as out:
        out.write("// Generated from the sketch by gen_prototypes.py. Do not edit.\n#pragma once\n")
        out.write("\n".join(protos) + "\n")


if __name__ == "__main__":
    main()
