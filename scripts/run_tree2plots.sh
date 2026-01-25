#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
  echo "Usage: $0 <input_root_file> <output_root_file> [input_tree_path]"
  exit 1
fi

IN="$1"
OUT="$2"
TREE="${3:-scoutingTree/vertexTree}"

# Prefer XRootD path for EOS (batch nodes may not have /eos mounted)
to_xrd() {
  local p="$1"
  if [[ "$p" == /eos/user/* ]]; then
    echo "root://eosuser.cern.ch//$p"
  elif [[ "$p" == /eos/cms/* ]]; then
    echo "root://eoscms.cern.ch//$p"
  else
    echo "$p"
  fi
}

IN_XRD="$(to_xrd "$IN")"

# CMSSW environment
export VO_CMS_SW_DIR=/cvmfs/cms.cern.ch
source /cvmfs/cms.cern.ch/cmsset_default.sh

# Use absolute path to CMSSW base (passed as env var or hardcoded)
CMSSW_BASE="${CMSSW_BASE_PATH:-/afs/cern.ch/user/a/amalhotr/CMSSW_14_0_18_patch1}"
cd "$CMSSW_BASE/src"
eval "$(scramv1 runtime -sh)"

# Run
cd "$CMSSW_BASE/src/Run3ScoutingAnalysisTools"
echo "Running cmsRun with:"
echo "  inputFile  = $IN_XRD"
echo "  inputTree  = $TREE"
echo "  outputFile = $OUT"

mkdir -p "$(dirname "$OUT")"
cmsRun Tree2PlotsConfig_batch.py inputFile="$IN_XRD" inputTree="$TREE" outputFile="$OUT" maxEvents=-1
