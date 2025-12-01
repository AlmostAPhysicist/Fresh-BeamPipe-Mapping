import FWCore.ParameterSet.Config as cms

process = cms.Process("TREE2PLOTS")

process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkSummary.reportEvery = 1000
process.MessageLogger.cerr.FwkReport.reportEvery = 1000
process.MessageLogger.cerr.threshold = cms.untracked.string('INFO')
process.MessageLogger.debugModules = cms.untracked.vstring()

process.options = cms.untracked.PSet(
    wantSummary = cms.untracked.bool(True),
    TryToContinue = cms.untracked.vstring('ProductNotFound'),
)

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(1)  # Only need 1 "event" since we process TTree in beginJob
)

# ========================== INPUT: TTREE FILE ==========================
process.source = cms.Source("EmptySource")

# ========================== OUTPUT: HISTOGRAMS =========================
process.TFileService = cms.Service("TFileService",
    fileName = cms.string("test-outputs/Tree2Plots_Output_v2.root")
)

# ========================== TREE TO PLOTS ANALYZER =====================
# Reads TTree from ScoutingTreeMakerRun3 and makes same plots as ScoutingPlotMakerRun3
process.tree2plots = cms.EDAnalyzer('Tree2PlotsRun3',
    # Input TTree file and tree name
    inputFile = cms.string("test-outputs/ScoutingTreeTest_v2.root"),
    inputTree = cms.string("vertexTree"),
    
    # SAME CUTS AS UnifiedScoutingVertexingPlotsMaker.py
    cut_ntk = cms.VPSet(
        cms.PSet(values = cms.vint32(3)),      # Only ntk=3
        cms.PSet(values = cms.vint32(3, 4)),   # ntk=3 OR ntk=4
    ),
    
    cut_opening_angle_min = cms.vdouble(-1, 0.05, 0.1, 0.25, 0.5, 1.0),
    
    required_invmass = cms.double(1.0),
    required_chi2 = cms.double(-1),
    required_dBV_min = cms.double(-1),
    required_dBV_max = cms.double(-1),
    
    PVBoundary1 = cms.int32(20),
    PVBoundary2 = cms.int32(40),
)

process.p = cms.Path(process.tree2plots)
