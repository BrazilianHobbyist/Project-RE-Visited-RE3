#!/usr/bin/env python3
r"""pgxp_seams.py: crack metric for PGXP geometry correction from pgxp_tri_ring.

Loads slot 9 saved by an earlier smoke_test.py --save-state 9 session, turns
geometry correction on with and without the vertex weld, and for a few frames
counts edges that share integer endpoints in the packet but get different
precise endpoints (a crack the background shows through). Rings are saved to
OUT for further analysis.

  py -3 tools\pgxp_seams.py tools <session with slot 9> <out folder>
"""
import sys, os, subprocess, time, json, collections
sys.path.insert(0, sys.argv[1]); import smoke_test as st
src = os.path.abspath(sys.argv[2]); out = os.path.abspath(sys.argv[3]); os.makedirs(out, exist_ok=True)
cards = st.prepare_memcards(out); st.import_state(cards, src, 9)
log = open(os.path.join(out, "stdout.log"), "w")
p = subprocess.Popen([st.DBG_EXE, "--bios", st.bios_arg(), "--debug-port", "4370", "--memcard-dir", cards], cwd=st.PCPORT, stdout=log, stderr=subprocess.STDOUT)
st.GAME_PROC = p

def analyse(tris):
    edges = collections.defaultdict(list); kinds = collections.Counter()
    for t in tris:
        v = t["v"]; srcs = [x[6] for x in v]
        kinds["precise" if all(srcs) else "native" if not any(srcs) else "mixed"] += 1
        for a, b in ((0,1),(1,2),(2,0)):
            ia, ib = (v[a][2], v[a][3]), (v[b][2], v[b][3])
            pa, pb = (v[a][4], v[a][5]), (v[b][4], v[b][5])
            if ia > ib: ia, ib, pa, pb = ib, ia, pb, pa
            edges[(ia, ib)].append((pa, pb))
    shared = cracked = 0; gaps = []
    for k, lst in edges.items():
        if len(lst) < 2: continue
        shared += 1
        ref = lst[0]
        g = max(max(abs(a[0]-ref[0][0]), abs(a[1]-ref[0][1]), abs(b[0]-ref[1][0]), abs(b[1]-ref[1][1])) for a, b in lst[1:]) / 65536.0
        if g > 0.01: cracked += 1; gaps.append(g)
    gaps.sort()
    return {"tris": len(tris), "kinds": dict(kinds), "shared_edges": shared, "split_edges": cracked,
            "gap_p50": gaps[len(gaps)//2] if gaps else 0, "gap_max": gaps[-1] if gaps else 0}
res = {}
try:
    for _ in range(60):
        time.sleep(0.5)
        if st.send({"cmd": "ping"}).get("ok"): break
    st.wait_frames(200)
    for name, v in (("weld", {"geometry": 1, "weld": 1}), ("noweld", {"geometry": 1, "weld": 0})):
        st.send({"cmd": "pgxp", **v}); st.state_request("load", 9); st.wait_frames(60)
        rows = []
        for k in range(3):
            f = st.frame() - 2
            r = st.send({"cmd": "pgxp_tri_ring", "frame": f, "count": 20000}, timeout=30)
            tris = [t for t in r.get("tris", []) if t["f"] == f]
            rows.append(analyse(tris)); st.wait_frames(7)
            if tris and not os.path.exists(os.path.join(out, f"ring_{name}.json")): json.dump({"tris": tris}, open(os.path.join(out, f"ring_{name}.json"), "w"))
        res[name] = rows
    print(json.dumps(res, indent=1))
finally:
    st.send({"cmd": "quit"}); p.wait(15)
