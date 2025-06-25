import FWCore.ParameterSet.Config as cms

process = cms.Process("VertexProducer")

process.load("FWCore.MessageService.MessageLogger_cfi")
#process.options = cms.untracked.PSet(
#    wantSummary = cms.untracked.bool(True)
#)

# process.MessageLogger.cerr.FwkSummary.reportEvery = 100
process.MessageLogger.cerr.FwkReport.reportEvery = 1000

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(-1) )

process.source = cms.Source("PoolSource",
    # Test file generated on CMSSW 13.3.0
    fileNames = cms.untracked.vstring( 
        # 'file:///afs/cern.ch/user/a/amalhotr/hexcms_mount/ad749229-3094-410f-94e3-2e408518e395.root'
        'file:root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2560000/ad749229-3094-410f-94e3-2e408518e395.root'
    )
)


process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load("Configuration.StandardSequences.MagneticField_cff")
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')


#Choosing the GlobalTag  
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '133X_mcRun3_2024_realistic_v9', '')  

process.load("RecoVertex.BeamSpotProducer.BeamSpot_cfi")
process.load("TrackingTools.TransientTrack.TransientTrackBuilder_cfi")

# Input tags to the EDProducer
process.Vertexer = cms.EDProducer('Vertexer',

                                #   seed_tracks_src = cms.InputTag('HLTSCOUT', 'Track'), # 'hltScoutingUnpackProducer', 'Track' -> ModuleLabel and Label(productInstanceName) for the type std::vector<reco::Track> -> Initial Attempt
                                # seed_tracks_src = cms.InputTag('hltScoutingTrackPacker', 'Track'), # 'hltScoutingUnpackProducer', 'Track' -> ModuleLabel and Label(productInstanceName) for the type std::vector<reco::Track> Within RecreatedScoutUnpack.root
                                seed_tracks_src = cms.InputTag('displacedTracks', ''), # 'hltScoutingUnpackProducer', 'Track' -> ModuleLabel and Label(productInstanceName) for the type std::vector<reco::Track> Within input file -> vector<reco::Track>                   "displacedTracks"           ""                "RECO" ( Type                                  Module                      Label             Process)
                                  #There's only one object with module 'displacedTracks' and label '' in the input file which makes sense since it's the only object of type reco::Track that we would want.
                                  #kvr_params = kvr_params,
                                  #do_track_refinement = cms.bool(False), # remove tracks + trim out tracks with IP significance larger than trackrefine_sigmacut and trackrefine_trimmax, respectively   
                                  resolve_split_vertices_loose = cms.bool(False), # an alternative merging routine with `loose` criteria, to merge any nearby vertices within a given dist or significance
                                  resolve_split_vertices_tight = cms.bool(False), # merging routine, based on vtx dphi and dVV -> Changed from True to False since we do not want quality cuts on the merged vertices -> Vertices nearby in phi and z are merged which would reduce the number of vertices
                                  investigate_merged_vertices = cms.bool(False), # investigate quality cuts on merged vertices from tight merging 
                                  #resolve_shared_jets = cms.bool(True),       # shared-jet mitigation
                                  #resolve_shared_jets_src = cms.InputTag('selectedPatJets'), 
                                  beamspot_src = cms.InputTag('offlineBeamSpot'),
                                  n_tracks_per_seed_vertex = cms.int32(2),
                                  max_seed_vertex_chi2 = cms.double(-1), # default seed vertex quality cut -> Changed from 5 to -1 since we do not want quality cuts on the merged vertices
                                  use_2d_vertex_dist = cms.bool(False),
                                  use_2d_track_dist = cms.bool(False),
                                  merge_anyway_dist = cms.double(-1),
                                  merge_anyway_sig = cms.double(4), # merging criteria for loose merging (*only* if resolve_split_vertices_loose is True)
                                  merge_shared_dist = cms.double(-1),
                                  merge_shared_sig = cms.double(-1), # default merging shared-track vertices  -> Changed from 4 to -1 since we do not want quality cuts on the merged vertices
                                  max_track_vertex_dist = cms.double(-1),
                                  max_track_vertex_sig = cms.double(-1), # default track arbitration -> Changed from 5 to -1 since we do not want quality cuts on the merged vertices 
                                  #"Yes, it indeed should not be discarding other reco-level collections: but it may have requirements on the quality of tracks used to build the vertices (a common choice would be a large transverse impact parameter, dxy, relative to its uncertainty dxyerr. Their ratio, |dxy|/dxyerr may also be called the dxy significance)."
                                  min_track_vertex_sig_to_remove = cms.double(0), # default track arbitration -> Changed from 1.5 to 0 since we do not want quality cuts on the merged vertices
                                  remove_one_track_at_a_time = cms.bool(True),
                                  max_nm1_refit_dist3 = cms.double(-1),
                                  max_nm1_refit_distz = cms.double(-1), # default track arbitration -> Changed from 0.005 to -1 since we do not want quality cuts on the merged vertices
                                  max_nm1_refit_count = cms.int32(-1),
                                  #trackrefine_sigmacut = cms.double(5), # track refinement criteria (*only* if do_track_refinement = True)
                                  #trackrefine_trimmax = cms.double(5), # track refinement criteria (*only* if do_track_refinement = True)
                                  verbose = cms.bool(False),
                                  )

# Save only the scouting collections on the output file
process.out = cms.OutputModule("PoolOutputModule",
    fileName = cms.untracked.string('/eos/user/a/amalhotr/DY2M_VertexerOutputScreen.root'),
)

# Usually it is better to put producers on a task instead of a path
# but paths also work.
process.p = cms.Path(process.offlineBeamSpot+process.Vertexer)
process.e = cms.EndPath(process.out)
