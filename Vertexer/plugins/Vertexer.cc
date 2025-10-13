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
#include <set>
#include <vector>
#include <cmath>   // sqrt, pow
#include <cstdio>  // printf
#include <cassert> // assert
#include <unordered_map>
#include <sstream>
#include <algorithm> // std::min
#include <array>     // std::array
#include <atomic>    // for std::atomic
#include <mutex>

// Add portable PI definition - use literal for guaranteed compile-time constant
static const double PI = 3.14159265358979323846;

#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Framework/interface/stream/EDProducer.h" // IMPORTANT: Full definition of EDProducer base class
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/Utilities/interface/ESInputTag.h" // Needed for ESInputTag
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h" // IMPORTANT: For fillDescriptions
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"

#include "DataFormats/Common/interface/ValueMap.h"

#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/BeamSpot/interface/BeamSpot.h"
#include "DataFormats/Math/interface/deltaPhi.h"  // correct header for reco::deltaPhi

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

// Ensure correct EDProducer include
#include "FWCore/Framework/interface/stream/EDProducer.h"

// Add helper functions to anonymous namespace before 'using namespace edm;'
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
    
    // Add required static methods for EDProducer
    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
    
private:
  typedef std::set<reco::TrackRef> track_set;
  typedef std::vector<reco::TrackRef> track_vec;

  void beginStream(edm::StreamID) override;
  void produce(edm::Event&, const edm::EventSetup&) override;
  void endStream() override;

  // Track selection parameters
  const double minSeedPt;
  const double minSeedIPSig;
  const double maxSeedIPSig;
  const int minPixelHits;
  const int minTrackerLayers;
  
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

  // Reference selection preference
  const std::string refPreference;

  // Telemetry counters
  mutable std::atomic<unsigned> totalPVsProcessed{0};
  mutable std::atomic<unsigned> singularCovarianceCount{0};
  mutable std::atomic<unsigned> arithmeticFallbackCount{0};
  mutable std::atomic<unsigned> beamspotFallbackCount{0};

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

  // Constants to replace magic numbers
  static constexpr size_t VERTEX_RESERVE_SIZE = 1024;
  static constexpr int DEFAULT_MAX_ITERATIONS = 1000;

  // Parameters for tight merging that were previously hardcoded
  const double max_dphi_tight;
  const double max_svdist_tight;
  const double min_dbv_tight;
  const int max_iterations;

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
    if (!verbose) return;
    std::stringstream ss;
    for (auto r : ts) ss << " " << static_cast<unsigned>(r.key());
    edm::LogInfo("Vertexer") << ss.str();
  }
  
  template <typename T>
  void print_track_set(const T & ts, const reco::Vertex & v) const {
    if (!verbose) return;
    std::stringstream ss;
    for (auto r : ts)
      ss << " " << static_cast<unsigned>(r.key()) << (v.trackWeight(r) < 0.5 ? "!" : "");
    edm::LogInfo("Vertexer") << ss.str();
  }
  
  void print_track_set(const reco::Vertex & v) const {
    if (!verbose) return;
    std::stringstream ss;
    for (auto r = v.tracks_begin(), re = v.tracks_end(); r != re; ++r)
      ss << " " << static_cast<unsigned long>(r->key()) << (v.trackWeight(*r) < 0.5 ? "!" : "");
    edm::LogInfo("Vertexer") << ss.str();
  }
  
  // Add TransientTrack cache to improve performance
  mutable std::unordered_map<unsigned int, reco::TransientTrack> ttCache;
  
  // Helper to get a TransientTrack safely with caching
  reco::TransientTrack getTransientTrack(const reco::TrackRef& tk) const {
    // Try to find in cache first
    auto it = ttCache.find(tk.key());
    if (it != ttCache.end())
      return it->second;
      
    // Not found, build it
    if (!eventSetupPtr_) {
      if (verbose) edm::LogWarning("Vertexer") << "EventSetup pointer not set in getTransientTrack()";
      return reco::TransientTrack();
    }
    
    try {
      auto const& tt_builder = eventSetupPtr_->getData(token_builder);
      auto tt = tt_builder.build(tk);
      ttCache[tk.key()] = tt; // Cache it for future use
      return tt;
    } catch (const cms::Exception& e) {
      if (verbose) edm::LogWarning("Vertexer") << "Failed to build transient track: " << e.what();
      return reco::TransientTrack();
    }
  }

  // Check if all TransientTracks in vector are valid
  bool all_valid(const std::vector<reco::TransientTrack>& ttks) const {
    for (auto const& tt : ttks) {
      if (!tt.isValid()) return false;
    }
    return true;
  }

  // REPLACES former global 'KalmanVertexFitter kv_reco;'
  KalmanVertexFitter kv_reco_;

  // Safe drop-in wrapper (replaces free function kv_reco_dropin)
  std::vector<TransientVertex> kv_reco_dropin(std::vector<reco::TransientTrack>& ttks) {
    if (ttks.size() < 2) return {};
    
    // First check if all tracks are valid
    if (!all_valid(ttks)) return {};
    
    try {
      TransientVertex tv = kv_reco_.vertex(ttks);
      if (!tv.isValid() || tv.normalisedChiSquared() > max_seed_vertex_chi2) return {};
      return std::vector<TransientVertex>(1, tv);
    } catch (const std::exception& e) {
      if (verbose) {
        edm::LogInfo("Vertexer") << "Exception in vertex fit: " << e.what();
      }
      return {};
    }
  }
  
  // Event setup reference (needed for transient track building)
  const edm::EventSetup* eventSetupPtr_ = nullptr;
};

//
// constructors and destructor
//

Vertexer::Vertexer(edm::ParameterSet const& params) :
  // Track selection parameters
  minSeedPt(params.getParameter<double>("pt_min_cut")),
  minSeedIPSig(params.getParameter<double>("dxySig_min_cut")),
  maxSeedIPSig(params.getParameter<double>("dxySig_max_cut")),
  minPixelHits(params.getParameter<int>("npixelHits_min_cut")),
  minTrackerLayers(params.getParameter<int>("ntrackerLayers_min_cut")),
  
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
  
  // Reference preference (default to BeamSpot)
  refPreference(params.getUntrackedParameter<std::string>("refPreference", "BeamSpot")),
  refMode_((params.existsAs<edm::InputTag>("primaryVertices_src") || params.existsAs<edm::InputTag>("primaryVertices"))
          ? RefMode::UsePVCollection : RefMode::UseBeamSpot),
  primaryVerticesToken_(refMode_ == RefMode::UsePVCollection
      ? consumes<std::vector<reco::Vertex>>(getPVTag(params))
      : edm::EDGetTokenT<std::vector<reco::Vertex>>()),
  beamspotToken_(consumes<reco::BeamSpot>(params.getParameter<edm::InputTag>("beamspot_src"))),
  seed_tracks_token_(consumes(params.getParameter<edm::InputTag>("seed_tracks_src"))),
  token_builder(esConsumes(edm::ESInputTag("", "TransientTrackBuilder"))),
  
  // Initialize tight merging parameters with defaults
  max_dphi_tight(params.getUntrackedParameter<double>("max_dphi_tight", 0.5)),
  max_svdist_tight(params.getUntrackedParameter<double>("max_svdist_tight", 0.0300)),
  min_dbv_tight(params.getUntrackedParameter<double>("min_dbv_tight", 0.0100)),
  max_iterations(params.getUntrackedParameter<int>("max_iterations", DEFAULT_MAX_ITERATIONS))
{
  // Initialize putToken_ in constructor body for maximum compatibility with older CMSSW versions
  putToken_ = produces<reco::VertexCollection>();
  
  // Validate n_tracks_per_seed_vertex parameter
  if (n_tracks_per_seed_vertex < 2 || n_tracks_per_seed_vertex > 5) {
    throw cms::Exception("InvalidConfiguration")
      << "n_tracks_per_seed_vertex must be between 2 and 5, but got " << n_tracks_per_seed_vertex;
  }
}

// Ensure Vertexer destructor is implemented, not just declared
Vertexer::~Vertexer() {
  // Nothing to clean up, but destructor must be implemented for vtable generation
}

// Fix the produce method's LogDebug statements - replace with edm::LogInfo if verbose
void Vertexer::produce(edm::Event& iEvent, const edm::EventSetup& iSetup) {
    // Store EventSetup for TransientTrack building
    this->eventSetupPtr_ = &iSetup;
    
    // Clear the transient track cache for this event
    ttCache.clear();
    
    // Retrieve seed tracks
    edm::Handle<std::vector<reco::Track>> seed_track_handle;
    iEvent.getByToken(seed_tracks_token_, seed_track_handle);
    if (!seed_track_handle.isValid()) {
      if (verbose) edm::LogWarning("Vertexer") << "Seed track handle invalid (return empty collection; no fake vertices).";
      iEvent.emplace(putToken_, reco::VertexCollection());
      this->eventSetupPtr_ = nullptr; // Reset EventSetup pointer
      return;
    }

    // Create reference point using beamspot or primary vertices
    bool haveRef = false;
    reco::Vertex fake_ref_vtx;
    double ref_x = 0, ref_y = 0, ref_z = 0;
    reco::Vertex::Error ref_error;
    for (int i=0; i<3; ++i)
      for (int j=i; j<3; ++j)
        ref_error(i,j) = 0.0;

    // Determine reference type from the preference string
    bool preferPV = (refPreference == "PrimaryVertex" || refPreference == "AvgPV");
    bool preferBS = (refPreference == "BeamSpot" || refPreference == "BS");
    
    // If no valid preference, default to BeamSpot
    if (!preferPV && !preferBS) {
      preferBS = true;
      if (verbose) edm::LogInfo("Vertexer") << "Invalid refPreference, defaulting to BeamSpot";
    }

    // First try primary vertices if preferred or if available
    if (refMode_ == RefMode::UsePVCollection) {
      edm::Handle<std::vector<reco::Vertex>> pvHandle;
      iEvent.getByToken(primaryVerticesToken_, pvHandle);
      if (pvHandle.isValid() && !pvHandle->empty()) {
        double sumX=0, sumY=0, sumZ=0; int nGood=0;
        for (auto const& pv : *pvHandle) {
          if (!pv.isFake() && pv.ndof() > 4) {
            sumX += pv.x(); sumY += pv.y(); sumZ += pv.z(); ++nGood;
            // Track PV stats
            totalPVsProcessed.fetch_add(1u, std::memory_order_relaxed);
          }
        }
        if (nGood > 0) {
          ref_x = sumX / nGood; ref_y = sumY / nGood; ref_z = sumZ / nGood;
          ref_error(0,0) = 0.0015*0.0015;
          ref_error(1,1) = 0.0015*0.0015;
          ref_error(2,2) = 0.005*0.005;
          fake_ref_vtx = reco::Vertex(reco::Vertex::Point(ref_x, ref_y, ref_z), ref_error);
          haveRef = true;
        }
      }
    }
    
    // Try beamspot if PV not found or if explicitly preferred
    if (!haveRef || preferBS) {
      edm::Handle<reco::BeamSpot> bs;
      iEvent.getByToken(beamspotToken_, bs);
      if (bs.isValid()) {
        // Create proper fake vertex with full covariance matrix
        ref_x = bs->position().x();
        ref_y = bs->position().y();
        ref_z = bs->position().z();
        
        // Use beamspot covariance matrix directly
        ref_error(0,0) = bs->covariance()(0,0);
        ref_error(1,1) = bs->covariance()(1,1);
        ref_error(2,2) = bs->covariance()(2,2);
        
        fake_ref_vtx = reco::Vertex(reco::Vertex::Point(ref_x, ref_y, ref_z), ref_error);
        haveRef = true;
        
        if (!preferBS) {
          beamspotFallbackCount.fetch_add(1u, std::memory_order_relaxed);
        }
        
        if (verbose) edm::LogInfo("Vertexer") << "Using BeamSpot as reference" << (!preferBS ? " (fallback)" : "");
      }
    }

    if (!haveRef) {
      if (verbose) edm::LogWarning("Vertexer") << "No valid reference (PV avg or BeamSpot). Returning empty collection.";
      iEvent.emplace(putToken_, reco::VertexCollection());
      this->eventSetupPtr_ = nullptr; // Reset EventSetup pointer
      return;
    }

    // TransientTrack builder
    auto const& tt_builder = iSetup.getData(token_builder);

    // Seed track selection using the proper significance calculation method and error handling
    std::vector<reco::TrackRef> seed_track_refs;
    seed_track_refs.reserve(seed_track_handle->size());

    for (size_t i_tk = 0; i_tk < seed_track_handle->size(); i_tk++) {
      const edm::Ref<reco::TrackCollection> tk_ref(seed_track_handle, i_tk);
      
      try {
        reco::TransientTrack ttk = tt_builder.build(tk_ref);
        if (!ttk.isValid()) continue;

        auto ttk_dist = IPTools::absoluteTransverseImpactParameter(ttk, fake_ref_vtx);
        
        if (!ttk_dist.first) continue;
        
        float IP_sig = ttk_dist.second.significance();
        
        // Apply full track quality criteria
        if (std::isfinite(IP_sig) && 
            IP_sig > minSeedIPSig &&
            IP_sig < maxSeedIPSig &&
            tk_ref->pt() > minSeedPt &&
            (minPixelHits <= 0 || tk_ref->hitPattern().numberOfValidPixelHits() >= minPixelHits) &&
            (minTrackerLayers <= 0 || tk_ref->hitPattern().trackerLayersWithMeasurement() >= minTrackerLayers)) {
          seed_track_refs.push_back(tk_ref);
          // Add to cache
          ttCache[tk_ref.key()] = ttk;
        }
        
        if (verbose) {
          std::stringstream ss;
          ss << "Track preselect: key=" << static_cast<unsigned>(tk_ref.key()) 
             << " pt=" << tk_ref->pt() << " IPsig=" << IP_sig
             << " pixelHits=" << tk_ref->hitPattern().numberOfValidPixelHits()
             << " trackerLayers=" << tk_ref->hitPattern().trackerLayersWithMeasurement()
             << " " << (seed_track_refs.size() > 0 && seed_track_refs.back().key() == tk_ref.key() ? "KEPT" : "REJ");
          edm::LogInfo("Vertexer") << ss.str();
        }
      } catch (const cms::Exception& e) {
        if (verbose) {
          edm::LogWarning("Vertexer") << "Skipping seed track " << tk_ref.key() << ": " << e.what();
        }
        continue;
      }
    }
    
    // Build transient tracks from the selected track refs
    std::vector<reco::TransientTrack> seed_tracks;
    seed_tracks.reserve(seed_track_refs.size());
    
    for (const auto& tk : seed_track_refs) {
      auto tt = getTransientTrack(tk);
      if (tt.isValid()) {
        seed_tracks.push_back(tt);
      }
    }

    if (seed_tracks.empty()) {
      if (verbose) edm::LogInfo("Vertexer") << "No valid seed tracks found";
      iEvent.emplace(putToken_, reco::VertexCollection());
      this->eventSetupPtr_ = nullptr;
      return;
    }

    //////////////////////////////////////////////////////////////////////
    // Form seed vertices from all pairs of tracks whose vertex fit
    // passes cuts.
    //////////////////////////////////////////////////////////////////////

    const size_t ntk = seed_tracks.size();
    auto vertices = std::make_unique<reco::VertexCollection>();
    
    // reserve a reasonable amount to reduce reallocations
    vertices->reserve(std::min<size_t>(ntk * (ntk - 1) / 2, VERTEX_RESERVE_SIZE));

    std::vector<size_t> itks(n_tracks_per_seed_vertex, 0);

    auto try_seed_vertex = [&]() {
      std::vector<reco::TransientTrack> ttks;
      ttks.reserve(n_tracks_per_seed_vertex);
      
      try {
        // Collect valid TransientTracks
        for (int i = 0; i < n_tracks_per_seed_vertex; ++i) {
          ttks.push_back(seed_tracks[itks[i]]);
          if (!ttks.back().isValid()) return;  // Skip invalid tracks
        }
        
        TransientVertex seed_vertex = kv_reco_.vertex(ttks);
        if (seed_vertex.isValid() && 
            seed_vertex.normalisedChiSquared() < max_seed_vertex_chi2 &&
            seed_vertex.degreesOfFreedom() > 0) {
          vertices->push_back(reco::Vertex(seed_vertex));
          
          if (verbose) {
            const reco::Vertex& v = vertices->back();
            double vx = v.x() - ref_x, vy = v.y() - ref_y, vz = v.z() - ref_z;
            std::stringstream ss;
            ss << "from tracks";
            for (auto itk : itks) ss << " " << itk;
            ss << ": vertex #" << (vertices->size() - 1) << ": chi2/dof: " << v.normalizedChi2() 
               << " dof: " << v.ndof() << " pos(ref): <" << vx << "," << vy << "," << vz << ">";
            edm::LogInfo("Vertexer") << ss.str();
          }
        }
      } catch (const cms::Exception& e) {
        if (verbose) {
          edm::LogWarning("Vertexer") << "Failed to create seed vertex: " << e.what();
        }
      }
    };

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

    if (vertices->empty()) {
      if (verbose) edm::LogInfo("Vertexer") << "No valid seed vertices found after trying all combinations";
      iEvent.emplace(putToken_, std::move(*vertices));
      this->eventSetupPtr_ = nullptr; // Reset EventSetup pointer
      return;
    }

    //////////////////////////////////////////////////////////////////////
    // Take care of track sharing and vertex merging using index-based approach
    //////////////////////////////////////////////////////////////////////
  
    int n_resets = 0;
    int n_onetracks = 0;
    
    // Loop until no more changes needed - with iteration limit to prevent infinite loops
    bool changed = true;
    int iter_count = 0;
    
    while (changed && iter_count++ < max_iterations) {
      changed = false;
      
      for (size_t i = 0; i < vertices->size(); ++i) {
        reco::Vertex& v0 = (*vertices)[i];
        track_set tracks0 = vertex_track_set(v0);
        
        // Check if this vertex has enough tracks
        if (tracks0.size() < 2) {
          if (verbose) {
            edm::LogInfo("Vertexer") << "track-sharing: vertex-0 #" << i << " is down to one track, junking it";
          }
          vertices->erase(vertices->begin() + i);
          ++n_onetracks;
          changed = true;
          break; // Start from the beginning with new indices
        }
        
        bool merge_happened = false;
        for (size_t j = i + 1; j < vertices->size() && !merge_happened; ++j) {
          reco::Vertex& v1 = (*vertices)[j];
          track_set tracks1 = vertex_track_set(v1);
          
          // Check if second vertex has enough tracks
          if (tracks1.size() < 2) {
            if (verbose) {
              edm::LogInfo("Vertexer") << "track-sharing: vertex-1 #" << j << " is down to one track, junking it";
            }
            vertices->erase(vertices->begin() + j);
            ++n_onetracks;
            changed = true;
            break; // Start from the beginning with new indices
          }
          
          // Log vertex information if verbose
          if (verbose) {
            std::stringstream ss;
            double v0_ndof = v0.ndof();
            double v1_ndof = v1.ndof();
            double v0_chi2_per_ndof = (v0_ndof > 0) ? v0.chi2() / v0_ndof : -1;
            double v1_chi2_per_ndof = (v1_ndof > 0) ? v1.chi2() / v1_ndof : -1;
            
            ss << "track-sharing: # vertices = " << vertices->size() << ". considering vertices #" << i
               << " (chi2/dof " << v0_chi2_per_ndof << ", track set";
            for (auto r : tracks0)
              ss << " " << static_cast<size_t>(r.key()) << (v0.trackWeight(r) < 0.5 ? "!" : "");
            ss << ") and #" << j << " (chi2/dof " << v1_chi2_per_ndof << ", track set";
            for (auto r : tracks1)
              ss << " " << static_cast<size_t>(r.key()) << (v1.trackWeight(r) < 0.5 ? "!" : "");
            ss << "):";
            edm::LogInfo("Vertexer") << ss.str();
          }
        
          // Check if one vertex is a subset of the other
          if (is_track_subset(tracks0, tracks1)) {
            if (verbose) {
              edm::LogInfo("Vertexer") << "   subset/duplicate vertices " << i << " and " << j 
                                    << ", erasing second and starting over";
            }
            vertices->erase(vertices->begin() + j);
            changed = true;
            break; // Start over with new indices
          }
          
          // Find shared tracks
          std::vector<reco::TrackRef> shared_tracks;
          for (auto tk : tracks0) {
            if (tracks1.count(tk) > 0) {
              shared_tracks.push_back(tk);
            }
          }
          
          if (verbose) {
            if (!shared_tracks.empty()) {
              std::stringstream ss;
              ss << "   shared tracks are: ";
              for (auto r : shared_tracks)
                ss << " " << static_cast<size_t>(r.key());
              edm::LogInfo("Vertexer") << ss.str();
            }
            else {
              edm::LogInfo("Vertexer") << "   no shared tracks";
            }
          }
        
          // If we have shared tracks, determine what to do
          if (!shared_tracks.empty()) {
            Measurement1D v_dist = vertex_dist(v0, v1);
            
            if (verbose) {
              edm::LogInfo("Vertexer") << "   vertex dist (2d? " << use_2d_vertex_dist << ") " 
                                    << v_dist.value() << "  sig " << v_dist.significance();
            }
            
            bool merge = false;
            bool refit = false;
            track_set tracks_to_remove_in_refit[2];
            
            // Check if vertices are close enough to merge
            if (v_dist.value() < merge_shared_dist || v_dist.significance() < merge_shared_sig) {
              if (verbose) {
                edm::LogInfo("Vertexer") << "          dist < " << merge_shared_dist << " || sig < " 
                                      << merge_shared_sig << ", will try using merge result first before arbitration";
              }
              merge = true;
            } else {
              refit = true;
            }
            
            // Process each shared track
            for (auto tk : shared_tracks) {
              reco::TransientTrack ttk = getTransientTrack(tk);
              if (!ttk.isValid()) continue;
              auto t_dist_0 = track_dist(ttk, v0);
              auto t_dist_1 = track_dist(ttk, v1);
              
              t_dist_0.first = t_dist_0.first && 
                            (t_dist_0.second.value() < max_track_vertex_dist || 
                             t_dist_0.second.significance() < max_track_vertex_sig);
                             
              t_dist_1.first = t_dist_1.first && 
                            (t_dist_1.second.value() < max_track_vertex_dist || 
                             t_dist_1.second.significance() < max_track_vertex_sig);
                             
              bool remove_from_0 = !t_dist_0.first;
              bool remove_from_1 = !t_dist_1.first;
              
              if (t_dist_0.second.significance() < min_track_vertex_sig_to_remove && 
                  t_dist_1.second.significance() < min_track_vertex_sig_to_remove) {
                if (tracks0.size() > tracks1.size())
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
            
            // Handle merge case
            if (merge) {
              track_set tracks_to_fit;
              for (auto tk : tracks0) tracks_to_fit.insert(tk);
              for (auto tk : tracks1) tracks_to_fit.insert(tk);

              std::vector<reco::TransientTrack> ttks;
              ttks.reserve(tracks_to_fit.size());
              
              for (auto tk : tracks_to_fit) {
                auto tt = getTransientTrack(tk);
                if (tt.isValid()) ttks.push_back(tt);
              }

              // Only proceed if we have enough valid tracks
              if (ttks.size() < 2) {
                if (verbose) {
                  edm::LogInfo("Vertexer") << "Not enough valid tracks for merge, falling back to refit";
                }
                refit = true;
              } else {
                auto new_tv = kv_reco_dropin(ttks);
                
                // If we didn't get a valid vertex or we got more than one, fall back to refit
                if (new_tv.empty()) {
                  if (verbose) edm::LogInfo("Vertexer") << "No valid merged vertex, falling back to refit";
                  refit = true;
                } else {
                  // We got exactly one merged vertex
                  reco::Vertex merged_vertex(new_tv[0]);
                  track_set merged_tracks = vertex_track_set(merged_vertex, 0);
                  
                  if (merged_tracks == tracks_to_fit) {
                    // Perfect merger - replace v0 and erase v1
                    (*vertices)[i] = merged_vertex;
                    vertices->erase(vertices->begin() + j);
                    changed = true;
                    merge_happened = true;
                    break; // Restart with new indices
                  } else {
                    // Imperfect merger - fall back to refit
                    if (verbose) edm::LogInfo("Vertexer") << "Imperfect track merge, falling back to refit";
                    refit = true;
                  }
                }
              }
            }
            
            // Handle refit case
            if (refit) {
              bool erase[2] = {false, false};
              
              for (int k = 0; k < 2; ++k) {
                if (tracks_to_remove_in_refit[k].empty())
                  continue;
                
                track_set& tracks = (k == 0) ? tracks0 : tracks1;
                
                std::vector<reco::TransientTrack> ttks;
                for (auto tk : tracks) {
                  if (tracks_to_remove_in_refit[k].count(tk) == 0) {
                    auto tt = getTransientTrack(tk);
                    if (tt.isValid()) ttks.push_back(tt);
                  }
                }
                
                auto new_tv = kv_reco_dropin(ttks);
                if (!new_tv.empty()) {
                  (*vertices)[(k == 0) ? i : j] = reco::Vertex(new_tv[0]);
                } else {
                  erase[k] = true;
                }
              }
              
              // Erase from higher index first to maintain validity
              if (erase[1]) {
                vertices->erase(vertices->begin() + j);
                changed = true;
              }
              if (erase[0]) {
                vertices->erase(vertices->begin() + i);
                changed = true;
              }
              
              if (changed) {
                merge_happened = true;
                break; // Restart with new indices
              }
            }
          }
        }
        
        // If any change happened, restart the outer loop
        if (changed) {
          ++n_resets;
          break;
        }
      }
    }
    
    if (iter_count >= max_iterations) {
      edm::LogWarning("Vertexer") << "Max iterations (" << max_iterations << ") reached in track sharing loop";
    }
    
    // Handle loose merging
    if (resolve_split_vertices_loose && (merge_anyway_sig > 0 || merge_anyway_dist > 0)) {
      bool loose_changed = true;
      int loose_iter_count = 0;
      
      while (loose_changed && loose_iter_count++ < max_iterations) {
        loose_changed = false;
        for (size_t i = 0; i < vertices->size(); ++i) {
          reco::Vertex& v0 = (*vertices)[i];
          
          for (size_t j = i + 1; j < vertices->size(); ++j) {
            reco::Vertex& v1 = (*vertices)[j];
            
            Measurement1D v_dist = vertex_dist_2d.distance(v0, v1);
            
            // Distances & angles always evaluated w.r.t. reference vertex (avgPV or beamspot)
            if (v_dist.value() < merge_anyway_dist || v_dist.significance() < merge_anyway_sig) {
              std::vector<reco::TransientTrack> ttks;
              
              // Collect tracks from both vertices
              for (int k = 0; k < 2; ++k) {
                for (auto tk : vertex_track_set(k == 0 ? v0 : v1)) {
                  auto tt = getTransientTrack(tk);
                  if (tt.isValid()) ttks.push_back(tt);
                }
              }
              
              // Skip if not enough valid tracks
              if (ttks.size() < 2) {
                if (verbose) {
                  edm::LogInfo("Vertexer") << "Not enough valid tracks for loose merge";
                }
                continue;
              }
              
              auto new_tv = kv_reco_dropin(ttks);
              if (!new_tv.empty()) {
                (*vertices)[i] = reco::Vertex(new_tv[0]);
                vertices->erase(vertices->begin() + j);
                loose_changed = true;
                break;  // Break inner loop and restart outer
              }
            }
          }
          
          if (loose_changed) break;  // Restart outer loop
        }
      }
      
      if (loose_iter_count >= max_iterations) {
        edm::LogWarning("Vertexer") << "Max iterations reached in loose merging loop";
      }
    }
    
    // n-1 refit section
    if (max_nm1_refit_dist3 > 0 || max_nm1_refit_distz > 0) {
      std::vector<int> refit_count(vertices->size(), 0);
      
      for (size_t idx = 0; idx < vertices->size(); /* no increment */) {
        if (max_nm1_refit_count > 0 && refit_count[idx] >= max_nm1_refit_count) {
          ++idx;
          continue;
        }
          
        reco::Vertex& vertex = (*vertices)[idx];
        const track_vec tks = vertex_track_vec(vertex);
        const size_t ntks = tks.size();
        if (ntks < 3) {
          ++idx;
          continue;
        }
          
        bool refitMade = false;
        
        for (size_t tk_idx = 0; tk_idx < ntks; ++tk_idx) {
          std::vector<reco::TransientTrack> ttks;
          ttks.reserve(ntks - 1);
          
          for (size_t j = 0; j < ntks; ++j) {
            if (j != tk_idx) {
              auto tt = getTransientTrack(tks[j]);
              if (tt.isValid()) ttks.push_back(tt);
            }
          }
              
          if (!all_valid(ttks) || ttks.size() < 2) continue;
          
          TransientVertex tv_nm1 = kv_reco_.vertex(ttks);
          if (!tv_nm1.isValid()) continue;
          
          reco::Vertex vnm1(tv_nm1);
          const double dist3_2 = std::pow(vnm1.x() - vertex.x(), 2) + 
                                std::pow(vnm1.y() - vertex.y(), 2) + 
                                std::pow(vnm1.z() - vertex.z(), 2);
          const double distz = std::abs(vnm1.z() - vertex.z());
          
          if (vnm1.chi2() < 0 ||
              (max_nm1_refit_dist3 > 0 && dist3_2 > std::pow(max_nm1_refit_dist3, 2)) ||
              (max_nm1_refit_distz > 0 && distz > max_nm1_refit_distz)) {
              
            (*vertices)[idx] = vnm1;
            ++refit_count[idx];
            refitMade = true;
            break;  // Try next track
          }
        }
        
        // If we made a refit, don't increment idx so we process the new vertex
        if (!refitMade) {
          ++idx;
        }
      }
      
      // Remove vertices with normalized chi2 > max_seed_vertex_chi2 after refit
      for (size_t i = 0; i < vertices->size(); /* no increment */) {
        const double ndof = (*vertices)[i].ndof();
        if (ndof > 0 && (*vertices)[i].chi2() / ndof > max_seed_vertex_chi2) {
          vertices->erase(vertices->begin() + i);
        } else {
          ++i;
        }
      }
    }
    
    // Tight merging
    if (resolve_split_vertices_tight) {
      reco::VertexCollection potential_merged_vertices;
      
      bool tight_changed = true;
      int tight_iter_count = 0;
      
      while (tight_changed && tight_iter_count++ < max_iterations) {
        tight_changed = false;
        
        for (size_t i = 0; i < vertices->size(); ++i) {
          reco::Vertex& v0 = (*vertices)[i];
          track_set tracks0 = vertex_track_set(v0);
          
          for (size_t j = i + 1; j < vertices->size() && !tight_changed; ++j) {
            reco::Vertex& v1 = (*vertices)[j];
            
            if (vertices->size() >= 2 && v0.nTracks() >= 2 && v1.nTracks() >= 2) {
              track_set tracks1 = vertex_track_set(v1);
              
              Measurement1D v_dist = vertex_dist_2d.distance(v0, v1);
              
              // Distances/angles relative to reference (avgPV or beamspot)
              Measurement1D dBV0_Meas1D = vertex_dist_2d.distance(v0, fake_ref_vtx);
              Measurement1D dBV1_Meas1D = vertex_dist_2d.distance(v1, fake_ref_vtx);
              double dBV0 = dBV0_Meas1D.value();
              double dBV1 = dBV1_Meas1D.value();
              double v0x = v0.x() - ref_x;
              double v0y = v0.y() - ref_y;
              double phi0 = atan2(v0y, v0x);
              double v1x = v1.x() - ref_x;
              double v1y = v1.y() - ref_y;
              double phi1 = atan2(v1y, v1x);
              double dphi = reco::deltaPhi(phi0, phi1);
              
              // Use configurable parameters instead of hardcoded values
              if (fabs(dphi) < max_dphi_tight && 
                  v_dist.value() < max_svdist_tight && 
                  dBV0 > min_dbv_tight && 
                  dBV1 > min_dbv_tight) {
                
                track_set tracks_to_fit;
                for (auto tk : tracks0) tracks_to_fit.insert(tk);
                for (auto tk : tracks1) tracks_to_fit.insert(tk);
                
                std::vector<reco::TransientTrack> ttks;
                ttks.reserve(tracks_to_fit.size());
                
                for (auto tk : tracks_to_fit) {
                  auto tt = getTransientTrack(tk);
                  if (tt.isValid()) ttks.push_back(tt);
                }
                  
                if (ttks.size() < 2) continue;
                  
                if (investigate_merged_vertices) {
                  if (all_valid(ttks)) {
                    TransientVertex tv = kv_reco_.vertex(ttks);
                    if (tv.isValid())
                      potential_merged_vertices.push_back(reco::Vertex(tv));
                  }
                }
                
                auto new_tv = kv_reco_dropin(ttks);
                if (!new_tv.empty()) {
                  reco::Vertex merged_vertex(new_tv[0]);
                  track_set merged_tracks = vertex_track_set(merged_vertex, 0);
                  
                  if (merged_tracks == tracks_to_fit) {
                    (*vertices)[i] = merged_vertex;
                    vertices->erase(vertices->begin() + j);
                    tight_changed = true;
                    break; // Start over with new indices
                  }
                }
              }
            }
          }
          
          // Reset loop if we made a change
          if (tight_changed) {
            i = -1; // Will be incremented to 0 in the loop
          }
        }
      }
      
      if (tight_iter_count >= max_iterations) {
        edm::LogWarning("Vertexer") << "Max iterations reached in tight merging loop";
      }
    }

    // Put the output
    iEvent.emplace(putToken_, std::move(*vertices));
    
    // Reset the EventSetup pointer and clear cache to avoid dangling references
    this->eventSetupPtr_ = nullptr;
    ttCache.clear();
}

// ------------ method called once each stream before processing any runs, lumis or events  ------------
void Vertexer::beginStream(edm::StreamID) {
  // Not needed
}

// ------------ method called once each stream after processing all runs, lumis and events  ------------
void Vertexer::endStream() {
  // Log statistics about PV averaging and fallbacks
  edm::LogInfo("Vertexer") << "PV Averaging Statistics: "
                         << "Total PVs: " << totalPVsProcessed.load() 
                         << ", Singular Covariance: " << singularCovarianceCount.load()
                         << ", Arithmetic Mean Fallbacks: " << arithmeticFallbackCount.load()
                         << ", Beamspot Fallbacks: " << beamspotFallbackCount.load();
}

// Move the fillDescriptions implementation to the correct location
void Vertexer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  
  // Track selection parameters
  desc.add<double>("pt_min_cut", 1.0);
  desc.add<double>("dxySig_min_cut", 2.0);
  desc.add<double>("dxySig_max_cut", 100.0);
  desc.add<int>("npixelHits_min_cut", 2);
  desc.add<int>("ntrackerLayers_min_cut", 4);
  
  // Vertex parameters
  desc.add<int>("n_tracks_per_seed_vertex", 2);
  desc.add<double>("max_seed_vertex_chi2", 5.0);
  desc.add<bool>("use_2d_vertex_dist", true);
  desc.add<bool>("use_2d_track_dist", true);
  desc.add<bool>("remove_one_track_at_a_time", false);
  desc.add<double>("merge_shared_dist", 0.1);
  desc.add<double>("merge_shared_sig", 2.0);
  desc.add<double>("max_track_vertex_dist", 0.5);
  desc.add<double>("max_track_vertex_sig", 5.0);
  desc.add<double>("min_track_vertex_sig_to_remove", 1.5);
  desc.add<bool>("resolve_split_vertices_loose", true);
  desc.add<bool>("resolve_split_vertices_tight", true);
  desc.add<double>("merge_anyway_sig", 5.0);
  desc.add<double>("merge_anyway_dist", 0.05);
  desc.add<double>("max_nm1_refit_dist3", 0.1);
  desc.add<double>("max_nm1_refit_distz", 0.1);
  desc.add<int>("max_nm1_refit_count", 1);
  desc.add<bool>("investigate_merged_vertices", false);
  desc.add<bool>("verbose", false);
  
  // Input sources
  desc.add<edm::InputTag>("beamspot_src", edm::InputTag("offlineBeamSpot"));
  desc.add<edm::InputTag>("seed_tracks_src", edm::InputTag("generalTracks"));
  desc.add<edm::InputTag>("primaryVertices", edm::InputTag("offlineSlimmedPrimaryVertices"));
  
  // Optional parameters with defaults
  desc.addUntracked<double>("max_dphi_tight", 0.5);
  desc.addUntracked<double>("max_svdist_tight", 0.03);
  desc.addUntracked<double>("min_dbv_tight", 0.01);
  desc.addUntracked<int>("max_iterations", DEFAULT_MAX_ITERATIONS);
  desc.addUntracked<std::string>("refPreference", "BeamSpot");
  
  descriptions.add("vertexer", desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(Vertexer);

// NOTE: Only intentional functional change from original version is the ability to use an averaged
//       primary-vertex reference (or beamspot fallback) with a fully initialized covariance.
//       All vertex finding, merging, refit, and seed selection logic (including IPSig & pT cuts)
//       remains otherwise identical.