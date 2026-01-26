#!/usr/bin/env python3
"""
tree2plots_condor_waves.py

- DRY RUN supported (no condor submission)
- REAL RUN supported
- Primary jobs: 2 files/job, max 2 jobs/wave
- Secondary hadd jobs: 2 files/job
- NEVER deletes source files
- Everything written to CERNBox (EOS)
"""

import subprocess
import uuid
import time
import argparse
from pathlib import Path
from condor_test.condor_status import watch_cluster

# ============================================================
# FIXED PARAMETERS (AS REQUESTED)
# ============================================================

INPUT_LIST = Path("test_input.txt")

PRIMARY_FILES_PER_JOB = 2
MAX_PRIMARY_JOBS_PER_WAVE = 2
HADD_FILES_PER_JOB = 2

TREE2PLOTS_CFG = Path("Tree2PlotsConfig.py")

EOS_BASE = Path("/eos/user/a/amalhotr/tree2plots_test")
LOG_DIR = EOS_BASE / "logs"
PRIMARY_DIR = EOS_BASE / "primary"
HADD_DIR = EOS_BASE / "hadd"
FINAL_DIR = EOS_BASE / "final"

JOB_FLAVOUR = "longlunch"
POLL_INTERVAL = 30

TMP_DIR = Path("condor_tmp")

# ============================================================
# SETUP
# ============================================================

for d in [LOG_DIR, PRIMARY_DIR, HADD_DIR, FINAL_DIR, TMP_DIR]:
    d.mkdir(parents=True, exist_ok=True)

# ============================================================
# UTILS
# ============================================================

def run(cmd):
    return subprocess.run(cmd, shell=True, check=True, text=True, capture_output=True)

def submit(jdl, dry_run):
    if dry_run:
        print(f"[DRY RUN] condor_submit {jdl}")
        return "DRYRUN"
    out = run(f"condor_submit {jdl}").stdout
    return out.strip().split()[-1].strip(".")

def chunk(lst, n):
    for i in range(0, len(lst), n):
        yield lst[i:i+n]

def safe_unlink(p):
    try:
        p.unlink()
    except Exception:
        pass

# ============================================================
# JOB GENERATORS
# ============================================================

def make_primary_job(job_id, input_files):
    uid = uuid.uuid4().hex[:8]

    cfg_copy = TMP_DIR / f"Tree2PlotsConfig_{uid}.py"
    sh = TMP_DIR / f"run_primary_{uid}.sh"
    jdl = TMP_DIR / f"submit_primary_{uid}.jdl"
    out = PRIMARY_DIR / f"plot_primary_{job_id}.root"

    cfg = TREE2PLOTS_CFG.read_text()
    cfg = cfg.replace(
        "inputFiles = cms.vstring(inputFiles)",
        f"inputFiles = cms.vstring({', '.join(repr(f) for f in input_files)})"
    )
    cfg = cfg.replace(
        "fileName = cms.string(outputFile)",
        f'fileName = cms.string("{out}")'
    )
    cfg_copy.write_text(cfg)

    sh.write_text(f"""#!/bin/bash
set -e
cmsRun {cfg_copy}
""")
    sh.chmod(0o755)

    jdl.write_text(f"""
universe = vanilla
executable = {sh}
log    = {LOG_DIR}/primary_{uid}.log
output = {LOG_DIR}/primary_{uid}.out
error  = {LOG_DIR}/primary_{uid}.err
+JobFlavour = "{JOB_FLAVOUR}"
queue 1
""")

    return sh, jdl, cfg_copy, out

def make_hadd_job(job_id, inputs):
    uid = uuid.uuid4().hex[:8]

    sh = TMP_DIR / f"run_hadd_{uid}.sh"
    jdl = TMP_DIR / f"submit_hadd_{uid}.jdl"
    out = HADD_DIR / f"hadd_{job_id}.root"

    sh.write_text(f"""#!/bin/bash
set -e
hadd -f {out} {' '.join(str(f) for f in inputs)}
""")
    sh.chmod(0o755)

    jdl.write_text(f"""
universe = vanilla
executable = {sh}
log    = {LOG_DIR}/hadd_{uid}.log
output = {LOG_DIR}/hadd_{uid}.out
error  = {LOG_DIR}/hadd_{uid}.err
+JobFlavour = "{JOB_FLAVOUR}"
queue 1
""")

    return sh, jdl, out

# ============================================================
# MAIN
# ============================================================

def main(dry_run: bool):
    sources = [l.strip() for l in INPUT_LIST.read_text().splitlines() if l.strip()]

    print("\n=== TREE2PLOTS CONDOR PIPELINE ===")
    print(f"Dry run: {dry_run}")
    print(f"Source files: {len(sources)} (WILL NEVER BE DELETED)\n")

    # -------------------------------
    # PRIMARY WAVES
    # -------------------------------
    primary_outputs = []
    jobs = list(chunk(sources, PRIMARY_FILES_PER_JOB))

    for wave_start in range(0, len(jobs), MAX_PRIMARY_JOBS_PER_WAVE):
        wave_jobs = jobs[wave_start:wave_start + MAX_PRIMARY_JOBS_PER_WAVE]
        clusters = []
        cleanup = []

        print(f"\n--- PRIMARY WAVE {wave_start // MAX_PRIMARY_JOBS_PER_WAVE} ---")

        for j, files in enumerate(wave_jobs):
            sh, jdl, cfg, out = make_primary_job(f"w{wave_start}_j{j}", files)
            cid = submit(jdl, dry_run)
            clusters.append(cid)
            primary_outputs.append(out)
            cleanup += [sh, jdl, cfg]

            print(f"Primary job {j}:")
            print(f"  Inputs: {files}")
            print(f"  Output: {out}")

        if not dry_run:
            for cid in clusters:
                watch_cluster(cid, interval=POLL_INTERVAL, log_folder=LOG_DIR)

        for f in cleanup:
            safe_unlink(f)

    # -------------------------------
    # HADD WAVES
    # -------------------------------
    current = primary_outputs[:]
    wave = 0

    while len(current) > 1:
        print(f"\n--- HADD WAVE {wave} ---")

        clusters = []
        next_files = []
        cleanup = []

        for i, group in enumerate(chunk(current, HADD_FILES_PER_JOB)):
            sh, jdl, out = make_hadd_job(f"w{wave}_j{i}", group)
            cid = submit(jdl, dry_run)
            clusters.append(cid)
            next_files.append(out)
            cleanup += [sh, jdl]

            print(f"Hadd job {i}:")
            print(f"  Inputs: {group}")
            print(f"  Output: {out}")

        if not dry_run:
            for cid in clusters:
                watch_cluster(cid, interval=POLL_INTERVAL, log_folder=LOG_DIR)

            # delete only intermediate outputs
            for f in current:
                safe_unlink(f)

        for f in cleanup:
            safe_unlink(f)

        current = next_files
        wave += 1

    # -------------------------------
    # FINAL
    # -------------------------------
    final_out = FINAL_DIR / "Tree2Plots_FINAL.root"
    if not dry_run:
        current[0].rename(final_out)

    print("\n=== FINAL OUTPUT ===")
    print(final_out)
    print("====================\n")

# ============================================================
# CLI
# ============================================================

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--dry-run", action="store_true", help="Print plan, do not submit jobs")
    args = parser.parse_args()

    main(dry_run=args.dry_run)
