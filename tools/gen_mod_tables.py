#!/usr/bin/env python3
"""Generate the mod lookup tables from a PaperBoat checkout.

PaperBoat mods replace assets by resource path ("messages/MSG_Intro_0001",
"ui/filemenu/copyarrow", "charset/charset_standard", ...). This port reads the
same assets straight from ROM offsets, so it needs PaperBoat's path for each
message ID and each ROM offset. Both are positional data taken from PaperBoat:

  port/mod_msg_names.c    message ID -> "messages/MSG_<name>" suffix
                          (from include/assets/messages.h)
  port/mod_asset_table.c  ROM offset -> resource path, for textures and charset blobs
                          (from assets/yaml/us/**, checked against include/assets/*.h)

Map textures are not in the offset table: they come from compressed archives and
are matched by name at load time ("textures/<area>_tex/<name>").

Usage:
    python3 tools/gen_mod_tables.py /path/to/PaperBoat port/
"""
import os
import re
import sys

ROM_SIZE = 0x2800000
MSG_PREFIX = "messages/MSG_"

BPP = {"RGBA16": 16, "RGBA32": 32, "CI4": 4, "CI8": 8, "IA4": 4, "IA8": 8, "IA16": 16, "I4": 4, "I8": 8}

# yaml trees that are not addressed by ROM offset in this port
SKIP_TOP = {"textures", "sprites.yml", "messages.yml", "shapes.yml", "collisions.yml", "audio.yml"}


def parse_messages(header):
    text = open(header, encoding="utf-8").read()
    defs = dict(re.findall(r'char\s+(gMsg_\w+)\[\]\s*=\s*"__OTR__([^"]+)"', text))
    if not defs:
        sys.exit("error: no gMsg_* definitions in " + header)
    sections = {}
    for sec, body in re.findall(r"gMsgPaths_sec([0-9A-Fa-f]{2})\[\]\s*=\s*\{(.*?)\};", text, re.S):
        names = []
        for tok in (t.strip() for t in body.split(",")):
            if not tok:
                continue
            rel = defs.get(tok)
            if rel is None or not rel.startswith(MSG_PREFIX):
                sys.exit("error: bad message entry %r in section %s" % (tok, sec))
            name = rel[len(MSG_PREFIX):]
            if not re.fullmatch(r"[A-Za-z0-9_]+", name):
                sys.exit("error: bad message name %r" % name)
            names.append(name)
        sections[int(sec, 16)] = names
    order = re.search(r"gMsgSectionPaths\[\]\s*=\s*\{(.*?)\};", text, re.S)
    listed = [int(m, 16) for m in re.findall(r"gMsgPaths_sec([0-9A-Fa-f]{2})", order.group(1))]
    if listed != list(range(len(listed))) or sorted(sections) != listed:
        sys.exit("error: message sections are not 00..N")
    return [sections[i] for i in listed]


def known_paths(include_dir):
    """Resource paths and resource directories named in PaperBoat's asset headers."""
    paths, dirs = set(), set()
    for dirpath, _, files in os.walk(include_dir):
        for f in files:
            if f.endswith(".h"):
                text = open(os.path.join(dirpath, f), encoding="utf-8").read()
                for p in re.findall(r'"__OTR__([^"]+)"', text):
                    paths.add(p)
                    dirs.add(p.rsplit("/", 1)[0])
    return paths, dirs


def parse_int(s):
    return int(s, 0)


def yaml_entries(path):
    """Minimal reader for PaperBoat's one-line asset yaml entries."""
    segments, virtual = {}, None
    entries = []
    key = None
    for line in open(path, encoding="utf-8"):
        m = re.match(r"\s*-\s*\[\s*(0x[0-9A-Fa-f]+|\d+)\s*,\s*(0x[0-9A-Fa-f]+)\s*\]", line)
        if m and key is None:
            segments[parse_int(m.group(1))] = parse_int(m.group(2))
            continue
        m = re.match(r"\s*virtual:\s*\[\s*(0x[0-9A-Fa-f]+)\s*,\s*(0x[0-9A-Fa-f]+)\s*\]", line)
        if m:
            virtual = (parse_int(m.group(1)), parse_int(m.group(2)))
            continue
        m = re.match(r"^([^\s:#][^:]*):\s*$", line)
        if m:
            key = m.group(1).strip()
            continue
        m = re.search(r"\{(.*)\}", line)
        if m and key is not None:
            fields = dict(
                (k.strip(), v.strip())
                for k, v in (p.split(":", 1) for p in m.group(1).split(",") if ":" in p)
            )
            entries.append((key, fields))
            key = None
    return segments, virtual, entries


def rom_offset(off, segments, virtual):
    seg = off >> 24
    if seg in segments and seg != 0:
        base = segments[seg]
        rom = base + (off & 0xFFFFFF)
    elif virtual is not None and off >= virtual[0]:
        rom = virtual[1] + (off - virtual[0])
    else:
        rom = off
    return rom if rom < ROM_SIZE else None


def entry_size(f):
    t = f.get("type", "")
    if t == "TEXTURE":
        fmt = f.get("format", "")
        if fmt not in BPP or "width" not in f or "height" not in f:
            return None, None
        return parse_int(f["width"]) * parse_int(f["height"]) * BPP[fmt] // 8, 0
    if t in ("BLOB", "PM64:CHARSET") and "size" in f:
        return parse_int(f["size"]), 1
    return None, None


def collect_assets(root, valid):
    ydir = os.path.join(root, "assets", "yaml", "us")
    out = {}
    for dirpath, _, files in os.walk(ydir):
        rel_dir = os.path.relpath(dirpath, ydir)
        top = rel_dir.split(os.sep)[0] if rel_dir != "." else None
        if top in SKIP_TOP:
            continue
        for fn in files:
            if not fn.endswith((".yml", ".yaml")) or (rel_dir == "." and fn in SKIP_TOP):
                continue
            stem = os.path.splitext(fn)[0]
            prefix = stem if rel_dir == "." else os.path.join(rel_dir, stem).replace(os.sep, "/")
            segments, virtual, entries = yaml_entries(os.path.join(dirpath, fn))
            for key, f in entries:
                if f.get("type") == "TEXTURE" and f.get("format") == "TLUT":
                    continue
                size, kind = entry_size(f)
                if size is None or "offset" not in f:
                    continue
                path = prefix + "/" + key
                # entity and pause textures are only reached through display lists, so their
                # exact paths are not in the headers; accept them when the directory is
                if path not in valid[0] and prefix not in valid[1]:
                    continue
                rom = rom_offset(parse_int(f["offset"]), segments, virtual)
                if rom is None:
                    continue
                out.setdefault(rom, (size, kind, path))
    return sorted((rom, s, k, p) for rom, (s, k, p) in out.items())


def write_msg_table(sections, path):
    with open(path, "w") as w:
        w.write("/* GENERATED by tools/gen_mod_tables.py from PaperBoat's include/assets/messages.h. Do not edit. */\n")
        w.write('#include "mod_assets.h"\n\n')
        for i, names in enumerate(sections):
            w.write("static const char* const sSec%02X[] = {\n" % i)
            for n in names:
                w.write('    "%s",\n' % n)
            w.write("};\n\n")
        w.write("static const struct { const char* const* names; unsigned count; } sSections[] = {\n")
        for i, names in enumerate(sections):
            w.write("    { sSec%02X, %d },\n" % (i, len(names)))
        w.write("};\n\n")
        w.write("const char* port_mod_msg_suffix(unsigned int msgID) {\n")
        w.write("    unsigned int section = msgID >> 16;\n")
        w.write("    unsigned int index = msgID & 0xFFFF;\n\n")
        w.write("    if (section >= %d || index >= sSections[section].count) {\n" % len(sections))
        w.write("        return 0;\n    }\n")
        w.write("    return sSections[section].names[index];\n}\n")


def write_asset_table(assets, path):
    with open(path, "w") as w:
        w.write("/* GENERATED by tools/gen_mod_tables.py from PaperBoat's assets/yaml/us. Do not edit.\n")
        w.write(" * ROM offset -> PaperBoat resource path. kind 0 = texture, 1 = blob. Sorted by offset. */\n")
        w.write('#include "mod_assets.h"\n\n')
        w.write("const PortModAsset gPortModAssets[] = {\n")
        for rom, size, kind, p in assets:
            w.write('    { 0x%07X, 0x%X, %d, "%s" },\n' % (rom, size, kind, p))
        w.write("};\n\n")
        w.write("const unsigned int gPortModAssetCount = %d;\n" % len(assets))


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    root, outdir = sys.argv[1], sys.argv[2]
    sections = parse_messages(os.path.join(root, "include", "assets", "messages.h"))
    valid = known_paths(os.path.join(root, "include", "assets"))
    assets = collect_assets(root, valid)
    write_msg_table(sections, os.path.join(outdir, "mod_msg_names.c"))
    write_asset_table(assets, os.path.join(outdir, "mod_asset_table.c"))
    print("messages: %d, assets: %d (textures %d, blobs %d)" % (
        sum(len(s) for s in sections), len(assets),
        sum(1 for a in assets if a[2] == 0), sum(1 for a in assets if a[2] == 1)))


if __name__ == "__main__":
    main()
