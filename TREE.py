import FWCore.ParameterSet.Config as cms

process = cms.Process("TREE")

process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkSummary.reportEvery = 2000
process.MessageLogger.cerr.FwkReport.reportEvery = 2000
# Enable detailed debugging
process.MessageLogger.cerr.threshold = cms.untracked.string('DEBUG')
process.MessageLogger.debugModules = cms.untracked.vstring('scoutingTree')
process.MessageLogger.cerr.FwkReport.limit = cms.untracked.int32(100)  # Count processed events

process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(-1))

process.source = cms.Source("PoolSource",
    fileNames=cms.untracked.vstring('file://DY2M_VertexerOutput.root')
)

process.load("EventFilter.L1TRawToDigi.gtStage2Digis_cfi")
process.gtStage2Digis.InputLabel = cms.InputTag("hltFEDSelectorL1")

process.TFileService = cms.Service("TFileService",
    fileName=cms.string("DY2M_ScoutingTree_Output.root"),
    closeFileFast=cms.untracked.bool(True)
)

process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '133X_mcRun3_2024_realistic_v9', '')

L1Info = ["L1_HTT200er", "L1_HTT255er", "L1_HTT280er", "L1_HTT320er", "L1_HTT360er", "L1_HTT400er", "L1_HTT450er", "L1_ETT2000", "L1_SingleJet180", "L1_SingleJet200", "L1_DoubleJet30er2p5_Mass_Min250_dEta_Max1p5", "L1_DoubleJet30er2p5_Mass_Min300_dEta_Max1p5", "L1_DoubleJet30er2p5_Mass_Min330_dEta_Max1p5"]

process.scoutingTree = cms.EDAnalyzer('ScoutingTreeMakerRun3',
    required_ntk=cms.int32(2),
    triggerresults=cms.InputTag("TriggerResults", "", "HLT"),
    ReadPrescalesFromFile=cms.bool(False),
    AlgInputTag=cms.InputTag("gtStage2Digis"),
    l1tAlgBlkInputTag=cms.InputTag("gtStage2Digis"),
    l1tExtBlkInputTag=cms.InputTag("gtStage2Digis"),
    doL1=cms.bool(True),
    doPhiCorrection=cms.bool(False),
    luminosity=cms.double(108.96),
    crossSection=cms.double(1),
    l1Seeds=cms.vstring(L1Info),
    muons=cms.InputTag("hltScoutingMuonPacker"),
    electrons=cms.InputTag("hltScoutingEgammaPacker"),
    photons=cms.InputTag("hltScoutingEgammaPacker"),
    pfcands=cms.InputTag("hltScoutingPFPacker"),
    pfjets=cms.InputTag("hltScoutingPFPacker"),
    tracks=cms.InputTag("hltScoutingUnpackProducer", "Track"),
    primaryVertices=cms.InputTag("hltScoutingPrimaryVertexPacker", "primaryVtx"),
    displacedVertices=cms.InputTag("Vertexer"),
    pfMet=cms.InputTag("hltScoutingPFPacker", "pfMetPt"),
    pfMetPhi=cms.InputTag("hltScoutingPFPacker", "pfMetPhi"),
    rho=cms.InputTag("hltScoutingPFPacker", "rho"),
    beamspot_src=cms.InputTag('offlineBeamSpot'),
    genParticle_src=cms.InputTag("prunedGenParticles"),
    generatorName=cms.InputTag('generator')
)

process.options = cms.untracked.PSet(
    TryToContinue=cms.untracked.vstring('ProductNotFound')
)

process.p = cms.Path(process.gtStage2Digis + process.scoutingTree)