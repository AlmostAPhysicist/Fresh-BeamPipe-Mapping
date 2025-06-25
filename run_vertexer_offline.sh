#!/bin/bash
set -e  # Exit immediately if a command exits with a non-zero status
set -x  # Enable debugging output

# Load CMS environment
source /cvmfs/cms.cern.ch/cmsset_default.sh

# Navigate to the working directory
cd /afs/cern.ch/user/a/amalhotr/CMSSW_14_0_18_patch1/src/Run3ScoutingAnalysisTools
cmsenv

# Initialize VOMS proxy
voms-proxy-init -voms cms -valid 24:00
export X509_USER_PROXY=/tmp/x509up_u$(id -u)

# Run the VertexerOffline.py file
cmsRun VertexerOffline.py