import os
import FWCore.ParameterSet.Config as cms

process = cms.Process("SCOUTING3DCOMPARE")

process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = 1

process.options = cms.untracked.PSet(
    wantSummary     = cms.untracked.bool(True),
    numberOfThreads = cms.untracked.uint32(1),
    numberOfStreams = cms.untracked.uint32(0),
)

process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(1))
process.source    = cms.Source("EmptySource")

process.TFileService = cms.Service(
    "TFileService",
    fileName=cms.string("scouting_3d_comparison_fixed.root"),
)

process.scouting3DCompare = cms.EDAnalyzer(
    "ScoutingComparison3DPlotter",
    inputListFile     = cms.string(os.environ.get("INPUT_LIST", os.path.abspath("input_paths.txt"))),
    xyDiffHistPath    = cms.untracked.string("scoutingComparer/xy_diff_1"),
    matchDistHistPath = cms.untracked.string("scoutingComparer/match_distance_1"),
)

process.p = cms.Path(process.scouting3DCompare)