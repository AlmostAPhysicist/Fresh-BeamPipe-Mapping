# run_compare_cfg.py
import FWCore.ParameterSet.Config as cms
process = cms.Process("COMPARE")

process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = 1
process.options = cms.untracked.PSet(wantSummary = cms.untracked.bool(True))

process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(-1))

process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(
        # <-- set your input files here
        "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v3/110000/017d2bf5-4f3c-40ab-b6f9-c42381b6ae9b.root"
    )
)

# Global tag and geometry
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '140X_mcRun3_2024_realistic_v26', '')

process.load("Configuration.StandardSequences.GeometryRecoDB_cff")
process.load("Configuration.StandardSequences.MagneticField_cff")

# TFileService
process.TFileService = cms.Service("TFileService",
    fileName = cms.string("EventCompare_Output.root")
)

# BeamSpot & transient track builder
process.load("RecoVertex.BeamSpotProducer.BeamSpot_cfi")
process.load("TrackingTools.TransientTrack.TransientTrackBuilder_cfi")

# PackedCandidate -> Track (for offline MINIAOD)
process.packedCandidateToTrack = cms.EDProducer("PackedCandidateToTrackProducer",
    src = cms.InputTag("packedPFCandidates")
)

# HLT Scouting Unpack Producer (scouting)
process.hltScoutingUnpackProducer = cms.EDProducer('HLTScoutingUnpackProducer',
    scoutingTrack = cms.InputTag('hltScoutingTrackPacker'),
    scoutingPrimaryVertex = cms.InputTag('hltScoutingPrimaryVertexPacker', 'primaryVtx'),
    pfCand = cms.InputTag(''),
    lostTrack = cms.InputTag(''),
    isScouting = cms.bool(True),
    producePFCHSCandidate = cms.bool(False),
    mightGet = cms.optional.untracked.vstring
)

# Vertexer for offline (run on packedCandidateToTrack output)
process.VertexerOffline = cms.EDProducer('Vertexer',
    seed_tracks_src = cms.InputTag('packedCandidateToTrack','Track'),
    primaryVertices = cms.InputTag('offlineSlimmedPrimaryVertices'),
    beamspot_src = cms.InputTag('offlineBeamSpot'),
    refPreference = cms.untracked.string('BS'),
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
    max_track_vertex_sig = cms.double(5),
    min_track_vertex_sig_to_remove = cms.double(1.5),
    remove_one_track_at_a_time = cms.bool(True),
    max_nm1_refit_dist3 = cms.double(-1),
    max_nm1_refit_distz = cms.double(-1),
    max_nm1_refit_count = cms.int32(-1),
    verbose = cms.bool(False)
)

# Vertexer for scouting (unchanged)
process.Vertexer = cms.EDProducer('Vertexer',
    seed_tracks_src = cms.InputTag('hltScoutingUnpackProducer', 'Track'),
    primaryVertices = cms.InputTag("hltScoutingPrimaryVertexPacker","primaryVtx"),
    beamspot_src = cms.InputTag('offlineBeamSpot'),
    refPreference = cms.untracked.string('BS'),
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
    max_track_vertex_sig = cms.double(5),
    min_track_vertex_sig_to_remove = cms.double(1.5),
    remove_one_track_at_a_time = cms.bool(True),
    max_nm1_refit_dist3 = cms.double(-1),
    max_nm1_refit_distz = cms.double(-1),
    max_nm1_refit_count = cms.int32(-1),
    verbose = cms.bool(False)
)

# Analyzer
process.eventComparator2 = cms.EDAnalyzer('EventComparator2Analyzer',
    displacedVerticesOffline = cms.InputTag("VertexerOffline"),
    displacedVerticesScouting = cms.InputTag("Vertexer"),
    tracksOffline = cms.InputTag("packedCandidateToTrack","Track"),
    tracksScouting = cms.InputTag("hltScoutingUnpackProducer","Track"),
    nEventsToSave = cms.int32(10),
    vertex_min_ntracks = cms.int32(2),
    vertex_max_chi2 = cms.double(-1.0)
)

# Path: produce offline tracks -> vertexer offline, produce scouting tracks -> vertexer, then analyzer
process.p = cms.Path(
    process.packedCandidateToTrack +
    process.offlineBeamSpot +
    process.VertexerOffline +
    process.hltScoutingUnpackProducer +
    process.Vertexer +
    process.eventComparator2
)