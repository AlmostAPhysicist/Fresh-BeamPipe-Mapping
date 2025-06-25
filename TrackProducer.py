import FWCore.ParameterSet.Config as cms

process = cms.Process("TrackProducer")

process.load("FWCore.MessageService.MessageLogger_cfi")
process.options = cms.untracked.PSet(
    wantSummary=cms.untracked.bool(True)
)
process.MessageLogger.cerr.FwkSummary.reportEvery = 100
process.MessageLogger.cerr.FwkReport.reportEvery = 100

# Limit events for testing
process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(10))  # Test with ~10 events

process.source = cms.Source("PoolSource",
    fileNames=cms.untracked.vstring(
        'root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2560000/ad749229-3094-410f-94e3-2e408518e395.root'
    )  # For local testing; CRAB overrides with inputDataset
)

process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '133X_mcRun3_2024_realistic_v9', '')

process.hltScoutingUnpackProducer = cms.EDProducer('HLTScoutingUnpackProducer',
    scoutingTrack=cms.InputTag('hltScoutingTrackPacker'),
    scoutingPrimaryVertex=cms.InputTag("hltScoutingPrimaryVertexPacker", "primaryVtx"),
    producePFCHSCandidate=cms.bool(False),
    mightGet=cms.optional.untracked.vstring
)

process.out = cms.OutputModule("PoolOutputModule",
    fileName=cms.untracked.string('scout.root'),
    outputCommands=cms.untracked.vstring(
        'drop *',
        'keep *_hltGtStage2ObjectMap_*_*',
        'keep *_TriggerResults_*_*',
        'keep *_hltFEDSelectorL1_*_*',
        'keep *_hltScouting*_*_*',
        'keep *_genParticles_*_*',
        'keep *_prunedTrackingParticles_*_*',
        'keep GenEventInfoProduct_*_*_*',
        'keep *_prunedGenParticles_*_*',
        'keep *_particleFlow_*_*',
        'keep *_generalTracks_*_*',
        'keep *_lostTracks*_*_*',
        'keep *_packedGenParticles_*_*',
        'keep *_packedPFCandidate*_*_*'
    )
)

process.p = cms.Path(process.hltScoutingUnpackProducer)
process.e = cms.EndPath(process.out)
