import FWCore.ParameterSet.Config as cms

process = cms.Process("CHAIN")

process.load("FWCore.MessageService.MessageLogger_cfi")
# process.MessageLogger.cerr.FwkSummary.reportEvery = 5
# process.MessageLogger.cerr.FwkReport.reportEvery = 5
# process.MessageLogger.cerr.FwkSummary.reportEvery = 100
# process.MessageLogger.cerr.FwkReport.reportEvery = 100
process.MessageLogger.cerr.FwkSummary.reportEvery = 1000
process.MessageLogger.cerr.FwkReport.reportEvery = 1000

# process.MessageLogger.cerr.threshold = cms.untracked.string('DEBUG')
# process.MessageLogger.debugModules = cms.untracked.vstring('hltScoutingUnpackProducer', 'Vertexer', 'scoutingTree')
process.MessageLogger.cerr.threshold = cms.untracked.string('INFO')  # Reduced verbosity
process.MessageLogger.debugModules = cms.untracked.vstring()  # Disable debug for production


process.options = cms.untracked.PSet(
    wantSummary = cms.untracked.bool(True),
    TryToContinue = cms.untracked.vstring('ProductNotFound'),
    # numberOfThreads = cms.untracked.uint32(4),    # adjust to machine cores
    # numberOfStreams = cms.untracked.uint32(0),     # let framework pick sensible streams
)

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(100000)  # Limited events for testing
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
    "root://cmsxrootd.fnal.gov//store/data/Run2024H/ScoutingPFRun3/HLTSCOUT/v1/000/385/836/00000/02fa9546-0c14-45e9-906a-ddd16bdd30ba.root",
    # "root://cmsxrootd.fnal.gov//store/data/Run2024H/ScoutingPFRun3/HLTSCOUT/v1/000/385/933/00000/dc76810a-c42b-4f76-b965-7475a9b4fb96.root",
    # "root://cmsxrootd.fnal.gov//store/data/Run2024H/ScoutingPFRun3/HLTSCOUT/v1/000/385/836/00000/003ca643-43f8-40dd-92b3-4c6a4ccdc894.root", #EDM Number of events: 527035
    # "root://cmsxrootd.fnal.gov//store/data/Run2024H/ScoutingPFRun3/HLTSCOUT/v1/000/385/933/00000/51f2ac21-4b92-4144-8b4f-39f726f4351a.root",
    # "root://cmsxrootd.fnal.gov//store/data/Run2024H/ScoutingPFRun3/HLTSCOUT/v1/000/385/933/00000/51f2ac21-4b92-4144-8b4f-39f726f4351a.root", # Empty file
    # "root://cmsxrootd.fnal.gov//store/data/Run2024H/ScoutingPFRun3/HLTSCOUT/v1/000/385/836/00000/013b488b-7af4-450f-b175-b39623c72ae2.root",
    # MC Files
    # "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v3/110000/0639b06f-0a53-4150-ac4f-ffab0df5ef91.root",
    # "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v3/110000/06a339e5-cb52-4e75-b0e4-91285db66993.root"
    )
)

# process.source.skipEvents = cms.untracked.uint32(720)  # Start at 721st event

# Global tag
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '140X_dataRun3_Prompt_v4', '')
# process.GlobalTag = GlobalTag(process.GlobalTag, '140X_mcRun3_2024_realistic_v26', '')

# Geometry and Magnetic Field
process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('Configuration.StandardSequences.MagneticField_cff')

# TFileService for output
# -------------------------- OUTPUT PATH --------------------------------
#-----------------------------------------------------------------------
process.TFileService = cms.Service("TFileService",
    fileName = cms.string("test-outputs/ScoutingTreeTest_Size100k_incut.root")
    # fileName = cms.string("ScoutingTree_Output.root")  # TTree output (different from plots)
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

# Update Vertexer configuration with synchronized parameters
process.Vertexer = cms.EDProducer('Vertexer',
    seed_tracks_src = cms.InputTag('hltScoutingUnpackProducer', 'Track'),
    # Change to match the name in fillDescriptions (primaryVertices instead of primaryVertices_src)
    primaryVertices = cms.InputTag("hltScoutingUnpackProducer", "PrimaryVertex"),  # vector of PVs; Vertexer will average them
    beamspot_src = cms.InputTag('offlineBeamSpot'),
    refPreference = cms.untracked.string(referencePreference),

    # RESTORE original seed thresholds (used internally by Vertexer)
    minSeedIPSig = cms.untracked.double(4.0),
    minSeedPt    = cms.untracked.double(0.9),

    # Other parameters (kept for downstream, but not used in seed preselection) Again, NOTE: These are NOT used for seed selection within the Vertexer! These are only for ScoutingTreeMaker downstream (I should probably change this later for clarity)
    pt_min_cut = cms.double(0.9),
    dxySig_min_cut = cms.double(4.0),
    dxySig_max_cut = cms.double(100.0),
    npixelHits_min_cut = cms.int32(1),
    nstripHits_min_cut = cms.int32(0),
    ntrackerLayers_min_cut = cms.int32(5),

    n_tracks_per_seed_vertex = cms.int32(2),
    max_seed_vertex_chi2 = cms.double(5),
    resolve_split_vertices_loose = cms.bool(False),
    resolve_split_vertices_tight = cms.bool(False), # Turned to False from True
    investigate_merged_vertices = cms.bool(False),
    use_2d_vertex_dist = cms.bool(False), # Keep as 3D | Really this is not a good control since different parts of the algorithm require different kinds of distances.
    use_2d_track_dist = cms.bool(True), # Switched to using 2D track distance for IPsig (True)
    merge_anyway_dist = cms.double(-1),
    merge_anyway_sig = cms.double(4),
    merge_shared_dist = cms.double(-1),
    merge_shared_sig = cms.double(4),
    max_track_vertex_dist = cms.double(-1),
    max_track_vertex_sig = cms.double(5),
    min_track_vertex_sig_to_remove = cms.double(1.5),
    remove_one_track_at_a_time = cms.bool(True),
    max_nm1_refit_dist3 = cms.double(-1),
    max_nm1_refit_distz = cms.double(-1), # changed from 0.005 to -1
    max_nm1_refit_count = cms.int32(-1),
    verbose = cms.bool(False),
)

# Step 3: Scouting Tree Maker (stores comprehensive vertex+track info)
# ============================================================================
# TREEMAKER SELECTION CUTS (adjust these to control what gets saved)
# ============================================================================
TREE_MIN_NTRACKS = 3           # changed from 3 -> store ntk=2 so plot branches match
TREE_MAX_NTRACKS = 4         # Maximum tracks per vertex (-1 = no limit)
TREE_MAX_CHI2NDOF = 10.0       # Maximum χ²/ndof (10 = loose quality cut)
TREE_MIN_MASS = 2.0            # Minimum vertex mass [GeV] (1 GeV = very inclusive)
TREE_MIN_DBV = 0.2              # changed from 0.1/0 to -1 => no dBV cut at storage
TREE_MAX_DBV = -1.0            # Maximum displacement [cm] (-1 = no limit, keep all LLPs)
TREE_MAX_DBV_ERROR = 0.75       # Maximum dBV uncertainty [cm] (0.5 = reasonable precision)

# PV region boundaries for classification
TREE_PV_BOUNDARY_1 = 20        # nPV < 20: Region A (low pileup)
TREE_PV_BOUNDARY_2 = 40        # 20 ≤ nPV < 40: Region B, nPV ≥ 40: Region C (high pileup)

process.scoutingTree = cms.EDAnalyzer('ScoutingTreeMakerRun3',
    # TTree-specific selection: store vertices with 3+ tracks
    min_ntracks = cms.int32(TREE_MIN_NTRACKS),
    max_ntracks = cms.int32(TREE_MAX_NTRACKS),
    
    # Loose quality cuts to keep most vertices for offline study
    max_chi2ndof = cms.double(TREE_MAX_CHI2NDOF),
    min_mass = cms.double(TREE_MIN_MASS),
    min_dBV = cms.double(TREE_MIN_DBV),        # no dBV cut at storage
    max_dBV = cms.double(TREE_MAX_DBV),
    max_dBV_error = cms.double(TREE_MAX_DBV_ERROR),
    
    # Store all track information (no strict cuts)
    store_all_vertex_tracks = cms.bool(True),  # Store all tracks in selected vertices
    
    # Reference vertex settings (same as PlotMaker for consistency)
    refPreference = cms.untracked.string(referencePreference),
    
    # Input collections (same as Vertexer)
    displacedVertices = cms.InputTag("Vertexer"),
    beamspot_src = cms.InputTag('offlineBeamSpot'),
    tracks = cms.InputTag("hltScoutingUnpackProducer", "Track"),
    primaryVertices = cms.InputTag("hltScoutingUnpackProducer", "PrimaryVertex"),
    
    # PV region boundaries (for classification)
    PVBoundary1 = cms.int32(TREE_PV_BOUNDARY_1),
    PVBoundary2 = cms.int32(TREE_PV_BOUNDARY_2),
    
    # Seed-like track parameters (for reference, not used in TTree selection) - UNTRACKED
    seed_minIPSig        = cms.untracked.double(process.Vertexer.minSeedIPSig.value()),
    seed_minPt           = cms.untracked.double(process.Vertexer.minSeedPt.value()),
    seed_maxIPSig        = cms.untracked.double(process.Vertexer.dxySig_max_cut.value()),
    seed_minPixelHits    = cms.untracked.int32(process.Vertexer.npixelHits_min_cut.value()),
    seed_minStripHits    = cms.untracked.int32(process.Vertexer.nstripHits_min_cut.value()),
    seed_minTrackerLayers= cms.untracked.int32(process.Vertexer.ntrackerLayers_min_cut.value()),
    
    # Distance calculation toggles (for consistency with Vertexer) - TRACKED (required for fillDescriptions)
    use_2d_track_dist   = cms.bool(process.Vertexer.use_2d_track_dist.value()),
    use_2d_vertex_dist  = cms.bool(process.Vertexer.use_2d_vertex_dist.value())
)

process.scoutingTrackCount = cms.EDFilter('ScoutingTrackCountFilter',
    src = cms.InputTag('hltScoutingTrackPacker'),      # or 'hltScoutingTrack' depending on your input label
    minNumber = cms.untracked.uint32(1)                # require >=1 scouting track
)

# process.moduloEventFilter = cms.EDFilter('ModuloEventFilter',
#     modulo = cms.uint32(10),                           # keep 1-in-10 events
#     remainder = cms.untracked.uint32(9)               # keep events with eventNumber % 10 == 0
# )

# Full chain: vertex production (avgPV seeding) without offlineBeamSpot in Vertex Reconstructions
process.p = cms.Path(
    # process.moduloEventFilter +
    process.scoutingTrackCount +
    process.hltScoutingUnpackProducer +
    process.offlineBeamSpot +  # only for debug plots in tree
    process.Vertexer +
    process.scoutingTree
)

