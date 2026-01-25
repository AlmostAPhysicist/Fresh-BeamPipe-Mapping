#!/usr/bin/env python3
"""
condor_status.py

Reusable HTCondor job status monitor.

Usage:

    from condor_status import check_cluster_status, watch_cluster

    # single check (returns dict)
    info = check_cluster_status(14374116)

    # watch until cluster finishes (returns dict with polls + final)
    result = watch_cluster(14374116, interval=5)

or from terminal:

    python condor_status.py 14374116
    python condor_status.py 14418664 watch 3
"""

import subprocess
from pathlib import Path
import time
from datetime import datetime
from typing import Optional, Callable, Dict, Any, List

# Default log directory (matches your setup)
LOG_DIR = Path("./logs")

# HTCondor JobStatus meanings
JOB_STATUS_MAP = {
    1: "IDLE",
    2: "RUNNING",
    3: "REMOVED",
    4: "COMPLETED",
    5: "HELD",
    6: "TRANSFERRING_OUTPUT",
    7: "SUSPENDED",
}


def run_command(cmd: List[str]) -> Optional[str]:
    """
    Runs a shell command safely and returns stdout as string.
    Returns None if command fails (non-zero exit).
    """
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            check=True,
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError:
        return None


def _now_iso() -> str:
    return datetime.now().isoformat()


def check_cluster_status(cluster_id: int) -> Dict[str, Any]:
    """
    Check cluster status and return a structured dictionary.

    Returns a dict with keys:
      - state: "active" | "finished" | "not_found"
      - timestamp: ISO timestamp of check
      - ...additional fields depending on state

    It also prints a human friendly summary.
    """
    print(f"\n=== Cluster {cluster_id} Status ===")
    ts = _now_iso()

    # -------------------------
    # ACTIVE JOBS (condor_q)
    # -------------------------
    q_out = run_command([
        "condor_q",
        str(cluster_id),
        "-af",
        "ProcId",
        "JobStatus"
    ])

    if q_out:
        idle = running = held = unknown = 0
        total = 0
        for line in q_out.splitlines():
            parts = line.split()
            if not parts:
                continue
            # assume last token is JobStatus
            try:
                status = int(parts[-1])
            except ValueError:
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

        data = {
            "state": "active",
            "timestamp": ts,
            "total": total,
            "idle": idle,
            "running": running,
            "held": held,
            "unknown": unknown,
        }

        print("ACTIVE JOBS:")
        print(f"  IDLE:    {idle}")
        print(f"  RUNNING: {running}")
        print(f"  HELD:    {held}")
        print(f"  UNKNOWN: {unknown}")
        print(f"  TOTAL:   {total}")
        return data

    # -------------------------
    # FINISHED JOBS (condor_history)
    # -------------------------
    hist_out = run_command([
        "condor_history",
        str(cluster_id),
        "-af",
        "ProcId",
        "ExitCode"
    ])

    if hist_out:
        success = failed = 0
        exitcodes = []
        for line in hist_out.splitlines():
            parts = line.split()
            if not parts:
                continue
            # Expect: "ProcId ExitCode" (two tokens)
            # If only ExitCode provided, handle that too.
            try:
                exitcode = int(parts[-1])
            except ValueError:
                continue
            exitcodes.append(exitcode)
            if exitcode == 0:
                success += 1
            else:
                failed += 1

        total = success + failed
        data = {
            "state": "finished",
            "timestamp": ts,
            "total": total,
            "success": success,
            "failed": failed,
            "exitcodes": exitcodes,
        }

        print("FINISHED JOBS:")
        print(f"  SUCCESS: {success}")
        print(f"  FAILED:  {failed}")
        print(f"  TOTAL:   {total}")
        return data

    # -------------------------
    # Not found anywhere
    # -------------------------
    print("No jobs found in queue or history.")
    return {"state": "not_found", "timestamp": ts}


def watch_cluster(cluster_id: int,
                  interval: int = 5,
                  on_complete: Optional[Callable[[Dict[str, Any]], None]] = None,
                  max_polls: Optional[int] = None) -> Dict[str, Any]:
    """
    Poll the cluster every `interval` seconds until it finishes.
    Returns a dict with:
      {
        "polls": [ {...}, {...}, ... ],  # each poll is same format as check_cluster_status
        "final": { ... }                 # final finished summary
      }

    Parameters:
      cluster_id: cluster number
      interval: seconds between polls
      on_complete: optional callback called with final summary dict when cluster finishes
      max_polls: optional limit to avoid infinite loops (None = no limit)
    """
    print(f"\n=== Watching Cluster {cluster_id} every {interval}s ===\n")
    polls: List[Dict[str, Any]] = []
    poll_count = 0

    while True:
        poll_count += 1
        info = check_cluster_status(cluster_id)
        # we append the raw info but also stamp poll index/time
        info_with_poll = {"poll": poll_count, "checked_at": _now_iso(), **info}
        polls.append(info_with_poll)

        # If cluster active, continue polling
        if info.get("state") == "active":
            # print short summary again for convenience
            total = info.get("total", 0)
            print(
                f"[ACTIVE] poll={poll_count} Total={total} | "
                f"IDLE={info.get('idle',0)} RUNNING={info.get('running',0)} HELD={info.get('held',0)}"
            )
        elif info.get("state") == "finished":
            final = info
            print("\n=== Cluster Finished ===")
            print(f"  SUCCESS: {final.get('success',0)}")
            print(f"  FAILED:  {final.get('failed',0)}")
            # call optional callback
            if on_complete:
                try:
                    on_complete(final)
                except Exception as e:
                    print(f"on_complete callback raised: {e}")
            result = {"polls": polls, "final": final}
            return result
        else:
            # not found in queue or history
            print("Cluster not found in queue or history (stopping watcher).")
            result = {"polls": polls, "final": info}
            return result

        # max polls guard (optional)
        if max_polls is not None and poll_count >= max_polls:
            print(f"Reached max_polls={max_polls}, exiting watcher.")
            return {"polls": polls, "final": {"state": "max_polls_reached"}}

        time.sleep(interval)


# convenience alias
wait_for_cluster = watch_cluster


# ----------------------------------------
# CLI Support
# ----------------------------------------
if __name__ == "__main__":
    import sys

    if len(sys.argv) < 2:
        print("Usage:")
        print("  python condor_status.py <cluster_id>")
        print("  python condor_status.py <cluster_id> watch [interval_seconds]")
        sys.exit(1)

    cid = int(sys.argv[1])

    if len(sys.argv) >= 3 and sys.argv[2] == "watch":
        interval = 5
        if len(sys.argv) >= 4 and sys.argv[3].isdigit():
            interval = int(sys.argv[3])
        # run watcher and exit when complete
        _res = watch_cluster(cid, interval=interval)
        # optionally: the CLI could write _res to file or print a summary here
    else:
        _ = check_cluster_status(cid)


# Example (from Python REPL):
# >>> from condor_status import watch_cluster
# >>> result = watch_cluster(14418664, interval=5)
# >>> # result["final"] has the final summary; result["polls"] is a list of poll dicts
# Example (from terminal):
# $ python condor_status.py 14418664 watch 3