# import FWCore.ParameterSet.Config as cms
# import os

# process = cms.Process("CHAIN")

# process.load("FWCore.MessageService.MessageLogger_cfi")
# process.MessageLogger.cerr.FwkSummary.reportEvery = 2000
# process.MessageLogger.cerr.FwkReport.reportEvery = 2000

# process.options = cms.untracked.PSet(
#     wantSummary = cms.untracked.bool(True),
#     TryToContinue = cms.untracked.vstring('ProductNotFound')  # In case something missing in few events
# )

# process.maxEvents = cms.untracked.PSet(
#     input = cms.untracked.int32(10)  # -1 for all events
# )

# # Input source
# process.source = cms.Source("PoolSource",
#     fileNames = cms.untracked.vstring(
#         # crab will fill this
#         # "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2560000/cc384b7d-2aa8-4d19-aef5-037c86a6969d.root"
#         # "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2560000/cc384b7d-2aa8-4d19-aef5-037c86a6969d.root"
#         # "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/110000/33ef87b0-5585-44a4-9da1-7d7621257fac.root"  
#         # "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/130001/c3344a36-e63a-4c68-be0f-54f50f89434e.root"
#         # "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2560002/25d57a3c-2ef1-4517-bbd3-99e0a6f57a94.root"
#         # "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2550001/352a68f8-ec01-4403-b117-48d1fb7cd2fe.root"
#         "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/130000/b7867cb3-0c5b-407f-a8c3-3edf960415e3.root"
#     )
# )

# # Global tag
# process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
# from Configuration.AlCa.GlobalTag import GlobalTag
# process.GlobalTag = GlobalTag(process.GlobalTag, '133X_mcRun3_2024_realistic_v9', '')

# # Geometry and Magnetic Field
# process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
# process.load('Configuration.StandardSequences.MagneticField_cff')

# # L1 Unpacking
# process.load("EventFilter.L1TRawToDigi.gtStage2Digis_cfi")
# process.gtStage2Digis.InputLabel = cms.InputTag("hltFEDSelectorL1")

# # TFileService for final output
# process.TFileService = cms.Service("TFileService",
#     fileName = cms.string("DY2M_ScoutingTree_Output.root")  # This is the only output saved
# )

# # Step 1: HLT Scouting Unpacker
# process.hltScoutingUnpackProducer = cms.EDProducer('HLTScoutingUnpackProducer',
#     scoutingTrack = cms.InputTag('hltScoutingTrackPacker'),
#     scoutingPrimaryVertex = cms.InputTag('hltScoutingPrimaryVertexPacker', 'primaryVtx'),
#     producePFCHSCandidate = cms.bool(False),
#     mightGet = cms.optional.untracked.vstring
# )

# # Step 2: Vertexer
# process.load("RecoVertex.BeamSpotProducer.BeamSpot_cfi")
# process.load("TrackingTools.TransientTrack.TransientTrackBuilder_cfi")

# process.Vertexer = cms.EDProducer('Vertexer',
#     seed_tracks_src = cms.InputTag('hltScoutingUnpackProducer', 'Track'),
#     beamspot_src = cms.InputTag('offlineBeamSpot'),
#     n_tracks_per_seed_vertex = cms.int32(2),
#     max_seed_vertex_chi2 = cms.double(5),
#     resolve_split_vertices_loose = cms.bool(False),
#     resolve_split_vertices_tight = cms.bool(True),
#     investigate_merged_vertices = cms.bool(False),
#     use_2d_vertex_dist = cms.bool(False),
#     use_2d_track_dist = cms.bool(False),
#     merge_anyway_dist = cms.double(-1),
#     merge_anyway_sig = cms.double(4),
#     merge_shared_dist = cms.double(-1),
#     merge_shared_sig = cms.double(4),
#     max_track_vertex_dist = cms.double(-1),
#     max_track_vertex_sig = cms.double(5),
#     min_track_vertex_sig_to_remove = cms.double(1.5),
#     remove_one_track_at_a_time = cms.bool(True),
#     max_nm1_refit_dist3 = cms.double(-1),
#     max_nm1_refit_distz = cms.double(0.005),
#     max_nm1_refit_count = cms.int32(-1),
#     verbose = cms.bool(False),
# )

# # Step 3: Scouting Tree Maker
# L1Info = [
#     "L1_HTT200er", "L1_HTT255er", "L1_HTT280er", "L1_HTT320er",
#     "L1_HTT360er", "L1_HTT400er", "L1_HTT450er", "L1_ETT2000",
#     "L1_SingleJet180", "L1_SingleJet200",
#     "L1_DoubleJet30er2p5_Mass_Min250_dEta_Max1p5",
#     "L1_DoubleJet30er2p5_Mass_Min300_dEta_Max1p5",
#     "L1_DoubleJet30er2p5_Mass_Min330_dEta_Max1p5"
# ]

# process.scoutingTree = cms.EDAnalyzer('ScoutingTreeMakerRun3',
#     required_ntk = cms.int32(2),
#     triggerresults = cms.InputTag("TriggerResults", "", "HLT"),
#     ReadPrescalesFromFile = cms.bool(False),
#     AlgInputTag = cms.InputTag("gtStage2Digis"),
#     l1tAlgBlkInputTag = cms.InputTag("gtStage2Digis"),
#     l1tExtBlkInputTag = cms.InputTag("gtStage2Digis"),
#     doTrigger = cms.bool(True),
#     doL1 = cms.bool(True),
#     doPhiCorrection = cms.bool(False),
#     luminosity = cms.double(108.96),   # fb-1
#     crossSection = cms.double(1),       # fb
#     l1Seeds = cms.vstring(L1Info),
#     muons = cms.InputTag("hltScoutingMuonPacker"),
#     electrons = cms.InputTag("hltScoutingEgammaPacker"),
#     photons = cms.InputTag("hltScoutingEgammaPacker"),
#     pfcands = cms.InputTag("hltScoutingPFPacker"),
#     pfjets = cms.InputTag("hltScoutingPFPacker"),
#     tracks = cms.InputTag("hltScoutingUnpackProducer", "Track"),
#     primaryVertices = cms.InputTag("hltScoutingPrimaryVertexPacker", "primaryVtx"),
#     displacedVertices = cms.InputTag("Vertexer"),
#     pfMet = cms.InputTag("hltScoutingPFPacker", "pfMetPt"),
#     pfMetPhi = cms.InputTag("hltScoutingPFPacker", "pfMetPhi"),
#     rho = cms.InputTag("hltScoutingPFPacker", "rho"),
#     beamspot_src = cms.InputTag('offlineBeamSpot'),
#     genParticle_src = cms.InputTag('prunedGenParticles'),
#     generatorName = cms.InputTag('generator')
# )

# # Full chain schedule
# process.p = cms.Path(
#     process.hltScoutingUnpackProducer +
#     process.gtStage2Digis +
#     process.offlineBeamSpot +
#     process.Vertexer +
#     process.scoutingTree
# )



# -------

import FWCore.ParameterSet.Config as cms

process = cms.Process("CHAIN")

process.load("FWCore.MessageService.MessageLogger_cfi")
# process.MessageLogger.cerr.FwkSummary.reportEvery = 500
# process.MessageLogger.cerr.FwkReport.reportEvery = 500
process.MessageLogger.cerr.FwkSummary.reportEvery = 10
process.MessageLogger.cerr.FwkReport.reportEvery = 10

# process.MessageLogger.cerr.threshold = cms.untracked.string('DEBUG')
# process.MessageLogger.debugModules = cms.untracked.vstring('hltScoutingUnpackProducer', 'Vertexer', 'scoutingTree')
process.MessageLogger.cerr.threshold = cms.untracked.string('INFO')  # Reduced verbosity
process.MessageLogger.debugModules = cms.untracked.vstring()  # Disable debug for production


process.options = cms.untracked.PSet(
    wantSummary = cms.untracked.bool(True),
    # TryToContinue = cms.untracked.vstring('ProductNotFound')
    numberOfThreads = cms.untracked.uint32(4),     # adjust to machine cores
    numberOfStreams = cms.untracked.uint32(0),     # let framework pick sensible streams
)

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(1000)  # Limited events for testing
    # input = cms.untracked.int32(-1)  # Process all events
)

# Input source
process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(
        # "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/130000/b7867cb3-0c5b-407f-a8c3-3edf960415e3.root"
    # "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/110000/000e726a-ca68-420b-b531-23f6733ba1e4.root"
    # "file:/tmp/test.root"
    # "root://cmsxrootd.fnal.gov//store/data/Run2024H/ScoutingPFRun3/HLTSCOUT/v1/000/385/836/00000/02fa9546-0c14-45e9-906a-ddd16bdd30ba.root"
    # "root://cmsxrootd.fnal.gov//store/data/Run2024H/ScoutingPFRun3/HLTSCOUT/v1/000/385/933/00000/dc76810a-c42b-4f76-b965-7475a9b4fb96.root"
    # "root://cmsxrootd.fnal.gov//store/data/Run2024H/ScoutingPFRun3/HLTSCOUT/v1/000/385/836/00000/003ca643-43f8-40dd-92b3-4c6a4ccdc894.root" #EDM Number of events: 527035
    # "root://cmsxrootd.fnal.gov//store/data/Run2024H/ScoutingPFRun3/HLTSCOUT/v1/000/385/933/00000/51f2ac21-4b92-4144-8b4f-39f726f4351a.root"
    # "root://cmsxrootd.fnal.gov//store/data/Run2024H/ScoutingPFRun3/HLTSCOUT/v1/000/385/933/00000/51f2ac21-4b92-4144-8b4f-39f726f4351a.root",
    "root://cmsxrootd.fnal.gov//store/data/Run2024H/ScoutingPFRun3/HLTSCOUT/v1/000/385/836/00000/013b488b-7af4-450f-b175-b39623c72ae2.root"
    )
)

# process.source.skipEvents = cms.untracked.uint32(720)  # Start at 721st event

# Global tag
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '133X_mcRun3_2024_realistic_v9', '')

# Geometry and Magnetic Field
process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('Configuration.StandardSequences.MagneticField_cff')

# TFileService for output
process.TFileService = cms.Service("TFileService",
    # fileName = cms.string("DY2M_ScoutingTree_Output.root")
    fileName = cms.string("./test-outputs/DY2M_ScoutingTree_Output_Cleanup.root")  # This is the only output saved
)

# Step 1: HLT Scouting Unpacker
process.hltScoutingUnpackProducer = cms.EDProducer('HLTScoutingUnpackProducer',
    scoutingTrack = cms.InputTag('hltScoutingTrackPacker'),
    scoutingPrimaryVertex = cms.InputTag('hltScoutingPrimaryVertexPacker', 'primaryVtx'),
    producePFCHSCandidate = cms.bool(False),
    mightGet = cms.optional.untracked.vstring
)

# Step 2: Vertexer
process.load("RecoVertex.BeamSpotProducer.BeamSpot_cfi")
process.load("TrackingTools.TransientTrack.TransientTrackBuilder_cfi")

process.Vertexer = cms.EDProducer('Vertexer',
    seed_tracks_src = cms.InputTag('hltScoutingUnpackProducer', 'Track'),
    beamspot_src = cms.InputTag('offlineBeamSpot'),
    n_tracks_per_seed_vertex = cms.int32(2),
    max_seed_vertex_chi2 = cms.double(5),
    resolve_split_vertices_loose = cms.bool(False),
    resolve_split_vertices_tight = cms.bool(True), # Can be set to False to disable
    investigate_merged_vertices = cms.bool(False),
    use_2d_vertex_dist = cms.bool(False),
    use_2d_track_dist = cms.bool(False),
    merge_anyway_dist = cms.double(-1),
    merge_anyway_sig = cms.double(4),
    merge_shared_dist = cms.double(-1),
    merge_shared_sig = cms.double(4),
    max_track_vertex_dist = cms.double(-1),
    max_track_vertex_sig = cms.double(5),
    min_track_vertex_sig_to_remove = cms.double(1.5),
    remove_one_track_at_a_time = cms.bool(True),
    max_nm1_refit_dist3 = cms.double(-1),
    max_nm1_refit_distz = cms.double(0.005),
    max_nm1_refit_count = cms.int32(-1),
    verbose = cms.bool(False)  # Enable verbose output for debugging
)

# Step 3: Scouting Tree Maker

process.scoutingTree = cms.EDAnalyzer('ScoutingTreeMakerRun3',
    required_ntk_min = cms.int32(2),
    required_ntk_max = cms.int32(3),
    required_invmass = cms.double(-1),
    required_chi2 = cms.double(-1),
    required_dBV_min = cms.double(-1),
    required_dBV_max = cms.double(-1),
    required_dxy_min = cms.double(-1),
    required_dxy_max = cms.double(-1),
    required_dBV_error = cms.double(-1),
    required_dxy_error = cms.double(-1),
    displacedVertices = cms.InputTag("Vertexer"),
    beamspot_src = cms.InputTag('offlineBeamSpot'),
    tracks = cms.InputTag("hltScoutingUnpackProducer", "Track")  # May need to change to "generalTracks"
)

# Full chain schedule
process.p = cms.Path(
    process.hltScoutingUnpackProducer +
    process.offlineBeamSpot +
    process.Vertexer +
    process.scoutingTree
)
