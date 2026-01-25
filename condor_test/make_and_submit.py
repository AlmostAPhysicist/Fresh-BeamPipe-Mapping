import subprocess
from pathlib import Path

SH_FILE = Path("python_generated_job.sh")
SUBMIT = Path("python_generated_job.jdl")
LOGS = Path("logs")

LOGS.mkdir(exist_ok=True)

# --- generate shell script ---
SH_FILE.write_text(
    """#!/bin/bash

echo "Job started on $(hostname)"
echo "Process ID: $1"
echo "Cluster ID: $2"
echo "Proc ID: $3"

INPUT_FILE="/eos/user/a/amalhotr/condor_test/input.txt"
OUTPUT_FILE="/eos/user/a/amalhotr/condor_test/output.$2.$3.txt"

echo "Reading input file..."
INPUT_CONTENT=$(cat "$INPUT_FILE")

echo "Writing output file..."
echo "Input was:" > "$OUTPUT_FILE"
echo "$INPUT_CONTENT" >> "$OUTPUT_FILE"
echo "Processed by job $1" >> "$OUTPUT_FILE"

sleep 30

echo "Job finished successfully"
"""
)
SH_FILE.chmod(0o755) # make executable

# --- generate submit file ---
SUBMIT.write_text(
    f"""universe = vanilla

executable = python_generated_job.sh
arguments = $(Process) $(ClusterId) $(ProcId)

log    = logs/job.$(ClusterId).log
output = logs/job.$(ClusterId).$(Process).out
error  = logs/job.$(ClusterId).$(Process).err

customAttribute = myID

should_transfer_files = NO

+JobFlavour = "microcentury"

queue 5
"""
)

# --- submit ---
#out = subprocess.check_output(["condor_submit", str(SUBMIT)], text=True)
#print(out) 
# this works, and returns the standard output. But i Prefer 
bash_command = subprocess.run("condor_submit " + str(SUBMIT), shell=True, check=True, capture_output=True, text=True)
out = bash_command.stdout
print(out)

cluster_id = out.strip().split()[-1].strip(".")
print("Submitted ClusterId:", cluster_id)
