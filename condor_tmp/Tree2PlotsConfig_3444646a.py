import FWCore.ParameterSet.Config as cms

process = cms.Process("TREE2PLOTS")

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
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(1))  # analyzer loops over full trees


#---------
# INPUT 
# inputFile = "test-outputs/ScoutingTreeTest_Size100k_new_incut.root"
inputFile = "/eos/user/a/amalhotr/ScoutingPFRun3/ScoutingData_2024H_TreeMakerRedone/251209_202708/0000/ScoutingTree_Output_18.root"
inputFiles = [
    "/eos/user/a/amalhotr/ScoutingPFRun3/ScoutingData_2024H_TreeMakerRedone/251209_202708/0000/ScoutingTree_Output_18.root",
    "/eos/user/a/amalhotr/ScoutingPFRun3/ScoutingData_2024H_TreeMakerRedone/251209_202708/0000/ScoutingTree_Output_1.root",
    "/eos/user/a/amalhotr/ScoutingPFRun3/ScoutingData_2024H_TreeMakerRedone/251209_202708/0000/ScoutingTree_Output_2.root",
    "/eos/user/a/amalhotr/ScoutingPFRun3/ScoutingData_2024H_TreeMakerRedone/251209_202708/0000/ScoutingTree_Output_3.root",
    "/eos/user/a/amalhotr/ScoutingPFRun3/ScoutingData_2024H_TreeMakerRedone/251209_202708/0000/ScoutingTree_Output_4.root",
    ]

inputTree = "scoutingTree/vertexTree"
#---------
# OUTPUT
# outputFile = "test-outputs/Tree2Plots_Output_Size100k_new_incut.root"
outputFile = "outputs/TreeMakerData_test_multipleFiles.root"
process.TFileService = cms.Service("TFileService",
    fileName = cms.string("/eos/user/a/amalhotr/tree2plots_streaming/primary/plot_primary_3444646a.root")
)
#--------

process.tree2plots = cms.EDAnalyzer('Tree2PlotsRun3',
    # Path to the ROOT file and the TTree path inside it.
    # Example: the tree was saved under TDirectory "scoutingTree" as "vertexTree" ->
    # path = "scoutingTree/vertexTree".
    # inputFile = cms.string(inputFile),
    inputFiles = cms.vstring('/eos/user/a/amalhotr/ScoutingPFRun3/ScoutingData_2024H_TreeMakerRedone/251209_202708/0000/ScoutingTree_Output_119.root', '/eos/user/a/amalhotr/ScoutingPFRun3/ScoutingData_2024H_TreeMakerRedone/251209_202708/0000/ScoutingTree_Output_11.root', '/eos/user/a/amalhotr/ScoutingPFRun3/ScoutingData_2024H_TreeMakerRedone/251209_202708/0000/ScoutingTree_Output_120.root', '/eos/user/a/amalhotr/ScoutingPFRun3/ScoutingData_2024H_TreeMakerRedone/251209_202708/0000/ScoutingTree_Output_121.root'),
    inputTree = cms.string(inputTree),

    # Branching & cuts (match UnifiedScoutingVertexingPlotsMaker)
    cut_ntk = cms.VPSet(
        # cms.PSet(values = cms.vint32(2)),
        cms.PSet(values = cms.vint32(3)),
        cms.PSet(values = cms.vint32(3, 4)),
    ),
    cut_opening_angle_min = cms.vdouble(-1, 0.05, 0.1, 0.25, 0.5, 1.0),

    required_invmass = cms.double(2.0),
    required_chi2_max = cms.double(-1),
    required_chi2norm_max = cms.double(-1),
    required_dBV_min = cms.double(-1),
    required_dBV_max = cms.double(-1),

    PVBoundary1 = cms.int32(20),
    PVBoundary2 = cms.int32(40),
    progressEveryPercent = cms.untracked.int32(10),
    verbose = cms.untracked.bool(True),
)

process.p = cms.Path(process.tree2plots)
