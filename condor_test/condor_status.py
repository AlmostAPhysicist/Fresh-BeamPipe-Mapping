#!/usr/bin/env python3
"""
condor_status.py

HTCondor job status monitor.

Key features:
- check_cluster_status(cluster_id, log_folder=None) -> dict with consistent keys:
    state, timestamp, total, idle, running, completed, held, failed, exitcodes, unknown
- watch_cluster(cluster_id, interval=5, log_folder=None, on_complete=None, max_polls=None)
    polls every `interval` seconds; watch_cluster's 3rd argument after interval is log_folder.
- Prefers reading local log files first (fast) before calling condor_history (slow).
- CLI supports named args: python condor_status.py <cluster_id> [--watch] [--interval N] [--logdir PATH]
"""

from pathlib import Path
from typing import Optional, Callable, Dict, Any, List, Union
from datetime import datetime
import subprocess
import time
import re
import argparse
import json

# --- Default candidate log directories (searched in order) ---
DEFAULT_LOG_DIRS: List[Path] = [
    Path("./logs"),
    Path("/afs/cern.ch/user/a/amalhotr/CMSSW_14_0_18_patch1/src/Run3ScoutingAnalysisTools/condor_logs"),
    Path("/eos/user/a/amalhotr/condor_logs"),
]


# HTCondor JobStatus meanings (kept for reference)
JOB_STATUS_MAP = {
    1: "IDLE",
    2: "RUNNING",
    3: "REMOVED",
    4: "COMPLETED",
    5: "HELD",
    6: "TRANSFERRING_OUTPUT",
    7: "SUSPENDED",
}

# Retry settings for cluster status checks (tunable)
STATUS_RETRY_TIMEOUT = 120  # seconds
STATUS_RETRY_INTERVAL = 5  # seconds


# -------------------------
# Utility helpers
# -------------------------
def _now_iso() -> str:
    return datetime.now().isoformat()


def run_command(cmd: List[str], timeout: int = 8) -> Optional[str]:
    """Run command, return stdout (str) or None if failed/timeout."""
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, check=True, timeout=timeout)
        return r.stdout.strip()
    except subprocess.TimeoutExpired:
        return None
    except subprocess.CalledProcessError:
        return None
    except FileNotFoundError:
        return None


def find_log_file(cluster_id: Union[int, str], log_folders: Optional[List[Union[str, Path]]] = None) -> Optional[Path]:
    """
    Search for a log file containing the cluster id in a set of candidate folders.
    Returns the first matching Path or None if not found.

    Looks for:
      - job.<cluster>.log
      - <cluster>.log
      - any *.log that contains the cluster id
    """
    cid_full = str(cluster_id)
    cid = cid_full.split(".")[0]
    search_dirs = []

    if log_folders:
        for p in log_folders:
            search_dirs.append(Path(p).expanduser())
    search_dirs.extend(DEFAULT_LOG_DIRS)

    seen = set()
    for d in search_dirs:
        if not d:
            continue
        d = Path(d)
        if d in seen:
            continue
        seen.add(d)
        if not d.exists() or not d.is_dir():
            continue

        # Explicit names
        candidates = [
            d / f"job.{cid}.log",
            d / f"{cid}.log",
        ]
        for c in candidates:
            if c.exists():
                return c

        # More flexible: find any log file containing the cluster id in filename
        for p in d.glob(f"*{cid}*.log"):
            return p

    return None


def parse_condor_log(path: Path) -> Dict[str, Any]:
    """
    Heuristic parser for condor job log file.

    Returns:
      {
        "completed": int,   # heuristic count of job terminations with return code 0
        "failed": int,      # heuristic count of terminations with non-zero return
        "held": int,        # heuristic count of held events
        "exitcodes": List[int],  # collected numeric return codes seen
        "lines_analyzed": int
      }

    Note: condor logs vary in format. This is a robust heuristic:
      - collect all 'return value' / 'Return value' / 'return value' numbers
      - collect 'Normal termination (return value X)' variants
      - count 'Job was held' and 'Job was aborted' occurrences
      - fallback: count "Job terminated." occurrences (but this can be noisy)
    """
    text = ""
    try:
        text = path.read_text(errors="ignore")
    except Exception:
        return {"completed": 0, "failed": 0, "held": 0, "exitcodes": [], "lines_analyzed": 0}

    exitcodes: List[int] = []

    # Patterns that commonly appear in condor logs
    patterns = [
        re.compile(r"return value\s*[:=]?\s*([0-9]+)", re.IGNORECASE),
        re.compile(r"return code\s*[:=]?\s*([0-9]+)", re.IGNORECASE),
        re.compile(r"ExitCode[:=]?\s*([0-9]+)", re.IGNORECASE),
        re.compile(r"Normal termination.*return value\s*[:=]?\s*([0-9]+)", re.IGNORECASE),
        re.compile(r"Normal termination.*exit value\s*[:=]?\s*([0-9]+)", re.IGNORECASE),
    ]

    for pat in patterns:
        for m in pat.finditer(text):
            try:
                exitcodes.append(int(m.group(1)))
            except Exception:
                pass

    # Count held/aborted occurrences heuristically
    held = len(re.findall(r"Job was held", text, re.IGNORECASE))
    aborted = len(re.findall(r"Job was aborted", text, re.IGNORECASE))
    removed = len(re.findall(r"Job was removed", text, re.IGNORECASE))

    # If no exitcodes found, use "Job terminated." occurrences as an approximate completion count
    if not exitcodes:
        term_count = len(re.findall(r"Job terminated", text, re.IGNORECASE))
        # treat as completed (unknown exit code)
        completed = term_count
        failed = 0
    else:
        completed = sum(1 for c in exitcodes if c == 0)
        failed = sum(1 for c in exitcodes if c != 0)

    lines_analyzed = len(text.splitlines())
    return {
        "completed": completed,
        "failed": failed,
        "held": held + aborted,
        "exitcodes": exitcodes,
        "lines_analyzed": lines_analyzed,
    }


# -------------------------
# Core API
# -------------------------
def _check_cluster_status_once(cluster_id: str,
                               log_folder: Optional[Union[str, Path, List[Union[str, Path]]]] = None
                               ) -> Dict[str, Any]:
    """
    Single attempt at checking cluster status.
    DOES NOT retry.
    """
    ts = _now_iso()
    cid = str(cluster_id).split(".")[0]

    result = {
        "state": "not_found",
        "timestamp": ts,
        "total": 0,
        "idle": 0,
        "running": 0,
        "completed": 0,
        "held": 0,
        "failed": 0,
        "exitcodes": [],
        "unknown": 0,
    }

    # --- condor_q ---
    q_out = run_command(["condor_q", cid, "-af", "ProcId", "JobStatus"])
    if q_out and q_out.strip():
        idle = running = held = unknown = total = 0
        for line in q_out.splitlines():
            parts = line.strip().split()
            if not parts:
                continue
            try:
                status = int(parts[-1])
            except Exception:
                status = None
            total += 1
            if status == 1:
                idle += 1
            elif status == 2:
                running += 1
            elif status == 5:
                held += 1
            else:
                unknown += 1

        result.update({
            "state": "active",
            "total": total,
            "idle": idle,
            "running": running,
            "held": held,
            "unknown": unknown,
        })
        return result

    # --- logs ---
    log_dirs = None
    if log_folder:
        if isinstance(log_folder, (list, tuple)):
            log_dirs = [Path(p).expanduser() for p in log_folder]
        else:
            log_dirs = [Path(log_folder).expanduser()]

    log_path = find_log_file(cid, log_dirs)
    if log_path:
        parsed = parse_condor_log(log_path)
        completed = parsed.get("completed", 0)
        failed = parsed.get("failed", 0)
        exitcodes = parsed.get("exitcodes", [])
        result.update({
            "state": "finished",
            "total": completed + failed,
            "completed": completed,
            "failed": failed,
            "exitcodes": exitcodes,
        })
        return result

    # --- history ---
    hist_out = run_command(["condor_history", cid, "-af", "ProcId", "ExitCode"])
    if hist_out and hist_out.strip():
        exitcodes = []
        success = failed = 0
        for line in hist_out.splitlines():
            parts = line.strip().split()
            if not parts:
                continue
            try:
                code = int(parts[-1])
            except Exception:
                continue
            exitcodes.append(code)
            if code == 0:
                success += 1
            else:
                failed += 1

        result.update({
            "state": "finished",
            "total": success + failed,
            "completed": success,
            "failed": failed,
            "exitcodes": exitcodes,
        })
        return result

    return result


def check_cluster_status(cluster_id: Union[int, str],
                         log_folder: Optional[Union[str, Path, List[Union[str, Path]]]] = None
                         ) -> Dict[str, Any]:
    """
    Retry-aware cluster status check.
    Retries for up to STATUS_RETRY_TIMEOUT seconds before giving up.
    """
    start = time.time()
    cid = str(cluster_id).split(".")[0]

    while True:
        info = _check_cluster_status_once(cid, log_folder=log_folder)

        # Any meaningful signal -> return immediately
        if info["state"] in ("active", "finished"):
            # print human-friendly summary
            if info["state"] == "active":
                print(f"\n=== Cluster {cid} Status ===")
                print("ACTIVE JOBS:")
                print(f"  IDLE:    {info.get('idle', 0)}")
                print(f"  RUNNING: {info.get('running', 0)}")
                print(f"  HELD:    {info.get('held', 0)}")
                print(f"  UNKNOWN: {info.get('unknown', 0)}")
                print(f"  TOTAL:   {info.get('total', 0)}")
            else:
                # finished
                print(f"\n=== Cluster {cid} Status ===")
                print("FINISHED JOBS:")
                print(f"  COMPLETED: {info.get('completed', 0)}")
                print(f"  FAILED:    {info.get('failed', 0)}")
                if info.get('held', 0) > 0:
                    print(f"  HELD:      {info.get('held', 0)}")
                print(f"  TOTAL:     {info.get('total', 0)}")
            return info

        # Still nothing -> retry or timeout
        if time.time() - start >= STATUS_RETRY_TIMEOUT:
            raise TimeoutError(
                f"Cluster {cid}: no condor_q, log, or history info "
                f"after {STATUS_RETRY_TIMEOUT} seconds"
            )

        time.sleep(STATUS_RETRY_INTERVAL)


def watch_cluster(cluster_id: Union[int, str],
                  interval: int = 5,
                  log_folder: Optional[Union[str, Path, List[Union[str, Path]]]] = None,
                  on_complete: Optional[Callable[[Dict[str, Any]], None]] = None,
                  max_polls: Optional[int] = None) -> Dict[str, Any]:
    """
    Poll the cluster every `interval` seconds until it finishes.

    Arguments:
      cluster_id: cluster number
      interval: seconds between polls
      log_folder: optional path or list of paths to search for job logs (checked before condor_history)
                  (this is the third param after interval as you requested)
      on_complete: optional callable(final_summary_dict) fired when finished
      max_polls: optional maximum polls to avoid infinite loops

    Returns:
      {"polls": [poll_dicts], "final": final_summary_dict}
    """
    cid = str(cluster_id)
    print(f"\n=== Watching Cluster {cid} every {interval}s ===\n")
    polls: List[Dict[str, Any]] = []
    poll_count = 0

    while True:
        poll_count += 1
        info = check_cluster_status(cid, log_folder=log_folder)
        info_with_poll = {"poll": poll_count, "checked_at": _now_iso(), **info}
        polls.append(info_with_poll)

        if info.get("state") == "active":
            print(f"[ACTIVE] poll={poll_count} Total={info.get('total',0)} | "
                  f"IDLE={info.get('idle',0)} RUNNING={info.get('running',0)} HELD={info.get('held',0)}")
        elif info.get("state") == "finished":
            final = info
            print("\n=== Cluster Finished ===")
            print(f"  COMPLETED: {final.get('completed',0)}")
            print(f"  FAILED:    {final.get('failed',0)}")
            if on_complete:
                try:
                    on_complete(final)
                except Exception as e:
                    print(f"on_complete raised: {e}")
            return {"polls": polls, "final": final}
        else:
            # not found - stop and return the collected polls
            print("Cluster not found in queue/logs/history. Stopping watcher.")
            return {"polls": polls, "final": info}

        if max_polls is not None and poll_count >= max_polls:
            print(f"Reached max_polls={max_polls}. Exiting watcher.")
            return {"polls": polls, "final": {"state": "max_polls_reached"}}

        time.sleep(interval)


# convenience alias
wait_for_cluster = watch_cluster


# -------------------------
# CLI
# -------------------------
def _cli():
    parser = argparse.ArgumentParser(description="Condor cluster status monitor")
    parser.add_argument("cluster_id", help="Cluster id (e.g. 14418664)")
    parser.add_argument("--watch", action="store_true", help="Watch the cluster until it finishes")
    parser.add_argument("--interval", type=int, default=5, help="Polling interval in seconds (default: 5)")
    parser.add_argument("--logdir", help="Single log directory to check before condor_history")
    parser.add_argument("--max-polls", type=int, default=None, help="Max number of polls for watcher")
    parser.add_argument("--json-out", help="Write final output to JSON file (path)")
    args = parser.parse_args()

    cid = args.cluster_id
    logdir = args.logdir

    if args.watch:
        res = watch_cluster(cid, interval=args.interval, log_folder=logdir, max_polls=args.max_polls)
        final = res["final"]
        if args.json_out:
            try:
                Path(args.json_out).write_text(json.dumps(res, indent=2))
                print(f"Wrote watcher result to {args.json_out}")
            except Exception as e:
                print(f"Failed to write JSON: {e}")
    else:
        final = check_cluster_status(cid, log_folder=logdir)
        if args.json_out:
            try:
                Path(args.json_out).write_text(json.dumps(final, indent=2))
                print(f"Wrote result to {args.json_out}")
            except Exception as e:
                print(f"Failed to write JSON: {e}")


if __name__ == "__main__":
    _cli()
