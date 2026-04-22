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
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <unordered_set>

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
#include "CondFormats/DataRecord/interface/BeamSpotOnlineHLTObjectsRcd.h"
#include "CondFormats/BeamSpotObjects/interface/BeamSpotOnlineObjects.h"

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

// For Ordering Vertices by pT
// Comparator to sort vertices by descending pT.
// Used only for changing traversal order, not for physics changes.
struct order_seed_vtx_pt {
  bool operator()(const reco::Vertex& a, const reco::Vertex& b) const {
    return a.p4().pt() > b.p4().pt();
  }
};


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
  const bool printVertexerLogs_;
  const bool order_seed_vertex;
  const bool use_seed_tracks_raw;

  // REMOVE legacy-style seed knobs that changed physics
  // const double pt_min_cut_;
  // const double dxySig_min_cut_;
  // const double dxySig_max_cut_;
  // const int    npixelHits_min_cut_;
  // const int    nstripHits_min_cut_;
  // const int    ntrackerLayers_min_cut_;
  // RESTORE original seed knobs
  const double minSeedIPSig;
  const double minSeedPt;

  enum class RefMode { UsePVCollection, UseBeamSpot };
  enum class RefPreference { PreferPV, PreferBeamSpot };
  const RefPreference refPreference_;
  const bool useOnlineBeamSpot_;
  const bool logBeamspotSource_;
  const edm::InputTag seedTracksTag_;
  const edm::EDGetTokenT<std::vector<reco::Vertex>> primaryVerticesToken_;  // vector of PVs
  const edm::EDGetTokenT<reco::BeamSpot>            beamspotToken_;         // fallback
  const edm::ESGetToken<BeamSpotOnlineObjects, BeamSpotOnlineHLTObjectsRcd> beamspotOnlineToken_;
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

/*
Glossary (Vertexer):

- Impact Parameter (IP):
  The distance of closest approach (DCA) between a track and a reference point/vertex.
  Two common definitions:
    * Transverse IP (dxy): distance in the XY plane (perpendicular to beam).
    * 3D IP: full 3D distance to the reference.

- IP Significance:
  The impact parameter divided by its estimated uncertainty: IP / sigma(IP).
  We use IPTools to compute Measurement1D and take significance().
  Larger significance means more inconsistent with originating at the reference.

- Signed vs Absolute IP:
  IPTools provides signedTransverseImpactParameter (with a sign from track direction)
  and absolute(Transverse/3D)ImpactParameter (magnitude only). For seed selection we use
  absolute IP significance (magnitude), via track_dist() which switches between 2D/3D
  using the use_2d_track_dist configuration.

- fake_ref_vtx (reference vertex):
  A proxy reco::Vertex used for distances/IP:
    * avgPV: average position of good primary vertices with fixed diagonal covariance
      (0.0015^2, 0.0015^2, 0.005^2), off-diagonals = 0.
    * BeamSpot: beam spot position with diagonal covariance terms from the BeamSpot;
      off-diagonals = 0. Selected via refPreference (PV or BeamSpot) with fallback.

- TransientTrack:
  A wrapper (TrackingTools) around a reco::Track that provides trajectory state in the
  magnetic field and extrapolation helpers. Required by IPTools to compute IP and its
  uncertainty consistently.

- VertexDistanceXY / VertexDistance3D:
  Utilities to compute the distance and uncertainty between two vertices in XY (2D) or 3D.
  We use these to compute dBV (vertex-to-reference distance) and to decide split-vertex merges.

- Normalized chi^2 (chi2/ndof):
  From KalmanVertexFitter. The vertex fit chi^2 divided by the number of degrees of freedom.
  Lower is better; we keep seed vertices with normalizedChi2 < max_seed_vertex_chi2 (default 5).

- dBV:
  The distance between a displaced vertex and the chosen reference (avgPV or BeamSpot),
  computed in XY (2D) or 3D consistently with configuration. Used in merging/plots.

- Seed tracks vs Vertex tracks:
  * Seed tracks: global preselection (pt > minSeedPt and |IP|/err(ref) > minSeedIPSig),
    computed via track_dist and fake_ref_vtx. No hit/layer cuts and no IP upper bound.
  * Vertex tracks: the tracks attached to final fitted vertices after sharing resolution
    and merging; internally restricted by vertexing/arbitration logic.

- Track weight in vertex:
  reco::Vertex carries weights per track; analysis typically uses weight >= 0.5 as “in vertex”.

- KalmanVertexFitter / TransientVertex:
  The fitter returns TransientVertex objects; we accept vertices with normalisedChiSquared < 5
  and then convert to reco::Vertex for output.
*/

// ...existing code...

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
  printVertexerLogs_(params.getUntrackedParameter<bool>("printVertexerLogs", false)),
  order_seed_vertex(params.getUntrackedParameter<bool>("order_seed_vertex", false)),
  use_seed_tracks_raw(params.getUntrackedParameter<bool>("use_seed_tracks_raw", false)),
  
  // read new params (provide same defaults as current hard-coded values)
  // REMOVE these (do not read minSeed*)
  // minSeedIPSig(params.getUntrackedParameter<double>("minSeedIPSig", 4.0)),
  // minSeedPt(params.getUntrackedParameter<double>("minSeedPt", 0.9)),
  // Keep only legacy-style cuts; defaults keep current behavior
  // pt_min_cut_(             params.existsAs<double>("pt_min_cut")             ? params.getParameter<double>("pt_min_cut")             : -1.0),
  // dxySig_min_cut_(         params.existsAs<double>("dxySig_min_cut")         ? params.getParameter<double>("dxySig_min_cut")         : -1.0),
  // dxySig_max_cut_(         params.existsAs<double>("dxySig_max_cut")         ? params.getParameter<double>("dxySig_max_cut")         :  1e9),
  // npixelHits_min_cut_(     params.existsAs<int>("npixelHits_min_cut")        ? params.getParameter<int>("npixelHits_min_cut")        :  0),
  // nstripHits_min_cut_(     params.existsAs<int>("nstripHits_min_cut")        ? params.getParameter<int>("nstripHits_min_cut")        :  0),
  // ntrackerLayers_min_cut_( params.existsAs<int>("ntrackerLayers_min_cut")    ? params.getParameter<int>("ntrackerLayers_min_cut")    :  0),
  // RESTORE original seed thresholds (defaults match your working config)
  minSeedIPSig(params.getUntrackedParameter<double>("minSeedIPSig", 4.0)),
  minSeedPt   (params.getUntrackedParameter<double>("minSeedPt",    0.9)),
  refPreference_(params.getUntrackedParameter<std::string>("refPreference", "BeamSpot") == "PV" ? 
                  RefPreference::PreferPV : RefPreference::PreferBeamSpot),
  useOnlineBeamSpot_(params.getUntrackedParameter<bool>("useOnlineBeamSpot", false)),
  logBeamspotSource_(params.getUntrackedParameter<bool>("logBeamspotSource", false)),
  seedTracksTag_(params.getParameter<edm::InputTag>("seed_tracks_src")),
  primaryVerticesToken_((params.existsAs<edm::InputTag>("primaryVertices_src") || 
                        params.existsAs<edm::InputTag>("primaryVertices")) ?
                        consumes<std::vector<reco::Vertex>>( getPVTag(params) ) :
                        consumes<std::vector<reco::Vertex>>( edm::InputTag() )),
  beamspotToken_(params.existsAs<edm::InputTag>("beamspot_src") ?
                consumes<reco::BeamSpot>( params.getParameter<edm::InputTag>("beamspot_src") ) :
                consumes<reco::BeamSpot>( edm::InputTag("offlineBeamSpot") )),
  beamspotOnlineToken_(esConsumes<BeamSpotOnlineObjects, BeamSpotOnlineHLTObjectsRcd>()),
  seed_tracks_token_(consumes(seedTracksTag_)),
  token_builder(esConsumes(edm::ESInputTag("", "TransientTrackBuilder"))),

  putToken_{produces()} {}


Vertexer::~Vertexer() {}

//
// member functions
//

// ------------ method called to produce the data  ------------
void Vertexer::produce(edm::Event& iEvent, const edm::EventSetup& iSetup) {

  // ------------------------------------------------------------------
  // LOGGING SETUP: file name, event metadata, and run mode (scouting/offline)
  // Big picture:
  //   - This module builds seed vertices from displaced-track candidates.
  //   - Then it resolves shared-track ambiguities and optionally merges nearby
  //     vertices according to configured geometric/significance criteria.
  //   - Logging here is intentionally verbose to let us reconstruct each
  //     decision without changing the vertexing physics.
  // ------------------------------------------------------------------
  const std::string moduleLabel = moduleDescription().moduleLabel();

  // See if input is Offline or Scouting
  const bool isOfflineVertexer =
      (moduleLabel.find("Offline") != std::string::npos) ||
      (seedTracksTag_.label().find("packedCandidateToTrack") != std::string::npos);

  std::ofstream logFile;
  auto logLine = [&](const std::string& msg) {
    if (printVertexerLogs_ && logFile.is_open()) {
      logFile << msg << "\n";
    }
  };

  // Event metadata
  if (printVertexerLogs_) {
    std::ostringstream fileName;
    fileName << "vertexer_log_" << moduleLabel
             << "_run" << iEvent.id().run()
             << "_lumi" << iEvent.id().luminosityBlock()
             << "_event" << iEvent.id().event()
             << ".txt";
    logFile.open(fileName.str(), std::ios::out);
    if (logFile.is_open()) {
      logFile << std::fixed << std::setprecision(6);
      logFile << "============================================================\n";
      logFile << "Vertexer log for module: " << moduleLabel << "\n";
      logFile << "Event: run=" << iEvent.id().run()
              << " lumi=" << iEvent.id().luminosityBlock()
              << " event=" << iEvent.id().event() << "\n";
      logFile << "Mode: " << (isOfflineVertexer ? "Offline" : "Scouting") << "\n";
      logFile << "Reference preference config: "
              << (refPreference_ == RefPreference::PreferPV ? "PV" : "BeamSpot") << "\n";
      logFile << "Defaults note: refPreference defaults to BeamSpot unless configured otherwise.\n";
      logFile << "\n";
      logFile << "Notation legend:\n";
      logFile << "  - TrackN means pT-ordered label within this event (N=0 is highest pT).\n";
      logFile << "  - dxy and dxySig use transverse impact parameter of reco::Track relative to a point.\n";
      logFile << "  - dist/sig in TRACK_ARBITRATION use track_dist():\n";
      logFile << "      * if use_2d_track_dist=True -> absoluteTransverseImpactParameter (2D IP)\n";
      logFile << "      * if use_2d_track_dist=False -> absoluteImpactParameter3D (3D IP)\n";
      logFile << "  - dBV is vertex distance from chosen reference (PV-average or beamspot).\n";
      logFile << "  - dBVErr is inferred as dBV/significance when significance is non-zero.\n";
      logFile << "  - DROP_* entries are explicit removals and include the reason + content.\n";
      logFile << "  - MERGE_STEP entries document merge attempts/success/fallback points.\n";
      logFile << "  - TRACK_ARBITRATION entries show per-shared-track removal decisions.\n";
      logFile << "============================================================\n";
      logFile << "------------------------------------------------------------\n";
    }
  }

  // Retrieve seed tracks
  edm::Handle<std::vector<reco::Track>> seed_track_handle;
  iEvent.getByToken(seed_tracks_token_, seed_track_handle);
  if (!seed_track_handle.isValid()) {
    logLine("Seed track handle invalid. Returning empty vertex collection.");
    if (verbose) edm::LogWarning("Vertexer") << "Seed track handle invalid (return empty collection; no fake vertices).";
    iEvent.emplace(putToken_, reco::VertexCollection());
    return;
  }

  // ------------------------------------------------------------------
  // LOGGING PREP: prepare PV/BS references and a stable track label map (pT-ordered)
  // Why label by pT?
  //   We only change presentation: labels provide human-readable, deterministic
  //   names while preserving original track keys for algorithmic bookkeeping.
  // ------------------------------------------------------------------
  edm::Handle<std::vector<reco::Vertex>> pvForTrackLogHandle;
  if (!primaryVerticesToken_.isUninitialized()) {
    iEvent.getByToken(primaryVerticesToken_, pvForTrackLogHandle);
  }
  const bool havePVForTrackLog = pvForTrackLogHandle.isValid() && !pvForTrackLogHandle->empty();
  reco::Vertex::Point pvRefPoint(0.0, 0.0, 0.0);
  if (havePVForTrackLog) {
    pvRefPoint = pvForTrackLogHandle->front().position();
  }

  edm::Handle<reco::BeamSpot> bsForTrackLogHandle;
  iEvent.getByToken(beamspotToken_, bsForTrackLogHandle);
  const bool haveBSForTrackLog = bsForTrackLogHandle.isValid();
  reco::TrackBase::Point beamspotPoint(0.0, 0.0, 0.0);
  if (haveBSForTrackLog) {
    beamspotPoint = reco::TrackBase::Point(bsForTrackLogHandle->position().x(),
                                           bsForTrackLogHandle->position().y(),
                                           bsForTrackLogHandle->position().z());
  }

  std::vector<size_t> trackIdxByPt(seed_track_handle->size());
  for (size_t i = 0; i < seed_track_handle->size(); ++i) {
    trackIdxByPt[i] = i;
  }
  std::sort(trackIdxByPt.begin(), trackIdxByPt.end(),
            [&](size_t a, size_t b) { return seed_track_handle->at(a).pt() > seed_track_handle->at(b).pt(); });

  std::unordered_map<unsigned int, int> trackLabelByKey;
  for (size_t i = 0; i < trackIdxByPt.size(); ++i) {
    trackLabelByKey[static_cast<unsigned int>(trackIdxByPt[i])] = static_cast<int>(i);
  }

  // Determine reference vertex (average PV or beamspot)
  double ref_x=0, ref_y=0, ref_z=0;
  reco::Vertex::Error ref_error;              // will set all components explicitly
  for (int i=0;i<3;++i)                       // ensure no uninitialized covariance (prevents NaNs / segfaults)
    for (int j=i;j<3;++j)
      ref_error(i,j)=0.0;
  bool haveRef = false;

  // Try to get Primary Vertices
  bool havePV = false;
  edm::Handle<std::vector<reco::Vertex>> pvHandle;
  if (refPreference_ == RefPreference::PreferPV || beamspotToken_.isUninitialized()) {
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
        havePV = true;
        if (verbose) edm::LogInfo("Vertexer") << "Using average primary vertex as reference.";
      }
    }
  }

  // Try BeamSpot if needed
  if (!haveRef && (refPreference_ == RefPreference::PreferBeamSpot || !havePV)) {
    bool haveBeamspotReference = false;

    if (useOnlineBeamSpot_) {
      auto bsOnlineH = iSetup.getHandle(beamspotOnlineToken_);
      if (bsOnlineH.isValid()) {
        ref_x = bsOnlineH->x();
        ref_y = bsOnlineH->y();
        ref_z = bsOnlineH->z();
        for (int i=0;i<3;++i)
          for (int j=i;j<3;++j)
            ref_error(i,j)=bsOnlineH->covariance(i,j);
        haveBeamspotReference = true;
        if ( logBeamspotSource_) {
          edm::LogInfo("Vertexer")
            << "Using online beam spot as reference"
            << " x=" << ref_x << " y=" << ref_y << " z=" << ref_z;
        }
      } else {
        edm::LogError("Vertexer")
          << "useOnlineBeamSpot=True but BeamSpotOnlineHLTObjectsRcd is unavailable. "
          << "Falling back to offline beamspot if present.";
      }
    }

    if (!haveBeamspotReference) {
      edm::Handle<reco::BeamSpot> bs;
      iEvent.getByToken(beamspotToken_, bs);
      if (bs.isValid()) {
        ref_x = bs->position().x(); ref_y = bs->position().y(); ref_z = bs->position().z();
        ref_error = bs->covariance3D();
        haveBeamspotReference = true;
        if ( logBeamspotSource_) {
          edm::LogInfo("Vertexer")
            << "Using offline beam spot as reference"
            << " x=" << ref_x << " y=" << ref_y << " z=" << ref_z;
        }
      }
    }

    if (haveBeamspotReference) {
      haveRef = true;
    } else if (useOnlineBeamSpot_) {
      edm::LogError("Vertexer")
        << "No usable beamspot found (online missing and offline beamspot product invalid). "
        << "Will continue to other fallbacks if available.";
    }
  }

  // Try PV as fallback if we preferred BeamSpot but couldn't get it
  if (!haveRef && refPreference_ == RefPreference::PreferBeamSpot && !primaryVerticesToken_.isUninitialized()) {
    if (!pvHandle.isValid()) {
      iEvent.getByToken(primaryVerticesToken_, pvHandle);
    }
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
        haveRef = true;
        if (verbose) edm::LogInfo("Vertexer") << "Fallback to primary vertex average as reference.";
      }
    }
  }

  if (!haveRef) {
    logLine("No valid reference vertex/beamspot found. Returning empty vertex collection.");
    if (verbose) edm::LogWarning("Vertexer") << "No valid reference (PV avg or BeamSpot). Returning empty (no fake substitute).";
    iEvent.emplace(putToken_, reco::VertexCollection());
    return;
  }

  const reco::Vertex fake_ref_vtx(reco::Vertex::Point(ref_x, ref_y, ref_z), ref_error);

  {
    logLine(" ");
    logLine("-------------------- REFERENCE SELECTION --------------------");
    std::ostringstream header;
    header << "Reference vertex: x=" << ref_x << " y=" << ref_y << " z=" << ref_z;
    logLine(header.str());
    logLine("This reference is used for impact-parameter significance and dBV-like distances.");
    logLine(" ");
    logLine("---------------------- INPUT TRACK LIST ----------------------");
    {
      std::ostringstream srcLine;
      srcLine << "INPUT SOURCE: seed_tracks_src=" << seedTracksTag_.label();
      if (!seedTracksTag_.instance().empty()) srcLine << ":" << seedTracksTag_.instance();
      if (!seedTracksTag_.process().empty()) srcLine << " (process=" << seedTracksTag_.process() << ")";
      logLine(srcLine.str());
    }
    logLine("Tracks ordered by descending pT:");
  }

  // TransientTrack builder
  auto const& tt_builder = iSetup.getData(token_builder);

  // ==================================================
  // 1) Seed Track Selection
  // ==================================================
  // Seed Track Selection
  // Apply the track seed cuts:
  // - pT > 0.9 GeV
  // - |IP significance(reference)| > 4.0
  // Every surviving track is added to the seed track list.
  std::vector<reco::TrackRef> seed_tracks_raw;
  std::vector<reco::TrackRef> seed_tracks_pt_ordered;
  std::unordered_set<unsigned int> seedTrackKeySet;
  std::unordered_map<unsigned int, float> seedTrackIPSigByKey;

  for (size_t i_tk=0; i_tk<seed_track_handle->size(); ++i_tk) {
    edm::Ref<reco::TrackCollection> tk_ref(seed_track_handle, i_tk);
    reco::TransientTrack ttk = tt_builder.build(tk_ref);
    if (!ttk.isValid()) continue;
    // RESTORE original: use track_dist (2D/3D according to use_2d_track_dist)
    auto ttk_dist = track_dist(ttk, fake_ref_vtx);
    if (!ttk_dist.first) continue;

    const float IP_sig = ttk_dist.second.significance();
    if (!std::isfinite(IP_sig)) continue;

    // RESTORE original seed preselection: only IPsig and pT
    if (!(IP_sig > minSeedIPSig)) continue;
    if (!(tk_ref->pt() > minSeedPt)) continue;

    // Keep the seed
    seed_tracks_raw.push_back(tk_ref);
    seedTrackKeySet.insert(tk_ref.key());
    seedTrackIPSigByKey[tk_ref.key()] = IP_sig;

    if (verbose)
      printf("Seed preselect: key=%u pt=%.3f IPsig(ref)=%.3f KEPT\n",
             tk_ref.key(), tk_ref->pt(), IP_sig);
  }

  seed_tracks_pt_ordered.reserve(seed_tracks_raw.size());
  for (size_t orderIdx = 0; orderIdx < trackIdxByPt.size(); ++orderIdx) {
    const size_t tkIdx = trackIdxByPt[orderIdx];
    const unsigned int key = static_cast<unsigned int>(tkIdx);
    if (seedTrackKeySet.count(key) == 0) continue;
    edm::Ref<reco::TrackCollection> tk_ref(seed_track_handle, tkIdx);
    seed_tracks_pt_ordered.push_back(tk_ref);
  }

  {
    logLine(" ");
    logLine("[Seed Track Selection]");
    std::ostringstream selSummary;
    selSummary << "Seed-track summary: totalTracks=" << seed_track_handle->size()
               << " selectedSeedTracks=" << seed_tracks_pt_ordered.size();
    logLine(selSummary.str());
    logLine("Apply seed cuts: pT > 0.9 GeV and |IP significance(reference)| > 4.0.");
    logLine(" ");
    // Debug label consistency:
    // keep original full-event pT labels even after seed cuts.
    // This ensures pT-ordered and raw-order dumps use the same label space.
    logLine("[List of pT Ordered Tracks]");
    for (size_t orderIdx = 0; orderIdx < seed_tracks_pt_ordered.size(); ++orderIdx) {
      const reco::TrackRef& tk_ref = seed_tracks_pt_ordered[orderIdx];
      const int label = trackLabelByKey.count(tk_ref.key()) ? trackLabelByKey[tk_ref.key()] : -1;
      const reco::Track& tk = *tk_ref;
      const double dxyOrigin = tk.dxy(reco::TrackBase::Point(0.0, 0.0, 0.0));
      const double dxyBeamspot = haveBSForTrackLog ? tk.dxy(beamspotPoint) : std::numeric_limits<double>::quiet_NaN();
      const double dxySigOrigin = (tk.dxyError() > 0.0) ? (dxyOrigin / tk.dxyError()) : std::numeric_limits<double>::quiet_NaN();
      const double dxySigBeamspot = (tk.dxyError() > 0.0) ? (dxyBeamspot / tk.dxyError()) : std::numeric_limits<double>::quiet_NaN();
      const double dzOrigin = tk.dz(reco::TrackBase::Point(0.0, 0.0, 0.0));
      const double dzPV = havePVForTrackLog ? tk.dz(pvRefPoint) : std::numeric_limits<double>::quiet_NaN();
      const float ipSig = seedTrackIPSigByKey.count(tk_ref.key()) ? seedTrackIPSigByKey[tk_ref.key()] : std::numeric_limits<float>::quiet_NaN();
      std::ostringstream trkLine;
      trkLine << "Track" << label
              << " pt=" << tk.pt()
              << " eta=" << tk.eta()
              << " phi=" << tk.phi()
              << " dxy(beamspot)=" << dxyBeamspot
              << " dxySig(beamspot)=" << dxySigBeamspot
              << " dxy(0,0)=" << dxyOrigin
              << " dxySig(0,0)=" << dxySigOrigin
              << " dz(PV)=" << dzPV
              << " dz(0,0)=" << dzOrigin
              << " dxyErr=" << tk.dxyError()
              << " dzErr=" << tk.dzError()
              << " IPsig(ref)=" << ipSig;
      logLine(trkLine.str());
    }
    logLine("[Raw Selected Track Order (same labels)]");
    for (size_t slot = 0; slot < seed_tracks_raw.size(); ++slot) {
      const unsigned int key = seed_tracks_raw[slot].key();
      const int label = trackLabelByKey.count(key) ? trackLabelByKey[key] : -1;
      std::ostringstream rawLine;
      rawLine << "slot" << slot << " -> Track" << label;
      logLine(rawLine.str());
    }
  }

  // build safe lambda (bounds + cache) REPLACES previous version
  auto getTransientTrack = [&](auto const& tk) -> reco::TransientTrack {
    const unsigned int k = tk.key();
    if (k < seed_track_handle->size()) {
      edm::Ref<reco::TrackCollection> tr(seed_track_handle, k);
      return tt_builder.build(tr);
    }
    if (verbose) edm::LogWarning("Vertexer") << "getTransientTrack: out-of-range track key " << k;
    return reco::TransientTrack(); // invalid
  };

  //////////////////////////////////////////////////////////////////////
  // 2) Candidate Vertices and Vertex Seed Selection Cuts
  //////////////////////////////////////////////////////////////////////
  // Candidate Vertices and Vertex Seed Selection Cuts
  // Every possible pair of seed tracks is fit into a vertex
  // (Kalman Vertex Fitting) to get the vertex.
  // Apply the following cuts:
  // - vertex chi2 < maxSeedVertexChi2
  // If passed:
  // - store as candidate vertex
  // If failed:
  // - discard only this pair

  // ------------------------------------------------------------
  // Seed track source selection:
  // use_seed_tracks_raw = true  -> original selected-track order
  // use_seed_tracks_raw = false -> pT-ordered selected-track order
  //
  // Important:
  // the first ACCEPTED seed vertex is the first pair that PASSES
  // the chi2 cut in loop order, not necessarily the first pair visited.
  // ------------------------------------------------------------

  const auto& seed_tracks_for_vertexing =
      use_seed_tracks_raw ? seed_tracks_raw : seed_tracks_pt_ordered;

  const size_t ntk = seed_tracks_for_vertexing.size();
  
  std::unique_ptr<reco::VertexCollection> vertices(new reco::VertexCollection);
  
  if (ntk == 0) {
    logLine("No seed tracks passed selection. No seed vertices can be formed for this event.");
    iEvent.emplace(putToken_, std::move(*vertices));
    return;
  }

  
  std::vector<size_t> itks(n_tracks_per_seed_vertex, 0);

  auto try_seed_vertex = [&]() {
    std::vector<reco::TransientTrack> ttks(n_tracks_per_seed_vertex);
    for (int i = 0; i < n_tracks_per_seed_vertex; ++i) {
      ttks[i] = getTransientTrack(seed_tracks_for_vertexing[itks[i]]);
      if (!ttks[i].isValid()) return;  // safety
    }
    TransientVertex seed_vertex = kv_reco_.vertex(ttks);
    if (seed_vertex.isValid()) {
      const reco::Vertex candidate(seed_vertex);
      const auto dBV = vertex_dist(candidate, fake_ref_vtx);
      const double dBVErr = (std::abs(dBV.significance()) > 0.0) ? (dBV.value() / dBV.significance()) : std::numeric_limits<double>::quiet_NaN();

      std::ostringstream seedLine;
      seedLine << "[Candidate Vertices and Vertex Seed Selection Cuts] SEED_VERTEX_CANDIDATE tracks={";
      for (size_t i = 0; i < itks.size(); ++i) {
        const unsigned int key = seed_tracks_for_vertexing[itks[i]].key();
        const int label = trackLabelByKey.count(key) ? trackLabelByKey[key] : -1;
        if (i) seedLine << ",";
        seedLine << "Track" << label;
      }
      seedLine << "}"
               << " vertexPt=" << candidate.p4().pt()
               << " x=" << candidate.x()
               << " y=" << candidate.y()
               << " z=" << candidate.z()
               << " chi2=" << candidate.normalizedChi2()
               << " dBV=" << dBV.value()
               << " dBVErr=" << dBVErr
               << " passChi2Cut=" << (candidate.normalizedChi2() < max_seed_vertex_chi2 ? "YES" : "NO");
      logLine(seedLine.str());
    }

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

  auto formatTrackSetLabels = [&](const track_set& tset) {
    std::vector<int> labels;
    labels.reserve(tset.size());
    for (auto tk : tset) {
      const unsigned int key = tk.key();
      auto it = trackLabelByKey.find(key);
      labels.push_back(it != trackLabelByKey.end() ? it->second : -1);
    }
    std::sort(labels.begin(), labels.end());
    std::ostringstream out;
    out << "{";
    for (size_t i = 0; i < labels.size(); ++i) {
      if (i) out << ",";
      out << "Track" << labels[i];
    }
    out << "}";
    return out.str();
  };

  auto formatTrackLabel = [&](const reco::TrackRef& tk) {
    const unsigned int key = tk.key();
    auto it = trackLabelByKey.find(key);
    std::ostringstream out;
    out << "Track" << (it != trackLabelByKey.end() ? it->second : -1);
    return out.str();
  };

  auto vertexSummary = [&](const reco::Vertex& vv) {
    const auto dBV = vertex_dist(vv, fake_ref_vtx);
    const double dBVErr = (std::abs(dBV.significance()) > 0.0) ? (dBV.value() / dBV.significance()) : std::numeric_limits<double>::quiet_NaN();
    const auto tset = vertex_track_set(vv, 0.0);
    std::ostringstream out;
    out << "x=" << vv.x()
        << " y=" << vv.y()
        << " z=" << vv.z()
        << " vertexPt=" << vv.p4().pt()
        << " chi2=" << vv.normalizedChi2()
        << " dBV=" << dBV.value()
        << " dBVErr=" << dBVErr
        << " tracks=" << formatTrackSetLabels(tset);
    return out.str();
  };

  auto dumpVertices = [&](const std::string& header) {
    // Snapshot helper: emits the full currently-active vertex list so we can
    // audit state transitions after each merge/refit/reset step.
    logLine(header);
    for (size_t iv = 0; iv < vertices->size(); ++iv) {
      std::ostringstream line;
      line << "Vertex" << (iv + 1) << " " << vertexSummary(vertices->at(iv));
      logLine(line.str());
    }
  };

  auto dumpVerticesByPtOrder = [&](const std::string& header) {
    logLine(header);
    std::vector<size_t> vertexOrder(vertices->size());
    for (size_t i = 0; i < vertexOrder.size(); ++i) {
      vertexOrder[i] = i;
    }
    std::sort(vertexOrder.begin(), vertexOrder.end(), [&](size_t a, size_t b) {
      return vertices->at(a).p4().pt() > vertices->at(b).p4().pt();
    });
    for (size_t rank = 0; rank < vertexOrder.size(); ++rank) {
      const size_t vertexIndex = vertexOrder[rank];
      std::ostringstream line;
      line << "rank" << rank << " -> Vertex" << (vertexIndex + 1)
           << " " << vertexSummary(vertices->at(vertexIndex));
      logLine(line.str());
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

  dumpVertices("[Vertex Raw Insertion Order]");
  dumpVerticesByPtOrder("[List of pT Ordered Vertices]");
  logLine("------------------------------------------------------------");

  //////////////////////////////////////////////////////////////////////
  // 3) Vertex Merging and Cleanup Loop: Compare Vertex Pairs
  //////////////////////////////////////////////////////////////////////
  // Vertex Merging and Cleanup Loop:
  // Compare Vertex Pairs
  // - this loop continues until one full scan causes no changes
  // - any merge, drop, or refit causes Restart Loop

  logLine(" ");
  logLine("[Vertex Merging and Cleanup Loop: Compare Vertex Pairs]");
  logLine("This loop continues until one full scan causes no changes.");
  logLine("Any merge, drop, or refit causes [Restart Loop].");
  {
    std::ostringstream modeLine;
    modeLine << "Track-to-vertex compatibility distance mode: "
             << (use_2d_track_dist ? "2D transverse IP" : "3D absolute IP")
             << " (use_2d_track_dist=" << (use_2d_track_dist ? "True" : "False") << ")";
    logLine(modeLine.str());
  }
  {
    std::ostringstream thrLine;
    thrLine << "Configured thresholds used in this stage: "
            << "max_track_vertex_dist=" << max_track_vertex_dist
            << ", max_track_vertex_sig=" << max_track_vertex_sig
            << ", min_track_vertex_sig_to_remove=" << min_track_vertex_sig_to_remove
            << ", remove_one_track_at_a_time=" << (remove_one_track_at_a_time ? "True" : "False")
            << ", merge_shared_dist=" << merge_shared_dist
            << ", merge_shared_sig=" << merge_shared_sig;
    logLine(thrLine.str());
  }
  logLine("[Rule 1]");
  logLine("If Vertex B tracks are entirely contained in Vertex A tracks, Vertex B is removed as redundant.");
  logLine("Drop smaller redundant vertex, then [Restart Loop].");
  logLine("[Do 2 Vertices Have a Shared Track?]");
  logLine("If no: continue scanning remaining vertex pairs.");
  logLine("Only return reconstructed vertices after a full scan completes with no changes.");
  logLine("[Are the 2 Vertices Close?]");
  logLine("Compare vertex distances: distance(vertexA, vertexB) and significance(vertexA, vertexB).");
  logLine("Thresholds: max_vertex_dist (= merge_shared_dist) and max_vertex_sig (= merge_shared_sig).");
  logLine("This stage answers closeness and, in the same answer, marks merge (YES) or refit-only (NO).");
  logLine("This is vertex-to-vertex closeness. This is NOT track-to-vertex distance.");
  logLine("[Union Merge (Kalman fit a vertex for the union of set of tracks)]");
  logLine("Build a temporary merged vertex candidate from the union of all unique tracks.");
  logLine("If merged fit returns exactly one valid full-union vertex: replace source vertices, drop source vertices, [Restart Loop].");
  logLine("If merged fit does not pass valid full-union checks: ignore merged candidate and continue with original two vertices.");
  logLine("[Force Drop a Track (OR 2!)]");
  logLine("For each shared track and each vertex side, report exact threshold checks.");
  logLine("hardFail = track_dist invalid OR NOT( dist < max_track_vertex_dist OR sig < max_track_vertex_sig )");
  logLine("remove_from_0 = hardFail0; remove_from_1 = hardFail1");
  logLine("Further checks:");
  logLine("If both sig < min_track_vertex_sig_to_remove: keep bigger vertex; if equal size, drop from first vertex in hand.");
  logLine("Else: keep lower-significance side and drop from higher-significance side.");
  logLine("So either 1 OR 2 tracks can be marked for removal (at least 1). ");
  logLine("[Refit for the vertices whose tracks were dropped]");
  logLine("This stage performs actual removal by rebuilding with surviving tracks only.");
  logLine("Rebuild only affected vertices; unchanged vertices stay untouched.");
  logLine("If a refit side no longer has enough tracks for a valid vertex, drop that vertex; then [Restart Loop].");
  logLine("------------------------------------------------------------");
  
  // ============================================================
  // For Ordering Vertices by pT
  // Sort seed vertices by descending pT before pairwise cleanup.
  // This only changes traversal order, not the physics logic.
  // Useful for testing whether vertex scan order affects outcomes.
  // ============================================================
  if (order_seed_vertex) {
    std::sort(vertices->begin(), vertices->end(), order_seed_vtx_pt());
    if (printVertexerLogs_) {
      logLine("[Vertex Ordering] Sort initial seed vertices by descending pT before compare loop");
    }
  }
  
  //printf("entering the track sharing part\n");
  
  track_set discarded_tracks;
  int n_resets = 0;
  int n_onetracks = 0;
  std::vector<reco::Vertex>::iterator v[2];

  size_t ivtx[2];
  int sharedPhaseIteration = 0;
  
  for (v[0] = vertices->begin(); v[0] != vertices->end(); ++v[0]) {
    track_set tracks[2];
    ivtx[0] = v[0] - vertices->begin();
    tracks[0] = vertex_track_set(*v[0]);
    
    if (tracks[0].size() < 2) {
      {
        std::ostringstream msg;
        msg << "DROP_VERTEX reason=track-sharing vertex has <2 tracks"
            << " vertexIndex=" << (ivtx[0] + 1)
            << " content={" << vertexSummary(*v[0]) << "}";
        logLine(msg.str());
      }
      if (verbose)
        printf("track-sharing: vertex-0 #%zu is down to one track, junking it\n", ivtx[0]);
      // This is where the vertex is ACTUALLY removed from the system.
      // Reason: dropping a vertex that is already reduced below 2 tracks.
      v[0] = vertices->erase(v[0]) - 1;
      ++n_onetracks;
      continue;
    }

    bool duplicate = false;
    bool merge = false;
    bool refit = false;
    track_set tracks_to_remove_in_refit[2];
    
    for (v[1] = v[0] + 1; v[1] != vertices->end(); ++v[1]) {
      ++sharedPhaseIteration;
      ivtx[1] = v[1] - vertices->begin();
      tracks[1] = vertex_track_set(*v[1]);

      {
        std::ostringstream msg;
        msg << "================ [Vertex Merging and Cleanup Loop: Compare Vertex Pairs] Iteration " << sharedPhaseIteration << " ================";
        logLine(msg.str());
      }
      {
        std::ostringstream msg;
        msg << "Compare Vertex" << (ivtx[0] + 1) << " {" << vertexSummary(*v[0]) << "}"
            << "  vs  Vertex" << (ivtx[1] + 1) << " {" << vertexSummary(*v[1]) << "}";
        logLine(msg.str());
      }

      if (tracks[1].size() < 2) {
        {
          std::ostringstream msg;
          msg << "DROP_VERTEX reason=track-sharing vertex has <2 tracks"
              << " vertexIndex=" << (ivtx[1] + 1)
                << " content={" << vertexSummary(*v[1]) << "}"
                << " rule=R5";
          logLine(msg.str());
        }
        if (verbose)
          printf("track-sharing: vertex-1 #%zu is down to one track, junking it\n", ivtx[1]);	
  // This is where the vertex is ACTUALLY removed from the system.
  // Reason: dropping a vertex that is already reduced below 2 tracks.
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

      
      // 4) Rule 1
      if (is_track_subset(tracks[0], tracks[1])) {
        {
          std::ostringstream msg;
          msg << "[Rule 1] Vertex" << (ivtx[1] + 1) << " tracks are contained in Vertex" << (ivtx[0] + 1)
              << " and Vertex" << (ivtx[1] + 1)
              << " tracksA=" << formatTrackSetLabels(tracks[0])
              << " tracksB=" << formatTrackSetLabels(tracks[1])
              << " rule=R1";
          logLine(msg.str());

          std::ostringstream dropMsg;
          dropMsg << "[Rule 1] drop smaller redundant vertex"
                  << " droppedVertexIndex=" << (ivtx[1] + 1)
              << " content={" << vertexSummary(*v[1]) << "}"
              << " rule=R1";
          logLine(dropMsg.str());
          logLine("[Restart Loop]");
        }
        if (verbose)
          printf("   subset/duplicate vertices %zu and %zu, erasing second and starting over\n", ivtx[0], ivtx[1]);
        duplicate = true;
        // Stop searching more vertex partners for this v[0].
        // This pair already triggered cleanup logic.
        // Restart the full compare loop after this update.
        break;
      }

      std::vector<reco::TrackRef> shared_tracks;
      for (auto tk : tracks[0]){
        if (tracks[1].count(tk) > 0)
          shared_tracks.push_back(tk);
      }

      if (verbose) {
        if (shared_tracks.size()) {
          printf("   shared tracks are: ");
          print_track_set(shared_tracks);
          printf("\n");
        }
        else
          printf("   no shared tracks\n");
      }

      

      // 5) Do 2 Vertices Have a Shared Track?
      const bool hasSharedTracks = !shared_tracks.empty();
      if (!hasSharedTracks) {
        logLine("[Do 2 Vertices Have a Shared Track?] NO -> continue scanning remaining vertex pairs.");
      }

      if (hasSharedTracks){
    	// 6) Are the 2 Vertices Close?
	Measurement1D v_dist = vertex_dist(*v[0], *v[1]);
	{
	  std::ostringstream msg;
	  msg << "[Do 2 Vertices Have a Shared Track?] YES sharedTracks="
	      << formatTrackSetLabels(track_set(shared_tracks.begin(), shared_tracks.end()));
	  logLine(msg.str());
	}
  const bool distancePass = (v_dist.value() < merge_shared_dist);
  const bool significancePass = (v_dist.significance() < merge_shared_sig);
  const bool shouldMerge = (distancePass || significancePass);
  {
    std::ostringstream msg;
    msg << "[Are the 2 Vertices Close?] " << (shouldMerge ? "YES" : "NO");
    logLine(msg.str());
  }
  if (shouldMerge) {
    std::ostringstream reason;
    if (distancePass && significancePass) {
      reason << "Reason: distance threshold passed and significance threshold passed "
             << "(vertexDistance=" << v_dist.value() << " < max_vertex_dist=" << merge_shared_dist
             << ", vertexSignificance=" << v_dist.significance() << " < max_vertex_sig=" << merge_shared_sig << ")";
    } else if (distancePass) {
      reason << "Reason: distance threshold passed "
             << "(vertexDistance=" << v_dist.value() << " < max_vertex_dist=" << merge_shared_dist << ")";
    } else {
      reason << "Reason: significance threshold passed "
             << "(vertexSignificance=" << v_dist.significance() << " < max_vertex_sig=" << merge_shared_sig << ")";
    }
    logLine(reason.str());
    logLine("Action: marked for merge");
  } else {
    std::ostringstream reason;
    reason << "Reason: dist and significance both failed merge thresholds"
           << " (vertexDistance=" << v_dist.value() << " vs max_vertex_dist=" << merge_shared_dist
           << ", vertexSignificance=" << v_dist.significance() << " vs max_vertex_sig=" << merge_shared_sig << ")";
    logLine(reason.str());
    logLine("Action: marked for refit only");
  }
	
        if (verbose)
          printf("   vertex dist (2d? %i) %7.3f  sig %7.3f\n", use_2d_vertex_dist, v_dist.value(), v_dist.significance());
	
  if (shouldMerge) {
    // 7) Union Merge (Kalman fit a vertex for the union of set of tracks)
          {
            std::ostringstream msg;
            msg << "Pair: Vertex" << (ivtx[0] + 1) << " vs Vertex" << (ivtx[1] + 1);
            logLine(msg.str());
          }
          if (verbose) printf("          dist < %7.3f || sig < %7.3f, will try using merge result first before arbitration\n", merge_shared_dist, merge_shared_sig);
	  {
	    track_set union_tracks = tracks[0];
	    union_tracks.insert(tracks[1].begin(), tracks[1].end());
	    std::ostringstream msg;
	    msg << "Temporary union tracks=" << formatTrackSetLabels(union_tracks);
	    logLine(msg.str());
	  }
	  merge = true;
      }
	else {
    logLine("This stage ONLY marks possible removals.");
    logLine("Nothing is removed from the actual vertex yet.");
	  refit = true;
	}
	
        
  // 8) Mark to Drop a Track (From 1 OR 2 vertices!)
  // This stage reports exact threshold checks and ONLY marks tracks.
  // Actual removal happens later in [Refit for the vertices whose tracks were dropped].
  for (auto tk : shared_tracks) {
	  reco::TransientTrack ttk = getTransientTrack(tk);            // was 'const & ttk' (dangling)
	  if (!ttk.isValid()) continue;                                // new guard
    const reco::TrackBase::Point v0Point(v[0]->x(), v[0]->y(), v[0]->z());
    const reco::TrackBase::Point v1Point(v[1]->x(), v[1]->y(), v[1]->z());
    const double dxyToV0 = tk->dxy(v0Point);
    const double dxyToV1 = tk->dxy(v1Point);
    const double dzToV0 = tk->dz(v0Point);
    const double dzToV1 = tk->dz(v1Point);
    const double dxySigToV0 = (tk->dxyError() > 0.0) ? (dxyToV0 / tk->dxyError()) : std::numeric_limits<double>::quiet_NaN();
    const double dxySigToV1 = (tk->dxyError() > 0.0) ? (dxyToV1 / tk->dxyError()) : std::numeric_limits<double>::quiet_NaN();
    const double dzSigToV0 = (tk->dzError() > 0.0) ? (dzToV0 / tk->dzError()) : std::numeric_limits<double>::quiet_NaN();
    const double dzSigToV1 = (tk->dzError() > 0.0) ? (dzToV1 / tk->dzError()) : std::numeric_limits<double>::quiet_NaN();
    auto t_dist_0 = track_dist(ttk, *v[0]);
    auto t_dist_1 = track_dist(ttk, *v[1]);
    const bool trackDistValid0 = t_dist_0.first;
    const bool trackDistValid1 = t_dist_1.first;
    const bool v0DistPass = (t_dist_0.second.value() < max_track_vertex_dist);
    const bool v0SigPass = (t_dist_0.second.significance() < max_track_vertex_sig);
    const bool v1DistPass = (t_dist_1.second.value() < max_track_vertex_dist);
    const bool v1SigPass = (t_dist_1.second.significance() < max_track_vertex_sig);

    const bool v0CompatibilityPass = trackDistValid0 && (v0DistPass || v0SigPass);
    const bool v1CompatibilityPass = trackDistValid1 && (v1DistPass || v1SigPass);
    const bool v0CompatibilityFailed = !v0CompatibilityPass;
    const bool v1CompatibilityFailed = !v1CompatibilityPass;
    const bool bothBelowMinSig =
        (t_dist_0.second.significance() < min_track_vertex_sig_to_remove &&
         t_dist_1.second.significance() < min_track_vertex_sig_to_remove);
	  bool remove_from_0 = v0CompatibilityFailed;
	  bool remove_from_1 = v1CompatibilityFailed;
	  if (bothBelowMinSig) {
	    if (tracks[0].size() > tracks[1].size())
	    remove_from_1 = true;
	    else
	      remove_from_0 = true;
	  }
	  else if (t_dist_0.second.significance() < t_dist_1.second.significance())
	    remove_from_1 = true;
	  else
	    remove_from_0 = true;
	  
    if (remove_from_0) {
      // Track is only MARKED for later removal here.
      // It is still part of the current vertex until the refit stage below.
      tracks_to_remove_in_refit[0].insert(tk);
    }
    if (remove_from_1) {
      // Track is only MARKED for later removal here.
      // It is still part of the current vertex until the refit stage below.
      tracks_to_remove_in_refit[1].insert(tk);
    }

    std::ostringstream block;
    block << "[Mark to Drop a Track (From 1 OR 2 vertices!)] " << formatTrackLabel(tk) << "\n"
          << "track_dist_valid_v0=" << (trackDistValid0 ? 1 : 0)
          << " track_dist_valid_v1=" << (trackDistValid1 ? 1 : 0) << "\n"
          << "distToV0=" << t_dist_0.second.value() << " vs max_track_vertex_dist=" << max_track_vertex_dist
          << " -> " << (v0DistPass ? "PASS" : "FAIL") << "\n"
          << "sigToV0=" << t_dist_0.second.significance() << " vs max_track_vertex_sig=" << max_track_vertex_sig
          << " -> " << (v0SigPass ? "PASS" : "FAIL") << "\n"
          << "distToV1=" << t_dist_1.second.value() << " vs max_track_vertex_dist=" << max_track_vertex_dist
          << " -> " << (v1DistPass ? "PASS" : "FAIL") << "\n"
          << "sigToV1=" << t_dist_1.second.significance() << " vs max_track_vertex_sig=" << max_track_vertex_sig
          << " -> " << (v1SigPass ? "PASS" : "FAIL") << "\n"
          << "dxyToV0=" << dxyToV0 << " dxySigToV0=" << dxySigToV0
          << " dxyToV1=" << dxyToV1 << " dxySigToV1=" << dxySigToV1
          << " dzToV0=" << dzToV0 << " dzSigToV0=" << dzSigToV0
          << " dzToV1=" << dzToV1 << " dzSigToV1=" << dzSigToV1 << "\n";

    if (remove_from_0 && remove_from_1) {
      block << "Decision: removed from both (Vertex" << (ivtx[0] + 1) << " and Vertex" << (ivtx[1] + 1)
            << ") because compatibility failed with max_track_vertex_dist=" << max_track_vertex_dist
            << " and max_track_vertex_sig=" << max_track_vertex_sig << ".";
    } else if (remove_from_0) {
      if (v0CompatibilityFailed) {
        block << "Decision: removed from Vertex" << (ivtx[0] + 1)
              << " because compatibility failed (distToV0=" << t_dist_0.second.value()
              << ", sigToV0=" << t_dist_0.second.significance()
              << ", max_track_vertex_dist=" << max_track_vertex_dist
              << ", max_track_vertex_sig=" << max_track_vertex_sig << ").";
      } else if (bothBelowMinSig) {
        block << "Decision: kept in bigger vertex; removed from Vertex" << (ivtx[0] + 1)
              << " because both significances are below min_track_vertex_sig_to_remove="
              << min_track_vertex_sig_to_remove << " and size tie-break chose this side.";
      } else {
        block << "Decision: lower significance side won; removed from Vertex" << (ivtx[0] + 1)
              << " because sigToV0=" << t_dist_0.second.significance()
              << " is not lower than sigToV1=" << t_dist_1.second.significance() << ".";
      }
    } else if (remove_from_1) {
      if (v1CompatibilityFailed) {
        block << "Decision: removed from Vertex" << (ivtx[1] + 1)
              << " because compatibility failed (distToV1=" << t_dist_1.second.value()
              << ", sigToV1=" << t_dist_1.second.significance()
              << ", max_track_vertex_dist=" << max_track_vertex_dist
              << ", max_track_vertex_sig=" << max_track_vertex_sig << ").";
      } else if (bothBelowMinSig) {
        block << "Decision: kept in bigger vertex; removed from Vertex" << (ivtx[1] + 1)
              << " because both significances are below min_track_vertex_sig_to_remove="
              << min_track_vertex_sig_to_remove << " and size tie-break chose this side.";
      } else {
        block << "Decision: lower significance side won; removed from Vertex" << (ivtx[1] + 1)
              << " because sigToV1=" << t_dist_1.second.significance()
              << " is not lower than sigToV0=" << t_dist_0.second.significance() << ".";
      }
    } else {
      block << "Decision: lower significance side won; no removal was marked in this step.";
    }
    logLine(block.str());
	  
    if (remove_one_track_at_a_time) {
      logLine("[Mark to Drop a Track (From 1 OR 2 vertices!)] remove_one_track_at_a_time=True -> stop after first shared-track decision.");
      // Important subtlety:
      // one shared-track conflict is resolved before restart,
      // but this one conflict may still remove the track from 1 OR 2 vertices.
      // Stop searching more shared tracks inside this vertex pair.
      // Only one shared-track conflict is resolved before restart.
      break;
    }
	}
	
    	// Stop searching more vertex partners for this v[0].
    	// This pair already triggered cleanup logic.
    	// Restart the full compare loop after this update.
	break;
	
      }
    }
        
    if (duplicate) {
      // This is where the vertex is ACTUALLY removed from the system.
      // Reason: dropping a redundant duplicate vertex.
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

      {
        std::ostringstream msg;
        msg << "[Temporary Merge Candidate]";
        logLine(msg.str());
      }
      {
        std::ostringstream msg;
        msg << "count=" << new_vertices.size();
        logLine(msg.str());
      }
      if (new_vertices.size() > 0) {
        std::ostringstream msg;
        msg << "candidate0={" << vertexSummary(new_vertices[0]) << "}";
        logLine(msg.str());
      }
      if (new_vertices.size() > 1) {
        std::ostringstream msg;
        msg << "candidate1={" << vertexSummary(new_vertices[1]) << "}";
        logLine(msg.str());
      }

      if (new_vertices.size() > 1) {
        assert(new_vertices.size() == 2);
        logLine("[Merge Outcome] SPLIT_RESULT");
        logLine("Reason: temporary fit returned 2 vertices");
        logLine("Action: overwrite both original slots with split fit outputs");
        {
          std::ostringstream msg;
          msg << "[SPLIT RESULT UPDATE]\n"
              << "Overwrite Vertex" << (ivtx[0] + 1) << " and Vertex" << (ivtx[1] + 1)
              << " with split fit outputs";
          logLine(msg.str());
        }
        // Replace the old vertex in-place with the new refitted / merged vertex.
        // This is NOT adding a new vertex; it overwrites the existing slot.
        *v[1] = reco::Vertex(new_vertices[1]);
        // Replace the old vertex in-place with the new refitted / merged vertex.
        // This is NOT adding a new vertex; it overwrites the existing slot.
        *v[0] = reco::Vertex(new_vertices[0]);
	
      }
      else if (new_vertices.size() == 1 && vertex_track_set(new_vertices[0], 0) == tracks_to_fit) {
        logLine("[Merge Outcome] SUCCESS");
        logLine("Reason: exactly one valid full-union vertex returned");
        logLine("Overrule: previously marked track drops are ignored");
        const reco::Vertex droppedVertex = *v[1];
        // Merge success overrules earlier marked drops for this pair.
        // We apply the full-union merged result directly.
        // This is where the vertex is ACTUALLY removed from the system.
        // Reason: dropping the source vertex after successful union merge.
        vertices->erase(v[1]);

        // Replace the old vertex in-place with the new refitted / merged vertex.
        // This is NOT adding a new vertex; it overwrites the existing slot.
        *v[0] = reco::Vertex(new_vertices[0]); // ok to use v[0] after the erase(v[1]) because v[0] is by construction before v[1]
	{
	  std::ostringstream msg;
	  msg << "[Vertex Update]\n"
	      << "SUCCESSFUL MERGE:\n"
	      << "Overwrite Vertex" << (ivtx[0] + 1) << " with merged vertex\n"
	      << "Erase Vertex" << (ivtx[1] + 1) << " from vertex collection\n"
	      << "MergedVertex={" << vertexSummary(*v[0]) << "}\n"
	      << "DroppedSourceVertex={" << vertexSummary(droppedVertex) << "}";
	  logLine(msg.str());
	}
	
      }
      else {
        logLine("[Merge Outcome] FAILURE");
        if (new_vertices.size() == 0) {
          logLine("Reason: zero vertices returned OR chi2 wrapper rejected fit");
        } else {
          logLine("Reason: one vertex missing union tracks");
        }
        logLine("Action: ignore temporary merge candidate, proceed to [Refit for the vertices whose tracks were dropped]");
        refit = true;
      }
      
    }
      // 9) Refit for the vertices whose tracks were dropped
      if (refit) {
  logLine("[Refit for the vertices whose tracks were dropped]");
  logLine("Marked tracks become actual removals here, by rebuilding from surviving tracks only.");
	bool erase[2] = { false };
	reco::Vertex vsave[2] = { *v[0], *v[1] };
	
	for (int i = 0; i < 2; ++i) {
	  if (tracks_to_remove_in_refit[i].empty())
	    continue;

        std::vector<reco::TransientTrack> ttks;
        // Actual removal happens here:
        // rebuild the vertex using only surviving tracks
        // (excluding tracks previously marked for removal).
        for (auto tk : tracks[i]) {
          if (tracks_to_remove_in_refit[i].count(tk) == 0) {
            auto tt = getTransientTrack(tk);
            if (tt.isValid()) ttks.push_back(tt);
          }
        }
        reco::VertexCollection new_vertices;
        for (const TransientVertex& tv : kv_reco_dropin(ttks))
          new_vertices.emplace_back(tv);
        {
          track_set removedTracks = tracks_to_remove_in_refit[i];
          track_set survivingTracks;
          for (auto tk : tracks[i]) {
            if (tracks_to_remove_in_refit[i].count(tk) == 0) survivingTracks.insert(tk);
          }
          std::ostringstream msg;
          msg << "[Refit Result]"
              << " VertexSide=" << i
              << " removedTracks=" << formatTrackSetLabels(removedTracks)
              << " survivingTracks=" << formatTrackSetLabels(survivingTracks);
          logLine(msg.str());
        }
        {
          std::ostringstream msg;
          if (new_vertices.size() > 0) {
            msg << "temporaryRefitCandidate={" << vertexSummary(new_vertices[0]) << "}";
          } else {
            msg << "temporaryRefitCandidate={NONE}";
          }
          logLine(msg.str());
        }
        if (new_vertices.size() == 1) {
          logLine("Outcome=SUCCESS_REPLACE");
          {
            std::ostringstream msg;
            msg << "[REFIT UPDATE]\n"
                << "Overwrite Vertex" << (ivtx[i] + 1)
                << " with surviving-track refit result";
            logLine(msg.str());
          }
          // Replace the old vertex in-place with the new refitted / merged vertex.
          // This is NOT adding a new vertex; it overwrites the existing slot.
          * v[i] = new_vertices[0];
        }
        else {
          if (ttks.size() < 2) {
            logLine("Outcome=FAILURE_DROP reason=<2 tracks survived");
          } else if (new_vertices.size() == 0) {
            logLine("Outcome=FAILURE_DROP reason=fit invalid OR chi2 wrapper rejected OR not exactly one valid replacement");
          } else {
            logLine("Outcome=FAILURE_DROP reason=not exactly one valid replacement");
          }
          erase[i] = true;
	}
	}

      if (erase[1]) {
        {
          std::ostringstream msg;
          msg << "[REFIT UPDATE]\n"
              << "Drop Vertex" << (ivtx[1] + 1)
              << " because no valid replacement was produced";
          logLine(msg.str());
        }
        // This is where the vertex is ACTUALLY removed from the system.
        // Reason: dropping an invalid vertex after refit failure.
        vertices->erase(v[1]);
      }
      if (erase[0]) {
        {
          std::ostringstream msg;
          msg << "[REFIT UPDATE]\n"
              << "Drop Vertex" << (ivtx[0] + 1)
              << " because no valid replacement was produced";
          logLine(msg.str());
        }
        // This is where the vertex is ACTUALLY removed from the system.
        // Reason: dropping an invalid vertex after refit failure.
        vertices->erase(v[0]);
      }

      if (erase[0] || erase[1]) {
        std::ostringstream msg;
        msg << "DROP_VERTEX reason=refit/arbitration could not produce a valid replacement"
            << " eraseV0=" << (erase[0] ? 1 : 0)
            << " eraseV1=" << (erase[1] ? 1 : 0)
            << " originalV0={" << vertexSummary(vsave[0]) << "}"
            << " originalV1={" << vertexSummary(vsave[1]) << "}";
        logLine(msg.str());
        logLine("Refit for this side did not return exactly one valid vertex, so that side is dropped.");
      }

      }

    // 10) Restart Loop
    // If we changed the vertices at all, start loop over completely.
    if (duplicate || merge || refit) {
      dumpVertices("State after a merge/drop/refit step:");
      logLine("[Restart Loop]");
      logLine("Reason: vertex list topology changed after merge/refit/drop");
      logLine("Restart full pairwise scan from beginning");
      logLine("============================================================");
      //printf("duplicate = %d, merge = %d, refit = %d\n", duplicate, merge, refit);
      
      // Restart Loop:
      // the vertex list changed, so restart the full pairwise scan from the beginning.
      // Previous pair ordering assumptions may no longer be valid.
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

    logLine(" ");
    logLine("------------------- LOOSE MERGE PHASE -----------------------");
    logLine("Goal: merge nearby vertices that satisfy loose distance/significance criteria.");

    
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
              const reco::Vertex droppedVertex = *v[1];
              {
                std::ostringstream msg;
                msg << "MERGE_STEP loose-merge succeeded between two nearby vertices"
                    << " droppedVertex={" << vertexSummary(droppedVertex) << "}"
                    << " mergedVertex={" << vertexSummary(merged_vertices[0]) << "}"
                    << " mergedTracks=" << formatTrackSetLabels(vertex_track_set(merged_vertices[0], 0.0));
                logLine(msg.str());
                logLine("Interpretation: loose geometric consistency favored replacing two vertices with one.");
              }
              
              //std::cout << "check no mem out of ranges (before) : " << v[1] - vertices->begin() << std::endl;
              // Replace the old vertex in-place with the new refitted / merged vertex.
              // This is NOT adding a new vertex; it overwrites the existing slot.
              *v[0] = merged_vertices[0];
              //std::cout << "check no mem out of ranges (after) : " << v[1] - vertices->begin() << std::endl;

              // This is where the vertex is ACTUALLY removed from the system.
              // Reason: dropping the source vertex after successful union merge.
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
    logLine(" ");
    logLine("----------------- N-1 TRACK DROP PHASE ----------------------");
    logLine("Goal: test whether removing one track yields a vertex movement beyond configured limits.");
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

          {
            std::ostringstream msg;
            msg << "DROP_TRACK_FROM_VERTEX reason=nm1-refit move-too-large-or-invalid"
                << " removedTrack=" << formatTrackLabel(tks[i])
                << " oldVertex={" << vertexSummary(*v[0]) << "}"
                << " newVertexAfterDrop={" << vertexSummary(vnm1) << "}"
                << " dist3=" << std::sqrt(dist3_2)
                << " distz=" << distz;
            logLine(msg.str());
            logLine("Interpretation: this track is incompatible with stable-vertex hypothesis under N-1 test.");
          }

          // Replace the old vertex in-place with the new refitted / merged vertex.
          // This is NOT adding a new vertex; it overwrites the existing slot.
          *v[0] = vnm1;
          ++refit_count[iv];
          --v[0], --iv;
          // Stop searching more track-removal candidates for this vertex in this pass.
          // One update was applied; control returns to continue the outer scan.
          break;
        }
      }
    }
    iv = 0; //some vertices after dz refiting have normalized chi2 > 5
    for (v[0] = vertices->begin(); v[0] != vertices->end(); ++v[0], ++iv) {
       if ((*v[0]).normalizedChi2() > 5) {
         {
           std::ostringstream msg;
           msg << "DROP_VERTEX reason=post-nm1 normalizedChi2>5"
               << " content={" << vertexSummary(*v[0]) << "}";
           logLine(msg.str());
           logLine("Interpretation: vertex quality no longer acceptable after iterative N-1 updates.");
         }
         // This is where the vertex is ACTUALLY removed from the system.
         // Reason: dropping an invalid vertex after refit failure.
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
    logLine(" ");
    logLine("------------------- TIGHT MERGE PHASE -----------------------");
    logLine("Goal: resolve split vertices with strict dPhi/svdist2d/dBV criteria.");
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
          double dBV0 = dBV0_Meas1D.value();
          double dBV1 = dBV1_Meas1D.value();
          double v0x = v[0]->x() - ref_x;
          double v0y = v[0]->y() - ref_y;
          double phi0 = atan2(v0y, v0x);
          double v1x = v[1]->x() - ref_x;
          double v1y = v[1]->y() - ref_y;
          double phi1 = atan2(v1y, v1x);
          // RESTORE original threshold: svdist2d < 0.0300
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

              const reco::Vertex droppedVertex = *v[1];

              {
                std::ostringstream msg;
                msg << "MERGE_STEP tight-merge succeeded"
                    << " droppedVertex={" << vertexSummary(droppedVertex) << "}"
                    << " mergedVertex={" << vertexSummary(merged_vertices[0]) << "}"
                    << " tracks=" << formatTrackSetLabels(tracks_to_fit);
                logLine(msg.str());
                logLine("Interpretation: tight split-vertex criteria confirmed a physically consistent merge.");
              }

              merge = true;

              // This is where the vertex is ACTUALLY removed from the system.
              // Reason: dropping the source vertex after successful union merge.
              v[1] = vertices->erase(v[1]) - 1; // (1) erase and point the iterator at the previous entry
              // Replace the old vertex in-place with the new refitted / merged vertex.
              // This is NOT adding a new vertex; it overwrites the existing slot.
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
  // 11) Return the reconstructed vertices
  //////////////////////////////////////////////////////////////////////

  logLine(" ");
  logLine("[Return the reconstructed vertices]");
  logLine("No pairs of vertices share a common track after a full scan:");
  logLine("Return the reconstructed vertices");
  dumpVertices("Final vertex state:");
  logLine("Return complete: only reconstructed output vertices are persisted to the event.");
  logLine("============================================================");
  
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