#!/usr/bin/env python3
"""
tree2plots_condor_streaming.py

Fully streaming Condor pipeline (CERN + CMSSW compliant)

- Submission sandbox on AFS/local
- Data on EOS
- Explicit CMSSW bootstrap inside jobs
- Streaming primaries + hadds
- Uses condor_status.py for monitoring
- Cleans up consumed intermediates (EOS + AFS)
"""

import subprocess
import uuid
import time
import argparse
from pathlib import Path
from collections import deque

from condor_test.condor_status import check_cluster_status

# ============================================================
# PARAMETERS
# ============================================================

INPUT_LIST = Path("path-text-files/TreeMakerOutputs.txt").resolve()

PRIMARY_FILES_PER_JOB = 4
MAX_PRIMARY_JOBS_IN_FLIGHT = 60  # idle + running primaries
HADD_FILES_PER_JOB = 12

CMSSW_BASE = Path("/afs/cern.ch/user/a/amalhotr/CMSSW_14_0_18_patch1/src").resolve()
TREE2PLOTS_CFG = CMSSW_BASE / "Run3ScoutingAnalysisTools/Tree2PlotsConfig.py"

# ---- DATA (EOS) ----
EOS_BASE = Path("/eos/user/a/amalhotr/tree2plots_streaming")
PRIMARY_DIR = EOS_BASE / "primary"
HADD_DIR = EOS_BASE / "hadd"
FINAL_DIR = EOS_BASE / "final"

# ---- SUBMISSION SANDBOX (AFS/local) ----
LOCAL_BASE = Path.cwd()
LOG_DIR = LOCAL_BASE / "condor_logs"
TMP_DIR = LOCAL_BASE / "condor_tmp"

JOB_FLAVOUR = "longlunch"
POLL_INTERVAL = 60

# ============================================================
# SETUP
# ============================================================

for d in [PRIMARY_DIR, HADD_DIR, FINAL_DIR]:
    d.mkdir(parents=True, exist_ok=True)

for d in [LOG_DIR, TMP_DIR]:
    d.mkdir(parents=True, exist_ok=True)

# ============================================================
# HELPERS
# ============================================================

def run(cmd):
    return subprocess.run(cmd, shell=True, text=True, capture_output=True)

def submit(jdl: Path, dry_run: bool) -> str:
    if dry_run:
        print(f"[DRY RUN] condor_submit {jdl}")
        return f"dry_{uuid.uuid4().hex[:8]}"

    res = run(f"condor_submit {jdl}")
    if res.returncode != 0:
        print("\n❌ condor_submit FAILED")
        print("STDOUT:\n", res.stdout)
        print("STDERR:\n", res.stderr)
        raise RuntimeError("condor_submit failed")

    return res.stdout.strip().split()[-1].strip(".")

def safe_unlink(p: Path):
    try:
        if p.exists():
            p.unlink()
    except Exception:
        pass

# ============================================================
# JOB BUILDERS
# ============================================================

CMSSW_SETUP = f"""
export VO_CMS_SW_DIR=/cvmfs/cms.cern.ch
source $VO_CMS_SW_DIR/cmsset_default.sh
cd {CMSSW_BASE}
eval `scramv1 runtime -sh`
cd -
"""

def make_primary_job(inputs):
    uid = uuid.uuid4().hex[:8]

    cfg = TMP_DIR / f"Tree2PlotsConfig_{uid}.py"
    sh  = TMP_DIR / f"run_primary_{uid}.sh"
    jdl = TMP_DIR / f"submit_primary_{uid}.jdl"
    out = PRIMARY_DIR / f"plot_primary_{uid}.root"

    text = TREE2PLOTS_CFG.read_text()
    text = text.replace(
        "inputFiles = cms.vstring(inputFiles)",
        f"inputFiles = cms.vstring({', '.join(repr(str(f)) for f in inputs)})"
    )
    text = text.replace(
        "fileName = cms.string(outputFile)",
        f'fileName = cms.string("{out}")'
    )
    cfg.write_text(text)

    sh.write_text(
        "#!/bin/bash\n"
        "set -e\n"
        + CMSSW_SETUP +
        f"\ncmsRun {cfg}\n"
    )
    sh.chmod(0o755)

    jdl.write_text(f"""universe = vanilla
executable = {sh}
should_transfer_files = NO
log    = {LOG_DIR}/primary_{uid}.log
output = {LOG_DIR}/primary_{uid}.out
error  = {LOG_DIR}/primary_{uid}.err
+JobFlavour = "{JOB_FLAVOUR}"
queue 1
""")

    return jdl, out, [cfg, sh, jdl]

def make_hadd_job(inputs):
    uid = uuid.uuid4().hex[:8]

    sh  = TMP_DIR / f"run_hadd_{uid}.sh"
    jdl = TMP_DIR / f"submit_hadd_{uid}.jdl"
    out = HADD_DIR / f"hadd_{uid}.root"

    sh.write_text(
        "#!/bin/bash\n"
        "set -e\n"
        + CMSSW_SETUP +
        f"\nhadd -f {out} {' '.join(map(str, inputs))}\n"
    )
    sh.chmod(0o755)

    jdl.write_text(f"""universe = vanilla
executable = {sh}
should_transfer_files = NO
log    = {LOG_DIR}/hadd_{uid}.log
output = {LOG_DIR}/hadd_{uid}.out
error  = {LOG_DIR}/hadd_{uid}.err
+JobFlavour = "{JOB_FLAVOUR}"
queue 1
""")

    return jdl, out, [sh, jdl]

# ============================================================
# MAIN
# ============================================================

def main(dry_run: bool):
    source_files = deque(
        Path(l.strip()) for l in INPUT_LIST.read_text().splitlines() if l.strip()
    )

    ready_files = deque()
    active = {}
    inflight_primaries = 0
    last_report = 0

    print("\n=== TREE2PLOTS STREAMING PIPELINE ===")
    print(f"Dry run: {dry_run}")
    print(f"Source files: {len(source_files)} (NEVER DELETED)\n")

    while source_files or active or len(ready_files) > 1:

        # ---------------- submit primaries ----------------
        while source_files and inflight_primaries < MAX_PRIMARY_JOBS_IN_FLIGHT:
            batch = [source_files.popleft()
                     for _ in range(min(PRIMARY_FILES_PER_JOB, len(source_files)))]
            jdl, out, cleanup = make_primary_job(batch)
            cid = submit(jdl, dry_run)

            print("PRIMARY SUBMITTED:", out)
            active[cid] = dict(type="primary", output=out, inputs=[], cleanup=cleanup)
            inflight_primaries += 1

        # ---------------- poll completions ----------------
        finished = []
        for cid, meta in active.items():
            if dry_run or check_cluster_status(cid, LOG_DIR)["state"] == "finished":
                finished.append(cid)

        for cid in finished:
            meta = active.pop(cid)
            ready_files.append(meta["output"])

            if meta["type"] == "primary":
                inflight_primaries -= 1

            # ✅ SAFE EOS CLEANUP: only AFTER hadd finished
            if meta["type"] == "hadd":
                for f in meta["inputs"]:
                    safe_unlink(f)

            # cleanup AFS artifacts
            for f in meta["cleanup"]:
                safe_unlink(f)

        # ---------------- submit hadds ----------------
        def nhadd():
            if len(ready_files) >= HADD_FILES_PER_JOB:
                return HADD_FILES_PER_JOB
            if not source_files and not active and len(ready_files) > 1:
                return len(ready_files)
            return 0

        while nhadd():
            n = nhadd()
            inputs = [ready_files.popleft() for _ in range(n)]
            jdl, out, cleanup = make_hadd_job(inputs)
            cid = submit(jdl, dry_run)

            print("HADD SUBMITTED:", out)
            active[cid] = dict(
                type="hadd",
                output=out,
                inputs=inputs,
                cleanup=cleanup
            )

        # ---------------- status ----------------
        if not dry_run:
            now = time.time()
            if now - last_report > POLL_INTERVAL:
                last_report = now
                print("\n--- PIPELINE STATUS ---")
                print(f"Active primaries : {sum(m['type']=='primary' for m in active.values())}")
                print(f"Active hadds     : {sum(m['type']=='hadd' for m in active.values())}")
                print(f"Ready files      : {len(ready_files)}")
                print(f"Remaining inputs : {len(source_files)}")
                print("-----------------------")
            time.sleep(POLL_INTERVAL)

    # ---------------- final ----------------
    final = FINAL_DIR / "Tree2Plots_FINAL_chi2normCut.root"
    print("\n=== FINAL OUTPUT ===")
    if dry_run:
        print(f"[DRY RUN] would produce: {final}")
    else:
        ready_files[0].rename(final)
        print(final)

# ============================================================
# CLI
# ============================================================

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    main(args.dry_run)

# To run:
# python3 tree2plots_condor_streaming.py --dry-run
# python3 tree2plots_condor_streaming.py