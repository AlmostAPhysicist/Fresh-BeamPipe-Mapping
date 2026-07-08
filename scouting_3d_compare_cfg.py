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
    fileName=cms.string("GenScoutPlots_g8_Higgs.root"),
)

process.scouting3DCompare = cms.EDAnalyzer(
    "ScoutingComparisonLinePlotter",
    inputListFile      = cms.string(os.environ.get("INPUT_LIST", os.path.abspath("input_paths.txt"))),
    
    # 3D core spatial mapping
    xyDiffHistPath     = cms.untracked.string("scoutingComparer/xy_diff_1"),
    matchDistHistPath  = cms.untracked.string("scoutingComparer/match_distance_1"),
    
    # 2D heatmap variables fetched straight from GenScoutingComparer
    nSelVtxHistPath    = cms.untracked.string("scoutingComparer/n_selected_vertices"),
    massRecoHistPath   = cms.untracked.string("scoutingComparer/mass_reco_1"),
    rDistGenHistPath   = cms.untracked.string("scoutingComparer/rdist_xy_beamspot_gen_1"),
    vtxNtracksHistPath = cms.untracked.string("scoutingComparer/vtx_ntracks_1"),
)

process.p = cms.Path(process.scouting3DCompare)