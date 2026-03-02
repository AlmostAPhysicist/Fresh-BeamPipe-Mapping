// EventComparatorAnalyzer.cc
// EDAnalyzer: creates per-event directories "Event_1", "Event_2", ...
// Each event folder contains Vertex/ and Track/ subfolders with
// offline vs scouting histograms and overlayed canvases (with legends).
//
// Usage: add module to your CMSSW config and set InputTags appropriately.

#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

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

#include "TFileDirectory.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "TLegend.h"

class EventComparatorAnalyzer : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit EventComparatorAnalyzer(const edm::ParameterSet &);
  ~EventComparatorAnalyzer() override = default;

  static void fillDescriptions(edm::ConfigurationDescriptions &descriptions);

private:
  void beginJob() override {}
  void analyze(const edm::Event &, const edm::EventSetup &) override;
  void endJob() override {}

  // Config
  const edm::InputTag displacedVerticesOfflineTag_;
  const edm::InputTag displacedVerticesScoutingTag_;
  const edm::InputTag tracksOfflineTag_;   // optional: may not exist in scouting MINIAOD
  const edm::InputTag tracksScoutingTag_;
  const int nEventsToSave_;
  const int vertex_min_ntracks_;
  const double vertex_max_chi2_;

  // bookkeeping
  int savedEvents_;
  edm::Service<TFileService> fs_;
};

EventComparatorAnalyzer::EventComparatorAnalyzer(const edm::ParameterSet &iConfig) :
  displacedVerticesOfflineTag_(iConfig.getParameter<edm::InputTag>("displacedVerticesOffline")),
  displacedVerticesScoutingTag_(iConfig.getParameter<edm::InputTag>("displacedVerticesScouting")),
  tracksOfflineTag_(iConfig.getParameter<edm::InputTag>("tracksOffline")),
  tracksScoutingTag_(iConfig.getParameter<edm::InputTag>("tracksScouting")),
  nEventsToSave_(iConfig.getParameter<int>("nEventsToSave")),
  vertex_min_ntracks_(iConfig.getParameter<int>("vertex_min_ntracks")),
  vertex_max_chi2_(iConfig.getParameter<double>("vertex_max_chi2")),
  savedEvents_(0)
{
  usesResource("TFileService");
}

void EventComparatorAnalyzer::analyze(const edm::Event &iEvent, const edm::EventSetup &iSetup) {
  using namespace edm;
  using namespace reco;

  if (savedEvents_ >= nEventsToSave_) return;

  // Get vertex collections (both offline and scouting)
  Handle<std::vector<Vertex>> vtxOfflineH;
  iEvent.getByLabel(displacedVerticesOfflineTag_, vtxOfflineH);
  const bool haveOffline = vtxOfflineH.isValid() && !vtxOfflineH->empty();

  Handle<std::vector<Vertex>> vtxScoutingH;
  iEvent.getByLabel(displacedVerticesScoutingTag_, vtxScoutingH);
  const bool haveScouting = vtxScoutingH.isValid() && !vtxScoutingH->empty();

  // Quick selection predicate: does either collection have >=1 vertex passing selection?
  auto vertexPasses = [&](const Vertex &v)->bool {
    // simple selection: min tracks and chi2 (user-tunable)
    const size_t ntracks = std::distance(v.tracks_begin(), v.tracks_end());
    if ((int)ntracks < vertex_min_ntracks_) return false;
    if (vertex_max_chi2_ >= 0.0 && v.normalizedChi2() > vertex_max_chi2_) return false;
    return true;
  };

  bool event_has_selected_vertex = false;
  if (haveOffline) {
    for (const auto &v : *vtxOfflineH) {
      if (vertexPasses(v)) { event_has_selected_vertex = true; break; }
    }
  }
  if (!event_has_selected_vertex && haveScouting) {
    for (const auto &v : *vtxScoutingH) {
      if (vertexPasses(v)) { event_has_selected_vertex = true; break; }
    }
  }

  if (!event_has_selected_vertex) return; // skip: no selected vertex in either collection

  // We will record this event
  ++savedEvents_;
  std::ostringstream evname; evname << "Event_" << savedEvents_;
  TFileDirectory evDir = fs_->mkdir(evname.str());

  // Create Vertex and Track subfolders
  TFileDirectory vDir = evDir.mkdir("Vertex");
  TFileDirectory tDir = evDir.mkdir("Track");

  // Histogram definitions (one hist for offline, one for scouting) — matching bins
  // Vertex histograms
  TH1F* h_vtx_xy_off_x = vDir.make<TH1F>("vtx_x_off", "vertex x (offline); x [cm]; entries", 200, -5.0, 5.0);
  TH1F* h_vtx_xy_off_y = vDir.make<TH1F>("vtx_y_off", "vertex y (offline); y [cm]; entries", 200, -5.0, 5.0);
  TH1F* h_vtx_xy_scout_x = vDir.make<TH1F>("vtx_x_scout", "vertex x (scouting); x [cm]; entries", 200, -5.0, 5.0);
  TH1F* h_vtx_xy_scout_y = vDir.make<TH1F>("vtx_y_scout", "vertex y (scouting); y [cm]; entries", 200, -5.0, 5.0);

  TH1F* h_vtx_dBV_off = vDir.make<TH1F>("vtx_dBV_off", "d_{BV} (offline); d_{BV} [cm]; Vertices", 200, 0.0, 10.0);
  TH1F* h_vtx_dBV_scout = vDir.make<TH1F>("vtx_dBV_scout", "d_{BV} (scouting); d_{BV} [cm]; Vertices", 200, 0.0, 10.0);

  TH1F* h_vtx_dBVerr_off = vDir.make<TH1F>("vtx_dBVerr_off", "d_{BV} error (offline); #sigma_{dBV} [cm]; Vertices", 200, 0.0, 0.1);
  TH1F* h_vtx_dBVerr_scout = vDir.make<TH1F>("vtx_dBVerr_scout", "d_{BV} error (scouting); #sigma_{dBV} [cm]; Vertices", 200, 0.0, 0.1);

  TH1F* h_vtx_chi2norm_off = vDir.make<TH1F>("vtx_chi2_off", "#chi^{2}/ndof (offline); #chi^{2}/ndof; Vertices", 200, 0.0, 20.0);
  TH1F* h_vtx_chi2norm_scout = vDir.make<TH1F>("vtx_chi2_scout", "#chi^{2}/ndof (scouting); #chi^{2}/ndof; Vertices", 200, 0.0, 20.0);

  TH1F* h_vtx_pt_off = vDir.make<TH1F>("vtx_pt_off", "vertex p_{T} (offline); p_{T} [GeV]; Vertices", 100, 0.0, 100.0);
  TH1F* h_vtx_pt_scout = vDir.make<TH1F>("vtx_pt_scout", "vertex p_{T} (scouting); p_{T} [GeV]; Vertices", 100, 0.0, 100.0);

  TH1F* h_vtx_eta_off = vDir.make<TH1F>("vtx_eta_off", "vertex eta (offline); #eta; Vertices", 100, -5.0, 5.0);
  TH1F* h_vtx_eta_scout = vDir.make<TH1F>("vtx_eta_scout", "vertex eta (scouting); #eta; Vertices", 100, -5.0, 5.0);

  TH1F* h_vtx_phi_off = vDir.make<TH1F>("vtx_phi_off", "vertex phi (offline); #phi; Vertices", 100, -3.1416, 3.1416);
  TH1F* h_vtx_phi_scout = vDir.make<TH1F>("vtx_phi_scout", "vertex phi (scouting); #phi; Vertices", 100, -3.1416, 3.1416);

  TH1F* h_vtx_mass_off = vDir.make<TH1F>("vtx_mass_off", "vertex mass (offline); mass [GeV]; Vertices", 100, 0.0, 10.0);
  TH1F* h_vtx_mass_scout = vDir.make<TH1F>("vtx_mass_scout", "vertex mass (scouting); mass [GeV]; Vertices", 100, 0.0, 10.0);

  TH1F* h_vtx_ntracks_off = vDir.make<TH1F>("vtx_ntracks_off", "vertex ntracks (offline); ntracks; Vertices", 50, 0, 50);
  TH1F* h_vtx_ntracks_scout = vDir.make<TH1F>("vtx_ntracks_scout", "vertex ntracks (scouting); ntracks; Vertices", 50, 0, 50);

  // Track histograms (flattened lists of all constituent tracks from first selected vertex)
  TH1F* h_trk_pt_off = tDir.make<TH1F>("trk_pt_off", "track p_{T} (offline); p_{T} [GeV]; Tracks", 100, 0.0, 100.0);
  TH1F* h_trk_pt_scout = tDir.make<TH1F>("trk_pt_scout", "track p_{T} (scouting); p_{T} [GeV]; Tracks", 100, 0.0, 100.0);

  TH1F* h_trk_eta_off = tDir.make<TH1F>("trk_eta_off", "track eta (offline); #eta; Tracks", 100, -5.0, 5.0);
  TH1F* h_trk_eta_scout = tDir.make<TH1F>("trk_eta_scout", "track eta (scouting); #eta; Tracks", 100, -5.0, 5.0);

  TH1F* h_trk_phi_off = tDir.make<TH1F>("trk_phi_off", "track phi (offline); #phi; Tracks", 100, -3.1416, 3.1416);
  TH1F* h_trk_phi_scout = tDir.make<TH1F>("trk_phi_scout", "track phi (scouting); #phi; Tracks", 100, -3.1416, 3.1416);

  TH1F* h_trk_p_off = tDir.make<TH1F>("trk_p_off", "track p (offline); p [GeV]; Tracks", 100, 0.0, 200.0);
  TH1F* h_trk_p_scout = tDir.make<TH1F>("trk_p_scout", "track p (scouting); p [GeV]; Tracks", 100, 0.0, 200.0);

  TH1F* h_trk_npix_off = tDir.make<TH1F>("trk_npix_off", "track nPixelHits (offline); npix; Tracks", 10, 0, 10);
  TH1F* h_trk_npix_scout = tDir.make<TH1F>("trk_npix_scout", "track nPixelHits (scouting); npix; Tracks", 10, 0, 10);

  TH1F* h_trk_nstrip_off = tDir.make<TH1F>("trk_nstrip_off", "track nStripHits (offline); nstrip; Tracks", 30, 0, 30);
  TH1F* h_trk_nstrip_scout = tDir.make<TH1F>("trk_nstrip_scout", "track nStripHits (scouting); nstrip; Tracks", 30, 0, 30);

  TH1F* h_trk_nlayers_off = tDir.make<TH1F>("trk_nlayers_off", "track nTrackerLayers (offline); nlayers; Tracks", 30, 0, 30);
  TH1F* h_trk_nlayers_scout = tDir.make<TH1F>("trk_nlayers_scout", "track nTrackerLayers (scouting); nlayers; Tracks", 30, 0, 30);

  TH1F* h_trk_dxy_off = tDir.make<TH1F>("trk_dxy_off", "track dxy (offline); dxy [cm]; Tracks", 200, -2.0, 2.0);
  TH1F* h_trk_dxy_scout = tDir.make<TH1F>("trk_dxy_scout", "track dxy (scouting); dxy [cm]; Tracks", 200, -2.0, 2.0);

  TH1F* h_trk_dxySig_off = tDir.make<TH1F>("trk_dxySig_off", "track dxySig (offline); |dxy|/#sigma; Tracks", 100, 0.0, 50.0);
  TH1F* h_trk_dxySig_scout = tDir.make<TH1F>("trk_dxySig_scout", "track dxySig (scouting); |dxy|/#sigma; Tracks", 100, 0.0, 50.0);

  TH1F* h_trk_dxyErr_off = tDir.make<TH1F>("trk_dxyErr_off", "track dxyError (offline); #sigma_{dxy} [cm]; Tracks", 200, 0.0, 0.05);
  TH1F* h_trk_dxyErr_scout = tDir.make<TH1F>("trk_dxyErr_scout", "track dxyError (scouting); #sigma_{dxy} [cm]; Tracks", 200, 0.0, 0.05);

  // Fill offline histograms from the first selected offline vertex (if present)
  if (haveOffline) {
    for (const auto &v : *vtxOfflineH) {
      if (!vertexPasses(v)) continue;

      // Fill vertex-level
      h_vtx_xy_off_x->Fill(v.x());
      h_vtx_xy_off_y->Fill(v.y());

      // Here we don't compute dBV precisely — we use sqrt(x^2+y^2) (2D distance from origin)
      const double dBV = std::hypot(v.x(), v.y());
      h_vtx_dBV_off->Fill(dBV);

      // No error available from reco::Vertex directly here; store 0 (user may compute elsewhere)
      h_vtx_dBVerr_off->Fill(0.0);

      h_vtx_chi2norm_off->Fill(v.normalizedChi2());

      // approximate vertex 4-vector by summing associated track 4-vectors with pion mass
      TLorentzVector sumVec(0,0,0,0);
      for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
        reco::TrackRef tr = it->castTo<reco::TrackRef>();
        if (!tr.isNonnull()) continue;
        constexpr double kPionMass = 0.13957;
        TLorentzVector tv;
        tv.SetPtEtaPhiM(tr->pt(), tr->eta(), tr->phi(), kPionMass);
        sumVec += tv;
      }
      h_vtx_pt_off->Fill(sumVec.Pt());
      h_vtx_eta_off->Fill(sumVec.Eta());
      h_vtx_phi_off->Fill(sumVec.Phi());
      h_vtx_mass_off->Fill(sumVec.M());
      h_vtx_ntracks_off->Fill(std::distance(v.tracks_begin(), v.tracks_end()));

      // Fill track-level histograms from the same vertex
      for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
        reco::TrackRef tr = it->castTo<reco::TrackRef>();
        if (!tr.isNonnull()) continue;
        h_trk_pt_off->Fill(tr->pt());
        h_trk_eta_off->Fill(tr->eta());
        h_trk_phi_off->Fill(tr->phi());
        h_trk_p_off->Fill(tr->p());
        h_trk_npix_off->Fill(tr->hitPattern().numberOfValidPixelHits());
        h_trk_nstrip_off->Fill(tr->stripPattern().numberOfValidStripHits()); // approximate; some releases differ
        h_trk_nlayers_off->Fill(tr->found() ? tr->found() : 0); // fallback: some releases provide found()
        h_trk_dxy_off->Fill(tr->dxy());
        if (tr->dxyError() > 0) h_trk_dxySig_off->Fill(std::fabs(tr->dxy()/tr->dxyError()));
        h_trk_dxyErr_off->Fill(tr->dxyError());
      }
      break; // only first selected offline vertex
    }
  }

  // Fill scouting histograms from the first selected scouting vertex (if present)
  if (haveScouting) {
    for (const auto &v : *vtxScoutingH) {
      if (!vertexPasses(v)) continue;

      h_vtx_xy_scout_x->Fill(v.x());
      h_vtx_xy_scout_y->Fill(v.y());
      const double dBV = std::hypot(v.x(), v.y());
      h_vtx_dBV_scout->Fill(dBV);
      h_vtx_dBVerr_scout->Fill(0.0);
      h_vtx_chi2norm_scout->Fill(v.normalizedChi2());

      TLorentzVector sumVec(0,0,0,0);
      for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
        reco::TrackRef tr = it->castTo<reco::TrackRef>();
        if (!tr.isNonnull()) continue;
        constexpr double kPionMass = 0.13957;
        TLorentzVector tv;
        tv.SetPtEtaPhiM(tr->pt(), tr->eta(), tr->phi(), kPionMass);
        sumVec += tv;
      }
      h_vtx_pt_scout->Fill(sumVec.Pt());
      h_vtx_eta_scout->Fill(sumVec.Eta());
      h_vtx_phi_scout->Fill(sumVec.Phi());
      h_vtx_mass_scout->Fill(sumVec.M());
      h_vtx_ntracks_scout->Fill(std::distance(v.tracks_begin(), v.tracks_end()));

      // Tracks
      for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
        reco::TrackRef tr = it->castTo<reco::TrackRef>();
        if (!tr.isNonnull()) continue;
        h_trk_pt_scout->Fill(tr->pt());
        h_trk_eta_scout->Fill(tr->eta());
        h_trk_phi_scout->Fill(tr->phi());
        h_trk_p_scout->Fill(tr->p());
        // For scouting tracks, some hit counters may not be present depending on unpacker version
        // We attempt to read pattern counts and fallback to 0 if not available.
        int npix = 0;
        int nstrip = 0;
        int nlayers = 0;
        try {
          npix = tr->hitPattern().numberOfValidPixelHits();
          // strip hits accessor may vary; try a safe call
          nstrip = tr->hitPattern().numberOfValidStripHits();
          nlayers = tr->found() ? tr->found() : 0;
        } catch (...) {
          npix = 0; nstrip = 0; nlayers = 0;
        }
        h_trk_npix_scout->Fill(npix);
        h_trk_nstrip_scout->Fill(nstrip);
        h_trk_nlayers_scout->Fill(nlayers);
        h_trk_dxy_scout->Fill(tr->dxy());
        if (tr->dxyError() > 0) h_trk_dxySig_scout->Fill(std::fabs(tr->dxy()/tr->dxyError()));
        h_trk_dxyErr_scout->Fill(tr->dxyError());
      }
      break; // only first selected scouting vertex
    }
  }

  // --- Create overlay canvases (vertex-level few examples). These get written into the output ROOT file ---
  // Helper lambda to draw 2 histograms into a canvas, add a legend, and Write()
  auto makeOverlayCanvas = [&](TH1F* h_off, TH1F* h_scout, const std::string &cname, const std::string &title) {
    TCanvas *c = new TCanvas(cname.c_str(), title.c_str(), 800, 600);
    h_off->SetLineColor(kBlue);
    h_off->SetLineWidth(2);
    h_scout->SetLineColor(kRed);
    h_scout->SetLineWidth(2);

    h_off->Draw();                 // draw offline first
    h_scout->Draw("SAME");         // overlay scouting
    TLegend leg(0.70, 0.75, 0.90, 0.90);
    leg.AddEntry(h_off, "Offline", "l");
    leg.AddEntry(h_scout, "Scouting", "l");
    leg.Draw();
    c->Write();
    delete c; // canvas object is written into file; safe to delete to avoid memory accumulation
  };

  // Create overlays for a representative set
  makeOverlayCanvas(h_vtx_pt_off, h_vtx_pt_scout, (evname.str() + "_vtx_pt").c_str(), "Vertex pT overlay");
  makeOverlayCanvas(h_vtx_dBV_off, h_vtx_dBV_scout, (evname.str() + "_vtx_dBV").c_str(), "Vertex dBV overlay");
  makeOverlayCanvas(h_vtx_chi2norm_off, h_vtx_chi2norm_scout, (evname.str() + "_vtx_chi2").c_str(), "Vertex chi2 overlay");
  makeOverlayCanvas(h_vtx_mass_off, h_vtx_mass_scout, (evname.str() + "_vtx_mass").c_str(), "Vertex mass overlay");
  makeOverlayCanvas(h_vtx_ntracks_off, h_vtx_ntracks_scout, (evname.str() + "_vtx_ntracks").c_str(), "Vertex ntracks overlay");

  makeOverlayCanvas(h_trk_pt_off, h_trk_pt_scout, (evname.str() + "_trk_pt").c_str(), "Track pT overlay");
  makeOverlayCanvas(h_trk_eta_off, h_trk_eta_scout, (evname.str() + "_trk_eta").c_str(), "Track eta overlay");
  makeOverlayCanvas(h_trk_dxy_off, h_trk_dxy_scout, (evname.str() + "_trk_dxy").c_str(), "Track dxy overlay");

  // done for this event
  return;
}

void EventComparatorAnalyzer::fillDescriptions(edm::ConfigurationDescriptions &descriptions) {
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("displacedVerticesOffline", edm::InputTag("offlineDisplacedVertices", ""));
  desc.add<edm::InputTag>("displacedVerticesScouting", edm::InputTag("Vertexer", ""));
  desc.add<edm::InputTag>("tracksOffline", edm::InputTag("generalTracks", ""));
  desc.add<edm::InputTag>("tracksScouting", edm::InputTag("hltScoutingUnpackProducer", "Track"));
  desc.add<int>("nEventsToSave", 5);
  desc.add<int>("vertex_min_ntracks", 2);
  desc.add<double>("vertex_max_chi2", -1.0); // negative => no cut
  descriptions.add("eventComparator", desc);
}

DEFINE_FWK_MODULE(EventComparatorAnalyzer);