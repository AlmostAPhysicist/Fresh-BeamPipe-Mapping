# runEventComparator_cfg.py
# Unified python config that:
#  - runs HLTScoutingUnpackProducer (unchanged)
#  - runs Vertexer (unchanged)
#  - runs the EventComparatorAnalyzer to produce per-event folders with overlays
#
# Edit fileNames, global tag, and input tags to match your setup.

import FWCore.ParameterSet.Config as cms
process = cms.Process("COMPARE")

process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = 100

process.options = cms.untracked.PSet(
    wantSummary = cms.untracked.bool(True)
)

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(-1)
)

process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(
        # ADD YOUR INPUTS HERE (offline MINIAOD / scouting files)
        # Example:
        # "root://cmsxrootd.fnal.gov//store/data/Run2024H/ScoutingPFRun3/HLTSCOUT/....root"
    )
)

# Global tag & geometry
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '140X_mcRun3_2024_realistic_v26', '')

process.load("Configuration.StandardSequences.GeometryRecoDB_cff")
process.load("Configuration.StandardSequences.MagneticField_cff")

# TFileService: file containing histograms and canvases created by analyzer
process.TFileService = cms.Service("TFileService",
    fileName = cms.string("EventComparator_Output.root")
)

# --- HLT Scouting Unpack Producer (do NOT modify per your constraint) ---
process.hltScoutingUnpackProducer = cms.EDProducer('HLTScoutingUnpackProducer',
    scoutingTrack = cms.InputTag('hltScoutingTrackPacker'),
    scoutingPrimaryVertex = cms.InputTag('hltScoutingPrimaryVertexPacker', 'primaryVtx'),
    pfCand = cms.InputTag(''),
    lostTrack = cms.InputTag(''),
    isScouting = cms.bool(True),
    producePFCHSCandidate = cms.bool(False),
    mightGet = cms.optional.untracked.vstring
)

# --- Vertexer (do NOT modify) ---
process.load("RecoVertex.BeamSpotProducer.BeamSpot_cfi")
process.load("TrackingTools.TransientTrack.TransientTrackBuilder_cfi")

process.Vertexer = cms.EDProducer('Vertexer',
    seed_tracks_src = cms.InputTag('hltScoutingUnpackProducer', 'Track'),
    primaryVertices = cms.InputTag("hltScoutingUnpackProducer", "PrimaryVertex"),
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
    verbose = cms.bool(False),
)

# --- Our new analyzer (EventComparator) ---
process.eventComparator = cms.EDAnalyzer('EventComparatorAnalyzer',
    displacedVerticesOffline = cms.InputTag("offlinePrimaryVertices"),   # example: adjust to your offline displaced vertices
    displacedVerticesScouting = cms.InputTag("Vertexer"),               # scouting displaced vertices produced by Vertexer
    tracksOffline = cms.InputTag("generalTracks"),                      # adjust if offline tracks are named differently
    tracksScouting = cms.InputTag("hltScoutingUnpackProducer","Track"),
    nEventsToSave = cms.int32(10),      # first N events that have a selected vertex (either offline or scouting)
    vertex_min_ntracks = cms.int32(3),  # minimal tracks in a vertex to count as 'selected'
    vertex_max_chi2 = cms.double(4.0)  # negative => no chi2 cut
)

# Path: unpack scouting, beamspot, vertexer, analyzer
process.p = cms.Path(
    process.hltScoutingUnpackProducer +
    process.offlineBeamSpot +  # make sure beamspot is available
    process.Vertexer +
    process.eventComparator
)