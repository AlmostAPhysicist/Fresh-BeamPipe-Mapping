import FWCore.ParameterSet.Config as cms

process = cms.Process("VertexProducer")

process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkSummary.reportEvery = 2500
process.MessageLogger.cerr.FwkReport.reportEvery = 2500

process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(-1))

process.source = cms.Source("PoolSource",
    fileNames=cms.untracked.vstring('file://scout.root')
)

process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load("Configuration.StandardSequences.MagneticField_cff")
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')

from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '133X_mcRun3_2024_realistic_v9', '')

process.load("RecoVertex.BeamSpotProducer.BeamSpot_cfi")
process.load("TrackingTools.TransientTrack.TransientTrackBuilder_cfi")

process.Vertexer = cms.EDProducer('Vertexer',
    seed_tracks_src=cms.InputTag('hltScoutingUnpackProducer', 'Track'),
    beamspot_src=cms.InputTag('offlineBeamSpot'),
    n_tracks_per_seed_vertex=cms.int32(2),
    max_seed_vertex_chi2=cms.double(5),
    use_2d_vertex_dist=cms.bool(False),
    use_2d_track_dist=cms.bool(False),
    merge_anyway_dist=cms.double(-1),
    merge_anyway_sig=cms.double(4),
    merge_shared_dist=cms.double(-1),
    merge_shared_sig=cms.double(4),
    max_track_vertex_dist=cms.double(-1),
    max_track_vertex_sig=cms.double(5),
    min_track_vertex_sig_to_remove=cms.double(1.5),
    remove_one_track_at_a_time=cms.bool(True),
    max_nm1_refit_dist3=cms.double(-1),
    max_nm1_refit_distz=cms.double(0.005),
    max_nm1_refit_count=cms.int32(-1),
    resolve_split_vertices_loose=cms.bool(False),
    resolve_split_vertices_tight=cms.bool(True),
    investigate_merged_vertices=cms.bool(False),
    verbose=cms.bool(False)
)

process.out = cms.OutputModule("PoolOutputModule",
    fileName=cms.untracked.string('DY2M_VertexerOutput.root')
)

process.p = cms.Path(process.offlineBeamSpot + process.Vertexer)
process.e = cms.EndPath(process.out)
