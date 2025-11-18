#!/bin/bash

PROCESS=$1
FILELIST=$2
FILES_PER_JOB=$3
OUTPUTDIR=$4
NJOBS=$5

echo "=== Job ${PROCESS} started at $(date) ==="
echo "Input file list: ${FILELIST}"
echo "Files per job: ${FILES_PER_JOB}"
echo "Output directory: ${OUTPUTDIR}"

# Setup CMSSW environment
source /cvmfs/cms.cern.ch/cmsset_default.sh
cd /afs/cern.ch/user/a/amalhotr/CMSSW_14_0_18_patch1/src
eval `scramv1 runtime -sh`

# Calculate which files this job should process
TOTAL_FILES=$(wc -l < ${FILELIST})
START_LINE=$((PROCESS * FILES_PER_JOB + 1))
END_LINE=$(((PROCESS + 1) * FILES_PER_JOB))

# Don't go past the end of the file
if [ ${END_LINE} -gt ${TOTAL_FILES} ]; then
    END_LINE=${TOTAL_FILES}
fi

echo "Processing lines ${START_LINE} to ${END_LINE} of ${TOTAL_FILES}"

# Extract the files for this job
CHUNK_FILE="/tmp/chunk_${PROCESS}_$$.txt"
sed -n "${START_LINE},${END_LINE}p" ${FILELIST} > ${CHUNK_FILE}

CHUNK_SIZE=$(wc -l < ${CHUNK_FILE})
echo "Chunk contains ${CHUNK_SIZE} files"

# Run hadd if chunk has files
if [ -s ${CHUNK_FILE} ]; then
    OUTPUT_FILE="${OUTPUTDIR}/chunk_${PROCESS}.root"
    echo "Running hadd to create ${OUTPUT_FILE}"
    
    # Use @ syntax if more than one file, direct if single file
    if [ ${CHUNK_SIZE} -eq 1 ]; then
        cp $(cat ${CHUNK_FILE}) ${OUTPUT_FILE}
        echo "Single file - copied directly"
    else
        hadd -f ${OUTPUT_FILE} @${CHUNK_FILE}
        HADD_STATUS=$?
        if [ ${HADD_STATUS} -ne 0 ]; then
            echo "ERROR: hadd failed with status ${HADD_STATUS}"
            rm -f ${CHUNK_FILE}
            exit 1
        fi
    fi
    
    if [ -f ${OUTPUT_FILE} ]; then
        SIZE=$(stat -f%z ${OUTPUT_FILE} 2>/dev/null || stat -c%s ${OUTPUT_FILE} 2>/dev/null)
        echo "Successfully created ${OUTPUT_FILE} (${SIZE} bytes)"
    else
        echo "ERROR: Output file not created"
        rm -f ${CHUNK_FILE}
        exit 1
    fi
else
    echo "ERROR: No files to process for job ${PROCESS}"
    rm -f ${CHUNK_FILE}
    exit 1
fi

# Cleanup
rm -f ${CHUNK_FILE}

echo "=== Job ${PROCESS} completed at $(date) ==="
exit 0
