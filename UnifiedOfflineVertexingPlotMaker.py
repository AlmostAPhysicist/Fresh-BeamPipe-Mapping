import FWCore.ParameterSet.Config as cms

process = cms.Process("CHAIN")

# ---------------- Message Logger ----------------
process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkSummary.reportEvery = 5000
process.MessageLogger.cerr.FwkReport.reportEvery = 5000
process.MessageLogger.cerr.threshold = cms.untracked.string('INFO')

process.options = cms.untracked.PSet(
    wantSummary = cms.untracked.bool(True),
    TryToContinue = cms.untracked.vstring('ProductNotFound'),
    # numberOfThreads = cms.untracked.uint32(8),    # adjust to machine cores
    # numberOfStreams = cms.untracked.uint32(0),     # let framework pick sensible streams
)

process.maxEvents = cms.untracked.PSet(
    # input = cms.untracked.int32(5000000)  # for testing
    input = cms.untracked.int32(-1)  # process all events
)

# ---------------- Input ----------------
process.source = cms.Source(
    "PoolSource",
    fileNames = cms.untracked.vstring(
        # "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/130000/b7867cb3-0c5b-407f-a8c3-3edf960415e3.root"
        "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v3/110000/017d2bf5-4f3c-40ab-b6f9-c42381b6ae9b.root"
    )
)

# ---------------- Conditions / Geometry ----------------
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(
    process.GlobalTag,
    '140X_mcRun3_2024_realistic_v26',
    ''
)

process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('Configuration.StandardSequences.MagneticField_cff')

# ---------------- Output ----------------
process.TFileService = cms.Service(
    "TFileService",
    # fileName = cms.string("outputs/OfflineVertexingPlots.root")
    fileName = cms.string("OfflineData_VertexingPlots_DYto2Mu_MINIAODSIM.root")
)

# ---------------- BeamSpot & TransientTracks ----------------
process.load("RecoVertex.BeamSpotProducer.BeamSpot_cfi")
process.load("TrackingTools.TransientTrack.TransientTrackBuilder_cfi")

# ---------------- Step 1: PackedCandidate → Track ----------------
process.packedCandidateToTrack = cms.EDProducer(
    "PackedCandidateToTrackProducer",
    src = cms.InputTag("packedPFCandidates")
)

# ---------------- Step 2: Vertexer ----------------
referencePreference = 'BS'  # or 'PV'

process.Vertexer = cms.EDProducer(
    "Vertexer",
    seed_tracks_src = cms.InputTag("packedCandidateToTrack", "Track"),
    primaryVertices = cms.InputTag("offlineSlimmedPrimaryVertices"),
    beamspot_src    = cms.InputTag("offlineBeamSpot"),
    refPreference   = cms.untracked.string(referencePreference),

    minSeedIPSig = cms.untracked.double(4.0),
    minSeedPt    = cms.untracked.double(0.9),

    n_tracks_per_seed_vertex = cms.int32(2),
    max_seed_vertex_chi2 = cms.double(5),

    resolve_split_vertices_loose = cms.bool(False),
    resolve_split_vertices_tight = cms.bool(False),
    investigate_merged_vertices = cms.bool(False),

    use_2d_vertex_dist = cms.bool(False),
    use_2d_track_dist  = cms.bool(True),

    merge_anyway_dist = cms.double(-1),
    merge_anyway_sig  = cms.double(4),
    merge_shared_dist = cms.double(-1),
    merge_shared_sig  = cms.double(4),

    max_track_vertex_dist = cms.double(-1),
    max_track_vertex_sig  = cms.double(5),
    min_track_vertex_sig_to_remove = cms.double(1.5),

    remove_one_track_at_a_time = cms.bool(True),
    max_nm1_refit_dist3 = cms.double(-1),
    max_nm1_refit_distz = cms.double(-1),
    max_nm1_refit_count = cms.int32(-1),

    verbose = cms.bool(False),
)

# ---------------- Step 3: Plot Maker ----------------
process.offlinePlots = cms.EDAnalyzer(
    "ScoutingPlotMakerRun3",   # same analyzer
    displacedVertices = cms.InputTag("Vertexer"),
    tracks = cms.InputTag("packedCandidateToTrack", "Track"),
    primaryVertices = cms.InputTag("offlineSlimmedPrimaryVertices"),
    beamspot_src = cms.InputTag("offlineBeamSpot"),
    refPreference = cms.untracked.string(referencePreference),

    cut_ntk = cms.VPSet(
        cms.PSet(values = cms.vint32(3)),
        cms.PSet(values = cms.vint32(3, 4)),
    ),

    cut_opening_angle_min = cms.vdouble(-1, 0.05, 0.1, 0.25, 0.5, 1.0),

    required_invmass = cms.double(2.0),
    required_chi2 = cms.double(-1),
    required_dBV_min = cms.double(-1),
    required_dBV_max = cms.double(-1),
    required_dxy_min = cms.double(-1),
    required_dxy_max = cms.double(-1),
    required_dBV_error = cms.double(-1),
    required_dxy_error = cms.double(-1),

    seed_minIPSig = cms.untracked.double(process.Vertexer.minSeedIPSig.value()),
    seed_minPt    = cms.untracked.double(process.Vertexer.minSeedPt.value()),

    use_2d_track_dist  = cms.bool(process.Vertexer.use_2d_track_dist.value()),
    use_2d_vertex_dist = cms.bool(process.Vertexer.use_2d_vertex_dist.value()),
)

# ---------------- Path ----------------
process.p = cms.Path(
    process.packedCandidateToTrack +
    process.offlineBeamSpot +
    process.Vertexer +
    process.offlinePlots
)
