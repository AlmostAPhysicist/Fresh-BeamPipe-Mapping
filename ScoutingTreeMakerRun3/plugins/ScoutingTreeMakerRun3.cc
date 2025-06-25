// -*- C++ -*-
// Package:    Vertexing/ScoutingTreeMakerRun3
// Class:      ScoutingTreeMakerRun3
// 
// Original Author:  David Sperka
//         Created:  Tue, 14 May 2024 14:23:11 GMT

#include <memory>
#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TTree.h"
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
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/Math/interface/deltaPhi.h"
#include "DataFormats/BeamSpot/interface/BeamSpot.h"
#include "DataFormats/Candidate/interface/Candidate.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "RecoVertex/VertexTools/interface/VertexDistanceXY.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "TLorentzVector.h"
#include "TMath.h"
#include <vector>
#include <set>
#include <string>
#include <fstream>

class ScoutingTreeMakerRun3 : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
    explicit ScoutingTreeMakerRun3(const edm::ParameterSet&);
    ~ScoutingTreeMakerRun3() override;

    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
    void beginJob() override;
    void analyze(const edm::Event&, const edm::EventSetup&) override;
    void endJob() override;

    // Parameter declarations with cut values
    const int required_ntk_min;         // minimum number of tracks required in vertex (if not -1)
    const int required_ntk_max;         // maximum number of tracks allowed in vertex (if not -1)
    const double required_invmass;      // minimum invariant mass of vertex (if not -1)
    const double required_chi2;         // vertex.normalizedChi2() must be below this value
    const double required_dBV_min;      // vertex dBV must be >= this value (if not -1)
    const double required_dBV_max;      // vertex dBV must be <= this value (if not -1)
    const double required_dxy_min;      // average track dxy must be >= this value (if not -1)
    const double required_dxy_max;      // average track dxy must be <= this value (if not -1)
    const double required_dBV_error;    // vertex dBV error must be <= this value (if not -1)
    const double required_dxy_error;    // average track dxy error must be <= this value (if not -1)

    const edm::EDGetTokenT<std::vector<reco::Vertex>> verticesToken;
    const edm::EDGetTokenT<reco::BeamSpot> beamspot_token;
    const edm::EDGetTokenT<std::vector<reco::Track>> tracksToken;
    
    // Histogram declarations for vertices and tracks.
    TH2F* h_vertex_xy_global;          // vertex XY positions in global coordinates (x vs y)
    TH2F* h_vertex_xy_beamspot;        // vertex XY positions relative to the beamspot (x-bs.x0 vs y-bs.y0)
    TH1F* h_ntracks_global;            // number of tracks per vertex (after selection)
    TH1F* h_track_momenta_global;      // momentum magnitude (p) of all tracks in selected vertices
    TH1F* h_radial_distance_global;    // radial distance of vertices from the origin (sqrt(x^2 + y^2)), global
    TH1F* h_radial_distance_beamspot;  // radial distance of vertices from the beamspot (sqrt((x-bs.x0)^2 + (y-bs.y0)^2))
    TH1F* h_eta_distribution_global;   // pseudorapidity (eta) of all tracks in selected vertices (eta = -ln(tan(theta/2)), 0 at track perpendicular to beam and -inf,+inf at beamline. theta=0 when particle moving along the positive beam axis, theta=pi when moving along the negative beam axis, theta=pi/2 when moving perpendicular to the beam axis)
    TH1F* h_phi_distribution_global;   // azimuthal angle (phi) of all tracks in selected vertices (the angle in the transverse plane, 0 at the positive x-axis, pi/2 at the positive y-axis, pi at the negative x-axis, and 3pi/2 at the negative y-axis)
    TH2F* h_beamspot_global;           // beamspot position in global coordinates (x0 vs y0)
    TH1F* h_vertex_pt;                 // transverse momentum (pT) of the vertex (sum of track 4-vectors)
    TH1F* h_vertex_eta;                // pseudorapidity (eta) of the vertex (sum of track 4-vectors)
    TH1F* h_vertex_phi;                // azimuthal angle (phi) of the vertex (sum of track 4-vectors)
    TH1F* h_vertex_mass;               // invariant mass of the vertex (sum of track 4-vectors, pion mass hypothesis)
    TH1F* h_vertex_eta_barrel;         // vertex eta for vertices in the barrel region (|eta| < 1.0)
    TH1F* h_vertex_eta_endcap;         // vertex eta for vertices in the endcap region (|eta| >= 1.0)
    TH1F* h_vertex_dBV_barrel;         // d_{BV} (transverse distance from beamspot) for barrel vertices
    TH1F* h_vertex_dBV_endcap;         // d_{BV} (transverse distance from beamspot) for endcap vertices
    TH1F* h_vertex_mass_barrel;        // vertex invariant mass for barrel vertices
    TH1F* h_vertex_mass_endcap;        // vertex invariant mass for endcap vertices
    TH2F* h_vertex_xy_barrel_global;   // vertex XY positions for barrel vertices (global coordinates)
    TH2F* h_vertex_xy_endcap_global;   // vertex XY positions for endcap vertices (global coordinates)
    TH2F* h_vertex_xy_barrel_beamspot; // vertex XY positions for barrel vertices (beamspot-centered)
    TH2F* h_vertex_xy_endcap_beamspot; // vertex XY positions for endcap vertices (beamspot-centered)
    TH1F* h_vertex_dBV;                // d_{BV} (transverse distance from beamspot) for all selected vertices
    TH1F* h_vertex_dBV_error;          // uncertainty (error) on d_{BV} for all selected vertices
    TH1F* h_nvertices_ntk;             // number of candidate vertices per event (after ntk cut)
    TH1F* h_track_dxy_barrel;          // track d_{xy} in the barrel (signed distance of closest approach to beamspot in transverse plane)
    TH1F* h_track_dxy_endcap;          // track d_{xy} in the endcap (signed distance of closest approach to beamspot in transverse plane)
    TH1F* h_track_dxyError_barrel;     // uncertainty (error) on track d_{xy} in the barrel
    TH1F* h_track_dxyError_endcap;     // uncertainty (error) on track d_{xy} in the endcap

    // Output file stream for track method diagnostics (if used)
    std::ofstream trackMethodsFile_;
    // Counter for tracks processed (if used)
    int trackCounter_;
    // Added overall track dxy histograms
    TH1F* h_track_dxy;
    TH1F* h_track_dxyError;

    typedef std::set<reco::TrackRef> track_set;
    typedef std::vector<reco::TrackRef> track_vec;
    track_set vertex_track_set(const reco::Vertex & v, const double min_weight = 0.5) const {
        track_set result;
        for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
            const double w = v.trackWeight(*it);
            const bool use = w >= min_weight;
            assert(use);
            if (use) result.insert(it->castTo<reco::TrackRef>());
        }
        return result;
    }
    track_vec vertex_track_vec(const reco::Vertex & v, const double min_weight = 0.5) const {
        track_set s = vertex_track_set(v, min_weight);
        return track_vec(s.begin(), s.end());
    }
};

ScoutingTreeMakerRun3::ScoutingTreeMakerRun3(const edm::ParameterSet& iConfig):
    required_ntk_min(iConfig.getParameter<int>("required_ntk_min")),
    required_ntk_max(iConfig.getParameter<int>("required_ntk_max")),
    required_invmass(iConfig.getParameter<double>("required_invmass")),
    required_chi2(iConfig.getParameter<double>("required_chi2")),
    required_dBV_min(iConfig.getParameter<double>("required_dBV_min")),
    required_dBV_max(iConfig.getParameter<double>("required_dBV_max")),
    required_dxy_min(iConfig.getParameter<double>("required_dxy_min")),
    required_dxy_max(iConfig.getParameter<double>("required_dxy_max")),
    required_dBV_error(iConfig.getParameter<double>("required_dBV_error")),
    required_dxy_error(iConfig.getParameter<double>("required_dxy_error")),
    verticesToken(consumes<std::vector<reco::Vertex>>(iConfig.getParameter<edm::InputTag>("displacedVertices"))),
    beamspot_token(consumes<reco::BeamSpot>(iConfig.getParameter<edm::InputTag>("beamspot_src"))),
    tracksToken(consumes<std::vector<reco::Track>>(iConfig.getParameter<edm::InputTag>("tracks")))
{
    usesResource("TFileService");
}

ScoutingTreeMakerRun3::~ScoutingTreeMakerRun3() {
    // ...existing cleanup if necessary...
}

void ScoutingTreeMakerRun3::beginJob() {
    edm::Service<TFileService> fs;
    // Overall histograms
    h_vertex_xy_global        = fs->make<TH2F>("vertex_xy_global", "Vertex XY Position (Global); X [cm]; Y [cm]", 2000, -10, 10, 2000, -10, 10);
    h_vertex_xy_beamspot      = fs->make<TH2F>("vertex_xy_beamspot", "Vertex XY Position (Beamspot); X [cm]; Y [cm]", 2000, -10, 10, 2000, -10, 10);
    h_ntracks_global          = fs->make<TH1F>("ntracks_global", "Number of Tracks; Number of Tracks; Vertices", 200, 0, 100);
    h_track_momenta_global    = fs->make<TH1F>("track_momenta_global", "Track Momenta; Momentum [GeV/c]; Tracks", 400, 0, 100);
    h_radial_distance_global  = fs->make<TH1F>("radial_distance_global", "Radial Distance (Global); Distance [cm]; Vertices", 700, 0, 7);
    h_radial_distance_beamspot= fs->make<TH1F>("radial_distance_beamspot", "Radial Distance (Beamspot); Distance [cm]; Vertices", 700, 0, 7);
    h_eta_distribution_global = fs->make<TH1F>("eta_distribution_global", "Eta Distribution; Eta; Tracks", 400, -3, 3);
    h_phi_distribution_global   = fs->make<TH1F>("phi_distribution_global", "Phi Distribution; Phi; Tracks", 400, -3.142, 3.142);
    h_beamspot_global         = fs->make<TH2F>("beamspot_global", "Beamspot Position (Global); X [cm]; Y [cm]", 400, -1, 1, 400, -1, 1);
    h_vertex_pt               = fs->make<TH1F>("vertex_pt", "Vertex pT; pT [GeV/c]; Vertices", 200, 0., 100.);
    h_vertex_eta              = fs->make<TH1F>("vertex_eta", "Vertex eta; eta; Vertices", 200, -5., 5.);
    h_vertex_phi              = fs->make<TH1F>("vertex_phi", "Vertex phi; phi; Vertices", 200, -3.14, 3.14);
    h_vertex_mass             = fs->make<TH1F>("vertex_mass", "Vertex mass; mass [GeV/c^2]; Vertices", 200, 0., 10.);
    h_vertex_dBV              = fs->make<TH1F>("vertex_dBV", "Vertex d_{BV} [cm]; Vertices / 0.05 cm", 200, 0, 10);
    h_vertex_dBV_error        = fs->make<TH1F>("vertex_dBV_error", "Vertex d_{BV} Uncertainty; d_{BV} Uncertainty [cm]; Entries", 1000, 0, 0.1);
    h_nvertices_ntk           = fs->make<TH1F>("nvertices_ntk", "Number of Candidate Vertices (ntk within cut); Number of Vertices; Events", 1000, 0, 1000);
    h_track_dxy               = fs->make<TH1F>("track_dxy", "Track dxy; dxy [cm]; Tracks", 1000, -5, 5);
    h_track_dxyError          = fs->make<TH1F>("track_dxyError", "Track dxy Error; dxy Error [cm]; Tracks", 1000, 0, 0.1);
    // Barrel histograms
    h_vertex_eta_barrel       = fs->make<TH1F>("vertex_eta_barrel", "Vertex Eta (Barrel); eta; Vertices", 200, -3.0, 3.0);
    h_vertex_dBV_barrel       = fs->make<TH1F>("vertex_dBV_barrel", "Vertex d_{BV} (Barrel); d_{BV} [cm]; Vertices", 200, 0, 10);
    h_vertex_mass_barrel      = fs->make<TH1F>("vertex_mass_barrel", "Vertex Mass (Barrel); mass [GeV/c^{2}]; Vertices", 200, 0, 10);
    h_vertex_xy_barrel_global = fs->make<TH2F>("vertex_xy_barrel_global", "Vertex XY (Barrel, Global); X [cm]; Y [cm]", 2000, -10, 10, 2000, -10, 10);
    h_vertex_xy_barrel_beamspot = fs->make<TH2F>("vertex_xy_barrel_beamspot", "Vertex XY (Barrel, Beamspot-Centered); X [cm]; Y [cm]", 2000, -10, 10, 2000, -10, 10);
    h_track_dxy_barrel        = fs->make<TH1F>("track_dxy_barrel", "Track d_{xy} (Barrel); d_{xy} [cm]; Tracks", 1000, -5, 5);
    h_track_dxyError_barrel   = fs->make<TH1F>("track_dxyError_barrel", "Track d_{xy} Error (Barrel); d_{xy} Error [cm]; Tracks", 1000, 0, 0.1);
    // Endcap histograms
    h_vertex_eta_endcap       = fs->make<TH1F>("vertex_eta_endcap", "Vertex Eta (Endcap); eta; Vertices", 200, -3.0, 3.0);
    h_vertex_dBV_endcap       = fs->make<TH1F>("vertex_dBV_endcap", "Vertex d_{BV} (Endcap); d_{BV} [cm]; Vertices", 200, 0, 10);
    h_vertex_mass_endcap      = fs->make<TH1F>("vertex_mass_endcap", "Vertex Mass (Endcap); mass [GeV/c^{2}]; Vertices", 200, 0, 10);
    h_vertex_xy_endcap_global = fs->make<TH2F>("vertex_xy_endcap_global", "Vertex XY (Endcap, Global); X [cm]; Y [cm]", 2000, -10, 10, 2000, -10, 10);
    h_vertex_xy_endcap_beamspot = fs->make<TH2F>("vertex_xy_endcap_beamspot", "Vertex XY (Endcap, Beamspot-Centered); X [cm]; Y [cm]", 2000, -10, 10, 2000, -10, 10);
    h_track_dxy_endcap        = fs->make<TH1F>("track_dxy_endcap", "Track d_{xy} (Endcap); d_{xy} [cm]; Tracks", 1000, -5, 5);
    h_track_dxyError_endcap   = fs->make<TH1F>("track_dxyError_endcap", "Track d_{xy} Error (Endcap); d_{xy} Error [cm]; Tracks", 1000, 0, 0.1);
}

void ScoutingTreeMakerRun3::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
    using namespace edm; using namespace std; using namespace reco;
    Handle<BeamSpot> beamspot;
    iEvent.getByToken(beamspot_token, beamspot);
    if(!beamspot.isValid()){
        LogError("ScoutingTreeMakerRun3") << "Beamspot handle invalid!";
        return;
    }
    const Vertex fake_bs_vtx(beamspot->position(), beamspot->covariance3D());
    
    Handle<vector<Vertex>> verticesH;
    iEvent.getByToken(verticesToken, verticesH);
    if(!verticesH.isValid()){
        LogError("ScoutingTreeMakerRun3") << "Vertex handle invalid!";
        return;
    }
    
    vector<Vertex> selVertices;
    VertexDistanceXY vertex_dist_2d;
    
    for (unsigned int t = 0; t < verticesH->size(); t++) {
        Vertex v = verticesH->at(t);
        vector<TrackRef> tks = vertex_track_vec(v);
        int ntk = tks.size();
        
        TLorentzVector sumVec(0,0,0,0);
        double sum_dxy = 0.0, sum_dxyErr = 0.0;
        for(auto track : tks) {
            if(!track.isNonnull()) continue; // Safety: skip null track references
            TLorentzVector trackVec;
            trackVec.SetPtEtaPhiM(track->pt(), track->eta(), track->phi(), 0.13957);
            sumVec += trackVec;
            sum_dxy += track->dxy(beamspot->position());
            sum_dxyErr += track->dxyError();
        }
        double invMass = sumVec.M();
        double avg_dxy = (ntk > 0 ? sum_dxy/ntk : 0);
        double avg_dxyErr = (ntk > 0 ? sum_dxyErr/ntk : 0);
        double dBV = vertex_dist_2d.distance(v, fake_bs_vtx).value();
        double dBV_err = vertex_dist_2d.distance(v, fake_bs_vtx).error();
        
        // Apply cuts (skip cut if parameter is -1)
        if(required_ntk_min != -1 && ntk < required_ntk_min) continue;
        if(required_ntk_max != -1 && ntk > required_ntk_max) continue;
        if(required_invmass != -1 && invMass < required_invmass) continue;
        if(required_chi2 != -1 && v.normalizedChi2() > required_chi2) continue;
        if(required_dBV_min != -1 && dBV < required_dBV_min) continue;
        if(required_dBV_max != -1 && dBV > required_dBV_max) continue;
        if(required_dxy_min != -1 && avg_dxy < required_dxy_min) continue;
        if(required_dxy_max != -1 && avg_dxy > required_dxy_max) continue;
        if(required_dBV_error != -1 && dBV_err > required_dBV_error) continue;
        if(required_dxy_error != -1 && avg_dxyErr > required_dxy_error) continue;
        
    // Vertex passed selection:
    
        selVertices.push_back(v);

        // Fill missing histograms for vertices that pass cuts:
        h_beamspot_global->Fill(beamspot->x0(), beamspot->y0());

        h_ntracks_global->Fill(ntk);
        h_vertex_xy_global->Fill(v.x(), v.y());
        h_vertex_xy_beamspot->Fill(v.x() - beamspot->x0(), v.y() - beamspot->y0());
        
        h_vertex_dBV->Fill(dBV);
        h_vertex_dBV_error->Fill(dBV_err);
        // Compute radial distances and fill:
        double rad_global = TMath::Sqrt(v.x()*v.x() + v.y()*v.y());
        double rad_beam = TMath::Sqrt(pow(v.x()-beamspot->x0(),2) + pow(v.y()-beamspot->y0(),2));
        h_radial_distance_global->Fill(rad_global);
        h_radial_distance_beamspot->Fill(rad_beam);
        
        if(fabs(sumVec.Eta()) < 1.0) {
            h_vertex_eta_barrel->Fill(sumVec.Eta());
            h_vertex_dBV_barrel->Fill(dBV);
            h_vertex_mass_barrel->Fill(invMass);
            h_vertex_xy_barrel_global->Fill(v.x(), v.y());
            h_vertex_xy_barrel_beamspot->Fill(v.x() - beamspot->x0(), v.y() - beamspot->y0());
        } else {
            h_vertex_eta_endcap->Fill(sumVec.Eta());
            h_vertex_dBV_endcap->Fill(dBV);
            h_vertex_mass_endcap->Fill(invMass);
            h_vertex_xy_endcap_global->Fill(v.x(), v.y());
            h_vertex_xy_endcap_beamspot->Fill(v.x() - beamspot->x0(), v.y() - beamspot->y0());
        }
        h_vertex_pt->Fill(sumVec.Pt());
        h_vertex_phi->Fill(sumVec.Phi());
        
        for(auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
            reco::TrackRef track = it->castTo<reco::TrackRef>();
            if(!track.isNonnull()){
                LogWarning("ScoutingTreeMakerRun3") << "Null track reference in vertex " << t;
                continue;
            }
            h_track_momenta_global->Fill(track->p());
            h_eta_distribution_global->Fill(track->eta());
            h_phi_distribution_global->Fill(track->phi());
            if(fabs(track->eta()) < 1.0) {
                h_track_dxy_barrel->Fill(track->dxy(beamspot->position()));
                h_track_dxyError_barrel->Fill(track->dxyError());
            } else {
                h_track_dxy_endcap->Fill(track->dxy(beamspot->position()));
                h_track_dxyError_endcap->Fill(track->dxyError());
            }
            h_track_dxy->Fill(track->dxy(beamspot->position()));
            h_track_dxyError->Fill(track->dxyError());
        }
    }
    h_nvertices_ntk->Fill(selVertices.size());
    // ...existing code for inter-vertex quantities if needed...
}

void ScoutingTreeMakerRun3::endJob() {
    // Overall histograms
    h_vertex_xy_global->Draw();        h_vertex_xy_global->Write();
    h_vertex_xy_beamspot->Draw();      h_vertex_xy_beamspot->Write();
    h_ntracks_global->Draw();          h_ntracks_global->Write();
    h_track_momenta_global->Draw();    h_track_momenta_global->Write();
    h_radial_distance_global->Draw();  h_radial_distance_global->Write();
    h_radial_distance_beamspot->Draw();h_radial_distance_beamspot->Write();
    h_eta_distribution_global->Draw(); h_eta_distribution_global->Write();
    h_phi_distribution_global->Draw(); h_phi_distribution_global->Write();
    h_beamspot_global->Draw();         h_beamspot_global->Write();
    h_vertex_pt->Draw();               h_vertex_pt->Write();
    h_vertex_eta->Draw();              h_vertex_eta->Write();
    h_vertex_phi->Draw();              h_vertex_phi->Write();
    h_vertex_mass->Draw();             h_vertex_mass->Write();
    h_vertex_dBV->Draw();              h_vertex_dBV->Write();
    h_vertex_dBV_error->Draw();        h_vertex_dBV_error->Write();
    h_nvertices_ntk->Draw();           h_nvertices_ntk->Write();
    h_track_dxy->Draw();               h_track_dxy->Write();
    h_track_dxyError->Draw();          h_track_dxyError->Write();

    // Barrel histograms
    h_vertex_eta_barrel->Draw();       h_vertex_eta_barrel->Write();
    h_vertex_dBV_barrel->Draw();       h_vertex_dBV_barrel->Write();
    h_vertex_mass_barrel->Draw();      h_vertex_mass_barrel->Write();
    h_vertex_xy_barrel_global->Draw(); h_vertex_xy_barrel_global->Write();
    h_vertex_xy_barrel_beamspot->Draw(); h_vertex_xy_barrel_beamspot->Write();
    h_track_dxy_barrel->Draw();        h_track_dxy_barrel->Write();
    h_track_dxyError_barrel->Draw();   h_track_dxyError_barrel->Write();

    // Endcap histograms
    h_vertex_eta_endcap->Draw();       h_vertex_eta_endcap->Write();
    h_vertex_dBV_endcap->Draw();       h_vertex_dBV_endcap->Write();
    h_vertex_mass_endcap->Draw();      h_vertex_mass_endcap->Write();
    h_vertex_xy_endcap_global->Draw(); h_vertex_xy_endcap_global->Write();
    h_vertex_xy_endcap_beamspot->Draw(); h_vertex_xy_endcap_beamspot->Write();
    h_track_dxy_endcap->Draw();        h_track_dxy_endcap->Write();
    h_track_dxyError_endcap->Draw();   h_track_dxyError_endcap->Write();
}

void ScoutingTreeMakerRun3::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    desc.setUnknown();
    descriptions.addDefault(desc);
}

DEFINE_FWK_MODULE(ScoutingTreeMakerRun3);