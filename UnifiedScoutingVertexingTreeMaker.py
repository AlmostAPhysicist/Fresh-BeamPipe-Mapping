import FWCore.ParameterSet.Config as cms

process = cms.Process("CHAIN")

process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkSummary.reportEvery = 5
process.MessageLogger.cerr.FwkReport.reportEvery = 5
# process.MessageLogger.cerr.FwkSummary.reportEvery = 100
# process.MessageLogger.cerr.FwkReport.reportEvery = 100
# process.MessageLogger.cerr.FwkSummary.reportEvery = 1000
# process.MessageLogger.cerr.FwkReport.reportEvery = 1000

# process.MessageLogger.cerr.threshold = cms.untracked.string('DEBUG')
# process.MessageLogger.debugModules = cms.untracked.vstring('hltScoutingUnpackProducer', 'Vertexer', 'scoutingTree')
process.MessageLogger.cerr.threshold = cms.untracked.string('INFO')  # Reduced verbosity
process.MessageLogger.debugModules = cms.untracked.vstring()  # Disable debug for production


process.options = cms.untracked.PSet(
    wantSummary = cms.untracked.bool(True),
    TryToContinue = cms.untracked.vstring('ProductNotFound')
    # numberOfThreads = cms.untracked.uint32(4),     # adjust to machine cores
    # numberOfStreams = cms.untracked.uint32(0),     # let framework pick sensible streams
)

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(5000)  # Limited events for testing
    # input = cms.untracked.int32(150000)  # Local
    # input = cms.untracked.int32(500000)  # Process all events
    # input = cms.untracked.int32(-1)  # Process all events
)

# -------------------------- INPUT PATH --------------------------------
#-----------------------------------------------------------------------
process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(
        # "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/130000/b7867cb3-0c5b-407f-a8c3-3edf960415e3.root"
    # "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/110000/000e726a-ca68-420b-b531-23f6733ba1e4.root"
    # "file:/tmp/test.root"
    # "root://cmsxrootd.fnal.gov//store/data/Run2024H/ScoutingPFRun3/HLTSCOUT/v1/000/385/836/00000/02fa9546-0c14-45e9-906a-ddd16bdd30ba.root",
    # "root://cmsxrootd.fnal.gov//store/data/Run2024H/ScoutingPFRun3/HLTSCOUT/v1/000/385/933/00000/dc76810a-c42b-4f76-b965-7475a9b4fb96.root",
    # "root://cmsxrootd.fnal.gov//store/data/Run2024H/ScoutingPFRun3/HLTSCOUT/v1/000/385/836/00000/003ca643-43f8-40dd-92b3-4c6a4ccdc894.root", #EDM Number of events: 527035
    # "root://cmsxrootd.fnal.gov//store/data/Run2024H/ScoutingPFRun3/HLTSCOUT/v1/000/385/933/00000/51f2ac21-4b92-4144-8b4f-39f726f4351a.root",
    # "root://cmsxrootd.fnal.gov//store/data/Run2024H/ScoutingPFRun3/HLTSCOUT/v1/000/385/933/00000/51f2ac21-4b92-4144-8b4f-39f726f4351a.root", # Empty file
    # "root://cmsxrootd.fnal.gov//store/data/Run2024H/ScoutingPFRun3/HLTSCOUT/v1/000/385/836/00000/013b488b-7af4-450f-b175-b39623c72ae2.root",
    # MC Files
    "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v3/110000/0639b06f-0a53-4150-ac4f-ffab0df5ef91.root",
    # "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v3/110000/06a339e5-cb52-4e75-b0e4-91285db66993.root"
    )
)

# process.source.skipEvents = cms.untracked.uint32(720)  # Start at 721st event

# Global tag
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
# process.GlobalTag = GlobalTag(process.GlobalTag, '140X_dataRun3_Prompt_v4', '')
process.GlobalTag = GlobalTag(process.GlobalTag, '140X_mcRun3_2024_realistic_v26', '')

# Geometry and Magnetic Field
process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('Configuration.StandardSequences.MagneticField_cff')

# TFileService for output
# -------------------------- OUTPUT PATH --------------------------------
#-----------------------------------------------------------------------
process.TFileService = cms.Service("TFileService",
    fileName = cms.string("test-outputs/ScoutingTree_MC_Local_test_3.root")
    # fileName = cms.string("outputs/Data_ScoutingTree_FullPV_Local_4.root")
    # fileName = cms.string("ScoutingTree_Output_PV.root")  # This is the only output saved
)
#-----------------------------------------------------------------------

# Step 1: HLT Scouting Unpacker
process.hltScoutingUnpackProducer = cms.EDProducer('HLTScoutingUnpackProducer',
    scoutingTrack = cms.InputTag('hltScoutingTrackPacker'),
    scoutingPrimaryVertex = cms.InputTag('hltScoutingPrimaryVertexPacker', 'primaryVtx'),
    pfCand = cms.InputTag(""),  # Add missing parameters
    lostTrack = cms.InputTag(""),  # Add missing parameters
    isScouting = cms.bool(True),  # Add missing parameter - this is crucial!
    producePFCHSCandidate = cms.bool(False),
    mightGet = cms.optional.untracked.vstring
)

# Step 2: Vertexer
process.load("RecoVertex.BeamSpotProducer.BeamSpot_cfi")
process.load("TrackingTools.TransientTrack.TransientTrackBuilder_cfi")

# Reference vertex preference options:
# 'BeamSpot' or 'BS': Use beam spot as primary reference (default)
# 'PrimaryVertex', 'AvgPV', or 'PV': Use average primary vertex as primary reference
referencePreference = 'BS'  # Default to BeamSpot

# Update Vertexer configuration with corrected parameter names
process.Vertexer = cms.EDProducer('Vertexer',
    seed_tracks_src = cms.InputTag('hltScoutingUnpackProducer', 'Track'),
    # Change to match the name in fillDescriptions (primaryVertices instead of primaryVertices_src)
    primaryVertices = cms.InputTag("hltScoutingUnpackProducer", "PrimaryVertex"),  # vector of PVs; Vertexer will average them
    beamspot_src = cms.InputTag('offlineBeamSpot'),
    refPreference = cms.untracked.string(referencePreference),
    
    # Track selection parameters - correct types
    pt_min_cut = cms.double(0.9),
    dxySig_min_cut = cms.double(4.0),
    dxySig_max_cut = cms.double(100.0),
    npixelHits_min_cut = cms.int32(2),
    ntrackerLayers_min_cut = cms.int32(4),
    
    # Existing parameters
    n_tracks_per_seed_vertex = cms.int32(2),
    max_seed_vertex_chi2 = cms.double(5),
    resolve_split_vertices_loose = cms.bool(False),
    resolve_split_vertices_tight = cms.bool(True),
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
    verbose = cms.bool(False),
)

# Fix minSeedIPSig/minSeedPt since they're defined in the untracked parameters
# Remove these unnecessary parameters
# process.Vertexer.minSeedIPSig = cms.untracked.double(4.0) # REMOVE
# process.Vertexer.minSeedPt = cms.untracked.double(0.9)    # REMOVE

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
    refPreference = cms.untracked.string(referencePreference),  # Same reference preference
    PVBoundary1 = cms.int32(20),  # Re-enable PV regions
    PVBoundary2 = cms.int32(40),  # Re-enable PV regions
    displacedVertices = cms.InputTag("Vertexer"),
    beamspot_src = cms.InputTag('offlineBeamSpot'),  # Same beamspot source as Vertexer
    tracks = cms.InputTag("hltScoutingUnpackProducer", "Track"), # Same tracks as Vertexer's seed_tracks_src
    primaryVertices = cms.InputTag("hltScoutingUnpackProducer", "PrimaryVertex") # Same PVs as Vertexer's primaryVertices
)

process.scoutingTrackCount = cms.EDFilter('ScoutingTrackCountFilter',
    src = cms.InputTag('hltScoutingTrackPacker'),      # or 'hltScoutingTrack' depending on your input label
    minNumber = cms.untracked.uint32(1)                # require >=1 scouting track
)

process.moduloEventFilter = cms.EDFilter('ModuloEventFilter',
    modulo = cms.uint32(1),                           # keep 1-in-1 events
    remainder = cms.untracked.uint32(0)               # keep events with eventNumber % 1 == 0
)

# Full chain: vertex production (avgPV seeding) without offlineBeamSpot in Vertex Reconstructions
process.p = cms.Path(
    process.moduloEventFilter +
    process.scoutingTrackCount +
    process.hltScoutingUnpackProducer +
    process.offlineBeamSpot +  # only for debug plots in tree
    process.Vertexer +
    process.scoutingTree
)