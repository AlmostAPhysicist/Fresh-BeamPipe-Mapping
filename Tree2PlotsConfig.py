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

# Use EmptySource because we're not reading EDM events; framework maxEvents now controls
# how many TTree entries are processed (one TTree entry per analyze() call).
process.source = cms.Source("EmptySource")

# Control how many TTree entries to process with framework maxEvents:
# - set process.maxEvents.input to N to process N TTree entries (one TTree entry per analyze() call)
# - set to -1 to allow the analyzer to run until the TTree is exhausted
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(100000))  # set to N to limit entries


# Output
process.TFileService = cms.Service("TFileService",
    fileName = cms.string("test-outputs/Tree2Plots_Output_Size100k_incut.root")
)


process.tree2plots = cms.EDAnalyzer('Tree2PlotsRun3',
    # Path to the ROOT file and the TTree path inside it.
    # Example: the tree was saved under TDirectory "scoutingTree" as "vertexTree" ->
    # path = "scoutingTree/vertexTree".
    inputFile = cms.string("test-outputs/ScoutingTreeTest_Size100k_incut.root"),
    inputTree = cms.string("scoutingTree/vertexTree"),

    # Branching & cuts (match UnifiedScoutingVertexingPlotsMaker)
    cut_ntk = cms.VPSet(
        # cms.PSet(values = cms.vint32(2)),
        cms.PSet(values = cms.vint32(3)),
        cms.PSet(values = cms.vint32(3, 4)),
    ),
    cut_opening_angle_min = cms.vdouble(-1, 0.05, 0.1, 0.25, 0.5, 1.0),

    required_invmass = cms.double(2.0),
    required_chi2 = cms.double(-1),
    required_dBV_min = cms.double(-1),
    required_dBV_max = cms.double(-1),

    PVBoundary1 = cms.int32(20),
    PVBoundary2 = cms.int32(40),

    # NOTE: do NOT add an analyzer-internal 'maxEntries' here — use process.maxEvents instead.
)

process.p = cms.Path(process.tree2plots)
