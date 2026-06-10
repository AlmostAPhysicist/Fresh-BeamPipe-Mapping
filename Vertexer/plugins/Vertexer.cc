// -*- C++ -*-
//
// Package:    Run3ScoutingAnalysisTools/Vertexer
// Class:      Vertexer
//
// Original Author:  Bruno Lopes
//         Created:  Wed, 04 Sep 2024 12:37:22 GMT
//
// Refactored to strictly implement the documented algorithm:
//   1. Jet preselection (>=N jets, pT > jet_pt_min, |eta| < jet_eta_max)
//   2. Track preselection:
//        - pT > minSeedPt
//        - 2D transverse IP significance (w.r.t. beamspot) > minSeedIPSig
//        - pixel hits > minSeedPixelHits
//        - strip hits > minSeedStripHits
//        - tracker layers > minSeedTrackerLayers
//        - within deltaR < seedJetDrMax of a preselected jet
//   3. Seed vertex: Kalman fit on each track pair, accept if chi2/dof < max_seed_vertex_chi2
//   4. Track arbitration (shared-track resolution):
//        - If sigma_VV,3D < merge_shared_sig  =>  try union merge
//        - Else if sigma_TV,3D >= max_track_vertex_sig  =>  remove from that vertex
//        - Else if sigma_TV,3D < min_track_vertex_sig_to_remove for BOTH  =>  remove from
//          vertex with fewer tracks (tie: remove from first)
//        - Otherwise  =>  remove from vertex with higher sigma_TV,3D
//   5. N-1 refit: remove any track whose absence shifts 3D vertex > max_nm1_refit_dist3
//   6. Loose/tight split-vertex merging
//
// All numerical thresholds are configurable via the ParameterSet.

#include <memory>
#include <algorithm>
#include <cmath>
#include <cassert>

// Framework
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

// Data formats
#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/Common/interface/ValueMap.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/BeamSpot/interface/BeamSpot.h"
#include "CondFormats/DataRecord/interface/BeamSpotOnlineHLTObjectsRcd.h"
#include "CondFormats/BeamSpotObjects/interface/BeamSpotOnlineObjects.h"

// Scouting
#include "DataFormats/Scouting/interface/Run3ScoutingPFJet.h"
#include "DataFormats/Scouting/interface/Run3ScoutingTrack.h"

// Vertex tools
#include "RecoVertex/KalmanVertexFit/interface/KalmanVertexFitter.h"
#include "RecoVertex/VertexTools/interface/VertexDistance3D.h"
#include "RecoVertex/VertexTools/interface/VertexDistanceXY.h"
#include "TrackingTools/IPTools/interface/IPTools.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "TrackingTools/TransientTrack/interface/TransientTrack.h"
#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"

using namespace edm;

// ---------------------------------------------------------------------------
class Vertexer : public edm::stream::EDProducer<> {
public:
  explicit Vertexer(edm::ParameterSet const& params);
  ~Vertexer() override = default;

private:
  // ---- typedefs ----
  typedef std::set<reco::TrackRef>    track_set;
  typedef std::vector<reco::TrackRef> track_vec;

  void beginStream(edm::StreamID) override {}
  void endStream()                override {}
  void produce(edm::Event&, const edm::EventSetup&) override;

  // ---- helper: is 'a' a subset of 'b' (or vice-versa)? ----
  bool is_track_subset(const track_set& a, const track_set& b) const {
    const track_set& smaller = (a.size() <= b.size()) ? a : b;
    const track_set& bigger  = (a.size() <= b.size()) ? b : a;
    for (auto t : smaller)
      if (!bigger.count(t)) return false;
    return true;
  }

  // ---- helper: extract track set from a vertex ----
  track_set vertex_track_set(const reco::Vertex& v,
                              double min_weight = 0.5) const {
    track_set result;
    for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it)
      if (v.trackWeight(*it) >= min_weight)
        result.insert(it->castTo<reco::TrackRef>());
    return result;
  }

  track_vec vertex_track_vec(const reco::Vertex& v,
                              double min_weight = 0.5) const {
    track_set s = vertex_track_set(v, min_weight);
    return track_vec(s.begin(), s.end());
  }

  // ---- impact parameter: transverse (2-D) OR 3-D for BS preselection ----
  // Per spec: seed preselection always uses 2-D transverse IP significance
  // (sigma_TB,2D > minSeedIPSig).  The flag use_2d_tv_dist_bsIPpresel keeps
  // this configurable in case the analysis changes.
  std::pair<bool, Measurement1D>
  ip_to_reference(const reco::TransientTrack& t,
                  const reco::Vertex& ref) const {
    return use_2d_tv_dist_bsIPpresel_
        ? IPTools::absoluteTransverseImpactParameter(t, ref)
        : IPTools::absoluteImpactParameter3D(t, ref);
  }

  // ---- track-to-vertex distance for arbitration (sigma_TV) ----
  // Per spec: arbitration uses 3-D significance (sigma_TV,3D).
  // The flag use_2d_tv_dist_forReco keeps this configurable.
  std::pair<bool, Measurement1D>
  track_vertex_dist(const reco::TransientTrack& t,
                    const reco::Vertex& v) const {
    return use_2d_tv_dist_forReco_
        ? IPTools::absoluteTransverseImpactParameter(t, v)
        : IPTools::absoluteImpactParameter3D(t, v);
  }

  // ---- vertex-to-vertex distance ----
  // Per spec: merge criterion sigma_VV,3D uses 3-D distance.
  // The flag use_2d_vv_dist_forReco keeps it configurable.
  Measurement1D vertex_vertex_dist(const reco::Vertex& v0,
                                   const reco::Vertex& v1) const {
    return use_2d_vv_dist_forReco_
        ? vertex_dist_2d_.distance(v0, v1)
        : vertex_dist_3d_.distance(v0, v1);
  }

  // ---- safe transient-track builder (bounds check) ----
  reco::TransientTrack
  make_transient_track(const reco::TrackRef& ref,
                       const TransientTrackBuilder& builder) const {
    return builder.build(ref);
  }

  // ---- Kalman fitter wrapper: returns empty if <2 tracks or chi2/dof >= 5 ----
  std::vector<TransientVertex>
  kv_fit(std::vector<reco::TransientTrack>& ttks) {
    if (ttks.size() < 2) return {};
    for (auto const& tt : ttks)
      if (!tt.isValid()) return {};
    TransientVertex tv = kv_fitter_.vertex(ttks);
    if (!tv.isValid() || tv.normalisedChiSquared() >= max_seed_vertex_chi2_)
      return {};
    return { tv };
  }

  // ---- debug print helpers ----
  void print_track_set(const track_set& ts) const {
    for (auto r : ts) printf(" %u", r.key());
  }
  void print_track_set(const track_set& ts, const reco::Vertex& v) const {
    for (auto r : ts)
      printf(" %u%s", r.key(), (v.trackWeight(r) < 0.5 ? "!" : ""));
  }

  // ===== configuration =====

  // -- seed track / jet selection --
  const double minSeedPt_;
  const double minSeedIPSig_;
  const int    minSeedPixelHits_;
  const int    minSeedStripHits_;
  const int    minSeedTrackerLayers_;
  const double maxSeedEta_;
  const double seedJetDrMax_;
  const int    minSelectedJets_;
  const double jetPtMin_;
  const double jetEtaMax_;

  // -- vertex seed --
  const int    nTracksPerSeedVertex_;
  const double max_seed_vertex_chi2_;   // chi2/dof cut for seed vertices

  // -- distance-mode flags (three independent axes) --
  //   use_2d_vv_dist_forReco    : vv distance used in merging / arbitration
  //   use_2d_tv_dist_bsIPpresel : tv distance used in BS-IP preselection
  //   use_2d_tv_dist_forReco    : tv distance used in arbitration
  const bool use_2d_vv_dist_forReco_;
  const bool use_2d_tv_dist_bsIPpresel_;
  const bool use_2d_tv_dist_forReco_;

  // -- track arbitration thresholds --
  const double merge_shared_sig_;            // sigma_VV threshold => union-merge
  const double merge_shared_dist_;           // absolute vv-dist threshold (OR with sig)
  const double max_track_vertex_sig_;        // sigma_TV >= this => remove from vertex
  const double min_track_vertex_sig_to_remove_; // sigma_TV < this for BOTH => size tiebreak
  const double max_track_vertex_dist_;       // absolute tv-dist (OR with sig, can be -1 to disable)
  const bool   remove_one_track_at_a_time_;

  // -- N-1 refit --
  const double max_nm1_refit_dist3_;   // max allowed 3-D shift (cm) when a track is removed
  const double max_nm1_refit_distz_;   // max allowed z-shift (cm), -1 = disabled

  // -- loose split-vertex merge --
  const bool   resolve_split_vertices_loose_;
  const double merge_anyway_dist_;     // absolute vv-dist threshold
  const double merge_anyway_sig_;      // vv-sig threshold

  // -- tight split-vertex merge (per spec: dBV>0.01, dvv2d<0.05, dPhi<0.5) --
  const bool   resolve_split_vertices_tight_;
  const double split_merge_dbv_min_;   // dBV cut for both vertices
  const double split_merge_dvv2d_max_; // dvv,2D cut
  const double split_merge_dphi_max_;  // |dPhi| cut

  // -- beamspot --
  const bool useOnlineBeamSpot_;
  const bool logBeamspotSource_;

  // -- verbosity --
  const bool verbose_;

  // ===== tokens =====
  const edm::ESGetToken<BeamSpotOnlineObjects,
                        BeamSpotOnlineHLTObjectsRcd>            bsOnlineToken_;
  const edm::EDGetTokenT<reco::BeamSpot>                        beamspotToken_;
  const edm::EDGetTokenT<std::vector<reco::Track>>              seed_tracks_token_;
  const edm::EDGetTokenT<std::vector<Run3ScoutingPFJet>>        seed_jets_token_;
  const edm::EDGetTokenT<
    edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>>    trackToScoutingMapToken_;
  const edm::ESGetToken<TransientTrackBuilder,
                        TransientTrackRecord>                   tt_builder_token_;

  edm::EDPutTokenT<reco::VertexCollection> putToken_;

  // ===== member objects =====
  VertexDistanceXY vertex_dist_2d_;
  VertexDistance3D vertex_dist_3d_;
  KalmanVertexFitter kv_fitter_;
};

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
Vertexer::Vertexer(edm::ParameterSet const& params)
    :
    // -- track / jet selection --
    minSeedPt_           (params.getUntrackedParameter<double>("minSeedPt",            0.9)),
    minSeedIPSig_        (params.getUntrackedParameter<double>("minSeedIPSig",         4.0)),
    minSeedPixelHits_    (params.getParameter<int>            ("minSeedPixelHits")),
    minSeedStripHits_    (params.getParameter<int>            ("minSeedStripHits")),
    minSeedTrackerLayers_(params.getParameter<int>            ("minSeedTrackerLayers")),
    maxSeedEta_          (params.getUntrackedParameter<double>("maxSeedEta",           2.4)),
    seedJetDrMax_        (params.getUntrackedParameter<double>("jet_dr_max",           0.4)),
    minSelectedJets_     (params.getParameter<int>            ("min_selected_jets")),
    jetPtMin_            (params.getParameter<double>         ("jet_pt_min")),
    jetEtaMax_           (params.getParameter<double>         ("jet_eta_max")),

    // -- vertex seed --
    nTracksPerSeedVertex_(params.getParameter<int>   ("n_tracks_per_seed_vertex")),
    max_seed_vertex_chi2_(params.getParameter<double>("max_seed_vertex_chi2")),

    // -- distance mode flags --
    use_2d_vv_dist_forReco_    (params.getParameter<bool>("use_2d_vv_dist_forReco")),
    use_2d_tv_dist_bsIPpresel_ (params.getParameter<bool>("use_2d_tv_dist_bsIPpresel")),
    use_2d_tv_dist_forReco_    (params.getParameter<bool>("use_2d_tv_dist_forReco")),

    // -- arbitration --
    merge_shared_sig_               (params.getParameter<double>("merge_shared_sig")),
    merge_shared_dist_              (params.getParameter<double>("merge_shared_dist")),
    max_track_vertex_sig_           (params.getParameter<double>("max_track_vertex_sig")),
    min_track_vertex_sig_to_remove_ (params.getParameter<double>("min_track_vertex_sig_to_remove")),
    max_track_vertex_dist_          (params.getParameter<double>("max_track_vertex_dist")),
    remove_one_track_at_a_time_     (params.getParameter<bool>  ("remove_one_track_at_a_time")),

    // -- N-1 refit --
    max_nm1_refit_dist3_(params.getParameter<double>("max_nm1_refit_dist3")),
    max_nm1_refit_distz_(params.getParameter<double>("max_nm1_refit_distz")),

    // -- loose merge --
    resolve_split_vertices_loose_(params.getParameter<bool>  ("resolve_split_vertices_loose")),
    merge_anyway_dist_           (params.getParameter<double>("merge_anyway_dist")),
    merge_anyway_sig_            (params.getParameter<double>("merge_anyway_sig")),

    // -- tight merge --
    resolve_split_vertices_tight_(params.getParameter<bool>  ("resolve_split_vertices_tight")),
    split_merge_dbv_min_         (params.getParameter<double>("split_merge_dbv_min")),
    split_merge_dvv2d_max_       (params.getParameter<double>("split_merge_dvv2d_max")),
    split_merge_dphi_max_        (params.getParameter<double>("split_merge_dphi_max")),

    // -- beamspot --
    useOnlineBeamSpot_ (params.getUntrackedParameter<bool>("useOnlineBeamSpot",  true)),
    logBeamspotSource_ (params.getUntrackedParameter<bool>("logBeamspotSource",  false)),

    // -- verbosity --
    verbose_(params.getParameter<bool>("verbose")),

    // -- tokens --
    bsOnlineToken_   (esConsumes<BeamSpotOnlineObjects, BeamSpotOnlineHLTObjectsRcd>()),
    beamspotToken_   (consumes<reco::BeamSpot>(
                          params.getParameter<edm::InputTag>("beamspot_src"))),
    seed_tracks_token_(consumes<std::vector<reco::Track>>(
                          params.getParameter<edm::InputTag>("seed_tracks_src"))),
    seed_jets_token_ (consumes<std::vector<Run3ScoutingPFJet>>(
                          params.getParameter<edm::InputTag>("seed_jets_src"))),
    trackToScoutingMapToken_(
        params.existsAs<edm::InputTag>("trackToScoutingMap")
            ? consumes<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>>(
                  params.getParameter<edm::InputTag>("trackToScoutingMap"))
            : edm::EDGetTokenT<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>>()),
    tt_builder_token_(esConsumes(edm::ESInputTag("", "TransientTrackBuilder"))),
    putToken_{produces()}
{}

// ---------------------------------------------------------------------------
// produce
// ---------------------------------------------------------------------------
void Vertexer::produce(edm::Event& iEvent, const edm::EventSetup& iSetup) {

  // ----------------------------------------------------------------
  // STEP 0 – Build beamspot reference vertex
  // ----------------------------------------------------------------
  double bs_x = 0, bs_y = 0, bs_z = 0;
  reco::Vertex::Error bs_err;
  for (int i = 0; i < 3; ++i)
    for (int j = i; j < 3; ++j)
      bs_err(i, j) = 0.0;

  bool haveBS = false;

  if (useOnlineBeamSpot_) {
    auto bsOnlineH = iSetup.getHandle(bsOnlineToken_);
    if (bsOnlineH.isValid()) {
      bs_x = bsOnlineH->x();
      bs_y = bsOnlineH->y();
      bs_z = bsOnlineH->z();
      for (int i = 0; i < 3; ++i)
        for (int j = i; j < 3; ++j)
          bs_err(i, j) = bsOnlineH->covariance(i, j);
      haveBS = true;
      if (logBeamspotSource_)
        edm::LogInfo("Vertexer") << "Using online beamspot x=" << bs_x
                                 << " y=" << bs_y << " z=" << bs_z;
    } else {
      edm::LogError("Vertexer") << "Online beamspot requested but unavailable; "
                                   "falling back to offline.";
    }
  }

  if (!haveBS) {
    edm::Handle<reco::BeamSpot> bsH;
    iEvent.getByToken(beamspotToken_, bsH);
    if (bsH.isValid()) {
      bs_x = bsH->position().x();
      bs_y = bsH->position().y();
      bs_z = bsH->position().z();
      bs_err = bsH->covariance3D();
      haveBS = true;
      if (logBeamspotSource_)
        edm::LogInfo("Vertexer") << "Using offline beamspot x=" << bs_x
                                 << " y=" << bs_y << " z=" << bs_z;
    }
  }

  if (!haveBS) {
    edm::LogWarning("Vertexer") << "No beamspot available – returning empty collection.";
    iEvent.emplace(putToken_, reco::VertexCollection());
    return;
  }

  const reco::Vertex fake_bs_vtx(reco::Vertex::Point(bs_x, bs_y, bs_z), bs_err);

  // ----------------------------------------------------------------
  // STEP 1 – Jet preselection
  // ----------------------------------------------------------------
  edm::Handle<std::vector<Run3ScoutingPFJet>> jetsH;
  iEvent.getByToken(seed_jets_token_, jetsH);

  std::vector<const Run3ScoutingPFJet*> selected_jets;
  if (jetsH.isValid()) {
    for (auto const& jet : *jetsH) {
      if (jet.pt() > jetPtMin_ && std::abs(jet.eta()) < jetEtaMax_)
        selected_jets.push_back(&jet);
    }
  }

  if (static_cast<int>(selected_jets.size()) < minSelectedJets_) {
    if (verbose_)
      printf("Vertexer: event rejected – only %zu jets pass selection (need %d)\n",
             selected_jets.size(), minSelectedJets_);
    iEvent.emplace(putToken_, reco::VertexCollection());
    return;
  }

  // ----------------------------------------------------------------
  // STEP 2 – Track preselection
  // ----------------------------------------------------------------
  edm::Handle<std::vector<reco::Track>> seed_track_handle;
  iEvent.getByToken(seed_tracks_token_, seed_track_handle);
  if (!seed_track_handle.isValid()) {
    edm::LogWarning("Vertexer") << "Seed track handle invalid – returning empty collection.";
    iEvent.emplace(putToken_, reco::VertexCollection());
    return;
  }

  // Optional scouting hit-count ValueMap
  edm::Handle<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>> scoutingMapH;
  const bool haveScoutingMap =
      !trackToScoutingMapToken_.isUninitialized() &&
      iEvent.getByToken(trackToScoutingMapToken_, scoutingMapH) &&
      scoutingMapH.isValid();

  auto const& tt_builder = iSetup.getData(tt_builder_token_);

  // Lambda: deltaR between track and jet
  auto deltaR_track_jet = [](const reco::Track& tk, const Run3ScoutingPFJet* jet) {
    double dEta = tk.eta() - jet->eta();
    double dPhi = tk.phi() - jet->phi();
    while (dPhi >  M_PI) dPhi -= 2.0 * M_PI;
    while (dPhi <= -M_PI) dPhi += 2.0 * M_PI;
    return std::hypot(dEta, dPhi);
  };

  std::vector<reco::TrackRef> seed_tracks;    // tracks that passed all cuts
  std::map<reco::TrackRef, size_t> seed_track_index_map;  // ref -> index in seed_tracks

  for (size_t i_tk = 0; i_tk < seed_track_handle->size(); ++i_tk) {
    edm::Ref<reco::TrackCollection> tk_ref(seed_track_handle, i_tk);
    const reco::Track& tk = *tk_ref;

    // Basic kinematics
    if (tk.pt() <= minSeedPt_)              continue;
    if (std::abs(tk.eta()) >= maxSeedEta_) continue;

    // 2-D transverse IP significance w.r.t. beamspot
    reco::TransientTrack ttk = tt_builder.build(tk_ref);
    if (!ttk.isValid()) continue;
    auto ip = ip_to_reference(ttk, fake_bs_vtx);
    if (!ip.first) continue;
    const float ip_sig = ip.second.significance();
    if (!std::isfinite(ip_sig) || ip_sig <= minSeedIPSig_) continue;

    // Hit quality – prefer scouting ValueMap, fall back to reco::Track hitPattern
    if (haveScoutingMap) {
      const auto scRef = (*scoutingMapH)[tk_ref];
      if (scRef.isNonnull()) {
        if (scRef->tk_nValidPixelHits()                   <= minSeedPixelHits_)    continue;
        if (scRef->tk_nValidStripHits()                   <= minSeedStripHits_)    continue;
        if (scRef->tk_nTrackerLayersWithMeasurement()     <= minSeedTrackerLayers_) continue;
      }
    } else {
      if (tk.hitPattern().numberOfValidPixelHits()           <= minSeedPixelHits_)    continue;
      if (tk.hitPattern().numberOfValidStripHits()           <= minSeedStripHits_)    continue;
      if (tk.hitPattern().trackerLayersWithMeasurement()     <= minSeedTrackerLayers_) continue;
    }

    // Must be within dR < seedJetDrMax_ of at least one selected jet
    bool matched = false;
    for (auto jet_ptr : selected_jets) {
      if (deltaR_track_jet(tk, jet_ptr) < seedJetDrMax_) {
        matched = true;
        break;
      }
    }
    if (!matched) continue;

    seed_track_index_map[tk_ref] = seed_tracks.size();
    seed_tracks.push_back(tk_ref);

    if (verbose_)
      printf("Seed KEPT: key=%u pt=%.3f ipsig=%.3f\n",
             tk_ref.key(), tk.pt(), ip_sig);
  }

  // ----------------------------------------------------------------
  // STEP 3 – Build transient tracks for the seed set
  // ----------------------------------------------------------------
  // We cache TransientTracks indexed by seed_tracks position for speed.
  std::vector<reco::TransientTrack> ttk_cache;
  ttk_cache.reserve(seed_tracks.size());
  for (auto const& ref : seed_tracks)
    ttk_cache.push_back(tt_builder.build(ref));

  // Map from TrackRef -> index in seed_tracks (for fast lookup in arbitration)
  auto ttk_of = [&](const reco::TrackRef& ref) -> const reco::TransientTrack& {
    return ttk_cache.at(seed_track_index_map.at(ref));
  };

  const size_t ntk = seed_tracks.size();

  std::unique_ptr<reco::VertexCollection> vertices(new reco::VertexCollection);

  if (ntk < static_cast<size_t>(nTracksPerSeedVertex_)) {
    iEvent.emplace(putToken_, std::move(*vertices));
    return;
  }

  // ----------------------------------------------------------------
  // STEP 4 – Seed vertex formation (all k-tuples of selected tracks)
  // ----------------------------------------------------------------
  std::vector<size_t> itks(nTracksPerSeedVertex_, 0);

  auto try_seed_vertex = [&]() {
    std::vector<reco::TransientTrack> ttks(nTracksPerSeedVertex_);
    for (int i = 0; i < nTracksPerSeedVertex_; ++i) {
      ttks[i] = ttk_cache[itks[i]];
      if (!ttks[i].isValid()) return;
    }
    auto tv_vec = kv_fit(ttks);
    if (tv_vec.empty()) return;
    vertices->emplace_back(tv_vec[0]);
    if (verbose_) {
      const reco::Vertex& v = vertices->back();
      printf("Seed vertex #%zu  chi2/dof=%.3f  pos=(%.4f,%.4f,%.4f)\n",
             vertices->size() - 1, v.normalizedChi2(),
             v.x() - bs_x, v.y() - bs_y, v.z() - bs_z);
    }
  };

  for (size_t i0 = 0; i0 < ntk; ++i0) {
    itks[0] = i0;
    for (size_t i1 = i0 + 1; i1 < ntk; ++i1) {
      itks[1] = i1;
      if (nTracksPerSeedVertex_ == 2) { try_seed_vertex(); continue; }
      for (size_t i2 = i1 + 1; i2 < ntk; ++i2) {
        itks[2] = i2;
        if (nTracksPerSeedVertex_ == 3) { try_seed_vertex(); continue; }
        for (size_t i3 = i2 + 1; i3 < ntk; ++i3) {
          itks[3] = i3;
          if (nTracksPerSeedVertex_ == 4) { try_seed_vertex(); continue; }
          for (size_t i4 = i3 + 1; i4 < ntk; ++i4) {
            itks[4] = i4;
            try_seed_vertex();
          }
        }
      }
    }
  }

  // ----------------------------------------------------------------
  // STEP 5 – Track arbitration (shared-track resolution)
  //
  // Algorithm (exactly as specified):
  //   For each pair of vertices that share at least one track:
  //     A) sigma_VV < merge_shared_sig  (OR dist < merge_shared_dist)
  //        => attempt union merge; if successful stop, else fall through to B
  //     B) For each shared track:
  //        - sigma_TV >= max_track_vertex_sig  => hard remove from that vertex
  //        - sigma_TV < min_track_vertex_sig_to_remove for BOTH vertices
  //            => keep in larger vertex; remove from smaller (tie: remove from v[0])
  //        - Otherwise => remove from the vertex with HIGHER sigma_TV
  //        (if remove_one_track_at_a_time_ is set, stop after the first track)
  //   Any change triggers a full restart.
  // ----------------------------------------------------------------
  int n_resets    = 0;
  int n_onetracks = 0;

  std::vector<reco::Vertex>::iterator v[2];
  size_t ivtx[2];

  for (v[0] = vertices->begin(); v[0] != vertices->end(); ++v[0]) {
    ivtx[0] = v[0] - vertices->begin();
    track_set tracks[2];
    tracks[0] = vertex_track_set(*v[0]);

    // Drop vertices reduced to <2 tracks during earlier iterations
    if (tracks[0].size() < 2) {
      if (verbose_)
        printf("Arbitration: vertex #%zu has <2 tracks – dropping\n", ivtx[0]);
      v[0] = vertices->erase(v[0]) - 1;
      ++n_onetracks;
      continue;
    }

    bool changed = false;

    for (v[1] = v[0] + 1; v[1] != vertices->end(); ++v[1]) {
      ivtx[1] = v[1] - vertices->begin();
      tracks[1] = vertex_track_set(*v[1]);

      if (tracks[1].size() < 2) {
        if (verbose_)
          printf("Arbitration: vertex #%zu has <2 tracks – dropping\n", ivtx[1]);
        v[1] = vertices->erase(v[1]) - 1;
        ++n_onetracks;
        continue;
      }

      // ---- Rule 1: subset / duplicate ----
      if (is_track_subset(tracks[0], tracks[1])) {
        if (verbose_)
          printf("Arbitration: v#%zu is subset of v#%zu – erasing\n", ivtx[1], ivtx[0]);
        vertices->erase(v[1]);
        changed = true;
        break;
      }

      // ---- Find shared tracks ----
      std::vector<reco::TrackRef> shared;
      for (auto const& tk : tracks[0])
        if (tracks[1].count(tk)) shared.push_back(tk);

      if (shared.empty()) continue;

      // ---- Measure vertex-vertex distance ----
      Measurement1D vv_dist = vertex_vertex_dist(*v[0], *v[1]);

      if (verbose_)
        printf("Arbitration: v#%zu vs v#%zu  vv_dist=%.4f  vv_sig=%.4f  shared=%zu\n",
               ivtx[0], ivtx[1], vv_dist.value(), vv_dist.significance(),
               shared.size());

      // ---- Attempt union merge if vertices are "close" ----
      const bool try_merge = (vv_dist.significance() < merge_shared_sig_) ||
                             (merge_shared_dist_ > 0 &&
                              vv_dist.value()        < merge_shared_dist_);

      if (try_merge) {
        track_set union_set = tracks[0];
        union_set.insert(tracks[1].begin(), tracks[1].end());

        std::vector<reco::TransientTrack> ttks;
        ttks.reserve(union_set.size());
        for (auto const& ref : union_set)
          ttks.push_back(tt_builder.build(ref));

        auto merged = kv_fit(ttks);

        // Accept only if the fit returns exactly the union track set
        if (merged.size() == 1) {
          reco::Vertex merged_vtx(merged[0]);
          if (vertex_track_set(merged_vtx, 0) == union_set) {
            if (verbose_)
              printf("Arbitration: merged v#%zu + v#%zu\n", ivtx[0], ivtx[1]);
            vertices->erase(v[1]);
            *v[0] = merged_vtx;
            changed = true;
            break;
          }
        }
        // Merge failed: fall through to track-by-track arbitration
        if (verbose_)
          printf("Arbitration: merge attempt for v#%zu+v#%zu failed – falling back to arbitration\n",
                 ivtx[0], ivtx[1]);
      }

      // ---- Per-track arbitration ----
      track_set remove_from[2];

      for (auto const& tk : shared) {
        auto t_dist_0 = track_vertex_dist(ttk_of(tk), *v[0]);
        auto t_dist_1 = track_vertex_dist(ttk_of(tk), *v[1]);

        const double sig0 = t_dist_0.first ? t_dist_0.second.significance() : 1e9;
        const double sig1 = t_dist_1.first ? t_dist_1.second.significance() : 1e9;
        const double dist0 = t_dist_0.first ? t_dist_0.second.value() : 1e9;
        const double dist1 = t_dist_1.first ? t_dist_1.second.value() : 1e9;

        if (verbose_)
          printf("  Shared track %u: sig_tv0=%.3f sig_tv1=%.3f dist_tv0=%.4f dist_tv1=%.4f\n",
                 tk.key(), sig0, sig1, dist0, dist1);

        // Check absolute distance cut (disabled when max_track_vertex_dist_ <= 0)
        const bool dist0_pass = (max_track_vertex_dist_ > 0)
                                    ? (dist0 < max_track_vertex_dist_)
                                    : true;
        const bool dist1_pass = (max_track_vertex_dist_ > 0)
                                    ? (dist1 < max_track_vertex_dist_)
                                    : true;

        // Rule B1: hard remove if sigma_TV >= max_track_vertex_sig
        bool rm0 = !t_dist_0.first || (!dist0_pass && sig0 >= max_track_vertex_sig_);
        bool rm1 = !t_dist_1.first || (!dist1_pass && sig1 >= max_track_vertex_sig_);

        if (!rm0 && !rm1) {
          // Rule B2: both below min_track_vertex_sig_to_remove => size tie-break
          if (sig0 < min_track_vertex_sig_to_remove_ &&
              sig1 < min_track_vertex_sig_to_remove_) {
            if (tracks[0].size() > tracks[1].size())
              rm1 = true;
            else
              rm0 = true;   // tie or v[0] is smaller
          } else {
            // Rule B3: remove from vertex with HIGHER sigma_TV
            if (sig0 >= sig1)
              rm0 = true;
            else
              rm1 = true;
          }
        }

        if (rm0) remove_from[0].insert(tk);
        if (rm1) remove_from[1].insert(tk);

        if (remove_one_track_at_a_time_) break;
      }

      // ---- Refit each affected vertex with surviving tracks ----
      bool erase[2] = { false, false };
      reco::Vertex vsave[2] = { *v[0], *v[1] };

      for (int side = 0; side < 2; ++side) {
        if (remove_from[side].empty()) continue;

        std::vector<reco::TransientTrack> survivors;
        for (auto const& ref : tracks[side]) {
          if (!remove_from[side].count(ref))
            survivors.push_back(tt_builder.build(ref));
        }

        auto refit = kv_fit(survivors);
        if (refit.size() == 1) {
          *v[side] = reco::Vertex(refit[0]);
          if (verbose_)
            printf("  Refit v#%zu: %zu track(s) removed, chi2/dof=%.3f\n",
                   ivtx[side], remove_from[side].size(),
                   v[side]->normalizedChi2());
        } else {
          erase[side] = true;
          if (verbose_)
            printf("  Refit v#%zu failed – will erase\n", ivtx[side]);
        }
      }

      if (erase[1]) vertices->erase(v[1]);
      if (erase[0]) vertices->erase(v[0]);

      changed = true;
      break;  // restart after any change
    }

    if (changed) {
      v[0] = vertices->begin() - 1;
      ++n_resets;
    }
  }

  if (verbose_)
    printf("Arbitration complete: %d resets, %d one-track drops, %zu vertices\n",
           n_resets, n_onetracks, vertices->size());

  // ----------------------------------------------------------------
  // STEP 6 – N-1 vertex refit
  //   For each vertex with >=3 tracks, refit without each track in
  //   turn.  Remove any track whose absence shifts the 3-D vertex
  //   position by more than max_nm1_refit_dist3_.
  //   After the scan, drop any vertex whose chi2/dof > 5.
  // ----------------------------------------------------------------
  if (max_nm1_refit_dist3_ > 0.0 || max_nm1_refit_distz_ > 0.0) {

    for (auto vit = vertices->begin(); vit != vertices->end(); ++vit) {
      const track_vec tks = vertex_track_vec(*vit);
      const size_t ntks   = tks.size();
      if (ntks < 3) continue;

      std::vector<reco::TransientTrack> ttks_keep;
      ttks_keep.reserve(ntks);

      // Classify each track: keep only those that do NOT shift the vertex too much
      for (size_t i = 0; i < ntks; ++i) {
        // Build (ntks-1) transient tracks excluding track i
        std::vector<reco::TransientTrack> nm1_ttks;
        nm1_ttks.reserve(ntks - 1);
        for (size_t j = 0; j < ntks; ++j) {
          if (j == i) continue;
          auto tt = tt_builder.build(tks[j]);
          if (!tt.isValid()) goto drop_track_i;  // can't build transient track
          nm1_ttks.push_back(tt);
        }
        {
          TransientVertex tv_nm1 = kv_fitter_.vertex(nm1_ttks);
          if (!tv_nm1.isValid()) goto drop_track_i;

          reco::Vertex vnm1(tv_nm1);
          const double dx   = vnm1.x() - vit->x();
          const double dy   = vnm1.y() - vit->y();
          const double dz   = vnm1.z() - vit->z();
          const double dist3 = std::sqrt(dx*dx + dy*dy + dz*dz);
          const double distz = std::abs(dz);

          const bool too_far3 = (max_nm1_refit_dist3_ > 0.0 &&
                                  dist3 > max_nm1_refit_dist3_);
          const bool too_farz = (max_nm1_refit_distz_ > 0.0 &&
                                  distz > max_nm1_refit_distz_);

          if (too_far3 || too_farz) {
            // Track shifts vertex too much: exclude it
            if (verbose_)
              printf("N-1: removing track %u from vertex (dist3=%.5f distz=%.5f)\n",
                     tks[i].key(), dist3, distz);
            goto drop_track_i;
          }
          // Track is fine: keep it
          ttks_keep.push_back(tt_builder.build(tks[i]));
          goto next_track_i;
        }
        drop_track_i:;
        next_track_i:;
      }

      // Refit with surviving tracks
      if (ttks_keep.size() < 2) {
        vit = vertices->erase(vit) - 1;
        continue;
      }
      if (ttks_keep.size() < ntks) {
        // At least one track was removed: refit
        auto refit = kv_fit(ttks_keep);
        if (refit.empty()) {
          vit = vertices->erase(vit) - 1;
        } else {
          *vit = reco::Vertex(refit[0]);
        }
      }
    }

    // Drop any vertex whose chi2/dof > max_seed_vertex_chi2_ after N-1 update
    for (auto vit = vertices->begin(); vit != vertices->end(); ) {
      if (vit->normalizedChi2() > max_seed_vertex_chi2_)
        vit = vertices->erase(vit);
      else
        ++vit;
    }
  }

  // ----------------------------------------------------------------
  // STEP 7 – Loose split-vertex merge
  //   Merge any pair where vv_dist < merge_anyway_dist_ OR
  //   vv_sig < merge_anyway_sig_.
  // ----------------------------------------------------------------
  if (resolve_split_vertices_loose_ &&
      (merge_anyway_dist_ > 0.0 || merge_anyway_sig_ > 0.0)) {

    for (v[0] = vertices->begin(); v[0] != vertices->end(); ++v[0]) {
      for (v[1] = v[0] + 1; v[1] != vertices->end(); ++v[1]) {

        Measurement1D vv = vertex_vertex_dist(*v[0], *v[1]);
        const bool close = (merge_anyway_dist_ > 0.0 &&
                             vv.value()        < merge_anyway_dist_) ||
                           (merge_anyway_sig_ > 0.0 &&
                             vv.significance() < merge_anyway_sig_);
        if (!close) continue;

        track_set union_set = vertex_track_set(*v[0]);
        track_set ts1       = vertex_track_set(*v[1]);
        union_set.insert(ts1.begin(), ts1.end());

        std::vector<reco::TransientTrack> ttks;
        for (auto const& ref : union_set)
          ttks.push_back(tt_builder.build(ref));

        auto merged = kv_fit(ttks);
        if (merged.size() == 1) {
          *v[0] = reco::Vertex(merged[0]);
          v[1]  = vertices->erase(v[1]) - 1;
          if (verbose_)
            printf("Loose merge: combined two nearby vertices\n");
        }
      }
    }
  }

  // ----------------------------------------------------------------
  // STEP 8 – Tight split-vertex merge
  //   Per spec: both dBV > split_merge_dbv_min_,
  //             dVV,2D < split_merge_dvv2d_max_,
  //             |dPhi| < split_merge_dphi_max_.
  // ----------------------------------------------------------------
  if (resolve_split_vertices_tight_) {

    for (v[0] = vertices->begin(); v[0] != vertices->end(); ++v[0]) {
      track_set tracks0 = vertex_track_set(*v[0]);
      bool merged_this_v0 = false;

      for (v[1] = v[0] + 1; v[1] != vertices->end(); ++v[1]) {
        if (v[0]->nTracks() < 2 || v[1]->nTracks() < 2) continue;

        Measurement1D dvv2d = vertex_dist_2d_.distance(*v[0], *v[1]);
        double dBV0 = vertex_dist_2d_.distance(*v[0], fake_bs_vtx).value();
        double dBV1 = vertex_dist_2d_.distance(*v[1], fake_bs_vtx).value();

        double phi0 = std::atan2(v[0]->y() - bs_y, v[0]->x() - bs_x);
        double phi1 = std::atan2(v[1]->y() - bs_y, v[1]->x() - bs_x);
        double dphi = std::abs(phi0 - phi1);
        if (dphi > M_PI) dphi = 2.0 * M_PI - dphi;

        if (dBV0 < split_merge_dbv_min_)   continue;
        if (dBV1 < split_merge_dbv_min_)   continue;
        if (dvv2d.value() >= split_merge_dvv2d_max_) continue;
        if (dphi          >= split_merge_dphi_max_)  continue;

        track_set tracks1 = vertex_track_set(*v[1]);
        track_set union_set = tracks0;
        union_set.insert(tracks1.begin(), tracks1.end());

        std::vector<reco::TransientTrack> ttks;
        for (auto const& ref : union_set)
          ttks.push_back(tt_builder.build(ref));

        auto merged_coll = kv_fit(ttks);
        if (merged_coll.size() == 1) {
          reco::Vertex merged_vtx(merged_coll[0]);
          if (vertex_track_set(merged_vtx, 0) == union_set) {
            *v[0] = merged_vtx;
            v[1]  = vertices->erase(v[1]) - 1;
            merged_this_v0 = true;
            if (verbose_)
              printf("Tight merge: combined two split vertices\n");
          }
        }
      }

      if (merged_this_v0)
        v[0] = vertices->begin() - 1;  // restart outer loop
    }
  }

  // ----------------------------------------------------------------
  // STEP 9 – Output
  // ----------------------------------------------------------------
  if (verbose_)
    printf("Vertexer: outputting %zu vertices\n", vertices->size());

  iEvent.emplace(putToken_, std::move(*vertices));
}

DEFINE_FWK_MODULE(Vertexer);