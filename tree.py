# import FWCore.ParameterSet.Config as cms

# process = cms.Process("TREE")

# process.load("FWCore.MessageService.MessageLogger_cfi")

# process.MessageLogger.cerr.FwkSummary.reportEvery = 2000
# process.MessageLogger.cerr.FwkReport.reportEvery = 2000

# process.maxEvents = cms.untracked.PSet(
#     input = cms.untracked.int32(-1)
# )

# process.source = cms.Source("PoolSource",
#     fileNames = cms.untracked.vstring( 
#         # 'file:///eos/user/a/amalhotr/BeamGeo/CrabUnpackVertexerOutput.root'
#         'file://root://cms-xrd-global.cern.ch//store/user/amalhotr/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/DYto2Mu4Jets_Vertexer_100_EventAwareLumiBased/250328_003104/0000/DY2M_VertexerOutput_1.root'
#     )
# )

# process.load("EventFilter.L1TRawToDigi.gtStage2Digis_cfi")
# process.gtStage2Digis.InputLabel = cms.InputTag( "hltFEDSelectorL1" )

# process.TFileService = cms.Service("TFileService", 
#     # fileName = cms.string("/eos/user/a/amalhotr/BeamGeo/TrialCrabUnpackVertexertreeOutput.root")
#     # fileName = cms.string("root:///store/user/amalhotr/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/DYto2Mu4Jets_Vertexer_100_EventAwareLumiBased/250328_003104/0000/DY2M_VertexerOutput_1.root")
#     fileName = cms.string("DY2M_ScoutingTree_Output.root")
# )

# #process.ScoutingFilterPath = cms.Path(process.scoutingFilter)

# process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
# from Configuration.AlCa.GlobalTag import GlobalTag
# process.GlobalTag = GlobalTag(process.GlobalTag, '133X_mcRun3_2024_realistic_v9', '')  

# #The L1 seeds used for JetHT
# L1Info = ["L1_HTT200er", "L1_HTT255er", "L1_HTT280er", "L1_HTT320er", "L1_HTT360er", "L1_HTT400er", "L1_HTT450er", "L1_ETT2000", "L1_SingleJet180", "L1_SingleJet200", "L1_DoubleJet30er2p5_Mass_Min250_dEta_Max1p5", "L1_DoubleJet30er2p5_Mass_Min300_dEta_Max1p5", "L1_DoubleJet30er2p5_Mass_Min330_dEta_Max1p5"]

# #For CMSSW 13.3.0, the muon tag is hltScoutingMuonPacker, whilst for 14X it's hltScoutingMuonPackerNoVtx
# process.scoutingTree = cms.EDAnalyzer('ScoutingTreeMakerRun3',
#                                       required_ntk     = cms.int32(2),
#                                       triggerresults   = cms.InputTag("TriggerResults", "", "HLT"),
#                                       ReadPrescalesFromFile = cms.bool( False ),
#                                       AlgInputTag       = cms.InputTag("gtStage2Digis"),
#                                       l1tAlgBlkInputTag = cms.InputTag("gtStage2Digis"),
#                                       l1tExtBlkInputTag = cms.InputTag("gtStage2Digis"),
#                                       doL1 = cms.bool( True ),
#                                       doPhiCorrection = cms.bool( False ),
#                                       luminosity = cms.double(108.96), #2024 luminosity (fb-1)
#                                       crossSection = cms.double(1), # cross section in fb
#                                       l1Seeds           = cms.vstring(L1Info),
#                                       muons             = cms.InputTag("hltScoutingMuonPacker"),
#                                       electrons         = cms.InputTag("hltScoutingEgammaPacker"),
#                                       photons           = cms.InputTag("hltScoutingEgammaPacker"),
#                                       pfcands           = cms.InputTag("hltScoutingPFPacker"),
#                                       pfjets            = cms.InputTag("hltScoutingPFPacker"),
#                                       tracks            = cms.InputTag("hltScoutingUnpackProducer", "Track"), #for scouting tracks
#                                       #tracks            = cms.InputTag("generalTracks"), #for reco tracks
#                                       primaryVertices   = cms.InputTag("hltScoutingPrimaryVertexPacker","primaryVtx"),
#                                       displacedVertices = cms.InputTag("Vertexer"), 
#                                       pfMet             = cms.InputTag("hltScoutingPFPacker","pfMetPt"),
#                                       pfMetPhi          = cms.InputTag("hltScoutingPFPacker","pfMetPhi"),
#                                       rho               = cms.InputTag("hltScoutingPFPacker","rho"),
#                                       beamspot_src = cms.InputTag('offlineBeamSpot'),
#                                       genParticle_src = cms.InputTag("prunedGenParticles"),
#                                     #   trackingParticle_src = cms.InputTag('prunedTrackingParticles',''),
#                                       generatorName = cms.InputTag('generator'),
#                                   )

# process.options = cms.untracked.PSet(
#     TryToContinue = cms.untracked.vstring('ProductNotFound')
# )

# process.p = cms.Path(process.gtStage2Digis+process.scoutingTree)



# # import FWCore.ParameterSet.Config as cms
# # process = cms.Process("TREE")
# # process.load("FWCore.MessageService.MessageLogger_cfi")
# # process.MessageLogger.cerr.FwkReport.reportEvery = 1
# # process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(-1))
# # process.source = cms.Source("PoolSource",
# #     fileNames = cms.untracked.vstring('file:///eos/user/a/amalhotr/DY2M_ScoutedVertexer.root')
# # )
# # process.TFileService = cms.Service("TFileService", 
# #     fileName = cms.string("/eos/user/a/amalhotr/DY2M_ScoutedVertexerTreeDebug4.root")
# # )
# # process.scoutingTree = cms.EDAnalyzer('ScoutingTreeMakerRun3',
# #     required_ntk     = cms.int32(2),
# #     pfjets           = cms.InputTag("hltScoutingPFPacker", "", "HLT"),
# #     displacedVertices = cms.InputTag("Vertexer", "", "VertexProducer"),
# #     unpackVertices   = cms.InputTag("hltScoutingUnpackProducer", "PrimaryVertex", "TrackProducer3"),
# #     beamspot_src     = cms.InputTag("offlineBeamSpot", "", "VertexProducer")
# # )
# # process.p = cms.Path(process.scoutingTree)


# #--------------------------------------------------------------------------------

# # import FWCore.ParameterSet.Config as cms

# # process = cms.Process("TREE")

# # process.load("FWCore.MessageService.MessageLogger_cfi")

# # process.MessageLogger.cerr.FwkSummary.reportEvery = 1000
# # process.MessageLogger.cerr.FwkReport.reportEvery = 1000

# # process.maxEvents = cms.untracked.PSet(
# #     input = cms.untracked.int32(-1)
# # )

# # process.source = cms.Source("PoolSource",
# #     fileNames = cms.untracked.vstring( 
# #         'file:///eos/user/a/amalhotr/BeamGeo/CrabUnpackVertexerOutput.root'
# #     )
# # )

# # process.load("EventFilter.L1TRawToDigi.gtStage2Digis_cfi")
# # process.gtStage2Digis.InputLabel = cms.InputTag( "hltFEDSelectorL1" )

# # process.TFileService = cms.Service("TFileService", 
# #     fileName = cms.string("/eos/user/a/amalhotr/BeamGeo/CrabUnpackVertexertreeOutput.root")
# # )

# # #process.ScoutingFilterPath = cms.Path(process.scoutingFilter)
# # process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
# # process.load("Configuration.StandardSequences.MagneticField_cff")
# # process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
# # from Configuration.AlCa.GlobalTag import GlobalTag
# # process.GlobalTag = GlobalTag(process.GlobalTag, '133X_mcRun3_2024_realistic_v9', '')  
# # process.load("TrackingTools.TransientTrack.TransientTrackBuilder_cfi")

# # #The L1 seeds used for JetHT
# # L1Info = ["L1_HTT200er", "L1_HTT255er", "L1_HTT280er", "L1_HTT320er", "L1_HTT360er", "L1_HTT400er", "L1_HTT450er", "L1_ETT2000", "L1_SingleJet180", "L1_SingleJet200", "L1_DoubleJet30er2p5_Mass_Min250_dEta_Max1p5", "L1_DoubleJet30er2p5_Mass_Min300_dEta_Max1p5", "L1_DoubleJet30er2p5_Mass_Min330_dEta_Max1p5"]

# # #For CMSSW 13.3.0, the muon tag is hltScoutingMuonPacker, whilst for 14X it's hltScoutingMuonPackerNoVtx
# # process.scoutingTree = cms.EDAnalyzer('ScoutingTreeMakerRun3',
# #                                       required_ntk     = cms.int32(2),
# #                                       triggerresults   = cms.InputTag("TriggerResults", "", "HLT"),
# #                                       ReadPrescalesFromFile = cms.bool( False ),
# #                                       AlgInputTag       = cms.InputTag("gtStage2Digis"),
# #                                       l1tAlgBlkInputTag = cms.InputTag("gtStage2Digis"),
# #                                       l1tExtBlkInputTag = cms.InputTag("gtStage2Digis"),
# #                                       doL1 = cms.bool( True ),
# #                                       doPhiCorrection = cms.bool( False ),
# #                                       doGenMatching = cms.bool( False ),
# #                                       luminosity = cms.double(108.96), #2024 luminosity (fb-1)
# #                                       crossSection = cms.double(1), # cross section in fb
# #                                       l1Seeds           = cms.vstring(L1Info),
# #                                       muons             = cms.InputTag("hltScoutingMuonPacker"),
# #                                       electrons         = cms.InputTag("hltScoutingEgammaPacker"),
# #                                       photons           = cms.InputTag("hltScoutingEgammaPacker"),
# #                                       pfcands           = cms.InputTag("packedPFCandidates"),
# #                                       lostTracks        = cms.InputTag("lostTracks"),
# #                                       pfjets            = cms.InputTag("hltScoutingPFPacker"),
# #                                       tracks            = cms.InputTag("hltScoutingTrackPacker"), #for scouting tracks
# #                                       #tracks            = cms.InputTag("generalTracks"), #for reco tracks
# #                                       primaryVertices   = cms.InputTag("hltScoutingPrimaryVertexPacker","primaryVtx"),
# #                                       displacedVertices = cms.InputTag("Vertexer"), 
# #                                     #   pfMet             = cms.InputTag("hltScoutingPFPacker","pfMetPt"),
# #                                     #   pfMetPhi          = cms.InputTag("hltScoutingPFPacker","pfMetPhi"),
# #                                       rho               = cms.InputTag("hltScoutingPFPacker","rho"),
# #                                       beamspot_src = cms.InputTag('offlineBeamSpot'),
# #                                       genParticle_src = cms.InputTag("prunedGenParticles"),
# #                                       generatorName = cms.InputTag('generator'),
# #                                       trackRefs = cms.InputTag("hltScoutingUnpackProducer", "Track-RefToOriginal"),
# #                                   )

# # process.p = cms.Path(process.gtStage2Digis+process.scoutingTree)




# -------------------------

import FWCore.ParameterSet.Config as cms

process = cms.Process("TREE")

process.load("FWCore.MessageService.MessageLogger_cfi")

process.MessageLogger.cerr.FwkSummary.reportEvery = 1
process.MessageLogger.cerr.FwkReport.reportEvery = 1
process.MessageLogger.cerr.threshold = cms.untracked.string('DEBUG')
process.MessageLogger.debugModules = cms.untracked.vstring('scoutingTree')

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(100)
)

process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(
        'file://root://cms-xrd-global.cern.ch//store/user/amalhotr/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/DYto2Mu4Jets_Vertexer_100_EventAwareLumiBased/250328_003104/0000/DY2M_VertexerOutput_1.root'
    )
)

process.TFileService = cms.Service("TFileService",
    fileName = cms.string("DY2M_ScoutingTree_Output.root")
)

process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '133X_mcRun3_2024_realistic_v9', '')

process.scoutingTree = cms.EDAnalyzer('ScoutingTreeMakerRun3',
    required_ntk = cms.int32(2),
    displacedVertices = cms.InputTag("Vertexer"),
    beamspot_src = cms.InputTag("offlineBeamSpot"),
    tracks = cms.InputTag("hltScoutingUnpackProducer", "Track")
)

process.options = cms.untracked.PSet(
    TryToContinue = cms.untracked.vstring('ProductNotFound')
)

process.p = cms.Path(process.scoutingTree)