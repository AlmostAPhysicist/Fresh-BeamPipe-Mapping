# hello_world_cfg.py
import FWCore.ParameterSet.Config as cms

process = cms.Process("HELLO")

# -------------------- Logger --------------------
process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = 1

# -------------------- Options --------------------
process.options = cms.untracked.PSet(wantSummary = cms.untracked.bool(True))

# -------------------- Events --------------------
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(10))

# -------------------- Input --------------------
process.source = cms.Source(
    "PoolSource",
    fileNames = cms.untracked.vstring("root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_miniAOD_v2/250214_150834/0000/stop_dbar_miniAOD_5.root"),
    skipEvents = cms.untracked.uint32(0)
)

# -------------------- Output ROOT File --------------------
# process.TFileService = cms.Service(
#     "TFileService",
#     fileName = cms.string("output.root")
# )

# -------------------- Analyzer --------------------
process.demo = cms.EDAnalyzer(
    "HelloWorldAnalyzer",
    selectedEvents = cms.vint32(1, 2, 3, 4)
)

# -------------------- Path --------------------
process.p = cms.Path(process.demo)