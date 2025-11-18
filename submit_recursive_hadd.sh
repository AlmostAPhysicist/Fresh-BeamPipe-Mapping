#!/bin/bash

# Usage: ./submit_recursive_hadd.sh <start_line> <end_line> <files_per_job> <input_file>
# Example: ./submit_recursive_hadd.sh 1 500 5 outputs/dataRedone2024G.txt

START_LINE=$1
END_LINE=$2
FILES_PER_JOB=$3
INPUT_FILE=$4

if [ -z "$START_LINE" ] || [ -z "$END_LINE" ] || [ -z "$FILES_PER_JOB" ] || [ -z "$INPUT_FILE" ]; then
    echo "Usage: $0 <start_line> <end_line> <files_per_job> <input_file>"
    echo "Example: $0 1 500 5 outputs/dataRedone2024G.txt"
    exit 1
fi

BASEDIR="/afs/cern.ch/user/a/amalhotr/CMSSW_14_0_18_patch1/src/Run3ScoutingAnalysisTools"
cd ${BASEDIR}

# Setup directories
WORKDIR="${BASEDIR}/hadd_work_$(date +%Y%m%d_%H%M%S)"
LOGDIR="${WORKDIR}/logs"
mkdir -p ${LOGDIR}

LOGFILE="${WORKDIR}/master_log.txt"
SUMMARY="${WORKDIR}/summary.txt"

echo "=== Recursive HADD Job Started ===" | tee ${LOGFILE}
echo "Start time: $(date)" | tee -a ${LOGFILE}
echo "Input file: ${INPUT_FILE}" | tee -a ${LOGFILE}
echo "Lines to process: ${START_LINE} to ${END_LINE}" | tee -a ${LOGFILE}
echo "Files per job: ${FILES_PER_JOB}" | tee -a ${LOGFILE}
echo "Working directory: ${WORKDIR}" | tee -a ${LOGFILE}
echo "" | tee -a ${LOGFILE}

# Create initial file list for this range
CURRENT_LIST="${WORKDIR}/iteration_0_input.txt"
sed -n "${START_LINE},${END_LINE}p" ${BASEDIR}/${INPUT_FILE} > ${CURRENT_LIST}
TOTAL_FILES=$(wc -l < ${CURRENT_LIST})

echo "Total files to process: ${TOTAL_FILES}" | tee -a ${LOGFILE}
echo "" | tee -a ${LOGFILE}

ITERATION=0
FINAL_OUTPUT=""

while [ ${TOTAL_FILES} -gt 1 ]; do
    ITERATION=$((ITERATION + 1))
    echo "=== Iteration ${ITERATION} ===" | tee -a ${LOGFILE}
    echo "Input files: ${TOTAL_FILES}" | tee -a ${LOGFILE}
    
    NJOBS=$(((TOTAL_FILES + FILES_PER_JOB - 1) / FILES_PER_JOB))
    echo "Number of jobs: ${NJOBS}" | tee -a ${LOGFILE}
    
    ITER_OUTDIR="${WORKDIR}/iteration_${ITERATION}"
    mkdir -p ${ITER_OUTDIR}
    
    # Create JDL file for this iteration
    JDL_FILE="${WORKDIR}/submit_iter_${ITERATION}.jdl"
    cat > ${JDL_FILE} << EOF
universe = vanilla
log = ${LOGDIR}/iter${ITERATION}_log.\$(Process)
error = ${LOGDIR}/iter${ITERATION}_err.\$(Process)
output = ${LOGDIR}/iter${ITERATION}_out.\$(Process)
should_transfer_files = YES
transfer_output_files = ""
when_to_transfer_output = ON_EXIT_OR_EVICT
executable = ${BASEDIR}/condorbash.bash
RequestDisk = 5000000
arguments = \$(Process) ${CURRENT_LIST} ${FILES_PER_JOB} ${ITER_OUTDIR} ${NJOBS}
+JobFlavour = "tomorrow"
queue ${NJOBS}
EOF
    
    # Submit jobs
    echo "Submitting ${NJOBS} jobs..." | tee -a ${LOGFILE}
    CLUSTER_ID=$(condor_submit ${JDL_FILE} 2>&1 | grep "submitted to cluster" | awk '{print $6}' | tr -d '.')
    echo "Cluster ID: ${CLUSTER_ID}" | tee -a ${LOGFILE}
    
    # Wait for all jobs to complete
    echo "Waiting for jobs to complete..." | tee -a ${LOGFILE}
    WAIT_START=$(date +%s)
    
    while true; do
        RUNNING=$(condor_q ${CLUSTER_ID} -nobatch 2>/dev/null | grep -c "${CLUSTER_ID}")
        if [ ${RUNNING} -eq 0 ]; then
            break
        fi
        sleep 30
        ELAPSED=$(($(date +%s) - WAIT_START))
        echo "  Still running... (${ELAPSED}s elapsed, ${RUNNING} jobs in queue)" | tee -a ${LOGFILE}
    done
    
    WAIT_END=$(date +%s)
    DURATION=$((WAIT_END - WAIT_START))
    echo "Jobs completed in ${DURATION} seconds" | tee -a ${LOGFILE}
    
    # Check for output files and create file list for next iteration
    NEXT_LIST="${WORKDIR}/iteration_${ITERATION}_input.txt"
    ls ${ITER_OUTDIR}/chunk_*.root 2>/dev/null | sort -V > ${NEXT_LIST}
    
    OUTPUTS_CREATED=$(wc -l < ${NEXT_LIST})
    echo "Output files created: ${OUTPUTS_CREATED}" | tee -a ${LOGFILE}
    
    if [ ${OUTPUTS_CREATED} -eq 0 ]; then
        echo "ERROR: No output files created!" | tee -a ${LOGFILE}
        exit 1
    fi
    
    # Check for failed jobs
    EXPECTED_OUTPUTS=${NJOBS}
    if [ ${OUTPUTS_CREATED} -lt ${EXPECTED_OUTPUTS} ]; then
        echo "WARNING: Expected ${EXPECTED_OUTPUTS} outputs but got ${OUTPUTS_CREATED}" | tee -a ${LOGFILE}
    fi
    
    CURRENT_LIST=${NEXT_LIST}
    TOTAL_FILES=${OUTPUTS_CREATED}
    
    if [ ${TOTAL_FILES} -eq 1 ]; then
        FINAL_OUTPUT=$(cat ${NEXT_LIST})
        echo "Final output ready: ${FINAL_OUTPUT}" | tee -a ${LOGFILE}
    fi
    
    echo "" | tee -a ${LOGFILE}
done

# Move final output to outputs directory
FINAL_NAME="combined_output_lines_${START_LINE}_to_${END_LINE}_$(date +%Y%m%d_%H%M%S).root"
FINAL_PATH="${BASEDIR}/outputs/${FINAL_NAME}"
mv ${FINAL_OUTPUT} ${FINAL_PATH}

echo "=== Job Complete ===" | tee -a ${LOGFILE}
echo "End time: $(date)" | tee -a ${LOGFILE}
echo "Final output: ${FINAL_PATH}" | tee -a ${LOGFILE}
echo "" | tee -a ${LOGFILE}

# Generate summary
cat > ${SUMMARY} << EOF
=== HADD JOB SUMMARY ===
Job completed: $(date)

Input Configuration:
- Input file: ${INPUT_FILE}
- Line range: ${START_LINE} to ${END_LINE}
- Total input files: $(sed -n "${START_LINE},${END_LINE}p" ${BASEDIR}/${INPUT_FILE} | wc -l)
- Files per job: ${FILES_PER_JOB}

Processing Statistics:
- Total iterations: ${ITERATION}
- Working directory: ${WORKDIR}
- Final output: ${FINAL_PATH}

Iteration Breakdown:
EOF

for i in $(seq 1 ${ITERATION}); do
    ITER_DIR="${WORKDIR}/iteration_${i}"
    if [ -d "${ITER_DIR}" ]; then
        NUM_CHUNKS=$(ls ${ITER_DIR}/chunk_*.root 2>/dev/null | wc -l)
        echo "  Iteration ${i}: ${NUM_CHUNKS} output files" >> ${SUMMARY}
    fi
done

echo "" >> ${SUMMARY}
echo "Logs available in: ${LOGDIR}" >> ${SUMMARY}
echo "Master log: ${LOGFILE}" >> ${SUMMARY}

cat ${SUMMARY}
echo ""
echo "Summary written to: ${SUMMARY}"

# Cleanup intermediate files
echo "Cleaning up intermediate files..." | tee -a ${LOGFILE}
for i in $(seq 1 $((ITERATION - 1))); do
    ITER_DIR="${WORKDIR}/iteration_${i}"
    if [ -d "${ITER_DIR}" ]; then
        rm -rf ${ITER_DIR}
        echo "  Removed: ${ITER_DIR}" | tee -a ${LOGFILE}
    fi
done

# Keep the last iteration directory and logs
echo "Cleanup complete. Logs and final iteration preserved." | tee -a ${LOGFILE}
echo ""
echo "=== All Done! ===="
echo "Final output: ${FINAL_PATH}"
echo "Logs: ${LOGDIR}"
echo "Summary: ${SUMMARY}"
