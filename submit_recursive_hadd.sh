#!/bin/bash

# Usage: ./submit_recursive_hadd.sh <start_line> <end_line> <files_per_job> <input_file>
# Example: ./submit_recursive_hadd.sh 1 500 5 outputs/dataRedone2024G.txt
# Use -1 for defaults: ./submit_recursive_hadd.sh -1 -1 -1 outputs/dataRedone2024G.txt

START_LINE=$1
END_LINE=$2
FILES_PER_JOB=$3
INPUT_FILE=$4

if [ -z "$START_LINE" ] || [ -z "$END_LINE" ] || [ -z "$FILES_PER_JOB" ] || [ -z "$INPUT_FILE" ]; then
    echo "Usage: $0 <start_line> <end_line> <files_per_job> <input_file>"
    echo "Example: $0 1 500 5 outputs/dataRedone2024G.txt"
    echo "Use -1 for defaults: $0 -1 -1 -1 outputs/dataRedone2024G.txt"
    exit 1
fi

BASEDIR="/afs/cern.ch/user/a/amalhotr/CMSSW_14_0_18_patch1/src/Run3ScoutingAnalysisTools"
cd ${BASEDIR}

# Get total lines in input file
TOTAL_LINES=$(wc -l < ${BASEDIR}/${INPUT_FILE})

# Handle -1 defaults
if [ ${START_LINE} -eq -1 ]; then
    START_LINE=1
    echo "Using default start line: 1"
fi

if [ ${END_LINE} -eq -1 ]; then
    END_LINE=${TOTAL_LINES}
    echo "Using default end line: ${TOTAL_LINES} (entire file)"
fi

if [ ${FILES_PER_JOB} -eq -1 ]; then
    FILES_PER_JOB=100
    echo "Using default batch size: 100"
fi

# Setup directories in AFS
WORKDIR="${BASEDIR}/hadd_work_$(date +%Y%m%d_%H%M%S)"
mkdir -p ${WORKDIR}

LOGFILE="${WORKDIR}/logNsummary.txt"

echo "=== Recursive HADD Job Started ===" | tee ${LOGFILE}
echo "Start time: $(date)" | tee -a ${LOGFILE}
echo "Input file: ${INPUT_FILE}" | tee -a ${LOGFILE}
echo "Total lines in file: ${TOTAL_LINES}" | tee -a ${LOGFILE}
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
    ITER_LOGFILE="${WORKDIR}/iteration_${ITERATION}.log"
    mkdir -p ${ITER_OUTDIR}
    
    # Create JDL file for this iteration - single log file per iteration
    JDL_FILE="${WORKDIR}/submit_iter_${ITERATION}.jdl"
    cat > ${JDL_FILE} << EOF
universe = vanilla
log = ${ITER_LOGFILE}
error = /dev/null
output = /dev/null
should_transfer_files = YES
transfer_output_files = ""
when_to_transfer_output = ON_EXIT_OR_EVICT
executable = ${BASEDIR}/condorbash.bash
RequestDisk = 5000000
RequestMemory = 2000
arguments = \$(Process) ${CURRENT_LIST} ${FILES_PER_JOB} ${ITER_OUTDIR} ${NJOBS}
+JobFlavour = "workday"
queue ${NJOBS}
EOF
    
    # Submit jobs
    echo "Submitting ${NJOBS} jobs..." | tee -a ${LOGFILE}
    SUBMIT_OUTPUT=$(condor_submit ${JDL_FILE} 2>&1)
    SUBMIT_STATUS=$?
    
    if [ ${SUBMIT_STATUS} -ne 0 ]; then
        echo "ERROR: condor_submit failed with exit code ${SUBMIT_STATUS}" | tee -a ${LOGFILE}
        echo "${SUBMIT_OUTPUT}" | tee -a ${LOGFILE}
        exit 1
    fi
    
    # Extract cluster ID
    CLUSTER_ID=$(echo "${SUBMIT_OUTPUT}" | grep -oP 'cluster \K[0-9]+' | head -1)
    
    if [ -z "${CLUSTER_ID}" ]; then
        CLUSTER_ID=$(echo "${SUBMIT_OUTPUT}" | grep "submitted to cluster" | grep -oP '[0-9]+' | head -1)
    fi
    
    if [ -z "${CLUSTER_ID}" ]; then
        echo "ERROR: Failed to extract cluster ID!" | tee -a ${LOGFILE}
        echo "${SUBMIT_OUTPUT}" | tee -a ${LOGFILE}
        exit 1
    fi
    
    echo "Cluster ID: ${CLUSTER_ID}" | tee -a ${LOGFILE}
    
    # Wait for all jobs to complete
    echo "Waiting for jobs to complete..." | tee -a ${LOGFILE}
    WAIT_START=$(date +%s)
    sleep 5
    
    PREV_RUNNING=-1
    while true; do
        JOB_COUNT=$(condor_q ${CLUSTER_ID} -nobatch 2>/dev/null | grep "^${CLUSTER_ID}\." | wc -l)
        
        if [ ${JOB_COUNT} -eq 0 ]; then
            sleep 10
            JOB_COUNT=$(condor_q ${CLUSTER_ID} -nobatch 2>/dev/null | grep "^${CLUSTER_ID}\." | wc -l)
            if [ ${JOB_COUNT} -eq 0 ]; then
                echo "All jobs completed" | tee -a ${LOGFILE}
                break
            fi
        fi
        
        if [ ${JOB_COUNT} -ne ${PREV_RUNNING} ]; then
            ELAPSED=$(($(date +%s) - WAIT_START))
            IDLE=$(condor_q ${CLUSTER_ID} -af JobStatus 2>/dev/null | grep -c "^1$")
            RUNNING=$(condor_q ${CLUSTER_ID} -af JobStatus 2>/dev/null | grep -c "^2$")
            HELD=$(condor_q ${CLUSTER_ID} -af JobStatus 2>/dev/null | grep -c "^5$")
            
            echo "  [${ELAPSED}s] Total: ${JOB_COUNT} | Idle: ${IDLE} | Running: ${RUNNING} | Held: ${HELD}" | tee -a ${LOGFILE}
            PREV_RUNNING=${JOB_COUNT}
        fi
        
        sleep 30
    done
    
    WAIT_END=$(date +%s)
    DURATION=$((WAIT_END - WAIT_START))
    echo "Jobs completed in ${DURATION} seconds" | tee -a ${LOGFILE}
    
    # Check for output files
    NEXT_LIST="${WORKDIR}/iteration_${ITERATION}_input.txt"
    ls ${ITER_OUTDIR}/chunk_*.root 2>/dev/null | sort -V > ${NEXT_LIST}
    
    OUTPUTS_CREATED=$(wc -l < ${NEXT_LIST})
    echo "Output files created: ${OUTPUTS_CREATED}" | tee -a ${LOGFILE}
    
    if [ ${OUTPUTS_CREATED} -eq 0 ]; then
        echo "ERROR: No output files created!" | tee -a ${LOGFILE}
        exit 1
    fi
    
    if [ ${OUTPUTS_CREATED} -lt ${NJOBS} ]; then
        echo "WARNING: Expected ${NJOBS} outputs but got ${OUTPUTS_CREATED}" | tee -a ${LOGFILE}
    fi
    
    CURRENT_LIST=${NEXT_LIST}
    TOTAL_FILES=${OUTPUTS_CREATED}
    
    if [ ${TOTAL_FILES} -eq 1 ]; then
        FINAL_OUTPUT=$(cat ${NEXT_LIST})
        echo "Final output ready: ${FINAL_OUTPUT}" | tee -a ${LOGFILE}
    fi
    
    # Delete previous iteration (but NEVER iteration 0 which points to source files)
    if [ ${ITERATION} -gt 1 ]; then
        PREV_ITER=$((ITERATION - 1))
        PREV_DIR="${WORKDIR}/iteration_${PREV_ITER}"
        if [ -d "${PREV_DIR}" ]; then
            echo "Cleaning up iteration ${PREV_ITER}..." | tee -a ${LOGFILE}
            rm -rf ${PREV_DIR}
            echo "  Removed: ${PREV_DIR}" | tee -a ${LOGFILE}
        fi
    fi
    
    echo "" | tee -a ${LOGFILE}
done

# Move final output to outputs directory
FINAL_NAME="combined_output_lines_${START_LINE}_to_${END_LINE}_$(date +%Y%m%d_%H%M%S).root"
FINAL_PATH="${BASEDIR}/outputs/${FINAL_NAME}"
echo "Moving final output to outputs directory..." | tee -a ${LOGFILE}
mv ${FINAL_OUTPUT} ${FINAL_PATH}

if [ ! -f ${FINAL_PATH} ]; then
    echo "ERROR: Failed to move final output!" | tee -a ${LOGFILE}
    exit 1
fi

echo "" | tee -a ${LOGFILE}
echo "=== Job Complete ===" | tee -a ${LOGFILE}
echo "End time: $(date)" | tee -a ${LOGFILE}
echo "" | tee -a ${LOGFILE}

# Add summary section to same file
echo "======================================" | tee -a ${LOGFILE}
echo "=== SUMMARY ===" | tee -a ${LOGFILE}
echo "======================================" | tee -a ${LOGFILE}
echo "" | tee -a ${LOGFILE}
echo "Input Configuration:" | tee -a ${LOGFILE}
echo "  - Input file: ${INPUT_FILE}" | tee -a ${LOGFILE}
echo "  - Line range: ${START_LINE} to ${END_LINE}" | tee -a ${LOGFILE}
echo "  - Total input files processed: ${TOTAL_FILES}" | tee -a ${LOGFILE}
echo "  - Files per job: ${FILES_PER_JOB}" | tee -a ${LOGFILE}
echo "" | tee -a ${LOGFILE}
echo "Processing Statistics:" | tee -a ${LOGFILE}
echo "  - Total iterations: ${ITERATION}" | tee -a ${LOGFILE}
echo "  - Working directory: ${WORKDIR}" | tee -a ${LOGFILE}
echo "" | tee -a ${LOGFILE}
echo "Iteration Breakdown:" | tee -a ${LOGFILE}

for i in $(seq 1 ${ITERATION}); do
    ITER_LOG="${WORKDIR}/iteration_${i}.log"
    if [ -f "${ITER_LOG}" ]; then
        JOB_COUNT=$(grep -c "Job submitted" ${ITER_LOG} 2>/dev/null || echo "0")
        echo "  - Iteration ${i}: ${JOB_COUNT} jobs submitted" | tee -a ${LOGFILE}
    fi
done

echo "" | tee -a ${LOGFILE}
echo "Final Output:" | tee -a ${LOGFILE}
echo "  - Location: ${FINAL_PATH}" | tee -a ${LOGFILE}
echo "" | tee -a ${LOGFILE}

# Copy combined log to outputs directory
FINAL_LOGNSUMMARY="${BASEDIR}/outputs/logNsummary_lines_${START_LINE}_to_${END_LINE}_$(date +%Y%m%d_%H%M%S).txt"
cp ${LOGFILE} ${FINAL_LOGNSUMMARY}

echo "Combined log and summary: ${FINAL_LOGNSUMMARY}" | tee -a ${LOGFILE}
echo "" | tee -a ${LOGFILE}

# Cleanup work directory
echo "Cleaning up work directory..." | tee -a ${LOGFILE}
LAST_ITER_DIR="${WORKDIR}/iteration_${ITERATION}"
rm -rf ${LAST_ITER_DIR}
rm -f ${WORKDIR}/iteration_*.log
rm -f ${WORKDIR}/submit_iter_*.jdl
rm -f ${WORKDIR}/iteration_*_input.txt
rm -rf ${WORKDIR}

echo "" | tee -a ${FINAL_LOGNSUMMARY}
echo "=== All Done! ====" | tee -a ${FINAL_LOGNSUMMARY}
echo "Final output: ${FINAL_PATH}" | tee -a ${FINAL_LOGNSUMMARY}
echo "Log and summary: ${FINAL_LOGNSUMMARY}" | tee -a ${FINAL_LOGNSUMMARY}
