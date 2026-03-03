// EventComparator2Analyzer.cc
// Compare offline (packedCandidateToTrack -> VertexerOffline) vs scouting (hltScoutingUnpackProducer -> Vertexer)
//
// Writes histograms under two folders: "Offline" and "Scouting" and creates overlay canvases at endJob().

#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
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
#include "DataFormats/BeamSpot/interface/BeamSpot.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"

#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TLorentzVector.h"

class EventComparator2Analyzer : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit EventComparator2Analyzer(const edm::ParameterSet &);
  ~EventComparator2Analyzer() override = default;

  static void fillDescriptions(edm::ConfigurationDescriptions &descriptions);

private:
  void beginJob() override;
  void analyze(const edm::Event &, const edm::EventSetup &) override;
  void endJob() override;

  // Config parameters (InputTags)
  const edm::InputTag displacedVerticesOfflineTag_;
  const edm::InputTag displacedVerticesScoutingTag_;
  const edm::InputTag tracksOfflineTag_;
  const edm::InputTag tracksScoutingTag_;
  const edm::InputTag beamspotTag_;

  // consumes tokens
  edm::EDGetTokenT<std::vector<reco::Vertex>> vtxOfflineToken_;
  edm::EDGetTokenT<std::vector<reco::Vertex>> vtxScoutingToken_;
  edm::EDGetTokenT<std::vector<reco::Track>>  trkOfflineToken_;
  edm::EDGetTokenT<std::vector<reco::Track>>  trkScoutingToken_;
  edm::EDGetTokenT<reco::BeamSpot>            beamspotToken_;

  // Selection parameters
  const int vertex_min_ntracks_;
  const double vertex_max_chi2_;
  const int nEventsToSave_; // optional per-event saving not used heavily, but kept

  // bookkeeping
  int savedEvents_;

  // TFileService-owned histograms (two groups)
  struct VtxHistos {
    TH1F *pt= nullptr, *eta = nullptr, *phi=nullptr, *mass=nullptr;
    TH1F *ntracks=nullptr, *chi2=nullptr, *dBV=nullptr, *dBVerr=nullptr;
  };
  struct TrkHistos {
    TH1F *pt=nullptr, *eta=nullptr, *phi=nullptr, *p=nullptr;
    TH1F *npix=nullptr, *nstrip=nullptr, *nlayers=nullptr;
    TH1F *dxy=nullptr, *dxySig=nullptr, *dxyErr=nullptr;
  };

  VtxHistos v_off_, v_scout_;
  TrkHistos t_off_, t_scout_;

  // helper to create overlay canvases at endJob
  void makeOverlayCanvas(TH1F* h_off, TH1F* h_scout, const std::string &cname, const std::string &title);
};

//
// Implementation
//

EventComparator2Analyzer::EventComparator2Analyzer(const edm::ParameterSet &iConfig) :
  displacedVerticesOfflineTag_( iConfig.getParameter<edm::InputTag>("displacedVerticesOffline") ),
  displacedVerticesScoutingTag_( iConfig.getParameter<edm::InputTag>("displacedVerticesScouting") ),
  tracksOfflineTag_( iConfig.getParameter<edm::InputTag>("tracksOffline") ),
  tracksScoutingTag_( iConfig.getParameter<edm::InputTag>("tracksScouting") ),
  beamspotTag_( iConfig.getParameter<edm::InputTag>("beamspot_src") ),
  vertex_min_ntracks_( iConfig.getParameter<int>("vertex_min_ntracks") ),
  vertex_max_chi2_( iConfig.getParameter<double>("vertex_max_chi2") ),
  nEventsToSave_( iConfig.getParameter<int>("nEventsToSave") ),
  savedEvents_(0)
{
  usesResource("TFileService");

  // register consumes tokens
  vtxOfflineToken_  = consumes<std::vector<reco::Vertex>>(displacedVerticesOfflineTag_);
  vtxScoutingToken_ = consumes<std::vector<reco::Vertex>>(displacedVerticesScoutingTag_);
  trkOfflineToken_  = consumes<std::vector<reco::Track>>(tracksOfflineTag_);
  trkScoutingToken_ = consumes<std::vector<reco::Track>>(tracksScoutingTag_);
  beamspotToken_    = consumes<reco::BeamSpot>(beamspotTag_);
}

void EventComparator2Analyzer::beginJob() {
  edm::Service<TFileService> fs;

  // Make directories and histos for Offline
  TFileDirectory offDir = fs->mkdir("Offline");
  v_off_.pt   = offDir.make<TH1F>("vtx_pt_off","vertex p_{T} (offline); p_{T} [GeV]; Vertices",100,0,100);
  v_off_.eta  = offDir.make<TH1F>("vtx_eta_off","vertex #eta (offline); #eta; Vertices",100,-5,5);
  v_off_.phi  = offDir.make<TH1F>("vtx_phi_off","vertex #phi (offline); #phi; Vertices",100,-3.15,3.15);
  v_off_.mass = offDir.make<TH1F>("vtx_mass_off","vertex mass (offline); mass [GeV]; Vertices",100,0,10);
  v_off_.ntracks = offDir.make<TH1F>("vtx_ntracks_off","vertex ntracks (offline); ntracks; Vertices",50,0,50);
  v_off_.chi2 = offDir.make<TH1F>("vtx_chi2_off","#chi^{2}/ndof (offline); #chi^{2}/ndof; Vertices",200,0,20);
  v_off_.dBV = offDir.make<TH1F>("vtx_dBV_off","d_{BV} (offline); d_{BV} [cm]; Vertices",200,0,10);
  v_off_.dBVerr = offDir.make<TH1F>("vtx_dBVerr_off","d_{BV} error (offline); #sigma_{dBV} [cm]; Vertices",200,0,0.1);

  TFileDirectory troff = offDir.mkdir("Tracks");
  t_off_.pt = troff.make<TH1F>("trk_pt_off","track p_{T} (offline); p_{T} [GeV]; Tracks",100,0,100);
  t_off_.eta = troff.make<TH1F>("trk_eta_off","track #eta (offline); #eta; Tracks",100,-5,5);
  t_off_.phi = troff.make<TH1F>("trk_phi_off","track #phi (offline); #phi; Tracks",100,-3.15,3.15);
  t_off_.p = troff.make<TH1F>("trk_p_off","track p (offline); p [GeV]; Tracks",100,0,200);
  t_off_.npix = troff.make<TH1F>("trk_npix_off","track nPixelHits (offline); npix; Tracks",10,0,10);
  t_off_.nstrip = troff.make<TH1F>("trk_nstrip_off","track nStripHits (offline); nstrip; Tracks",30,0,30);
  t_off_.nlayers = troff.make<TH1F>("trk_nlayers_off","track nTrackerLayers (offline); nlayers; Tracks",30,0,30);
  t_off_.dxy = troff.make<TH1F>("trk_dxy_off","track dxy (offline); dxy [cm]; Tracks",200,-2.0,2.0);
  t_off_.dxySig = troff.make<TH1F>("trk_dxySig_off","track dxySig (offline); |dxy|/#sigma; Tracks",100,0,50);
  t_off_.dxyErr = troff.make<TH1F>("trk_dxyErr_off","track dxyErr (offline); #sigma_{dxy} [cm]; Tracks",200,0,0.05);

  // Make directories and histos for Scouting
  TFileDirectory scDir = fs->mkdir("Scouting");
  v_scout_.pt   = scDir.make<TH1F>("vtx_pt_scout","vertex p_{T} (scouting); p_{T} [GeV]; Vertices",100,0,100);
  v_scout_.eta  = scDir.make<TH1F>("vtx_eta_scout","vertex #eta (scouting); #eta; Vertices",100,-5,5);
  v_scout_.phi  = scDir.make<TH1F>("vtx_phi_scout","vertex #phi (scouting); #phi; Vertices",100,-3.15,3.15);
  v_scout_.mass = scDir.make<TH1F>("vtx_mass_scout","vertex mass (scouting); mass [GeV]; Vertices",100,0,10);
  v_scout_.ntracks = scDir.make<TH1F>("vtx_ntracks_scout","vertex ntracks (scouting); ntracks; Vertices",50,0,50);
  v_scout_.chi2 = scDir.make<TH1F>("vtx_chi2_scout","#chi^{2}/ndof (scouting); #chi^{2}/ndof; Vertices",200,0,20);
  v_scout_.dBV = scDir.make<TH1F>("vtx_dBV_scout","d_{BV} (scouting); d_{BV} [cm]; Vertices",200,0,10);
  v_scout_.dBVerr = scDir.make<TH1F>("vtx_dBVerr_scout","d_{BV} error (scouting); #sigma_{dBV} [cm]; Vertices",200,0,0.1);

  TFileDirectory trsc = scDir.mkdir("Tracks");
  t_scout_.pt = trsc.make<TH1F>("trk_pt_scout","track p_{T} (scouting); p_{T} [GeV]; Tracks",100,0,100);
  t_scout_.eta = trsc.make<TH1F>("trk_eta_scout","track #eta (scouting); #eta; Tracks",100,-5,5);
  t_scout_.phi = trsc.make<TH1F>("trk_phi_scout","track #phi (scouting); #phi; Tracks",100,-3.15,3.15);
  t_scout_.p = trsc.make<TH1F>("trk_p_scout","track p (scouting); p [GeV]; Tracks",100,0,200);
  t_scout_.npix = trsc.make<TH1F>("trk_npix_scout","track nPixelHits (scouting); npix; Tracks",10,0,10);
  t_scout_.nstrip = trsc.make<TH1F>("trk_nstrip_scout","track nStripHits (scouting); nstrip; Tracks",30,0,30);
  t_scout_.nlayers = trsc.make<TH1F>("trk_nlayers_scout","track nTrackerLayers (scouting); nlayers; Tracks",30,0,30);
  t_scout_.dxy = trsc.make<TH1F>("trk_dxy_scout","track dxy (scouting); dxy [cm]; Tracks",200,-2.0,2.0);
  t_scout_.dxySig = trsc.make<TH1F>("trk_dxySig_scout","track dxySig (scouting); |dxy|/#sigma; Tracks",100,0,50);
  t_scout_.dxyErr = trsc.make<TH1F>("trk_dxyErr_scout","track dxyErr (scouting); #sigma_{dxy} [cm]; Tracks",200,0,0.05);
}

void EventComparator2Analyzer::analyze(const edm::Event &iEvent, const edm::EventSetup &iSetup) {
  using namespace edm;
  using namespace reco;

  // Read offline vertices and tracks
  Handle<std::vector<Vertex>> vtxOffH;
  iEvent.getByToken(vtxOfflineToken_, vtxOffH);
  bool haveOffV = vtxOffH.isValid() && !vtxOffH->empty();

  Handle<std::vector<Track>> trkOffH;
  iEvent.getByToken(trkOfflineToken_, trkOffH);
  bool haveOffTrk = trkOffH.isValid();

  // Read scouting vertices and tracks
  Handle<std::vector<Vertex>> vtxScoutH;
  iEvent.getByToken(vtxScoutingToken_, vtxScoutH);
  bool haveScoutV = vtxScoutH.isValid() && !vtxScoutH->empty();

  Handle<std::vector<Track>> trkScoutH;
  iEvent.getByToken(trkScoutingToken_, trkScoutH);
  bool haveScoutTrk = trkScoutH.isValid();

  math::XYZPoint refPoint(0., 0., 0.);
  Handle<reco::BeamSpot> beamspotH;
  iEvent.getByToken(beamspotToken_, beamspotH);
  if (beamspotH.isValid()) {
    refPoint = beamspotH->position();
  }

  // quick vertex predicate
  auto vertexPasses = [&](const Vertex &v)->bool {
    const size_t ntracks = std::distance(v.tracks_begin(), v.tracks_end());
    if ((int)ntracks < vertex_min_ntracks_) return false;
    if (vertex_max_chi2_ >= 0.0 && v.normalizedChi2() > vertex_max_chi2_) return false;
    return true;
  };

  // Fill offline histograms from the first selected offline vertex (if present)
  if (haveOffV) {
    for (const auto &v : *vtxOffH) {
      if (!vertexPasses(v)) continue;

      // vertex-level fills
      TLorentzVector sumVec(0,0,0,0);
      for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
        TrackRef tr = it->castTo<TrackRef>();
        if (!tr.isNonnull()) continue;
        constexpr double kPionMass = 0.13957;
        TLorentzVector tv; tv.SetPtEtaPhiM(tr->pt(), tr->eta(), tr->phi(), kPionMass);
        sumVec += tv;
      }
      v_off_.pt->Fill(sumVec.Pt());
      v_off_.eta->Fill(sumVec.Eta());
      v_off_.phi->Fill(sumVec.Phi());
      v_off_.mass->Fill(sumVec.M());
      v_off_.ntracks->Fill(std::distance(v.tracks_begin(), v.tracks_end()));
      v_off_.chi2->Fill(v.normalizedChi2());
      double dBV = std::hypot(v.x(), v.y());
      v_off_.dBV->Fill(dBV);
      v_off_.dBVerr->Fill(0.0); // no direct vertex dBV error available here

      // track-level fills (from tracks referenced by vertex)
      for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
        TrackRef tr = it->castTo<TrackRef>();
        if (!tr.isNonnull()) continue;
        t_off_.pt->Fill(tr->pt());
        t_off_.eta->Fill(tr->eta());
        t_off_.phi->Fill(tr->phi());
        t_off_.p->Fill(tr->p());
        // robust accessors; some releases use hitPattern(), some have found()
        int npix = 0, nstrip = 0, nlayers = 0;
        try {
          npix = tr->hitPattern().numberOfValidPixelHits();
          nstrip = tr->hitPattern().numberOfValidStripHits();
        } catch (...) { npix = 0; nstrip = 0; }
        try { nlayers = tr->found() ? tr->found() : 0; } catch(...) { nlayers = 0; }
        t_off_.npix->Fill(npix);
        t_off_.nstrip->Fill(nstrip);
        t_off_.nlayers->Fill(nlayers);
        const double dxyRef = tr->dxy(refPoint);
        t_off_.dxy->Fill(dxyRef);
        if (tr->dxyError() > 0) t_off_.dxySig->Fill(std::fabs(dxyRef/tr->dxyError()));
        t_off_.dxyErr->Fill(tr->dxyError());
      }
      break; // only use the first selected offline vertex for event-level compare
    }
  }

  // Fill scouting histograms from the first selected scouting vertex (if present)
  if (haveScoutV) {
    for (const auto &v : *vtxScoutH) {
      if (!vertexPasses(v)) continue;

      TLorentzVector sumVec(0,0,0,0);
      for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
        TrackRef tr = it->castTo<TrackRef>();
        if (!tr.isNonnull()) continue;
        constexpr double kPionMass = 0.13957;
        TLorentzVector tv; tv.SetPtEtaPhiM(tr->pt(), tr->eta(), tr->phi(), kPionMass);
        sumVec += tv;
      }
      v_scout_.pt->Fill(sumVec.Pt());
      v_scout_.eta->Fill(sumVec.Eta());
      v_scout_.phi->Fill(sumVec.Phi());
      v_scout_.mass->Fill(sumVec.M());
      v_scout_.ntracks->Fill(std::distance(v.tracks_begin(), v.tracks_end()));
      v_scout_.chi2->Fill(v.normalizedChi2());
      double dBV = std::hypot(v.x(), v.y());
      v_scout_.dBV->Fill(dBV);
      v_scout_.dBVerr->Fill(0.0);

      // track-level fills
      for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
        TrackRef tr = it->castTo<TrackRef>();
        if (!tr.isNonnull()) continue;
        t_scout_.pt->Fill(tr->pt());
        t_scout_.eta->Fill(tr->eta());
        t_scout_.phi->Fill(tr->phi());
        t_scout_.p->Fill(tr->p());
        int npix = 0, nstrip = 0, nlayers = 0;
        try {
          npix = tr->hitPattern().numberOfValidPixelHits();
          nstrip = tr->hitPattern().numberOfValidStripHits();
        } catch (...) { npix = 0; nstrip = 0; }
        try { nlayers = tr->found() ? tr->found() : 0; } catch(...) { nlayers = 0; }
        t_scout_.npix->Fill(npix);
        t_scout_.nstrip->Fill(nstrip);
        t_scout_.nlayers->Fill(nlayers);
        const double dxyRef = tr->dxy(refPoint);
        t_scout_.dxy->Fill(dxyRef);
        if (tr->dxyError() > 0) t_scout_.dxySig->Fill(std::fabs(dxyRef/tr->dxyError()));
        t_scout_.dxyErr->Fill(tr->dxyError());
      }
      break; // only first selected scouting vertex
    }
  }

  // optional per-event saving count
  if ( (haveOffV || haveScoutV) && savedEvents_ < nEventsToSave_ ) {
    ++savedEvents_;
  }
}

void EventComparator2Analyzer::endJob() {
  // create overlay canvases (offline vs scouting) and write them into the output ROOT file
  makeOverlayCanvas(v_off_.pt, v_scout_.pt, "overlay_vtx_pt", "Vertex p_{T} overlay");
  makeOverlayCanvas(v_off_.mass, v_scout_.mass, "overlay_vtx_mass", "Vertex mass overlay");
  makeOverlayCanvas(v_off_.chi2, v_scout_.chi2, "overlay_vtx_chi2", "Vertex #chi^{2}/ndof overlay");
  makeOverlayCanvas(v_off_.ntracks, v_scout_.ntracks, "overlay_vtx_ntracks", "Vertex N_{tracks} overlay");
  makeOverlayCanvas(t_off_.pt, t_scout_.pt, "overlay_trk_pt", "Track p_{T} overlay");
  makeOverlayCanvas(t_off_.eta, t_scout_.eta, "overlay_trk_eta", "Track #eta overlay");
  makeOverlayCanvas(t_off_.dxy, t_scout_.dxy, "overlay_trk_dxy", "Track dxy overlay");
}

void EventComparator2Analyzer::makeOverlayCanvas(TH1F* h_off, TH1F* h_scout, const std::string &cname, const std::string &title) {
  if (!h_off || !h_scout) return;
  TCanvas *c = new TCanvas(cname.c_str(), title.c_str(), 800, 600);
  h_off->SetLineColor(kBlue);
  h_off->SetLineWidth(2);
  h_scout->SetLineColor(kRed);
  h_scout->SetLineWidth(2);
  h_off->Draw();
  h_scout->Draw("SAME");
  TLegend leg(0.70, 0.75, 0.90, 0.90);
  leg.AddEntry(h_off, "Offline", "l");
  leg.AddEntry(h_scout, "Scouting", "l");
  leg.Draw();
  c->Write();
  delete c;
}

void EventComparator2Analyzer::fillDescriptions(edm::ConfigurationDescriptions &descriptions) {
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("displacedVerticesOffline", edm::InputTag("VertexerOffline", ""));
  desc.add<edm::InputTag>("displacedVerticesScouting", edm::InputTag("Vertexer", ""));
  desc.add<edm::InputTag>("tracksOffline", edm::InputTag("packedCandidateToTrack","Track"));
  desc.add<edm::InputTag>("tracksScouting", edm::InputTag("hltScoutingUnpackProducer","Track"));
  desc.add<edm::InputTag>("beamspot_src", edm::InputTag("offlineBeamSpot"));
  desc.add<int>("nEventsToSave", 5);
  desc.add<int>("vertex_min_ntracks", 2);
  desc.add<double>("vertex_max_chi2", -1.0);
  descriptions.add("eventComparator2", desc);
}

DEFINE_FWK_MODULE(EventComparator2Analyzer);