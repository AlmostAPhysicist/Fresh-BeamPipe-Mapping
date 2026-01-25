#!/usr/bin/env python3
import argparse
import os
import subprocess
import time
from pathlib import Path

def to_xrd(p: str) -> str:
    if p.startswith("/eos/user/"):
        return f"root://eosuser.cern.ch//{p}"
    if p.startswith("/eos/cms/"):
        return f"root://eoscms.cern.ch//{p}"
    return p

def run(cmd, cwd=None):
    print("+", " ".join(cmd))
    return subprocess.check_output(cmd, cwd=cwd, text=True).strip()

def condor_wait(logfile):
    # Wait until all procs in this cluster finish
    run(["condor_wait", logfile])

def make_items(lines, items_path):
    with open(items_path, "w") as f:
        for line in lines:
            f.write(" ".join(line) + "\n")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--filelist", required=True, help="Text file with input ROOT file paths")
    ap.add_argument("--outdir", required=True, help="Output directory (EOS path allowed)")
    ap.add_argument("--batch-size", type=int, default=5, help="Jobs per wave (ideal 5)")
    ap.add_argument("--max-keep", type=int, default=10, help="Max number of output files kept on disk")
    ap.add_argument("--input-tree", default="scoutingTree/vertexTree", help="TTree path")
    ap.add_argument("--jobflavour", default="longlunch", help="HTCondor +JobFlavour")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    src_dir = Path(__file__).resolve().parents[1]
    condor_dir = src_dir / "condor"
    logs_dir = src_dir / "logs"
    waves_dir = condor_dir / "waves"
    items_dir = condor_dir / "items"
    submit_tpl = condor_dir / "tree2plots_wave.submit"

    for d in [logs_dir, waves_dir, items_dir]:
        d.mkdir(parents=True, exist_ok=True)

    # Create output directory if local
    if not args.outdir.startswith("/eos/"):
        Path(args.outdir).mkdir(parents=True, exist_ok=True)

    with open(args.filelist) as f:
        inputs = [l.strip() for l in f if l.strip() and not l.strip().startswith("#")]

    # Adjust batch size if max-keep < 2*batch
    batch_size = min(args.batch_size, max(1, args.max_keep // 2))

    # Prepare (input, output) pairs
    tasks = []
    for inp in inputs:
        base = os.path.basename(inp)
        if base.endswith(".root"):
            base = base[:-5]
        out = os.path.join(args.outdir, f"{base}_plots.root")
        tasks.append((inp, out))

    wave = 1
    idx = 0
    n = len(tasks)
    print(f"Total files: {n}, batch-size: {batch_size}, max-keep: {args.max_keep}")

    while idx < n:
        # Delete outputs from wave-2 to keep at most two waves
        if wave >= 3:
            old_list = waves_dir / f"wave_{wave-2:03d}.list"
            if old_list.exists():
                print(f"Deleting outputs from {old_list} to enforce max-keep...")
                with open(old_list) as f:
                    for out in f:
                        out = out.strip()
                        if out and os.path.exists(out):
                            try:
                                os.remove(out)
                                print(f"  rm {out}")
                            except Exception as e:
                                print(f"  warn: failed to remove {out}: {e}")

        # Prepare this wave
        batch = tasks[idx: idx + batch_size]
        idx += len(batch)

        # Save outputs of this wave
        this_wave_outs = waves_dir / f"wave_{wave:03d}.list"
        with open(this_wave_outs, "w") as f:
            for _, out in batch:
                f.write(out + "\n")

        # Make items file (arguments per proc)
        items_path = items_dir / f"wave_{wave:03d}.items"
        items_lines = []
        for inp, out in batch:
            items_lines.append([to_xrd(inp), out, args.input_tree, f"wave_{wave:03d}"])
        make_items(items_lines, items_path)

        # Create a temporary submit file for this wave with chosen flavour
        submit_file = condor_dir / f"wave_{wave:03d}.submit"
        with open(submit_file, "w") as sf, open(submit_tpl) as tpl:
            for line in tpl:
                if line.strip().startswith("+JobFlavour"):
                    sf.write(f'+JobFlavour     = "{args.jobflavour}"\n')
                elif line.strip().startswith("queue "):
                    sf.write(f"queue in, out, tree, wave from {items_path}\n")
                else:
                    sf.write(line)

        print(f"\n=== Submitting wave {wave} with {len(batch)} jobs ===")
        if not args.dry_run:
            out = run(["condor_submit", str(submit_file)])
            print(out)
            # Extract log path and wait
            log_file = logs_dir / f"wave_{wave:03d}.log"
            print(f"Waiting for wave {wave} to finish (condor_wait {log_file})")
            condor_wait(str(log_file))
            print(f"Wave {wave} completed!\n")
        else:
            print(f"DRY-RUN: would submit {submit_file}")

        wave += 1

    # Cleanup: after final wave, you can optionally delete previous wave outputs too
    print("\n=== All waves submitted and completed ===")
    print("Output files are in the latest two waves.")
    print(f"Check {args.outdir} for results.")

if __name__ == "__main__":
    main()
