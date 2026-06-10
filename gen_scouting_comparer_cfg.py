
import FWCore.ParameterSet.Config as cms

process = cms.Process("SCOUTINGCOMPARE")

# -------------------- Message Logger --------------------
process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = 1000

# -------------------- Options --------------------
process.options = cms.untracked.PSet(
    wantSummary = cms.untracked.bool(True),
    numberOfThreads = cms.untracked.uint32(8),
    numberOfStreams = cms.untracked.uint32(0),
)

# -------------------- Histogram Output --------------------
process.TFileService = cms.Service(
    "TFileService",
    fileName = cms.string("gen_scouting_comparer_v2_M200_1mm.root"),
)

# -------------------- Max Events To Process --------------------
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(100_000))

# -------------------- Input Source --------------------
process.source = cms.Source(
    "PoolSource",
    fileNames = cms.untracked.vstring(
        "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_miniAOD_v2/250214_150834/0000/stop_dbar_miniAOD_5.root"
        # "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151149/0000/stop_dbar_miniAOD_5.root"
        # "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAODv6/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-1_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/150X_mcRun3_2024_realistic_v2-v2/2810000/cb6c05c9-f007-46a5-a181-32c2ee64687a.root"
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

process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, "140X_mcRun3_2024_realistic_v26", "")

process.Vertexer = cms.EDProducer(
    "Vertexer",
    seed_tracks_src = cms.InputTag("hltScoutingUnpackProducer", "Track"),
    seed_jets_src = cms.InputTag("hltScoutingPFPacker"),
    primaryVertices = cms.InputTag("hltScoutingUnpackProducer", "PrimaryVertex"),
    trackToScoutingMap = cms.InputTag("hltScoutingUnpackProducer", "Track-RefToOriginal"),
    beamspot_src = cms.InputTag("offlineBeamSpot"),
    useOnlineBeamSpot = cms.untracked.bool(True),
    refPreference = cms.untracked.string("BeamSpot"),

    # Seed tracks
    minSeedIPSig = cms.untracked.double(4.0),
    minSeedPt = cms.untracked.double(0.9),
    minSeedPixelHits = cms.int32(2),
    minSeedStripHits = cms.int32(1),
    minSeedTrackerLayers = cms.int32(5),
    jet_dr_max = cms.untracked.double(0.4),
    n_tracks_per_seed_vertex = cms.int32(2),
    max_seed_vertex_chi2 = cms.double(5),

    # Merge / arbitration
    resolve_split_vertices_loose = cms.bool(True),
    resolve_split_vertices_tight = cms.bool(False),
    investigate_merged_vertices = cms.bool(False),
    use_2d_vv_dist_forReco = cms.bool(False),
    use_2d_tv_dist_bsIPpresel = cms.bool(True), # This is used for the 4 sigma cut for the seed tracks AN:138(σdTB,2D > 4)
    use_2d_tv_dist_forReco = cms.bool(False), 
    merge_anyway_dist = cms.double(0.05),
    merge_anyway_sig = cms.double(3),
    merge_shared_dist = cms.double(-1),
    merge_shared_sig = cms.double(4),
    max_track_vertex_dist = cms.double(-1),
    max_track_vertex_sig = cms.double(2),
    min_track_vertex_sig_to_remove = cms.double(1.5),
    remove_one_track_at_a_time = cms.bool(True),
    max_nm1_refit_dist3 = cms.double(0.003),
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
    masshint = cms.untracked.double(400.0),
    decayhint = cms.untracked.double(0.1),

    # Match mode
    match_distance_mode = cms.untracked.string("2D"),  # switch to "3D" for 3D pairing

    # Truth model
    parent_pdgids = cms.untracked.vint32(1000006, -1000006),
    daughter_pdgids = cms.untracked.vint32(1, -1),

    # Vertex cuts
    vtx_chi2_max = cms.double(2.5),
    vtx_dbv_min = cms.double(0.01),
    vtx_dbv_max = cms.double(2.0),
    vtx_tracks_min = cms.uint32(8),
    vtx_ddbv_max = cms.double(0.005),
    vtx_cosT_min = cms.double(0.0),

    # Track WP (seed preselection configured in Vertexer)

    # Jet selection
    jet_pt_min = cms.double(30.0),
    jet_eta_max = cms.double(2.5),
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
