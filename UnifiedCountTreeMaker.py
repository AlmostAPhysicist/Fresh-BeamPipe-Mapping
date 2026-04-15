import FWCore.ParameterSet.Config as cms

process = cms.Process("CHAIN")

# -------------------- MessageLogger / Options / Events --------------------
process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkSummary.reportEvery = 1
process.MessageLogger.cerr.FwkReport.reportEvery = 1

process.options = cms.untracked.PSet(
    wantSummary = cms.untracked.bool(True),
    TryToContinue = cms.untracked.vstring('ProductNotFound'),
)

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(1)
)



# -------------------------- INPUT PATH ------------------------------------
process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(
        "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v3/110000/0639b06f-0a53-4150-ac4f-ffab0df5ef91.root",
    )
)

# offset the event processing to skip events with no scouting tracks (if desired)
process.source.skipEvents = cms.untracked.uint32(60)
# -------------------------- GLOBAL TAG / GEOM ------------------------------
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '140X_mcRun3_2024_realistic_v26', '')

process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('Configuration.StandardSequences.MagneticField_cff')

# -------------------------- TFileService -----------------------------------
process.TFileService = cms.Service("TFileService",
    fileName = cms.string("test-outputs/vertex_counts_print1.root")
)

# -------------------------- HLT Scouting Unpacker --------------------------
process.hltScoutingUnpackProducer = cms.EDProducer('HLTScoutingUnpackProducer',
    scoutingTrack = cms.InputTag('hltScoutingTrackPacker'),
    scoutingPrimaryVertex = cms.InputTag('hltScoutingPrimaryVertexPacker', 'primaryVtx'),
    pfCand = cms.InputTag(""),
    lostTrack = cms.InputTag(""),
    isScouting = cms.bool(True),
    producePFCHSCandidate = cms.bool(False),
    mightGet = cms.optional.untracked.vstring
)

# -------------------------- Vertexer --------------------------------------
process.load("RecoVertex.BeamSpotProducer.BeamSpot_cfi")
process.load("TrackingTools.TransientTrack.TransientTrackBuilder_cfi")

# Build reco::Track collection from packed candidates for OFFLINE vertexing
process.packedCandidateToTrack = cms.EDProducer("PackedCandidateToTrackProducer",
    src = cms.InputTag("packedPFCandidates")
)

referencePreference = 'BS'
useOnlineBeamSpot = True
printVertexerLogs = True
offlineBeamSpotTag = cms.InputTag('offlineBeamSpot')

process.Vertexer = cms.EDProducer('Vertexer',
    seed_tracks_src = cms.InputTag('hltScoutingUnpackProducer', 'Track'),
    primaryVertices = cms.InputTag("hltScoutingUnpackProducer", "PrimaryVertex"),
    beamspot_src = offlineBeamSpotTag,
    useOnlineBeamSpot = cms.untracked.bool(useOnlineBeamSpot),
    refPreference = cms.untracked.string(referencePreference),

    minSeedIPSig = cms.untracked.double(4.0),
    minSeedPt    = cms.untracked.double(0.9),

    n_tracks_per_seed_vertex = cms.int32(2),
    max_seed_vertex_chi2 = cms.double(5),
    resolve_split_vertices_loose = cms.bool(False),
    resolve_split_vertices_tight = cms.bool(False),
    investigate_merged_vertices = cms.bool(False),
    use_2d_vertex_dist = cms.bool(False),
    use_2d_track_dist = cms.bool(True),
    merge_anyway_dist = cms.double(-1),
    merge_anyway_sig = cms.double(4),
    merge_shared_dist = cms.double(-1),
    merge_shared_sig = cms.double(4),
    max_track_vertex_dist = cms.double(-1),
    max_track_vertex_sig = cms.double(-1),
    min_track_vertex_sig_to_remove = cms.double(1.5),
    remove_one_track_at_a_time = cms.bool(True),
    max_nm1_refit_dist3 = cms.double(-1),
    max_nm1_refit_distz = cms.double(-1),
    max_nm1_refit_count = cms.int32(-1),
    logBeamspotSource = cms.untracked.bool(True),
    printVertexerLogs = cms.untracked.bool(printVertexerLogs),
    verbose = cms.bool(False),
)

# Offline vertexer (mirrors EventComparatorAnalyzer setup)
process.VertexerOffline = cms.EDProducer('Vertexer',
    seed_tracks_src = cms.InputTag('packedCandidateToTrack', 'Track'),
    primaryVertices = cms.InputTag('offlineSlimmedPrimaryVertices'),
    beamspot_src = offlineBeamSpotTag,
    useOnlineBeamSpot = cms.untracked.bool(useOnlineBeamSpot),
    refPreference = cms.untracked.string(referencePreference),

    minSeedIPSig = cms.untracked.double(4.0),
    minSeedPt    = cms.untracked.double(0.9),

    n_tracks_per_seed_vertex = cms.int32(2),
    max_seed_vertex_chi2 = cms.double(5),
    resolve_split_vertices_loose = cms.bool(False),
    resolve_split_vertices_tight = cms.bool(False),
    investigate_merged_vertices = cms.bool(False),
    use_2d_vertex_dist = cms.bool(False),
    use_2d_track_dist = cms.bool(True),
    merge_anyway_dist = cms.double(-1),
    merge_anyway_sig = cms.double(4),
    merge_shared_dist = cms.double(-1),
    merge_shared_sig = cms.double(4),
    max_track_vertex_dist = cms.double(-1),
    max_track_vertex_sig = cms.double(-1),
    min_track_vertex_sig_to_remove = cms.double(1.5),
    remove_one_track_at_a_time = cms.bool(True),
    max_nm1_refit_dist3 = cms.double(-1),
    max_nm1_refit_distz = cms.double(-1),
    max_nm1_refit_count = cms.int32(-1),
    logBeamspotSource = cms.untracked.bool(True),
    printVertexerLogs = cms.untracked.bool(printVertexerLogs),
    verbose = cms.bool(False),
)

# -------------------------- Optional: Scouting track-count filter ----------
process.scoutingTrackCount = cms.EDFilter('ScoutingTrackCountFilter',
    src = cms.InputTag('hltScoutingTrackPacker'),
    minNumber = cms.untracked.uint32(1)
)

# -------------------------- ScoutingCountTreeMakerRun3 --------------------
# Tree-based analyzer replacing the old counting analyzer.
process.ScoutingCountTreeMakerRun3 = cms.EDAnalyzer("ScoutingCountTreeMakerRun3",
    scoutingVertices = cms.InputTag("Vertexer"),
    offlineVertices = cms.InputTag("VertexerOffline"),
    primaryVertices = cms.InputTag("hltScoutingPrimaryVertexPacker", "primaryVtx"),
    beamspot_src = cms.InputTag("offlineBeamSpot"),
    tracks = cms.InputTag("hltScoutingUnpackProducer", "Track"),
    offlineTracks = cms.InputTag("packedCandidateToTrack", "Track"),

    cut_ntk = cms.PSet(
        values = cms.vint32(),   # accept all vertices regardless of track multiplicity
    ),

    cut_opening_angle_min = cms.double(-1.0),

    required_invmass = cms.double(-1.0),
    required_chi2 = cms.double(-1.0),
    required_dBV_min = cms.double(-1.0),
    required_dBV_max = cms.double(-1.0),
    required_dxy_min = cms.double(-1.0),
    required_dxy_max = cms.double(-1.0),
    required_dBV_error = cms.double(-1.0),
    required_dxy_error = cms.double(-1.0),

    track_pt_min_cut = cms.untracked.double(1.0),
    track_dxySig_min_cut = cms.untracked.double(4.0),
    track_dxySig_max_cut = cms.untracked.double(100.0),

    applyHitCuts = cms.bool(True),
    hit_minPixelHits = cms.untracked.int32(2),
    hit_minStripHits = cms.untracked.int32(6),
    hit_minTrackerLayers = cms.untracked.int32(6),

    seed_minIPSig = cms.untracked.double(process.Vertexer.minSeedIPSig.value()),
    seed_minPt = cms.untracked.double(process.Vertexer.minSeedPt.value()),
    seed_maxIPSig = cms.untracked.double(100.0),
    seed_use2DTrackDist = cms.untracked.bool(process.Vertexer.use_2d_track_dist.value()),

    useOnlineBeamSpot = cms.untracked.bool(useOnlineBeamSpot),
    refPreference = cms.untracked.string(referencePreference),

    verbose = cms.untracked.bool(True),
    printSummary = cms.untracked.bool(True)
)

# -------------------------- Path ------------------------------------------
process.p = cms.Path(
    process.scoutingTrackCount +
    process.packedCandidateToTrack +
    process.hltScoutingUnpackProducer +
    process.offlineBeamSpot +
    process.Vertexer +
    process.VertexerOffline +
    process.ScoutingCountTreeMakerRun3
)

