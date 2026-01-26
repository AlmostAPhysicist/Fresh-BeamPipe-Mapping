#!/bin/bash
set -e

export VO_CMS_SW_DIR=/cvmfs/cms.cern.ch
source $VO_CMS_SW_DIR/cmsset_default.sh
cd /afs/cern.ch/user/a/amalhotr/CMSSW_14_0_18_patch1/src
eval `scramv1 runtime -sh`
cd -

cmsRun /afs/cern.ch/user/a/amalhotr/CMSSW_14_0_18_patch1/src/Run3ScoutingAnalysisTools/condor_tmp/Tree2PlotsConfig_adeb04dc.py
