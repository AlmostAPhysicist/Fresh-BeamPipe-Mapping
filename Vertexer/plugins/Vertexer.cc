// -*- C++ -*-
//
// Package:    Run3ScoutingAnalysisTools/Vertexer
// Class:      Vertexer
//
/**\class Vertexer Vertexer.cc Run3ScoutingAnalysisTools/Vertexer/plugins/Vertexer.cc

 Description: [one line class summary]

 Implementation:
     [Notes on implementation]
*/
//
// Original Author:  Bruno Lopes
//         Created:  Wed, 04 Sep 2024 12:37:22 GMT
//
//

// system include files
#include <memory>

#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "MagneticField/Engine/interface/MagneticField.h"

// user include files
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "FWCore/Utilities/interface/ESGetToken.h"
#include "DataFormats/Common/interface/ValueMap.h"

#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/BeamSpot/interface/BeamSpot.h"

//Scouting data formats
#include "DataFormats/Scouting/interface/Run3ScoutingElectron.h"
#include "DataFormats/Scouting/interface/Run3ScoutingPhoton.h"
#include "DataFormats/Scouting/interface/Run3ScoutingPFJet.h"
#include "DataFormats/Scouting/interface/Run3ScoutingVertex.h"
#include "DataFormats/Scouting/interface/Run3ScoutingTrack.h"
#include "DataFormats/Scouting/interface/Run3ScoutingMuon.h"
#include "DataFormats/Scouting/interface/Run3ScoutingParticle.h"

//Vertex tools
#include "RecoVertex/KalmanVertexFit/interface/KalmanVertexFitter.h"
#include "RecoVertex/VertexTools/interface/VertexDistance3D.h"
#include "RecoVertex/VertexTools/interface/VertexDistanceXY.h"
#include "TrackingTools/IPTools/interface/IPTools.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "TrackingTools/TransientTrack/interface/TransientTrack.h"
#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"
#include <unordered_map>

// Helper to accept either 'primaryVertices_src' (new) or 'primaryVertices' (legacy)
namespace {
  edm::InputTag getPVTag(const edm::ParameterSet& p) {
    if (p.existsAs<edm::InputTag>("primaryVertices_src"))
      return p.getParameter<edm::InputTag>("primaryVertices_src");
    if (p.existsAs<edm::InputTag>("primaryVertices"))
      return p.getParameter<edm::InputTag>("primaryVertices");
    return edm::InputTag();
  }
}


using namespace edm;

//
// class declaration
//

class Vertexer : public edm::stream::EDProducer<> {
public:
  ~Vertexer() override;
  explicit Vertexer(edm::ParameterSet const& params);
private:
  typedef std::set<reco::TrackRef> track_set;
  typedef std::vector<reco::TrackRef> track_vec;

  void beginStream(edm::StreamID) override;
  void produce(edm::Event&, const edm::EventSetup&) override;
  void endStream() override;


  const int n_tracks_per_seed_vertex;
  const double max_seed_vertex_chi2;
  const bool use_2d_vertex_dist;
  const bool use_2d_track_dist;
  const bool remove_one_track_at_a_time;
  const double merge_shared_dist;
  const double merge_shared_sig;
  const double max_track_vertex_dist;
  const double max_track_vertex_sig;
  const double min_track_vertex_sig_to_remove;
  const bool resolve_split_vertices_loose;
  const bool resolve_split_vertices_tight;
  const double merge_anyway_sig;
  const double merge_anyway_dist;
  const double max_nm1_refit_dist3;
  const double max_nm1_refit_distz;
  const int max_nm1_refit_count;
  const bool investigate_merged_vertices;
  const bool verbose;

  // new, configurable seed thresholds (default values preserve current behaviour)
  const double minSeedIPSig;
  const double minSeedPt;

  enum class RefMode { UsePVCollection, UseBeamSpot };
  const RefMode refMode_;
  const edm::EDGetTokenT<std::vector<reco::Vertex>> primaryVerticesToken_;  // vector of PVs
  const edm::EDGetTokenT<reco::BeamSpot>            beamspotToken_;         // fallback
  const edm::EDGetTokenT<std::vector<reco::Track>>  seed_tracks_token_;
  const edm::ESGetToken<TransientTrackBuilder, TransientTrackRecord> token_builder;

  edm::EDPutTokenT<reco::VertexCollection> putToken_;

  // ----------member data ---------------------------

  VertexDistanceXY vertex_dist_2d;
  VertexDistance3D vertex_dist_3d;

  bool is_track_subset(const track_set & a, const track_set & b) const {
    bool is_subset = true;
    const track_set& smaller = a.size() <= b.size() ? a : b;
    const track_set& bigger = a.size() <= b.size() ? b : a;
    
    for (auto t : smaller)
      if (bigger.count(t) < 1) {
	is_subset = false;
	break;
      }
    
    return is_subset;
  }
  
  
  Measurement1D vertex_dist(const reco::Vertex & v0, const reco::Vertex & v1) {
    if (use_2d_vertex_dist)
      return vertex_dist_2d.distance(v0, v1);
    else
      return vertex_dist_3d.distance(v0, v1);
  }
  
    track_set vertex_track_set(const reco::Vertex & v, const double min_weight = 0.5) const {
     track_set result;
 
     for (auto it = v.tracks_begin(), ite = v.tracks_end(); it != ite; ++it) {
       const double w = v.trackWeight(*it);
       const bool use = w >= min_weight;
       if (use)
         result.insert(it->castTo<reco::TrackRef>());
     }
 
     return result;
   }

  
  
  std::pair<bool, Measurement1D> track_dist(const reco::TransientTrack& t, const reco::Vertex & v) const {
    if (use_2d_track_dist)
      return IPTools::absoluteTransverseImpactParameter(t, v);
    else
      return IPTools::absoluteImpactParameter3D(t, v);
  }

  track_vec vertex_track_vec(const reco::Vertex & v, const double min_weight = 0.5) const {
    track_set s = vertex_track_set(v, min_weight);
    return track_vec(s.begin(), s.end());
  }

  template <typename T>
  void print_track_set(const T& ts) const {
    for (auto r : ts)
      printf(" %u", r.key());
  }
  
  template <typename T>
  void print_track_set(const T & ts, const reco::Vertex & v) const {
    for (auto r : ts)
      printf(" %u%s", r.key(), (v.trackWeight(r) < 0.5 ? "!" : ""));
  }
  
  void print_track_set(const reco::Vertex & v) const {
    for (auto r = v.tracks_begin(), re = v.tracks_end(); r != re; ++r)
      printf(" %lu%s", r->key(), (v.trackWeight(*r) < 0.5 ? "!" : ""));
  }
  
  
  // REPLACES former global 'KalmanVertexFitter kv_reco;'
  KalmanVertexFitter kv_reco_;

  // Safe drop-in wrapper (replaces free function kv_reco_dropin)
  std::vector<TransientVertex> kv_reco_dropin(std::vector<reco::TransientTrack>& ttks) {
    if (ttks.size() < 2) return {};
    for (auto const& tt : ttks) if (!tt.isValid()) return {};
    std::vector<TransientVertex> v(1, kv_reco_.vertex(ttks));
    if (!v[0].isValid() || v[0].normalisedChiSquared() > 5) return {};
    return v;
  }

  // validity check helper
  bool all_valid(const std::vector<reco::TransientTrack>& vtt) const {
    for (auto const& tt : vtt) if (!tt.isValid()) return false;
    return true;
  }

};

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//

Vertexer::Vertexer(edm::ParameterSet const& params)
  :
  n_tracks_per_seed_vertex(params.getParameter<int>("n_tracks_per_seed_vertex")),
  max_seed_vertex_chi2(params.getParameter<double>("max_seed_vertex_chi2")),
  use_2d_vertex_dist(params.getParameter<bool>("use_2d_vertex_dist")),
  use_2d_track_dist(params.getParameter<bool>("use_2d_track_dist")),
  remove_one_track_at_a_time(params.getParameter<bool>("remove_one_track_at_a_time")),
  merge_shared_dist(params.getParameter<double>("merge_shared_dist")),
  merge_shared_sig(params.getParameter<double>("merge_shared_sig")),
  max_track_vertex_dist(params.getParameter<double>("max_track_vertex_dist")),
  max_track_vertex_sig(params.getParameter<double>("max_track_vertex_sig")),
  min_track_vertex_sig_to_remove(params.getParameter<double>("min_track_vertex_sig_to_remove")),
  resolve_split_vertices_loose(params.getParameter<bool>("resolve_split_vertices_loose")),
  resolve_split_vertices_tight(params.getParameter<bool>("resolve_split_vertices_tight")),
  merge_anyway_sig(params.getParameter<double>("merge_anyway_sig")),
  merge_anyway_dist(params.getParameter<double>("merge_anyway_dist")),
  max_nm1_refit_dist3(params.getParameter<double>("max_nm1_refit_dist3")),
  max_nm1_refit_distz(params.getParameter<double>("max_nm1_refit_distz")),
  max_nm1_refit_count(params.getParameter<int>("max_nm1_refit_count")),
  investigate_merged_vertices(params.getParameter<bool>("investigate_merged_vertices")),
  verbose(params.getParameter<bool>("verbose")),
  
  // read new params (provide same defaults as current hard-coded values)
  minSeedIPSig(params.getUntrackedParameter<double>("minSeedIPSig", 4.0)),
  minSeedPt(params.getUntrackedParameter<double>("minSeedPt", 0.9)),
  refMode_( (params.existsAs<edm::InputTag>("primaryVertices_src") || params.existsAs<edm::InputTag>("primaryVertices"))
              ? RefMode::UsePVCollection : RefMode::UseBeamSpot ),
  primaryVerticesToken_( refMode_ == RefMode::UsePVCollection
      ? consumes<std::vector<reco::Vertex>>( getPVTag(params) )
      : consumes<std::vector<reco::Vertex>>( edm::InputTag() ) ),
  beamspotToken_( refMode_ == RefMode::UseBeamSpot && params.existsAs<edm::InputTag>("beamspot_src")
      ? consumes<reco::BeamSpot>( params.getParameter<edm::InputTag>("beamspot_src") )
      : consumes<reco::BeamSpot>( edm::InputTag() ) ),
  seed_tracks_token_(consumes(params.getParameter<edm::InputTag>("seed_tracks_src"))),
  token_builder(esConsumes(edm::ESInputTag("", "TransientTrackBuilder"))),

  putToken_{produces()} {}


Vertexer::~Vertexer() {}

//
// member functions
//

// ------------ method called to produce the data  ------------
void Vertexer::produce(edm::Event& iEvent, const edm::EventSetup& iSetup) {
  // Retrieve seed tracks
  edm::Handle<std::vector<reco::Track>> seed_track_handle;
  iEvent.getByToken(seed_tracks_token_, seed_track_handle);
  if (!seed_track_handle.isValid()) {
    if (verbose) edm::LogWarning("Vertexer") << "Seed track handle invalid (return empty collection; no fake vertices).";
    iEvent.emplace(putToken_, reco::VertexCollection());
    return;
  }

  // Determine reference vertex (average PV or beamspot)
  double ref_x=0, ref_y=0, ref_z=0;
  reco::Vertex::Error ref_error;              // will set all components explicitly
  for (int i=0;i<3;++i)                       // ensure no uninitialized covariance (prevents NaNs / segfaults)
    for (int j=i;j<3;++j)
      ref_error(i,j)=0.0;
  bool haveRef = false;

  if (refMode_ == RefMode::UsePVCollection) {
    edm::Handle<std::vector<reco::Vertex>> pvHandle;
    iEvent.getByToken(primaryVerticesToken_, pvHandle);
    if (pvHandle.isValid() && !pvHandle->empty()) {
      double sumX=0, sumY=0, sumZ=0; int nGood=0;
      for (auto const& pv : *pvHandle) {
        if (!pv.isFake() && pv.ndof() > 4) {
          sumX += pv.x(); sumY += pv.y(); sumZ += pv.z(); ++nGood;
        }
      }
      if (nGood > 0) {
        ref_x = sumX / nGood; ref_y = sumY / nGood; ref_z = sumZ / nGood;
        ref_error(0,0)=0.0015*0.0015;
        ref_error(1,1)=0.0015*0.0015;
        ref_error(2,2)=0.005*0.005;
        // off-diagonals already zeroed
        haveRef = true;
      }
    }
  } else {
    // BeamSpot fallback
    edm::Handle<reco::BeamSpot> bs;
    iEvent.getByToken(beamspotToken_, bs);
    if (bs.isValid()) {
      ref_x = bs->position().x(); ref_y = bs->position().y(); ref_z = bs->position().z();
      // use diagonal terms; zeroed off-diagonals remain zero
      ref_error(0,0)=bs->covariance()(0,0);
      ref_error(1,1)=bs->covariance()(1,1);
      ref_error(2,2)=bs->covariance()(2,2);
      haveRef = true;
    }
  }

  if (!haveRef) {
    if (verbose) edm::LogWarning("Vertexer") << "No valid reference (PV avg or BeamSpot). Returning empty (no fake substitute).";
    iEvent.emplace(putToken_, reco::VertexCollection());
    return;
  }

  const reco::Vertex fake_ref_vtx(reco::Vertex::Point(ref_x, ref_y, ref_z), ref_error);

  // TransientTrack builder
  auto const& tt_builder = iSetup.getData(token_builder);

  // Seed track selection
  std::vector<reco::TransientTrack> seed_tracks;
  seed_tracks.reserve(seed_track_handle->size());
  std::unordered_map<unsigned int,size_t> seed_track_ref_map;
  seed_track_ref_map.reserve(seed_track_handle->size());

  for (size_t i_tk=0; i_tk<seed_track_handle->size(); ++i_tk) {
    edm::Ref<reco::TrackCollection> tk_ref(seed_track_handle, i_tk);
    reco::TransientTrack ttk = tt_builder.build(tk_ref);
    auto ttk_dist = track_dist(ttk, fake_ref_vtx);
    if (!ttk_dist.first) continue;
    float IP_sig = ttk_dist.second.significance();
    if (std::isfinite(IP_sig) && IP_sig > minSeedIPSig && tk_ref->pt() > minSeedPt) {
      seed_track_ref_map[tk_ref.key()] = seed_tracks.size();
      seed_tracks.emplace_back(std::move(ttk));
    }
    if (verbose)
      printf("Seed preselect: key=%u pt=%.3f IPsig(ref)=%.3f %s\n",
             tk_ref.key(), tk_ref->pt(), IP_sig,
             (IP_sig > minSeedIPSig ? "KEPT" : "REJ"));
  }

  // build safe lambda (bounds + cache) REPLACES previous version
  auto getTransientTrack = [&](auto const& tk) -> reco::TransientTrack {
    const unsigned int k = tk.key();
    auto it = seed_track_ref_map.find(k);
    if (it != seed_track_ref_map.end()) return seed_tracks[it->second];
    if (k < seed_track_handle->size()) {
      edm::Ref<reco::TrackCollection> tr(seed_track_handle, k);
      return tt_builder.build(tr);
    }
    if (verbose) edm::LogWarning("Vertexer") << "getTransientTrack: out-of-range track key " << k;
    return reco::TransientTrack(); // invalid
  };

  //////////////////////////////////////////////////////////////////////
  // Form seed vertices from all pairs of tracks whose vertex fit
  // passes cuts.
  //////////////////////////////////////////////////////////////////////

  const size_t ntk = seed_tracks.size();
  
  std::unique_ptr<reco::VertexCollection> vertices(new reco::VertexCollection);
  
  if (ntk == 0) {
    iEvent.emplace(putToken_, std::move(*vertices));
    return;
  }

  // reserve a reasonable amount to reduce reallocations
  vertices->reserve(std::min<size_t>(ntk * (ntk - 1) / 2, 1024));

  
  std::vector<size_t> itks(n_tracks_per_seed_vertex, 0);

  auto try_seed_vertex = [&]() {
    std::vector<reco::TransientTrack> ttks(n_tracks_per_seed_vertex);
    for (int i = 0; i < n_tracks_per_seed_vertex; ++i) {
      ttks[i] = seed_tracks[itks[i]];
      if (!ttks[i].isValid()) return;  // safety
    }
    TransientVertex seed_vertex = kv_reco_.vertex(ttks);
    if (seed_vertex.isValid() && seed_vertex.normalisedChiSquared() < max_seed_vertex_chi2) {
      vertices->push_back(reco::Vertex(seed_vertex));
      if (verbose) {
        const reco::Vertex& v = vertices->back();
        double vx = v.x() - ref_x, vy = v.y() - ref_y, vz = v.z() - ref_z;
        printf("from tracks"); for (auto itk : itks) printf(" %zu", itk);
        printf(": vertex #%zu: chi2/dof: %7.3f dof: %7.3f pos(ref): <%7.3f,%7.3f,%7.3f>\n",
               vertices->size() - 1, v.normalizedChi2(), v.ndof(), vx, vy, vz);
      }
    }
  };

  // ha
  for (size_t itk = 0; itk < ntk; ++itk) {
    itks[0] = itk;
    for (size_t jtk = itk + 1; jtk < ntk; ++jtk) {
      itks[1] = jtk;
      if (n_tracks_per_seed_vertex == 2) { try_seed_vertex(); continue; }
      for (size_t ktk = jtk + 1; ktk < ntk; ++ktk) {
        itks[2] = ktk;
        if (n_tracks_per_seed_vertex == 3) { try_seed_vertex(); continue; }
        for (size_t ltk = ktk + 1; ltk < ntk; ++ltk) {
          itks[3] = ltk;
          if (n_tracks_per_seed_vertex == 4) { try_seed_vertex(); continue; }
          for (size_t mtk = ltk + 1; mtk < ntk; ++mtk) {
            itks[4] = mtk;
            try_seed_vertex();
          }
        }
      }
    }
  }

  //////////////////////////////////////////////////////////////////////
  // Take care of track sharing. If a track is in two vertices, and
  // the vertices are "close", refit the tracks from the two together
  // as one vertex. If the vertices are not close, keep the track in
  // the vertex to which it is "closer".
  //////////////////////////////////////////////////////////////////////
  
  //printf("entering the track sharing part\n");
  
  track_set discarded_tracks;
  int n_resets = 0;
  int n_onetracks = 0;
  std::vector<reco::Vertex>::iterator v[2];

  size_t ivtx[2];
  
  for (v[0] = vertices->begin(); v[0] != vertices->end(); ++v[0]) {
    track_set tracks[2];
    ivtx[0] = v[0] - vertices->begin();
    tracks[0] = vertex_track_set(*v[0]);
    
    if (tracks[0].size() < 2) {
      if (verbose)
        printf("track-sharing: vertex-0 #%zu is down to one track, junking it\n", ivtx[0]);
      v[0] = vertices->erase(v[0]) - 1;
      ++n_onetracks;
      continue;
    }

    bool duplicate = false;
    bool merge = false;
    bool refit = false;
    track_set tracks_to_remove_in_refit[2];
    
    for (v[1] = v[0] + 1; v[1] != vertices->end(); ++v[1]) {
      ivtx[1] = v[1] - vertices->begin();
      tracks[1] = vertex_track_set(*v[1]);

      if (tracks[1].size() < 2) {
        if (verbose)
          printf("track-sharing: vertex-1 #%zu is down to one track, junking it\n", ivtx[1]);	
	v[1] = vertices->erase(v[1]) - 1;
        ++n_onetracks;
        continue;
      }


      
      if (verbose) {
        printf("track-sharing: # vertices = %zu. considering vertices #%zu (chi2/dof %.3f, track set",
               vertices->size(), ivtx[0], v[0]->chi2() / v[0]->ndof());
        print_track_set(tracks[0], *v[0]);
        printf(") and #%zu (chi2/dof %.3f, track set", ivtx[1], v[1]->chi2() / v[1]->ndof());
        print_track_set(tracks[1], *v[1]);
        printf("):\n");
      }

      
      if (is_track_subset(tracks[0], tracks[1])) {
        if (verbose)
          printf("   subset/duplicate vertices %zu and %zu, erasing second and starting over\n", ivtx[0], ivtx[1]);
        duplicate = true;
        break;
      }

      std::vector<reco::TrackRef> shared_tracks;
      for (auto tk : tracks[0])
        if (tracks[1].count(tk) > 0)
          shared_tracks.push_back(tk);

      if (verbose) {
        if (shared_tracks.size()) {
          printf("   shared tracks are: ");
          print_track_set(shared_tracks);
          printf("\n");
        }
        else
          printf("   no shared tracks\n");
      }

      
      if (shared_tracks.size() > 0){
	Measurement1D v_dist = vertex_dist(*v[0], *v[1]);
	
        if (verbose)
          printf("   vertex dist (2d? %i) %7.3f  sig %7.3f\n", use_2d_vertex_dist, v_dist.value(), v_dist.significance());
	
	if (v_dist.value() < merge_shared_dist || v_dist.significance() < merge_shared_sig) {
          if (verbose) printf("          dist < %7.3f || sig < %7.3f, will try using merge result first before arbitration\n", merge_shared_dist, merge_shared_sig);
	  merge = true;
      }
	else
	  refit = true;
	
        
	for (auto tk : shared_tracks) {
	  reco::TransientTrack ttk = getTransientTrack(tk);            // was 'const & ttk' (dangling)
	  if (!ttk.isValid()) continue;                                // new guard
	  auto t_dist_0 = track_dist(ttk, *v[0]);
	  auto t_dist_1 = track_dist(ttk, *v[1]);
	  
	  
	  t_dist_0.first = t_dist_0.first && (t_dist_0.second.value() < max_track_vertex_dist || t_dist_0.second.significance() < max_track_vertex_sig);
	  t_dist_1.first = t_dist_1.first && (t_dist_1.second.value() < max_track_vertex_dist || t_dist_1.second.significance() < max_track_vertex_sig);
	  bool remove_from_0 = !t_dist_0.first;
	  bool remove_from_1 = !t_dist_1.first;
	  if (t_dist_0.second.significance() < min_track_vertex_sig_to_remove && t_dist_1.second.significance() < min_track_vertex_sig_to_remove) {
	    if (tracks[0].size() > tracks[1].size())
	    remove_from_1 = true;
	    else
	      remove_from_0 = true;
	  }
	  else if (t_dist_0.second.significance() < t_dist_1.second.significance())
	    remove_from_1 = true;
	  else
	    remove_from_0 = true;
	  
	  if (remove_from_0) tracks_to_remove_in_refit[0].insert(tk);
	  if (remove_from_1) tracks_to_remove_in_refit[1].insert(tk);
	  
	  if (remove_one_track_at_a_time) break;
	}
	
	break;
	
      }
    }
        
    if (duplicate) {
      vertices->erase(v[1]);
    }

    else if (merge) {
      track_set tracks_to_fit;
      for (int i = 0; i < 2; ++i)
        for (auto tk : tracks[i])
          tracks_to_fit.insert(tk);

      std::vector<reco::TransientTrack> ttks;
      ttks.reserve(tracks_to_fit.size());
      for (auto tk : tracks_to_fit) {
        auto tt = getTransientTrack(tk);
        if (tt.isValid()) ttks.push_back(tt);
      }

      reco::VertexCollection new_vertices;
      for (const TransientVertex& tv : kv_reco_dropin(ttks))
        new_vertices.emplace_back(tv);

      // If we got two new vertices, maybe it took A B and A C D and made a better one from B C D, and left a broken one A B! C! D!.
      // If we get one that is truly the merger of the track lists, great. If it is just something like A B , A C . A B C!, or we get nothing, then default to arbitration.
      if (new_vertices.size() > 1) {
        assert(new_vertices.size() == 2);
        *v[1] = reco::Vertex(new_vertices[1]);
        *v[0] = reco::Vertex(new_vertices[0]);
	
      }
      else if (new_vertices.size() == 1 && vertex_track_set(new_vertices[0], 0) == tracks_to_fit) {
        vertices->erase(v[1]);
	
        *v[0] = reco::Vertex(new_vertices[0]); // ok to use v[0] after the erase(v[1]) because v[0] is by construction before v[1]
	
      }
      else refit = true;
      
    }
      if (refit) {
	bool erase[2] = { false };
	reco::Vertex vsave[2] = { *v[0], *v[1] };
	
	for (int i = 0; i < 2; ++i) {
	  if (tracks_to_remove_in_refit[i].empty())
	    continue;

        std::vector<reco::TransientTrack> ttks;
        for (auto tk : tracks[i]) {
          if (tracks_to_remove_in_refit[i].count(tk) == 0) {
            auto tt = getTransientTrack(tk);
            if (tt.isValid()) ttks.push_back(tt);
          }
        }
        reco::VertexCollection new_vertices;
        for (const TransientVertex& tv : kv_reco_dropin(ttks))
          new_vertices.emplace_back(tv);
        if (new_vertices.size() == 1)
          * v[i] = new_vertices[0];
        else
          erase[i] = true;
	}

      if (erase[1]) vertices->erase(v[1]);
      if (erase[0]) vertices->erase(v[0]);

      }

    // If we changed the vertices at all, start loop over completely.
    if (duplicate || merge || refit) {
      //printf("duplicate = %d, merge = %d, refit = %d\n", duplicate, merge, refit);
      
      v[0] = vertices->begin() - 1;  // -1 because about to ++sv
      ++n_resets;
      
      //if (n_resets == 3000)
      //  throw "I'm dumb";
    }
  }

//checkpoint
  
  //////////////////////////////////////////////////////////////////////////////////////////////
  // Merge vertices that are still "close" in 2D, aka "loose" merging (typically off by default)
  //////////////////////////////////////////////////////////////////////////////////////////////


  if (resolve_split_vertices_loose) {

    
    if (merge_anyway_sig > 0 || merge_anyway_dist > 0) {
      //double v0x;
      //double v0y;
      //double phi0;
      
      for (v[0] = vertices->begin(); v[0] != vertices->end(); ++v[0]) {
        //ivtx[0] = v[0] - vertices->begin();

        //double v1x;
        //double v1y;
        //double phi1;

        for (v[1] = v[0] + 1; v[1] != vertices->end(); ++v[1]) {

          //ivtx[1] = v[1] - vertices->begin();


          Measurement1D v_dist = vertex_dist(*v[0], *v[1]);

          //v0x = v[0]->x() - bsx;
          //v0y = v[0]->y() - bsy;
          //phi0 = atan2(v0y, v0x);
          //v1x = v[1]->x() - bsx;
          //v1y = v[1]->y() - bsy;
          //phi1 = atan2(v1y, v1x);

          // Distances & angles always evaluated w.r.t. reference vertex (avgPV or beamspot)
          if (v_dist.value() < merge_anyway_dist || v_dist.significance() < merge_anyway_sig) {

            std::vector<reco::TransientTrack> ttks;

            for (int i = 0; i < 2; ++i) {
              for (auto tk : vertex_track_set(*v[i])) {
                auto tt = getTransientTrack(tk);
                if (tt.isValid()) ttks.push_back(tt);
              }
            }

            reco::VertexCollection merged_vertices;
            for (const TransientVertex& tv : kv_reco_dropin(ttks)) {
              merged_vertices.push_back(reco::Vertex(tv));

              for (auto it = merged_vertices[0].tracks_begin(), ite = merged_vertices[0].tracks_end(); it != ite; ++it) {
                reco::TransientTrack seed_track = getTransientTrack(*it);
                std::pair<bool, Measurement1D> tk_vtx_dist = track_dist(seed_track, merged_vertices[0]);
              }
            }

	    if (merged_vertices.size() == 1) {
              
              //std::cout << "check no mem out of ranges (before) : " << v[1] - vertices->begin() << std::endl;
              *v[0] = merged_vertices[0];
              //std::cout << "check no mem out of ranges (after) : " << v[1] - vertices->begin() << std::endl;

              v[1] = vertices->erase(v[1]) - 1;
            }
          }
        }
      }
    }
  }
  
  //////////////////////////////////////////////////////////////////////
  // Drop tracks that "move" the vertex too much by refitting without each track.
  //////////////////////////////////////////////////////////////////////
  
  if (max_nm1_refit_dist3 > 0 || max_nm1_refit_distz > 0) {
    std::vector<int> refit_count(vertices->size(), 0);

    int iv = 0;
    for (v[0] = vertices->begin(); v[0] != vertices->end(); ++v[0], ++iv) {
      if (max_nm1_refit_count > 0 && refit_count[iv] >= max_nm1_refit_count)
        continue;

      const track_vec tks = vertex_track_vec(*v[0]);
      const size_t ntks = tks.size();
      if (ntks < 3)
        continue;


      std::vector<reco::TransientTrack> ttks(ntks - 1);
      for (size_t i = 0; i < ntks; ++i) {
        for (size_t j = 0; j < ntks; ++j)
          if (j != i)
            ttks[j - (j >= i)] = getTransientTrack(tks[j]);
        if (!all_valid(ttks)) continue;                 // NEW guard
        TransientVertex tv_nm1 = kv_reco_.vertex(ttks); // split for clarity
        reco::Vertex vnm1(tv_nm1);
        const double dist3_2 = (vnm1.x() - v[0]->x())*(vnm1.x() - v[0]->x()) + (vnm1.y() - v[0]->y())*(vnm1.y() - v[0]->y()) + (vnm1.z() - v[0]->z())*(vnm1.z() - v[0]->z());
        const double distz = sqrt( (vnm1.z() - v[0]->z()) * (vnm1.z() - v[0]->z()) );

        if (vnm1.chi2() < 0 ||
            (max_nm1_refit_dist3 > 0 && dist3_2 > pow(max_nm1_refit_dist3, 2)) ||
            (max_nm1_refit_distz > 0 && distz > max_nm1_refit_distz)) {

          *v[0] = vnm1;
          ++refit_count[iv];
          --v[0], --iv;
          break;
        }
      }
    }
    iv = 0; //some vertices after dz refiting have normalized chi2 > 5
    for (v[0] = vertices->begin(); v[0] != vertices->end(); ++v[0], ++iv) {
       if ((*v[0]).normalizedChi2() > 5) {
         v[0] = vertices->erase(v[0]) - 1;
         continue;
       }
    }
  }


  /////////////////////////////////////////////////////////////////////////////////////////////////////
  // Merge every pair of output vertices that satisfy the following criteria to resolve split-vertices:
  //   - >=2trk/vtx
  //   - dBV > 100 um
  //   - |dPhi(vtx0,vtx1)| < 0.5 
  //   - svdist2d < 300 um
  // Note that the merged vertex must pass chi2/dof < 5
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  
  if (resolve_split_vertices_tight) {
    reco::VertexCollection potential_merged_vertices;

    for (v[0] = vertices->begin(); v[0] != vertices->end(); ++v[0]) {

      track_set tracks[2];
      tracks[0] = vertex_track_set(*v[0]);

      bool merge = false;
      for (v[1] = v[0] + 1; v[1] != vertices->end(); ++v[1]) {
        if (vertices->size() >= 2 && v[0]->nTracks() >= 2 && v[1]->nTracks() >= 2) {

          tracks[1] = vertex_track_set(*v[1]);

          Measurement1D v_dist = vertex_dist_2d.distance(*v[0], *v[1]);

          // Distances/angles relative to reference (avgPV or beamspot)
          Measurement1D dBV0_Meas1D = vertex_dist_2d.distance(*v[0], fake_ref_vtx);
          Measurement1D dBV1_Meas1D = vertex_dist_2d.distance(*v[1], fake_ref_vtx);
          double dBV0 = dBV0_Meas1D.value();  // added
          double dBV1 = dBV1_Meas1D.value();  // added
          double v0x = v[0]->x() - ref_x;
          double v0y = v[0]->y() - ref_y;
          double phi0 = atan2(v0y, v0x);
          double v1x = v[1]->x() - ref_x;
          double v1y = v[1]->y() - ref_y;
          double phi1 = atan2(v1y, v1x);
          if (fabs(reco::deltaPhi(phi0, phi1)) < 0.5 && v_dist.value() < 0.0300 && dBV0 > 0.0100 && dBV1 > 0.0100) {
            track_set tracks_to_fit;
            for (int i = 0; i < 2; ++i)
              for (auto tk : tracks[i])
                tracks_to_fit.insert(tk);
            std::vector<reco::TransientTrack> ttks;
            for (auto tk : tracks_to_fit)
              ttks.push_back(getTransientTrack(tk));

            if (investigate_merged_vertices) {
              if (all_valid(ttks)) { // NEW guard
                std::vector<TransientVertex> tv(1, kv_reco_.vertex(ttks));
                if (tv[0].isValid())
                  potential_merged_vertices.push_back(reco::Vertex(tv[0]));
              }
            }

            reco::VertexCollection merged_vertices;
            for (const TransientVertex& tv : kv_reco_dropin(ttks)) {
              merged_vertices.push_back(reco::Vertex(tv));
            }

            if (merged_vertices.size() == 1 && vertex_track_set(merged_vertices[0], 0) == tracks_to_fit) {

              merge = true;

              v[1] = vertices->erase(v[1]) - 1; // (1) erase and point the iterator at the previous entry
              *v[0] = reco::Vertex(merged_vertices[0]); // (2) updated v[0] (ok to use v[0] after the erase(v[1]) because v[0] is by construction before v[1])
            }
          }
        }
      }
	  // going through all the pairs of of v[1] and a fixed v[0] for merging, if merge happens (1) each v[1] is erased (2) v[0] is updated (recurring until exit loop) (3) reset the combination again
	  if (merge)
		  v[0] = vertices->begin() - 1; // (3) reset the combination if a valid merge happens 

    }
  }



  //////////////////////////////////////////////////////////////////////
  // Put the output.
  //////////////////////////////////////////////////////////////////////
  
  //Save the vertices
  iEvent.emplace(putToken_, std::move(*vertices));

}

// ------------ method called once each stream before processing any runs, lumis or events  ------------
void Vertexer::beginStream(edm::StreamID) {
  // please remove this method if not needed
}

// ------------ method called once each stream after processing all runs, lumis and events  ------------
void Vertexer::endStream() {
  // please remove this method if not needed
}

// ------------ method called when starting to processes a run  ------------
/*
void
Vertexer::beginRun(edm::Run const&, edm::EventSetup const&)
{
}
*/

// ------------ method called when ending the processing of a run  ------------
/*
void
Vertexer::endRun(edm::Run const&, edm::EventSetup const&)
{
}
*/

// ------------ method called when starting to processes a luminosity block  ------------
/*
void
Vertexer::beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
}
*/

// ------------ method called when ending the processing of a luminosity block  ------------
/*
void
Vertexer::endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
}
*/

/* ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void Vertexer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}
*/

//define this as a plug-in
DEFINE_FWK_MODULE(Vertexer);

// NOTE: Only intentional functional change from original version is the ability to use an averaged
//       primary-vertex reference (or beamspot fallback) with a fully initialized covariance.
//       All vertex finding, merging, refit, and seed selection logic (including IPSig & pT cuts)
//       remains otherwise identical.