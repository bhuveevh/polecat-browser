#!/usr/bin/env python3
"""Lightweight static checks that do not need a compiler.

Checks:
  * brace / parenthesis balance in every .cpp and .h
  * every .cpp includes its own header
  * every declared signal/slot method in a header has a definition in the .cpp
    (a heuristic, name-based check)
  * no obvious leftover placeholders (TODO/FIXME/XXX, 'not implemented')
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

problems = []


def strip_comments_and_strings(text):
    out = []
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        nxt = text[i + 1] if i + 1 < n else ""
        if c == "/" and nxt == "/":
            while i < n and text[i] != "\n":
                i += 1
        elif c == "/" and nxt == "*":
            i += 2
            while i + 1 < n and not (text[i] == "*" and text[i + 1] == "/"):
                i += 1
            i += 2
        elif c == '"':
            i += 1
            while i < n and text[i] != '"':
                if text[i] == "\\":
                    i += 1
                i += 1
            i += 1
        elif c == "'":
            i += 1
            while i < n and text[i] != "'":
                if text[i] == "\\":
                    i += 1
                i += 1
            i += 1
        else:
            out.append(c)
            i += 1
    return "".join(out)


files = []
for dirpath, _, names in os.walk(SRC):
    for name in names:
        if name.endswith((".cpp", ".h")):
            files.append(os.path.join(dirpath, name))

for path in files:
    with open(path, encoding="utf-8") as f:
        raw = f.read()
    clean = strip_comments_and_strings(raw)

    for open_ch, close_ch, label in (("{", "}", "braces"), ("(", ")", "parens")):
        if clean.count(open_ch) != clean.count(close_ch):
            problems.append(
                f"{os.path.relpath(path, ROOT)}: unbalanced {label} "
                f"({clean.count(open_ch)} {open_ch} vs {clean.count(close_ch)} {close_ch})")

    if path.endswith(".cpp") and os.path.basename(path) != "main.cpp":
        base = os.path.basename(path)[:-4]
        header = f'#include "{os.path.relpath(path[:-4] + ".h", SRC)}"'
        # headers are included as e.g. #include "tabs/TabManager.h"
        rel = os.path.relpath(path[:-4] + ".h", SRC)
        if f'#include "{rel}"' not in raw:
            problems.append(f"{os.path.relpath(path, ROOT)}: does not include its own header ({rel})")

    for token in ("TODO", "FIXME", "XXX", "not implemented", "unimplemented"):
        if token in raw:
            problems.append(f"{os.path.relpath(path, ROOT)}: contains '{token}'")

# Heuristic: for each class header, collect declared member function names and
# check the matching .cpp defines them.
decl_re = re.compile(
    r"^\s*(?:virtual\s+)?(?:static\s+)?[A-Za-z_:<>,\s\*&]+?\b([A-Za-z_]\w*)\s*\([^;{]*\)\s*(?:const)?\s*(?:override)?\s*;")


def class_name(raw):
    m = re.search(r"\bclass\s+(\w+)", raw)
    return m.group(1) if m else None


for path in files:
    if not path.endswith(".h"):
        continue
    with open(path, encoding="utf-8") as f:
        raw = f.read()
    cls = class_name(raw)
    if not cls:
        continue
    cpp = path[:-2] + ".cpp"
    if not os.path.exists(cpp):
        continue
    with open(cpp, encoding="utf-8") as f:
        cpp_text = f.read()

    for m in decl_re.finditer(strip_comments_and_strings(raw)):
        name = m.group(1)
        if name in ("if", "for", "while", "switch", "return", "class", "struct"):
            continue
        if re.search(r"\b" + re.escape(cls) + r"::" + re.escape(name) + r"\s*\(", cpp_text):
            continue
        # Skip pure declarations that are intentionally header-only (inline).
        if re.search(r"\)\s*(?:const)?\s*(?:override)?\s*\{\s*\}", raw):
            continue
        problems.append(f"{os.path.relpath(path, ROOT)}: '{cls}::{name}' declared but not defined in {os.path.basename(cpp)}")

if problems:
    print("ISSUES FOUND:")
    for p in problems:
        print("  -", p)
    sys.exit(1)
print(f"OK: checked {len(files)} files, no structural issues found")
