# import FWCore.ParameterSet.Config as cms

# process = cms.Process("TrackProducer3")

# process.load("FWCore.MessageService.MessageLogger_cfi")
# process.options = cms.untracked.PSet(
#     wantSummary = cms.untracked.bool(True)
# )
# # process.MessageLogger.cerr.FwkSummary.reportEvery = 100
# process.MessageLogger.cerr.FwkReport.reportEvery = 1000

# process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(-1) )

# process.source = cms.Source("PoolSource",
#     # Test file generated on CMSSW 13.3.0
#     fileNames = cms.untracked.vstring( 'file:root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2560000/ad749229-3094-410f-94e3-2e408518e395.root' #add test file here
#     )
# )

# #Choosing the GlobalTag
# process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
# from Configuration.AlCa.GlobalTag import GlobalTag
# process.GlobalTag = GlobalTag(process.GlobalTag, '133X_mcRun3_2024_realistic_v9', '')  

# # Input tags to the EDProducer
# process.hltScoutingUnpackProducer = cms.EDProducer('HLTScoutingUnpackProducer',
#   scoutingTrack = cms.InputTag('hltScoutingTrackPacker'),
#   scoutingPrimaryVertex = cms.InputTag("hltScoutingPrimaryVertexPacker","primaryVtx"),
#   producePFCHSCandidate = cms.bool(False),
#   mightGet = cms.optional.untracked.vstring
# )

# # Save only the scouting collections on the output file
# process.out = cms.OutputModule("PoolOutputModule",
#     fileName = cms.untracked.string('/eos/user/a/amalhotr/DY2M_ScoutDefault.root'),
#     outputCommands = cms.untracked.vstring('drop *', 'keep *_hltGtStage2ObjectMap_*_*', 'keep *_TriggerResults_*_*', 'keep *_hltFEDSelectorL1_*_*', 'keep *_hltScouting*_*_*', 'keep *_genParticles_*_*', 'keep *_prunedTrackingParticles_*_*','keep GenEventInfoProduct_*_*_*')
# )

# # Usually it is better to put producers on a task instead of a path
# # but paths also work.
# process.p = cms.Path(process.hltScoutingUnpackProducer)
# process.e = cms.EndPath(process.out)


#--------------------------------------------------------------------------------

import FWCore.ParameterSet.Config as cms

process = cms.Process("TrackProducer")

process.load("FWCore.MessageService.MessageLogger_cfi")
process.options = cms.untracked.PSet(
    wantSummary = cms.untracked.bool(True)
)

process.MessageLogger.cerr.FwkSummary.reportEvery = 100
process.MessageLogger.cerr.FwkReport.reportEvery = 100

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(1000) )  # Limited events for testing

process.source = cms.Source("PoolSource",
    # Using the same scouting data file you were testing with
    fileNames = cms.untracked.vstring(
        # "root://cmsxrootd.fnal.gov//store/data/Run2024H/ScoutingPFRun3/HLTSCOUT/v1/000/385/836/00000/003ca643-43f8-40dd-92b3-4c6a4ccdc894.root"
        "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v3/110000/017d2bf5-4f3c-40ab-b6f9-c42381b6ae9b.root"

    )
)

#Choosing the GlobalTag
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '140X_mcRun3_2024_realistic_v26', '')  

# Input tags to the EDProducer
process.hltScoutingUnpackProducer = cms.EDProducer('HLTScoutingUnpackProducer',
  scoutingTrack = cms.InputTag('hltScoutingTrackPacker'),
  scoutingPrimaryVertex = cms.InputTag("hltScoutingPrimaryVertexPacker","primaryVtx"),
  pfCand = cms.InputTag(""),  # Add missing parameters
  lostTrack = cms.InputTag(""),  # Add missing parameters
  isScouting = cms.bool(True),  # Add missing parameter - this is crucial!
  producePFCHSCandidate = cms.bool(False),
  mightGet = cms.optional.untracked.vstring
)

# Save only the scouting collections on the output file
process.out = cms.OutputModule("PoolOutputModule",
    fileName = cms.untracked.string('test-outputs/HLTUnpackerScouting_test.root'),
    # outputCommands = cms.untracked.vstring(
    #     'drop *', 
    #     'keep *_hltGtStage2ObjectMap_*_*', 
    #     'keep *_TriggerResults_*_*', 
    #     'keep *_hltFEDSelectorL1_*_*', 
    #     'keep *_hltScouting*_*_*', 
    #     'keep *_hltScoutingUnpackProducer_*_*',  # Keep unpacked products
    #     'keep *_genParticles_*_*', 
    #     'keep *_prunedTrackingParticles_*_*',
    #     'keep GenEventInfoProduct_*_*_*', 
    #     'keep *_prunedGenParticles_*_*', 
    #     'keep *_particleFlow_*_*', 
    #     'keep *_generalTracks_*_*', 
    #     'keep *_lostTracks*_*_*', 
    #     'keep *_packedGenParticles_*_*', 
    #     'keep *_packedPFCandidate*_*_*',
    #     'keep *_offlineBeamSpot_*_*'  # Keep beamspot
    # )
)

# Usually it is better to put producers on a task instead of a path
# but paths also work.
process.p = cms.Path(process.hltScoutingUnpackProducer)
process.e = cms.EndPath(process.out)