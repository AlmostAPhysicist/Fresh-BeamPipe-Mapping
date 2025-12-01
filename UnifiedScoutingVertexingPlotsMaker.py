import FWCore.ParameterSet.Config as cms

process = cms.Process("CHAIN")

process.load("FWCore.MessageService.MessageLogger_cfi")
# process.MessageLogger.cerr.FwkSummary.reportEvery = 5
# process.MessageLogger.cerr.FwkReport.reportEvery = 5
process.MessageLogger.cerr.FwkSummary.reportEvery = 100
process.MessageLogger.cerr.FwkReport.reportEvery = 100
# process.MessageLogger.cerr.FwkSummary.reportEvery = 1000
# process.MessageLogger.cerr.FwkReport.reportEvery = 1000

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
    input = cms.untracked.int32(20000)  # Limited events for testing
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
    fileName = cms.string("test-outputs/ScoutingPlotsTest_v3.root")
    # fileName = cms.string("ScoutingPlots_Output.root")  # Histogram/plot output
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

# Step 3: Scouting Plot Maker (RENAMED)
process.scoutingPlots = cms.EDAnalyzer('ScoutingPlotMakerRun3',
    # Discrete ntk cuts - each creates a separate branch
    cut_ntk = cms.VPSet(
        # cms.PSet(values = cms.vint32()),           # No cut (accept all ntk)
        cms.PSet(values = cms.vint32(2)),          # Only ntk=2
        cms.PSet(values = cms.vint32(3)),          # Only ntk=3
        # cms.PSet(values = cms.vint32(2, 3))        # ntk=2 OR ntk=3
        cms.PSet(values = cms.vint32(3, 4)),          # Only ntk=4
        # ntk = 3,4,5 maybe
    ),
    
    # Opening angle cuts - each creates a separate branch
    cut_opening_angle_min = cms.vdouble(-1, 0.05, 0.1, 0.25, 0.5, 1.0),  # -1 = no cut, then 0.05 rad, 0.1 rad, 0.25 rad, 0.5 rad, 1.0 rad
    
    # Other (non-branching) cuts
    required_invmass = cms.double(2.0), # 2GeV  min
    required_chi2 = cms.double(-1),
    required_dBV_min = cms.double(-1),
    required_dBV_max = cms.double(-1),
    required_dxy_min = cms.double(-1),
    required_dxy_max = cms.double(-1),
    required_dBV_error = cms.double(-1),
    required_dxy_error = cms.double(-1),
    refPreference = cms.untracked.string(referencePreference),

    # Seed-like plots: mirror Vertexer’s thresholds and 2D/3D toggle (plots only; no selection)
    seed_minIPSig        = cms.untracked.double(process.Vertexer.minSeedIPSig.value()),
    seed_minPt           = cms.untracked.double(process.Vertexer.minSeedPt.value()),
    seed_maxIPSig        = cms.untracked.double(process.Vertexer.dxySig_max_cut.value()),
    seed_minPixelHits    = cms.untracked.int32(process.Vertexer.npixelHits_min_cut.value()),
    seed_minStripHits    = cms.untracked.int32(process.Vertexer.nstripHits_min_cut.value()),
    seed_minTrackerLayers= cms.untracked.int32(process.Vertexer.ntrackerLayers_min_cut.value()),
    seed_use2DTrackDist  = cms.untracked.bool(process.Vertexer.use_2d_track_dist.value()),

    # Shared toggles for consistent definitions
    use_2d_track_dist   = cms.bool(process.Vertexer.use_2d_track_dist.value()),
    use_2d_vertex_dist  = cms.bool(process.Vertexer.use_2d_vertex_dist.value()),

    PVBoundary1 = cms.int32(20),
    PVBoundary2 = cms.int32(40),
    displacedVertices = cms.InputTag("Vertexer"),
    beamspot_src = cms.InputTag('offlineBeamSpot'),  # Same beamspot source as Vertexer
    tracks = cms.InputTag("hltScoutingUnpackProducer", "Track"), # Same tracks as Vertexer's seed_tracks_src
    primaryVertices = cms.InputTag("hltScoutingUnpackProducer", "PrimaryVertex") # Same PVs as Vertexer's primaryVertices
)

process.scoutingTrackCount = cms.EDFilter('ScoutingTrackCountFilter',
    src = cms.InputTag('hltScoutingTrackPacker'),      # or 'hltScoutingTrack' depending on your input label
    minNumber = cms.untracked.uint32(1)                # require >=1 scouting track
)

# process.moduloEventFilter = cms.EDFilter('ModuloEventFilter',
#     modulo = cms.uint32(10),                           # keep 1-in-10 events
#     remainder = cms.untracked.uint32(9)               # keep events with eventNumber % 10 == 0
# )

# Full chain
process.p = cms.Path(
    process.scoutingTrackCount +
    process.hltScoutingUnpackProducer +
    process.offlineBeamSpot +
    process.Vertexer +
    process.scoutingPlots  # RENAMED from scoutingTree
)