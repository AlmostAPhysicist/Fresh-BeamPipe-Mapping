#!/usr/bin/env python3
"""
ultimate_condor_batch_submissions.py

Secondary-only recursive HADD pipeline.

Correct behavior:
- condor_q -> log -> condor_history
- NEVER treat "not_found" as failure
- Only fail if Condor explicitly reports failure
- Meaningful, traceable job names
- Safe against Condor race windows
- Create job.<cluster>.log/.out/.err symlinks right after submission so monitor can find logs by cluster id
"""

import subprocess
import uuid
import time
import re
import argparse
from pathlib import Path
from collections import deque

from condor_test.condor_status import check_cluster_status

# ================= CONFIG =================
INPUT_LIST = Path("path-text-files/MC_fix_things.txt").resolve()
OUTPUT_NAME = "ScoutingMC_fix_things.root"

HADD_FILES_PER_JOB = 32
MAX_HADD_JOBS_IN_FLIGHT = 64

EOS_BASE = Path("/eos/user/a/amalhotr/tree2plots_streaming")
HADD_DIR = EOS_BASE / "hadd"
FINAL_DIR = EOS_BASE / "final"

LOCAL_BASE = Path.cwd()
LOG_DIR = LOCAL_BASE / "condor_logs"
TMP_DIR = LOCAL_BASE / "condor_tmp"

JOB_FLAVOUR = "espresso"
# possible values (from https://batchdocs.web.cern.ch/local/submit.html#job-flavours):
# espresso     = 20 minutes
# microcentury = 1 hour
# longlunch    = 2 hours
# workday      = 8 hours
# tomorrow     = 1 day
# testmatch    = 3 days
# nextweek     = 1 week
POLL_INTERVAL = 30 # How frequently I fetch job status updates from Condor (seconds)

LOG_DIR_STR = str(LOG_DIR.resolve())
CMSSW_SETUP = ""  # hadd does not need CMSSW

# safe cleanup switch: when True, delete intermediate hadd files produced in HADD_DIR
# after they have been consumed by the next hadd. Default: False (safe).
DELETE_INTERMEDIATE_HADD = True

# ==========================================

for d in [HADD_DIR, FINAL_DIR, LOG_DIR, TMP_DIR]:
    d.mkdir(parents=True, exist_ok=True)

# ---------------- helpers ----------------
def run(cmd):
    return subprocess.run(cmd, shell=True, text=True, capture_output=True)

def shorthex():
    return uuid.uuid4().hex[:8]

def timestamp():
    return time.strftime("%Y%m%dT%H%M%S")

def make_traced_name(prefix: str, n_inputs: int):
    return f"{prefix}_{n_inputs}_{timestamp()}_{shorthex()}"

def submit_and_get_cluster(jdl: Path, dry_run: bool) -> str:
    """
    Submit JDL and return plain cluster id string.
    Additionally create job.<cluster>.log/.out/.err symlinks pointing to the named log/out/err
    files we asked condor to write (so monitoring can find them by cluster id).
    """
    if dry_run:
        cid = f"dry_{shorthex()}"
        print(f"[DRY RUN] condor_submit {jdl} -> {cid}")
        return cid

    r = run(f"condor_submit {jdl}")
    if r.returncode != 0:
        print("condor_submit failed:")
        print(r.stdout)
        print(r.stderr)
        raise RuntimeError("condor_submit failed")

    out = (r.stdout or "").strip()

    # parse cluster id (robust)
    m = re.search(r"submitted to cluster\s+(\d+)", out, re.IGNORECASE)
    if not m:
        m = re.search(r"ClusterId\s*=\s*(\d+)", out, re.IGNORECASE)
    if not m:
        nums = re.findall(r"\b(\d{3,})\b", out)
        if nums:
            cluster = nums[-1]
        else:
            # fallback: last token if numeric-like
            toks = out.split()
            cluster = toks[-1] if toks else f"dry_{shorthex()}"
    else:
        cluster = m.group(1)

    # create helper symlinks so condor_status can find logs using cluster id
    # our jdl writes to files named {LOG_DIR}/{name}.log/.out/.err
    # we add symlink job.<cluster>.log -> that file (if not already present)
    try:
        # attempt to find the actual log/out/err filenames referenced in the jdl
        # We assume the jdl used basename pattern "name" as in make_hadd_job.
        # Search for the most recently created file in LOG_DIR that matches 'hadd_*' or similar
        # But simpler & deterministic: parse the jdl text to extract the log path if possible.
        jdl_text = jdl.read_text(errors="ignore")
        # find log = <path>, output = <path>, error = <path>
        log_path = None
        out_path = None
        err_path = None
        for line in jdl_text.splitlines():
            line = line.strip()
            if line.lower().startswith("log"):
                parts = line.split("=", 1)
                if len(parts) == 2:
                    log_path = parts[1].strip()
            if line.lower().startswith("output"):
                parts = line.split("=", 1)
                if len(parts) == 2:
                    out_path = parts[1].strip()
            if line.lower().startswith("error"):
                parts = line.split("=", 1)
                if len(parts) == 2:
                    err_path = parts[1].strip()

        # remove potential variable interpolation and quotes
        def clean(p):
            if not p:
                return None
            return p.strip().strip('"').strip("'")

        log_path = clean(log_path)
        out_path = clean(out_path)
        err_path = clean(err_path)

        # If paths are relative, make them absolute relative to LOG_DIR
        def ensure_path(p):
            if not p:
                return None
            p = Path(p)
            if not p.is_absolute():
                return (Path(LOG_DIR) / p).resolve()
            return p.resolve()

        log_p = ensure_path(log_path)
        out_p = ensure_path(out_path)
        err_p = ensure_path(err_path)

        # symlink names we want
        symlink_log = LOG_DIR / f"job.{cluster}.log"
        symlink_out = LOG_DIR / f"job.{cluster}.out"
        symlink_err = LOG_DIR / f"job.{cluster}.err"

        # create symlinks (if the target exists); if target doesn't exist yet, make placeholder link
        if log_p:
            try:
                if not symlink_log.exists():
                    # if target exists, create symlink, otherwise create a small placeholder file if needed
                    if log_p.exists():
                        symlink_log.symlink_to(log_p)
                    else:
                        # create an empty file and symlink to it (so finder sees it); condor will append later
                        log_p.parent.mkdir(parents=True, exist_ok=True)
                        log_p.touch(exist_ok=True)
                        if not symlink_log.exists():
                            symlink_log.symlink_to(log_p)
            except Exception:
                pass

        if out_p:
            try:
                if not symlink_out.exists():
                    if out_p.exists():
                        symlink_out.symlink_to(out_p)
                    else:
                        out_p.parent.mkdir(parents=True, exist_ok=True)
                        out_p.touch(exist_ok=True)
                        if not symlink_out.exists():
                            symlink_out.symlink_to(out_p)
            except Exception:
                pass

        if err_p:
            try:
                if not symlink_err.exists():
                    if err_p.exists():
                        symlink_err.symlink_to(err_p)
                    else:
                        err_p.parent.mkdir(parents=True, exist_ok=True)
                        err_p.touch(exist_ok=True)
                        if not symlink_err.exists():
                            symlink_err.symlink_to(err_p)
            except Exception:
                pass

    except Exception:
        # don't fail submit just because symlink creation had an issue
        pass

    return str(cluster)

def safe_unlink(p: Path):
    try:
        if p.exists():
            p.unlink()
    except Exception:
        pass

# ---------------- job builder ----------------
def make_hadd_job(inputs):
    name = make_traced_name("hadd", len(inputs))
    out = HADD_DIR / f"{name}.root"

    sh = TMP_DIR / f"run_{name}.sh"
    jdl = TMP_DIR / f"submit_{name}.jdl"

    sh_text = (
        "#!/bin/bash\n"
        "set -euo pipefail\n"
        f"{CMSSW_SETUP}\n"
        f"hadd -f {out} {' '.join(map(str, inputs))}\n"
    )
    sh.write_text(sh_text)
    sh.chmod(0o755)

    jdl_text = (
        "universe = vanilla\n"
        f"executable = {sh}\n"
        "should_transfer_files = NO\n"
        f"log    = {LOG_DIR_STR}/{name}.log\n"
        f"output = {LOG_DIR_STR}/{name}.out\n"
        f"error  = {LOG_DIR_STR}/{name}.err\n"
        f'+JobFlavour = "{JOB_FLAVOUR}"\n'
        "queue 1\n"
    )
    jdl.write_text(jdl_text)
    return jdl, out, [sh, jdl, LOG_DIR / f"{name}.err", LOG_DIR / f"{name}.log"]

# ---------------- main pipeline ----------------
def main(dry_run: bool):
    ready = deque(Path(l.strip()) for l in INPUT_LIST.read_text().splitlines() if l.strip())
    active = {}  # cluster_id -> meta
    failed_jobs = []

    print("\n=== ULTIMATE HADD PIPELINE ===")
    print(f"Initial files : {len(ready)}")
    print(f"Log dir       : {LOG_DIR_STR}")
    print(f"Dry run       : {dry_run}\n")

    while len(ready) > 1 or active:
        # submit as many as we can (greedy)
        while len(ready) >= HADD_FILES_PER_JOB and len(active) < MAX_HADD_JOBS_IN_FLIGHT:
            inputs = [ready.popleft() for _ in range(HADD_FILES_PER_JOB)]
            jdl, out, cleanup = make_hadd_job(inputs)
            cluster = submit_and_get_cluster(jdl, dry_run)
            active[cluster] = dict(output=out, inputs=inputs, cleanup=cleanup)
            print(f"HADD SUBMITTED: {out}  (cluster={cluster})")

        # final wave (when < HADD_FILES_PER_JOB left and no active)
        if len(ready) > 1 and not active:
            inputs = list(ready)
            ready.clear()
            jdl, out, cleanup = make_hadd_job(inputs)
            cluster = submit_and_get_cluster(jdl, dry_run)
            active[cluster] = dict(output=out, inputs=inputs, cleanup=cleanup)
            print(f"FINAL WAVE SUBMITTED: {out}  (cluster={cluster})")

        # poll active clusters
        finished = []
        for cluster in list(active.keys()):
            if dry_run:
                finished.append(cluster)
                continue
            # make sure we pass the cluster id without any ".proc"
            cluster_id = str(cluster).split(".")[0]
            try:
                st = check_cluster_status(cluster_id, log_folder=LOG_DIR_STR)
            except TimeoutError as e:
                # transient — don't kill the pipeline; retry next poll
                print(f"[WARN] check_cluster_status timeout for {cluster_id}: {e}")
                continue
            except Exception as e:
                # unexpected errors: print and mark job for manual inspection (restore inputs)
                print(f"[ERROR] check_cluster_status failed for {cluster_id}: {e}")
                # restore inputs for manual retry
                meta = active.pop(cluster)
                for f in meta.get("inputs", []):
                    ready.appendleft(f)
                failed_jobs.append((cluster_id, meta))
                continue

            state = st.get("state", "").lower()
            if state == "active":
                # still running, skip
                continue
            if state == "finished":
                finished.append(cluster)
            else:
                # not_found -> treat as transient; do not fail
                continue

        # handle finished clusters
        # for cluster in finished:
        #     meta = active.pop(cluster)
        #     ready.append(meta["output"])
        #     print(f"HADD FINISHED: cluster={cluster} -> {meta['output']}")
        #     # for f in meta.get("inputs", []):
        #     #     safe_unlink(Path(f))
        #     # safe-cleanup local submission artifacts (script/jdl)
        #     for f in meta.get("cleanup", [])[:2]:
        #         safe_unlink(Path(f))
        
        # Handle finished clusters with optional intermediate file cleanup:
        for cluster in finished:
            meta = active.pop(cluster)
            ready.append(meta["output"])
            print(f"HADD FINISHED: cluster={cluster} -> {meta['output']}")

            # optionally delete intermediate hadd inputs --- VERY conservative:
            # only delete inputs that live under HADD_DIR and when the feature flag is True.
            if DELETE_INTERMEDIATE_HADD:
                for f in meta.get("inputs", []):
                    try:
                        p = Path(f).resolve()
                        # compare parents using resolved absolute paths
                        if p.parent == HADD_DIR.resolve():
                            safe_unlink(p)
                    except Exception:
                        # be intentionally silent on failures to avoid breaking the pipeline
                        pass

            # cleanup local submission artifacts (script/jdl) but keep logs for debugging
            for f in meta.get("cleanup", [])[:2]:
                safe_unlink(Path(f))







        # status report
        print(f"[STATUS] ready={len(ready)} active={len(active)} failed_jobs={len(failed_jobs)}")
        print("=" * 10, time.strftime("%Y-%m-%d %H:%M:%S"), "=" * 10)

        if not dry_run:
            time.sleep(POLL_INTERVAL)
        else:
            break

    # final move (if successful)
    if not dry_run and len(ready) == 1:
        final = FINAL_DIR / OUTPUT_NAME
        ready[0].rename(final)
        print("\nFINAL OUTPUT:", final)
    elif dry_run:
        print("\nDRY RUN finished; no real HADD files produced.")
    else:
        print("\nPipeline finished but did not produce a single final file. Check failed_jobs and logs.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    main(args.dry_run)
