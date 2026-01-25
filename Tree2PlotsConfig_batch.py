import FWCore.ParameterSet.Config as cms
import sys

# Simple command-line parsing
inputFile = "/eos/user/a/amalhotr/ScoutingPFRun3/ScoutingData_2024H_TreeMakerRedone/251209_202708/0000/ScoutingTree_Output_18.root"
inputTree = "scoutingTree/vertexTree"
outputFile = "outputs/TreeMakerData_test.root"

# Parse key=value arguments
for arg in sys.argv[1:]:
    if '=' in arg:
        key, val = arg.split('=', 1)
        if key == 'inputFile':
            inputFile = val
        elif key == 'inputTree':
            inputTree = val
        elif key == 'outputFile':
            outputFile = val

process = cms.Process("TREE2PLOTS")

process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkSummary.reportEvery = 100000
process.MessageLogger.cerr.FwkReport.reportEvery = 100000
process.MessageLogger.cerr.threshold = cms.untracked.string('INFO')
process.MessageLogger.debugModules = cms.untracked.vstring()

process.options = cms.untracked.PSet(
    wantSummary = cms.untracked.bool(True),
    TryToContinue = cms.untracked.vstring('ProductNotFound'),
)

process.source = cms.Source("EmptySource")
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(-1))

process.TFileService = cms.Service("TFileService",
    fileName = cms.string(outputFile)
)

process.tree2plots = cms.EDAnalyzer('Tree2PlotsRun3',
    inputFile = cms.string(inputFile),
    inputTree = cms.string(inputTree),

    cut_ntk = cms.VPSet(
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
)

process.p = cms.Path(process.tree2plots)
