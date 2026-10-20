"""Write aot/overlays.json (psxrecomp AOT methods v1) for RE3 from the disc.

Every hash, offset and size is read from the user's own disc; the profile holds
facts only (no game bytes). Run from the repo root:

    py -3 tools/make_aot_profile.py --cue "<path to .cue>"

Evidence for the images (see aot/README.md):
  * CD_DATA/BIN menu overlays: whole files at fixed addresses. 0x80194000 and
    0x801C2000 are named by the loader (words checks below); per file, the
    address is the unique strict jal-fit (internal calls land on prologues).
  * Enemy code modules inside room files R*.BIN: members tagged ...0001 are
    linked for 0x80100000, ...0002 for 0x8010D000 (every internal call of every
    module lands on a prologue at that base and never at the other one).
"""
import argparse, hashlib, json, os, struct, sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
sys.path.insert(0, str(ROOT / 'psxrecomp' / 'tools'))
sys.path.insert(0, str(ROOT / 'psxrecomp' / 'tools' / 'aot_overlay_spike'))
import aot_overlay_pipeline as pipe  # noqa: E402

SLOT = {1: 0x80100000, 2: 0x8010D000}
BIN_BASES = {
    0x80194000: ['DIEDEMO.BIN', 'ENDING.BIN', 'TITLE.BIN'],
    0x801C2000: ['EPILOG.BIN', 'GEARBOX.BIN', 'JILL_SEL.BIN', 'LTSOUT.BIN', 'MEM_CARD.BIN',
                 'MUSICBOX.BIN', 'OPENING.BIN', 'OPTION.BIN', 'PC_SYS.BIN', 'R214_OL.BIN',
                 'RESULT.BIN', 'SELECT.BIN', 'STAFF_R.BIN'],
    0x80184000: ['WARNING.BIN'],
}


def internal_fit(code, base):
    n = len(code) // 4
    w = struct.unpack_from('<%dI' % n, code)
    pro = {i * 4 for i, x in enumerate(w) if (x & 0xFFFF8000) == 0x27BD8000}
    hit = tot = 0
    for x in w:
        if x >> 26 == 3:
            t = ((x & 0x3FFFFFF) << 2) | 0x80000000
            if base <= t < base + len(code):
                tot += 1
                hit += (t - base) in pro
    return hit, tot


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cue', required=True)
    ap.add_argument('--out', default=str(ROOT / 'aot' / 'overlays.json'))
    a = ap.parse_args()
    disc = pipe.Disc(Path(a.cue))
    sha = pipe.digest(disc.binary)
    images = []
    for base, names in BIN_BASES.items():
        images.append(dict(method='fixed_address_files', load_addr=hex(base), allow_missing=True,
                           files=['CD_DATA/BIN/' + n for n in names]))
    extents, seen, inventory = [], {}, []
    rooms = sorted(n for n in disc.files if '/STAGE' in n and os.path.basename(n).startswith('R')
                   and n.endswith('.BIN'))
    for name in rooms:
        d = disc.read(name)
        total, count = struct.unpack_from('<II', d, 0)
        off = 0x800
        for i in range(count):
            size, tag = struct.unpack_from('<II', d, 8 + 8 * i)
            kind = tag & 0xFFFF
            if kind in SLOT:
                # Members carry their exact byte size (0xCA2D, 0xCC09, ...): the
                # tail is data. The image ends on the last whole word; the
                # recompiler rejects every jump table of a producer whose end
                # is not word aligned (C++ resolve_exact_bounded_jump_table;
                # the Python verifier has no such check).
                words = size & ~3
                body = d[off:off + words]
                h = hashlib.sha256(body).hexdigest()
                base = SLOT[kind]
                hit, tot = internal_fit(body, base)
                other = internal_fit(body, SLOT[3 - kind])
                assert other[0] == 0, (name, i, 'internal calls fit the other slot')
                if h not in seen:
                    seen[h] = len(extents)
                    item = dict(file=name, file_offset=hex(off), base=hex(base),
                                address=hex(base), size=hex(words), load_addr=hex(base),
                                sha256=h)
                    extents.append(item)
                    inventory.append(dict(sha256=h, tag='%08X' % tag, slot=hex(base),
                                          internal_calls_on_prologues='%d/%d' % (hit, tot),
                                          rooms=[]))
                inventory[seen[h]]['rooms'].append(os.path.basename(name))
            off += (size + 0x7FF) & ~0x7FF
    images.append(dict(method='fixed_address_extents', allow_missing=True, extents=extents))
    exe = 'SLUS_009.23'
    checks = [dict(method='words', file=exe, file_offset='0x800', base='0x80010000', values={
        # file loader destination: flag 0x200 -> 0x80100000, else 0x80194000
        '0x80032214': '0x3C028010', '0x80032218': '0x3C028019', '0x8003221C': '0x34424000',
        # menu overlay entry at 0x801C2000 + 0x14
        '0x800128FC': hex(struct.unpack_from('<I', disc.read(exe), 0x800 + 0x28FC)[0]),
    })]
    profile = dict(
        schema='psxrecomp AOT methods v1', game_id='SLUS-00923',
        disc_hashes=dict(sha256=sha),
        generic_exclude_files=[dict(
            files=[n for n in rooms],
            reason='Room containers: their executable members are declared as '
                   'fixed_address_extents; the whole file is not a memory image')],
        strict_bounds=True,
        # One recipe per image: 17 menu files + 21 enemy modules x 2 slots.
        checks=checks, images=images,
        expected_records=sum(len(v) for v in BIN_BASES.values()) + len(extents))
    Path(a.out).parent.mkdir(parents=True, exist_ok=True)
    Path(a.out).write_text(json.dumps(profile, indent=1) + '\n', encoding='utf-8')
    Path(a.out).with_name('enemy_modules.json').write_text(json.dumps(inventory, indent=1) + '\n',
                                                           encoding='utf-8')
    print('rooms', len(rooms), 'modules', len(extents), 'bin files',
          sum(len(v) for v in BIN_BASES.values()), '->', a.out)


if __name__ == '__main__':
    main()
