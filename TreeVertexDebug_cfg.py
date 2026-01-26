import FWCore.ParameterSet.Config as cms

process = cms.Process("DEBUG")

process.source = cms.Source("EmptySource")

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(1)
)

process.vertexDebug = cms.EDAnalyzer(
    "TreeVertexDebug",
    inputFile = cms.string(
        "/eos/user/a/amalhotr/ScoutingPFRun3/ScoutingData_2024H_TreeMakerRedone/251209_202708/0000/ScoutingTree_Output_1.root"
    ),
    inputTree = cms.string("scoutingTree/vertexTree")
)

process.p = cms.Path(process.vertexDebug)
