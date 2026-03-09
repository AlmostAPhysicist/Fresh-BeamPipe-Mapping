// EventComparatorAnalyzer.cc
#include <memory>
#include <vector>
#include <string>
#include <cmath>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"

#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/BeamSpot/interface/BeamSpot.h"
#include "DataFormats/TrackReco/interface/TrackBase.h" // for TrackBaseRef

#include "RecoVertex/VertexTools/interface/VertexDistanceXY.h"

#include "TH1D.h"
#include "TH2D.h"

class EventComparatorAnalyzer :
  public edm::one::EDAnalyzer<edm::one::SharedResources>
{
public:
  explicit EventComparatorAnalyzer(const edm::ParameterSet&);
  ~EventComparatorAnalyzer() override = default;

private:
  void beginJob() override;
  void analyze(const edm::Event&, const edm::EventSetup&) override;

  // tokens
  edm::EDGetTokenT<std::vector<reco::Vertex>> vtxOfflineToken_;
  edm::EDGetTokenT<std::vector<reco::Vertex>> vtxScoutingToken_;

  edm::EDGetTokenT<std::vector<reco::Track>> trkOfflineToken_;
  edm::EDGetTokenT<std::vector<reco::Track>> trkScoutingToken_;

  edm::EDGetTokenT<reco::BeamSpot> beamspotToken_;

  // cuts / config
  int vertex_min_ntracks_;
  double vertex_max_chi2_;

  double track_pt_min_cut_;
  double track_dxySig_min_cut_;
  bool apply_hit_cuts_;
  int hit_minPixelHits_;
  int hit_minStripHits_;
  int hit_minTrackerLayers_;

  int stopAfterSelectedVertices_;
  bool hardStopOnLimit_;
  int selectedEventsSeen_;

  TFileDirectory eventCompareDir_;

  // Offline histograms
  TH1D* h_vtx_pt_off_ = nullptr;
  TH1D* h_vtx_mass_off_ = nullptr;
  TH1D* h_vtx_chi2_off_ = nullptr;
  TH1D* h_vtx_ntrk_off_ = nullptr;
  TH1D* h_vtx_dbverr_off_ = nullptr;
  TH2D* h_vtx_xy_off_ = nullptr;
  TH2D* h_beamspot_xy_off_ = nullptr;

  TH1D* h_trk_pt_off_ = nullptr;
  TH1D* h_trk_eta_off_ = nullptr;
  TH1D* h_trk_phi_off_ = nullptr;
  TH1D* h_trk_dxy_off_ = nullptr;
  TH1D* h_trk_dxyerr_off_ = nullptr;
  TH1D* h_trk_ipsig_off_ = nullptr;

  // Scouting histograms
  TH1D* h_vtx_pt_scout_ = nullptr;
  TH1D* h_vtx_mass_scout_ = nullptr;
  TH1D* h_vtx_chi2_scout_ = nullptr;
  TH1D* h_vtx_ntrk_scout_ = nullptr;
  TH1D* h_vtx_dbverr_scout_ = nullptr;
  TH2D* h_vtx_xy_scout_ = nullptr;
  TH2D* h_beamspot_xy_scout_ = nullptr;

  TH1D* h_trk_pt_scout_ = nullptr;
  TH1D* h_trk_eta_scout_ = nullptr;
  TH1D* h_trk_phi_scout_ = nullptr;
  TH1D* h_trk_dxy_scout_ = nullptr;
  TH1D* h_trk_dxyerr_scout_ = nullptr;
  TH1D* h_trk_ipsig_scout_ = nullptr;
};

EventComparatorAnalyzer::EventComparatorAnalyzer(const edm::ParameterSet& cfg)
{
  usesResource("TFileService");

  vtxOfflineToken_ =
    consumes<std::vector<reco::Vertex>>(cfg.getParameter<edm::InputTag>("displacedVerticesOffline"));

  vtxScoutingToken_ =
    consumes<std::vector<reco::Vertex>>(cfg.getParameter<edm::InputTag>("displacedVerticesScouting"));

  trkOfflineToken_ =
    consumes<std::vector<reco::Track>>(cfg.getParameter<edm::InputTag>("tracksOffline"));

  trkScoutingToken_ =
    consumes<std::vector<reco::Track>>(cfg.getParameter<edm::InputTag>("tracksScouting"));

  beamspotToken_ =
    consumes<reco::BeamSpot>(cfg.getParameter<edm::InputTag>("beamspot_src"));

  vertex_min_ntracks_ =
    cfg.getParameter<int>("vertex_min_ntracks");

  vertex_max_chi2_ =
    cfg.getParameter<double>("vertex_max_chi2");

  track_pt_min_cut_ = cfg.getUntrackedParameter<double>("track_pt_min_cut", 0.9);
  track_dxySig_min_cut_ = cfg.getUntrackedParameter<double>("track_dxySig_min_cut", 4.0);
  apply_hit_cuts_ = cfg.getUntrackedParameter<bool>("applyHitCuts", false);
  hit_minPixelHits_ = cfg.getUntrackedParameter<int>("hit_minPixelHits", 3);
  hit_minStripHits_ = cfg.getUntrackedParameter<int>("hit_minStripHits", 2);
  hit_minTrackerLayers_ = cfg.getUntrackedParameter<int>("hit_minTrackerLayers", 6);

  stopAfterSelectedVertices_ =
    cfg.getUntrackedParameter<int>("stopAfterSelectedVertices", -1);

  hardStopOnLimit_ =
    cfg.getUntrackedParameter<bool>("hardStopOnLimit", false);

  selectedEventsSeen_ = 0;
}

namespace {
  const double PION_MASS = 0.13957039; // GeV
}

void EventComparatorAnalyzer::beginJob() {
  edm::Service<TFileService> fs;
  eventCompareDir_ = fs->mkdir("EventCompare");

  TFileDirectory offlineDir = eventCompareDir_.mkdir("Offline");
  TFileDirectory scoutingDir = eventCompareDir_.mkdir("Scouting");

  TFileDirectory offVtxDir = offlineDir.mkdir("Vertices");
  TFileDirectory offTrkDir = offlineDir.mkdir("Tracks");
  TFileDirectory scoutVtxDir = scoutingDir.mkdir("Vertices");
  TFileDirectory scoutTrkDir = scoutingDir.mkdir("Tracks");

  h_vtx_pt_off_ = offVtxDir.make<TH1D>("pt", "Offline Vertex p_{T}; p_{T} [GeV]; entries", 100, 0, 50);
  h_vtx_mass_off_ = offVtxDir.make<TH1D>("mass", "Offline Vertex mass; m [GeV]; entries", 100, 0, 10);
  h_vtx_chi2_off_ = offVtxDir.make<TH1D>("chi2", "Offline Vertex norm chi2; chi2_{norm}; entries", 100, 0, 50);
  h_vtx_ntrk_off_ = offVtxDir.make<TH1D>("nTracks", "Offline Vertex nTracks; n_{tracks}; entries", 20, 0, 20);
  h_vtx_dbverr_off_ = offVtxDir.make<TH1D>("dBV_error", "Offline Vertex d_{BV} error; #sigma_{dBV} [cm]; entries", 200, 0, 0.2);
  h_vtx_xy_off_ = offVtxDir.make<TH2D>("xy", "Offline Vertex XY; x [cm]; y [cm]", 400, -10, 10, 400, -10, 10);
  h_beamspot_xy_off_ = offVtxDir.make<TH2D>("beamspot_xy", "Offline Beamspot XY; x_{BS} [cm]; y_{BS} [cm]", 400, -1, 1, 400, -1, 1);

  h_trk_pt_off_ = offTrkDir.make<TH1D>("pt", "Offline Track p_{T}; p_{T} [GeV]; entries", 100, 0, 50);
  h_trk_eta_off_ = offTrkDir.make<TH1D>("eta", "Offline Track #eta; #eta; entries", 100, -3, 3);
  h_trk_phi_off_ = offTrkDir.make<TH1D>("phi", "Offline Track #phi; #phi; entries", 100, -M_PI, M_PI);
  h_trk_dxy_off_ = offTrkDir.make<TH1D>("dxy", "Offline Track dxy; dxy [cm]; entries", 100, -1.0, 1.0);
  h_trk_dxyerr_off_ = offTrkDir.make<TH1D>("dxy_error", "Offline Track dxy error; #sigma_{dxy} [cm]; entries", 200, 0, 0.05);
  h_trk_ipsig_off_ = offTrkDir.make<TH1D>("ipsig", "Offline Track IPsig; |dxy|/err; entries", 100, 0, 50);

  h_vtx_pt_scout_ = scoutVtxDir.make<TH1D>("pt", "Scouting Vertex p_{T}; p_{T} [GeV]; entries", 100, 0, 50);
  h_vtx_mass_scout_ = scoutVtxDir.make<TH1D>("mass", "Scouting Vertex mass; m [GeV]; entries", 100, 0, 10);
  h_vtx_chi2_scout_ = scoutVtxDir.make<TH1D>("chi2", "Scouting Vertex norm chi2; chi2_{norm}; entries", 100, 0, 50);
  h_vtx_ntrk_scout_ = scoutVtxDir.make<TH1D>("nTracks", "Scouting Vertex nTracks; n_{tracks}; entries", 20, 0, 20);
  h_vtx_dbverr_scout_ = scoutVtxDir.make<TH1D>("dBV_error", "Scouting Vertex d_{BV} error; #sigma_{dBV} [cm]; entries", 200, 0, 0.2);
  h_vtx_xy_scout_ = scoutVtxDir.make<TH2D>("xy", "Scouting Vertex XY; x [cm]; y [cm]", 400, -10, 10, 400, -10, 10);
  h_beamspot_xy_scout_ = scoutVtxDir.make<TH2D>("beamspot_xy", "Scouting Beamspot XY; x_{BS} [cm]; y_{BS} [cm]", 400, -1, 1, 400, -1, 1);

  h_trk_pt_scout_ = scoutTrkDir.make<TH1D>("pt", "Scouting Track p_{T}; p_{T} [GeV]; entries", 100, 0, 50);
  h_trk_eta_scout_ = scoutTrkDir.make<TH1D>("eta", "Scouting Track #eta; #eta; entries", 100, -3, 3);
  h_trk_phi_scout_ = scoutTrkDir.make<TH1D>("phi", "Scouting Track #phi; #phi; entries", 100, -M_PI, M_PI);
  h_trk_dxy_scout_ = scoutTrkDir.make<TH1D>("dxy", "Scouting Track dxy; dxy [cm]; entries", 100, -1.0, 1.0);
  h_trk_dxyerr_scout_ = scoutTrkDir.make<TH1D>("dxy_error", "Scouting Track dxy error; #sigma_{dxy} [cm]; entries", 200, 0, 0.05);
  h_trk_ipsig_scout_ = scoutTrkDir.make<TH1D>("ipsig", "Scouting Track IPsig; |dxy|/err; entries", 100, 0, 50);
}

void EventComparatorAnalyzer::analyze(
  const edm::Event& event,
  const edm::EventSetup&)
{
  // fetch beamspot (for dxy computation if available)
  edm::Handle<reco::BeamSpot> beamspotH;
  event.getByToken(beamspotToken_, beamspotH);
  const reco::BeamSpot *bs = beamspotH.isValid() ? &*beamspotH : nullptr;
  VertexDistanceXY vertexDist2D;

  reco::Vertex::Error refErr;
  for (int i = 0; i < 3; ++i) {
    for (int j = i; j < 3; ++j) {
      refErr(i, j) = 0.0;
    }
  }
  reco::Vertex refVtx;
  if (bs) {
    refErr(0, 0) = bs->covariance()(0, 0);
    refErr(1, 1) = bs->covariance()(1, 1);
    refErr(2, 2) = bs->covariance()(2, 2);
    refVtx = reco::Vertex(bs->position(), refErr);
    h_beamspot_xy_off_->Fill(bs->position().x(), bs->position().y());
    h_beamspot_xy_scout_->Fill(bs->position().x(), bs->position().y());
  } else {
    refErr(0, 0) = 1e-6;
    refErr(1, 1) = 1e-6;
    refErr(2, 2) = 1e-6;
    reco::Vertex::Point origin(0.0, 0.0, 0.0);
    refVtx = reco::Vertex(origin, refErr);
  }

  edm::Handle<std::vector<reco::Vertex>> vtxOff;
  event.getByToken(vtxOfflineToken_,vtxOff);

  edm::Handle<std::vector<reco::Vertex>> vtxScout;
  event.getByToken(vtxScoutingToken_,vtxScout);

  edm::Handle<std::vector<reco::Track>> trkOff;
  event.getByToken(trkOfflineToken_,trkOff);

  edm::Handle<std::vector<reco::Track>> trkScout;
  event.getByToken(trkScoutingToken_,trkScout);

  // quick early exit if nothing relevant
  if(!vtxOff.isValid() && !vtxScout.isValid()) return;
  if(!trkOff.isValid() && !trkScout.isValid()) return;

  if(stopAfterSelectedVertices_ > 0 && selectedEventsSeen_ >= stopAfterSelectedVertices_){
    return;
  }
  selectedEventsSeen_++;

  // decide whether vertex passes basic vertex selection
  auto passVertex = [&](const reco::Vertex &v) -> bool {
    int nt = std::distance(v.tracks_begin(), v.tracks_end());
    if(nt < vertex_min_ntracks_) return false;
    if(vertex_max_chi2_ > 0 && v.normalizedChi2() > vertex_max_chi2_) return false;
    return true;
  };

  // ---------- Fill vertex histograms for offline ----------
  auto processVertexCollection = [&](const std::vector<reco::Vertex> *vtxs, bool isOffline) {
    if(!vtxs) return;
    for(const auto &v : *vtxs){
      if(!passVertex(v)) continue;

      // number of tracks
      int ntrack = std::distance(v.tracks_begin(), v.tracks_end());

      // Attempt to compute sum pT and invariant mass from track refs (if present)
      double sum_px=0, sum_py=0, sum_pz=0, sum_e=0;
      for(auto it = v.tracks_begin(); it != v.tracks_end(); ++it){
        reco::TrackBaseRef baseRef = *it;
        reco::TrackRef tr = baseRef.castTo<reco::TrackRef>();
        if(tr.isNonnull()){
          const reco::Track &t = *tr;
          // apply analyzer-level track quality cuts before using in vertex-level sums
          if(t.pt() < track_pt_min_cut_) continue;
          double dxy = t.d0();
          double dxyerr = (t.d0Error()>0) ? t.d0Error() : 1e-6;
          double dxySig = std::abs(dxy)/dxyerr;
          if(dxySig < track_dxySig_min_cut_) continue;

          // optional hit cuts (best-effort: Track has numberOfValidHits(), but not pixel/strip split in reco::Track)
          if(apply_hit_cuts_){
            if(t.hitPattern().numberOfValidPixelHits() < hit_minPixelHits_) continue;
            if(t.hitPattern().numberOfValidStripHits() < hit_minStripHits_) continue;
            if(t.hitPattern().trackerLayersWithMeasurement() < hit_minTrackerLayers_) continue;
          }

          double pt = t.pt();
          double phi = t.phi();
          double eta = t.eta();
          double px = pt * std::cos(phi);
          double py = pt * std::sin(phi);
          double pz = pt * std::sinh(eta);
          double p2 = px*px + py*py + pz*pz;
          double e = std::sqrt(p2 + PION_MASS*PION_MASS);

          sum_px += px;
          sum_py += py;
          sum_pz += pz;
          sum_e  += e;
        }
      } // end tracks of vertex

      double vtx_pt = std::sqrt(sum_px*sum_px + sum_py*sum_py);
      double mass2 = sum_e*sum_e - (sum_px*sum_px + sum_py*sum_py + sum_pz*sum_pz);
      double vtx_mass = (mass2 > 0) ? std::sqrt(mass2) : 0.0;

      if(isOffline){
        Measurement1D dBV = vertexDist2D.distance(v, refVtx);
        h_vtx_pt_off_->Fill(vtx_pt);
        h_vtx_mass_off_->Fill(vtx_mass);
        h_vtx_chi2_off_->Fill(v.normalizedChi2());
        h_vtx_ntrk_off_->Fill(ntrack);
        h_vtx_dbverr_off_->Fill(dBV.error());
        h_vtx_xy_off_->Fill(v.x(), v.y());
      } else {
        Measurement1D dBV = vertexDist2D.distance(v, refVtx);
        h_vtx_pt_scout_->Fill(vtx_pt);
        h_vtx_mass_scout_->Fill(vtx_mass);
        h_vtx_chi2_scout_->Fill(v.normalizedChi2());
        h_vtx_ntrk_scout_->Fill(ntrack);
        h_vtx_dbverr_scout_->Fill(dBV.error());
        h_vtx_xy_scout_->Fill(v.x(), v.y());
      }
    }
  };

  processVertexCollection(vtxOff.isValid() ? &*vtxOff : nullptr, true);
  processVertexCollection(vtxScout.isValid() ? &*vtxScout : nullptr, false);

  // ---------- Fill track histograms ----------
  auto processTrackCollection = [&](const std::vector<reco::Track> *trks, bool isOffline) {
    if(!trks) return;
    for(const auto &t : *trks){
      // analyzer-level track quality
      if(t.pt() < track_pt_min_cut_) continue;
      double dxy = t.d0();
      double dxyerr = (t.d0Error()>0) ? t.d0Error() : 1e-6;
      double dxySig = std::abs(dxy)/dxyerr;
      if(dxySig < track_dxySig_min_cut_) continue;

      if(apply_hit_cuts_){
        if(t.hitPattern().numberOfValidPixelHits() < hit_minPixelHits_) continue;
        if(t.hitPattern().numberOfValidStripHits() < hit_minStripHits_) continue;
        if(t.hitPattern().trackerLayersWithMeasurement() < hit_minTrackerLayers_) continue;
      }

      if(isOffline){
        h_trk_pt_off_->Fill(t.pt());
        h_trk_eta_off_->Fill(t.eta());
        h_trk_phi_off_->Fill(t.phi());
        if(bs) h_trk_dxy_off_->Fill(t.d0()); else h_trk_dxy_off_->Fill(t.d0());
        h_trk_dxyerr_off_->Fill(dxyerr);
        h_trk_ipsig_off_->Fill(std::abs(dxySig));
      } else {
        h_trk_pt_scout_->Fill(t.pt());
        h_trk_eta_scout_->Fill(t.eta());
        h_trk_phi_scout_->Fill(t.phi());
        if(bs) h_trk_dxy_scout_->Fill(t.d0()); else h_trk_dxy_scout_->Fill(t.d0());
        h_trk_dxyerr_scout_->Fill(dxyerr);
        h_trk_ipsig_scout_->Fill(std::abs(dxySig));
      }
    }
  };

  processTrackCollection(trkOff.isValid() ? &*trkOff : nullptr, true);
  processTrackCollection(trkScout.isValid() ? &*trkScout : nullptr, false);

  // optionally throw to stop job early (user wanted this behavior)
  if(hardStopOnLimit_ && stopAfterSelectedVertices_ > 0 && selectedEventsSeen_ >= stopAfterSelectedVertices_){
    throw cms::Exception("StopJob") << "Reached requested number of selected events";
  }
}

DEFINE_FWK_MODULE(EventComparatorAnalyzer);