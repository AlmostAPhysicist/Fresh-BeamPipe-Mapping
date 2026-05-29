import FWCore.ParameterSet.Config as cms

process = cms.Process("SCOUTINGCOMPARE")

# -------------------- Message Logger --------------------
process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = 100

# -------------------- Options --------------------
process.options = cms.untracked.PSet(
    wantSummary = cms.untracked.bool(True),
    numberOfThreads = cms.untracked.uint32(4),    # adjust to machine cores
    numberOfStreams = cms.untracked.uint32(0),     # let framework pick sensible streams
    )

# -------------------- Histogram Output --------------------
process.TFileService = cms.Service(
    "TFileService",
    fileName = cms.string("gen_scouting_comparer.root"),
)

# -------------------- Max Events To Process --------------------
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(10000))

# -------------------- Input Source --------------------
process.source = cms.Source(
    "PoolSource",
    fileNames = cms.untracked.vstring(
        "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_miniAOD_v2/250214_150834/0000/stop_dbar_miniAOD_5.root"
    ),
)

# -------------------- Scouting unpacker and vertexer --------------------
process.hltScoutingUnpackProducer = cms.EDProducer(
    "HLTScoutingUnpackProducer",
    scoutingTrack = cms.InputTag("hltScoutingTrackPacker"),
    scoutingPrimaryVertex = cms.InputTag("hltScoutingPrimaryVertexPacker", "primaryVtx"),
    pfCand = cms.InputTag(""),
    lostTrack = cms.InputTag(""),
    isScouting = cms.bool(True),
    producePFCHSCandidate = cms.bool(False),
    mightGet = cms.optional.untracked.vstring,
)

process.load("Configuration.StandardSequences.GeometryRecoDB_cff")
process.load("Configuration.StandardSequences.MagneticField_cff")
process.load("TrackingTools.TransientTrack.TransientTrackBuilder_cfi")
process.load("RecoVertex.BeamSpotProducer.BeamSpot_cfi")

process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '140X_mcRun3_2024_realistic_v26', '')

process.Vertexer = cms.EDProducer(
    "Vertexer",
    seed_tracks_src = cms.InputTag("hltScoutingUnpackProducer", "Track"),
    primaryVertices = cms.InputTag("hltScoutingUnpackProducer", "PrimaryVertex"),
    beamspot_src = cms.InputTag("offlineBeamSpot"),
    useOnlineBeamSpot = cms.untracked.bool(False),
    refPreference = cms.untracked.string("BeamSpot"),
    
    # Internal Vertexer Seed Tracks logic
    minSeedIPSig = cms.untracked.double(4.0),
    minSeedPt = cms.untracked.double(0.9),
    n_tracks_per_seed_vertex = cms.int32(2),
    max_seed_vertex_chi2 = cms.double(5),
    
    # Arbitration & Merge parameters
    resolve_split_vertices_loose = cms.bool(True),       # ENABLED based on the C++ loose merge snippet
    resolve_split_vertices_tight = cms.bool(False),
    investigate_merged_vertices = cms.bool(False),
    use_2d_vertex_dist = cms.bool(False),
    use_2d_track_dist = cms.bool(True),
    merge_anyway_dist = cms.double(0.05),                # Set to 0.05 cm based on Analysis Note "nearby vertices" criteria
    merge_anyway_sig = cms.double(4),
    merge_shared_dist = cms.double(-1),
    merge_shared_sig = cms.double(4),
    max_track_vertex_dist = cms.double(-1),
    max_track_vertex_sig = cms.double(5),
    min_track_vertex_sig_to_remove = cms.double(1.5),
    remove_one_track_at_a_time = cms.bool(True),
    max_nm1_refit_dist3 = cms.double(-1),
    max_nm1_refit_distz = cms.double(-1),
    max_nm1_refit_count = cms.int32(-1),
    logBeamspotSource = cms.untracked.bool(False),
    printVertexerLogs = cms.untracked.bool(False),
    order_seed_vertex = cms.untracked.bool(False),
    use_seed_tracks_raw = cms.untracked.bool(False),
    verbose = cms.bool(False),
)

# -------------------- Custom Analyzer Module --------------------
process.scoutingComparer = cms.EDAnalyzer(
    "GenScoutingComparer",
    genParticles = cms.InputTag("prunedGenParticles"),
    scoutingVertices = cms.InputTag("Vertexer"),
    beamspot = cms.InputTag("offlineBeamSpot"),
    scoutingJets = cms.InputTag("hltScoutingPFPacker"),
    trackToScoutingMap = cms.InputTag("hltScoutingUnpackProducer", "Track-RefToOriginal"),

    # Vertex-Level Signal Cuts
    vtx_chi2_max = cms.double(3.0),       # Final Signal Region Cut (reduced chi2 < 3)
    vtx_dbv_min = cms.double(0.01),       # Preselection displacement min (cm)
    vtx_dbv_max = cms.double(2.0),        # Preselection displacement max (cm)
    vtx_tracks_min = cms.uint32(8),       # Final Signal Region Cut (>= 8 tracks)
    vtx_ddbv_max = cms.double(0.005),     # Transverse displacement uncertainty max (cm)
    vtx_cosT_min = cms.double(0.0),       # Collinearity requirement
    
    # Track-Level Working Point (15% FPR Working Point)
    # nTracks counts the vertex tracks that pass these inclusive hit-quality cuts.
    track_pixelHits_min = cms.int32(2),
    track_stripHits_min = cms.int32(1),
    track_trackerLayers_min = cms.int32(5),

    # Jet-level scouting selection
    jet_pt_min = cms.double(30.0),
    jet_eta_max = cms.double(2.4),
    min_selected_jets = cms.uint32(3),

    verbose = cms.untracked.bool(False),
    verbose_unselected = cms.untracked.bool(False),
    verbose_selected_only = cms.untracked.bool(False),
    ntracks_raw = cms.untracked.bool(False),
    cost_raw = cms.untracked.bool(False),
    require_jet_selection = cms.untracked.bool(True),
)

# -------------------- Execution Path --------------------
process.p = cms.Path(
    process.hltScoutingUnpackProducer +
    process.offlineBeamSpot +
    process.Vertexer +
    process.scoutingComparer
)