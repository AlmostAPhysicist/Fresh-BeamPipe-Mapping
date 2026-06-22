import FWCore.ParameterSet.Config as cms

process = cms.Process("SCOUTINGCOMPARE")

# -------------------- Message Logger --------------------
process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = 1000

# -------------------- Processing Options --------------------
process.options = cms.untracked.PSet(
    wantSummary = cms.untracked.bool(True),
    numberOfThreads = cms.untracked.uint32(8),
    numberOfStreams = cms.untracked.uint32(0),
)

# -------------------- Histogram Output Target --------------------
process.TFileService = cms.Service(
    "TFileService",
    fileName = cms.string("gen_scouting_comparer_GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-55.root"),
)

# -------------------- Event Limits --------------------
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(100_000))

# -------------------- Input Source Pool --------------------

# Pick one block, then set:
# process.source = cms.Source("PoolSource", fileNames = cms.untracked.vstring(*ACTIVE_FILES))

M200_CT1 = [
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_miniAOD_v2/250214_150834/0000/stop_dbar_miniAOD_5.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_miniAOD_v2/250214_150834/0000/stop_dbar_miniAOD_3.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_miniAOD_v2/250214_150834/0000/stop_dbar_miniAOD_2.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_miniAOD_v2/250214_150834/0000/stop_dbar_miniAOD_6.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_miniAOD_v2/250214_150834/0000/stop_dbar_miniAOD_8.root",
]

M200_CT3 = [
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_miniAOD_v2/250214_150918/0000/stop_dbar_miniAOD_10.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_miniAOD_v2/250214_150918/0000/stop_dbar_miniAOD_8.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_miniAOD_v2/250214_150918/0000/stop_dbar_miniAOD_7.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_miniAOD_v2/250214_150918/0000/stop_dbar_miniAOD_3.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_miniAOD_v2/250214_150918/0000/stop_dbar_miniAOD_4.root",
]

M200_CT10 = [
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_miniAOD_v2/250214_144856/0000/stop_dbar_miniAOD_2.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_miniAOD_v2/250214_144856/0000/stop_dbar_miniAOD_8.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_miniAOD_v2/250214_144856/0000/stop_dbar_miniAOD_9.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_miniAOD_v2/250214_144856/0000/stop_dbar_miniAOD_1.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_miniAOD_v2/250214_144856/0000/stop_dbar_miniAOD_4.root",
]

M400_CT1 = [
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151149/0000/stop_dbar_miniAOD_10.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151149/0000/stop_dbar_miniAOD_8.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151149/0000/stop_dbar_miniAOD_4.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151149/0000/stop_dbar_miniAOD_5.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151149/0000/stop_dbar_miniAOD_9.root",
]

M400_CT3 = [
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151323/0000/stop_dbar_miniAOD_2.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151323/0000/stop_dbar_miniAOD_7.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151323/0000/stop_dbar_miniAOD_4.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151323/0000/stop_dbar_miniAOD_9.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151323/0000/stop_dbar_miniAOD_10.root",
]

M400_CT10 = [
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151028/0000/stop_dbar_miniAOD_2.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151028/0000/stop_dbar_miniAOD_4.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151028/0000/stop_dbar_miniAOD_6.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151028/0000/stop_dbar_miniAOD_7.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151028/0000/stop_dbar_miniAOD_9.root",
]

M600_CT1 = [
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151500/0000/stop_dbar_miniAOD_8.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151500/0000/stop_dbar_miniAOD_5.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151500/0000/stop_dbar_miniAOD_3.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151500/0000/stop_dbar_miniAOD_7.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151500/0000/stop_dbar_miniAOD_2.root",
]

M600_CT3 = [
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151548/0000/stop_dbar_miniAOD_7.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151548/0000/stop_dbar_miniAOD_9.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151548/0000/stop_dbar_miniAOD_8.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151548/0000/stop_dbar_miniAOD_4.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151548/0000/stop_dbar_miniAOD_3.root",
]

M600_CT10 = [
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151410/0000/stop_dbar_miniAOD_3.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151410/0000/stop_dbar_miniAOD_6.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151410/0000/stop_dbar_miniAOD_10.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151410/0000/stop_dbar_miniAOD_4.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151410/0000/stop_dbar_miniAOD_2.root",
]

M800_CT1 = [
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151734/0000/stop_dbar_miniAOD_9.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151734/0000/stop_dbar_miniAOD_5.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151734/0000/stop_dbar_miniAOD_10.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151734/0000/stop_dbar_miniAOD_8.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151734/0000/stop_dbar_miniAOD_2.root",
]

M800_CT3 = [
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151828/0000/stop_dbar_miniAOD_4.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151828/0000/stop_dbar_miniAOD_2.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151828/0000/stop_dbar_miniAOD_3.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151828/0000/stop_dbar_miniAOD_1.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151828/0000/stop_dbar_miniAOD_6.root",
]

M800_CT10 = [
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151643/0000/stop_dbar_miniAOD_3.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151643/0000/stop_dbar_miniAOD_10.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151643/0000/stop_dbar_miniAOD_7.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151643/0000/stop_dbar_miniAOD_8.root",
    "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151643/0000/stop_dbar_miniAOD_6.root",
]

# Example:
ACTIVE_FILES = M200_CT1


process.source = cms.Source(
    "PoolSource",
    fileNames = cms.untracked.vstring(
        # "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_miniAOD_v2/250214_150834/0000/stop_dbar_miniAOD_5.root"
        # "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_miniAOD_v2/250214_150918/0000/stop_dbar_miniAOD_10.root"
        # "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_miniAOD_v2/250214_144856/0000/stop_dbar_miniAOD_2.root"
        # "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151149/0000/stop_dbar_miniAOD_10.root"
        # "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151323/0000/stop_dbar_miniAOD_2.root"
        # "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151500/0000/stop_dbar_miniAOD_8.root"
        # "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151734/0000/stop_dbar_miniAOD_9.root"
        # "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-15_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/130000/8a1fca64-198c-4723-aa1b-c56277938c06.root"
        # "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/110000/8e2233eb-7fb3-4d8e-94f2-a66f9b24b63d.root"
        *ACTIVE_FILES
    ),
)

# -------------------- Scouting Unpacker Setup --------------------
process.hltScoutingUnpackProducer = cms.EDProducer(
    "HLTScoutingUnpackProducer",
    scoutingTrack = cms.InputTag("hltScoutingTrackPacker"),
    scoutingPrimaryVertex = cms.InputTag("hltScoutingPrimaryVertexPacker", "primaryVtx"),
    pfCand = cms.InputTag(""),
    lostTrack = cms.InputTag(""),
    isScouting = cms.bool(True),
    producePFCHSCandidate = cms.bool(False),
)

# Standard Geometry, Field and Global Tag Records
process.load("Configuration.StandardSequences.GeometryRecoDB_cff")
process.load("Configuration.StandardSequences.MagneticField_cff")
process.load("TrackingTools.TransientTrack.TransientTrackBuilder_cfi")
process.load("RecoVertex.BeamSpotProducer.BeamSpot_cfi")
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")

from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, "140X_mcRun3_2024_realistic_v26", "")

# -------------------- Vertexer Configuration --------------------
process.Vertexer = cms.EDProducer(
    "Vertexer",
    seed_tracks_src = cms.InputTag("hltScoutingUnpackProducer", "Track"),
    seed_jets_src = cms.InputTag("hltScoutingPFPacker"),
    primaryVertices = cms.InputTag("hltScoutingUnpackProducer", "PrimaryVertex"),
    trackToScoutingMap = cms.InputTag("hltScoutingUnpackProducer", "Track-RefToOriginal"),
    beamspot_src = cms.InputTag("offlineBeamSpot"),

    useOnlineBeamSpot = cms.untracked.bool(True),
    logBeamspotSource = cms.untracked.bool(False),

    minSeedPt = cms.untracked.double(0.9),
    minSeedIPSig = cms.untracked.double(4.0),
    maxSeedEta = cms.untracked.double(2.4),
    # minSeedPixelHits = cms.int32(2),#0
    # minSeedStripHits = cms.int32(1),#0
    # minSeedTrackerLayers = cms.int32(5),#0
    minSeedPixelHits = cms.int32(0),#0
    minSeedStripHits = cms.int32(0),#0
    minSeedTrackerLayers = cms.int32(0),#0

    # jet_pt_min = cms.double(30.0),#0
    # jet_eta_max = cms.double(5.0),#100
    # min_selected_jets = cms.int32(3),#0
    # jet_dr_max = cms.untracked.double(0.4),#100

    jet_pt_min = cms.double(0.0),#0
    jet_eta_max = cms.double(100),#100
    min_selected_jets = cms.int32(0),#0
    jet_dr_max = cms.untracked.double(1000),#100

    n_tracks_per_seed_vertex = cms.int32(2),
    max_seed_vertex_chi2 = cms.double(5.0),
    use_2d_vv_dist_forReco = cms.bool(False),
    use_2d_tv_dist_bsIPpresel = cms.bool(True),
    use_2d_tv_dist_forReco = cms.bool(False),

    merge_shared_dist = cms.double(-1.0),
    merge_shared_sig = cms.double(4.0),
    max_track_vertex_dist = cms.double(-1.0),
    max_track_vertex_sig = cms.double(5.0),
    min_track_vertex_sig_to_remove = cms.double(2.0),
    remove_one_track_at_a_time = cms.bool(True),

    max_nm1_refit_dist3 = cms.double(0.003),
    max_nm1_refit_distz = cms.double(-1.0),

    resolve_split_vertices_loose = cms.bool(True),
    merge_anyway_dist = cms.double(0.05),
    merge_anyway_sig = cms.double(3.0),

    resolve_split_vertices_tight = cms.bool(False),
    split_merge_dbv_min = cms.double(0.01),
    split_merge_dvv2d_max = cms.double(0.05),
    split_merge_dphi_max = cms.double(0.5),
    verbose = cms.bool(False),
)

# -------------------- Custom Analyzer Module --------------------
process.scoutingComparer = cms.EDAnalyzer(
    "GenScoutingComparer",
    genParticles = cms.InputTag("prunedGenParticles"),
    scoutingVertices = cms.InputTag("Vertexer"),
    beamspot = cms.InputTag("offlineBeamSpot"),
    scoutingJets = cms.InputTag("hltScoutingPFPacker"),
    trackToScoutingMap = cms.InputTag("hltScoutingUnpackProducer", "Track-RefToOriginal"),
    
    # Enforce global beamspot matching with Vertexer setup
    useOnlineBeamSpot = cms.untracked.bool(True),

    masshint = cms.untracked.double(MASS_HINT),
    decayhint = cms.untracked.double(DECAY_HINT),

    # Matching Configuration
    match_distance_mode = cms.untracked.string("2D"),

    # Signal Truth Decay Configuration
    # parent_pdgids = cms.untracked.vint32(1000006, -1000006),
    parent_pdgids = cms.untracked.vint32(9000006, -9000006),
    daughter_pdgids = cms.untracked.vint32(1, -1),

    # Offline Signal Region Definition Vertex Cuts
    vtx_chi2_max = cms.double(2.5),
    vtx_dbv_min = cms.double(0.01),
    vtx_dbv_max = cms.double(2.0),
    vtx_tracks_min = cms.uint32(2),#8
    vtx_ddbv_max = cms.double(0.005),
    vtx_cosT_min = cms.double(0.0),

    # Analysis Event Level Jet Gate Configuration
    jet_pt_min = cms.double(30),#0
    jet_eta_max = cms.double(2.5),#10
    min_selected_jets = cms.uint32(3),
    require_jet_selection = cms.untracked.bool(False),

    verbose = cms.untracked.bool(False),
    verbose_unselected = cms.untracked.bool(False),
)

# -------------------- Execution Path --------------------
process.p = cms.Path(
    process.hltScoutingUnpackProducer +
    process.offlineBeamSpot +
    process.Vertexer +
    process.scoutingComparer
)