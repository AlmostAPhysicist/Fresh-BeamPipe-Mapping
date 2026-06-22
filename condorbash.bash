#!/bin/bash
set -e

SAMPLE_KEY=$1
CLUSTER_ID=$2
export SAMPLE_KEY

echo "=================================================="
echo "Starting Condor Job for Sample Key: ${SAMPLE_KEY}"
echo "Cluster ID Reference: ${CLUSTER_ID}"
echo "=================================================="

# Establish initial system default paths
source /cvmfs/cms.cern.ch/cmsset_default.sh

# 1. FIX: Switch folder back to core CMSSW area to build software paths cleanly
cd /afs/cern.ch/user/a/amalhotr/CMSSW_14_0_18_patch1/src
eval `scramv1 runtime -sh`

# 2. Navigate back down to execute your tool collection config
cd /afs/cern.ch/user/a/amalhotr/CMSSW_14_0_18_patch1/src/Run3ScoutingAnalysisTools

# Run your analyzer config over the sample target
cmsRun gen_scouting_comparer_cfg.py

echo "Job execution processing completed. Beginning clean-up and staging..."

# 3. FIX: Handle creating the output folder structure manually on EOS via XRDFS
EOS_DIR="/eos/user/a/amalhotr/Run3ScoutingAnalysisTools/condor_out/${CLUSTER_ID}"
xrdfs eosuser.cern.ch mkdir -p ${EOS_DIR}

# 4. FIX: Use xrdcp to ship the resulting file straight to your target EOS home folder
LOCAL_FILE="gen_scouting_comparer_${SAMPLE_KEY}.root"
echo "Staging ${LOCAL_FILE} to root://eosuser.cern.ch/${EOS_DIR}/${LOCAL_FILE}"

xrdcp -f ${LOCAL_FILE} root://eosuser.cern.ch/${EOS_DIR}/${LOCAL_FILE}

# 5. FIX: Wipe the file locally out of AFS so your user storage quota doesn't fill up
rm -f ${LOCAL_FILE}

echo "Job completed successfully."