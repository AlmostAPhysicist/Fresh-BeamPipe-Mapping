// -*- C++ -*-
// Package:    Vertexing/ScoutingTreeMakerRun3
// Class:      ScoutingTreeMakerRun3
//
// Author:     Adapted / fixed by an assistant (based on original by David Sperka)
// Created:    Tue, 14 May 2024 14:23:11 GMT
// Revised:    2025-09-09

#include <memory>
#include <vector>
#include <set>
#include <string>
#include <cmath>
#include <fstream>

#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TTree.h"
#include "TLorentzVector.h"
#include "TMath.h"

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
#include "DataFormats/Candidate/interface/Candidate.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"

#include "RecoVertex/VertexTools/interface/VertexDistanceXY.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "DataFormats/Math/interface/Point3D.h"

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
    const int required_ntk_min;
    const int required_ntk_max;
    const double required_invmass;
    const double required_chi2;
    const double required_dBV_min;
    const double required_dBV_max;
    const double required_dxy_min;
    const double required_dxy_max;
    const double required_dBV_error;
    const double required_dxy_error;
    const int PVBoundary1;
    const int PVBoundary2;

    const edm::EDGetTokenT<std::vector<reco::Vertex>> verticesToken;
    const edm::EDGetTokenT<reco::BeamSpot> beamspot_token;
    const edm::EDGetTokenT<std::vector<reco::Track>> tracksToken;
    const edm::EDGetTokenT<std::vector<reco::Vertex>> primaryVerticesToken;

    // Primary vertex histograms
    TH1F* h_nPrimaryVertices;
    TH2F* h_primaryVertices_xy_global;
    TH2F* h_avg_primary_vertex_vs_beamspot;
    
    // avgPV-based histograms (equivalent to beamspot-based ones)
    TH2F* h_vertex_xy_avgPV;
    TH1F* h_radial_distance_avgPV;
    TH1F* h_track_dxy_avgPV;
    TH1F* h_track_dxySig_avgPV;
    TH2F* h_vertex_xy_barrel_avgPV;
    TH2F* h_vertex_xy_endcap_avgPV;
    
    // PV region-based histograms (A, B, C regions)
    // Region A: nPV < PVBoundary1
    TH2F* h_vertex_xy_global_regionA;
    TH2F* h_vertex_xy_beamspot_regionA;
    TH2F* h_vertex_xy_avgPV_regionA;
    TH1F* h_vertex_dBV_regionA;
    TH1F* h_vertex_mass_regionA;
    TH1F* h_radial_distance_global_regionA;
    TH1F* h_radial_distance_beamspot_regionA;
    TH1F* h_radial_distance_avgPV_regionA;
    
    // Region B: PVBoundary1 <= nPV < PVBoundary2
    TH2F* h_vertex_xy_global_regionB;
    TH2F* h_vertex_xy_beamspot_regionB;
    TH2F* h_vertex_xy_avgPV_regionB;
    TH1F* h_vertex_dBV_regionB;
    TH1F* h_vertex_mass_regionB;
    TH1F* h_radial_distance_global_regionB;
    TH1F* h_radial_distance_beamspot_regionB;
    TH1F* h_radial_distance_avgPV_regionB;
    
    // Region C: nPV >= PVBoundary2
    TH2F* h_vertex_xy_global_regionC;
    TH2F* h_vertex_xy_beamspot_regionC;
    TH2F* h_vertex_xy_avgPV_regionC;
    TH1F* h_vertex_dBV_regionC;
    TH1F* h_vertex_mass_regionC;
    TH1F* h_radial_distance_global_regionC;
    TH1F* h_radial_distance_beamspot_regionC;
    TH1F* h_radial_distance_avgPV_regionC;

    // Left and right endcap histograms (replacing absolute value cuts)
    TH1F* h_vertex_eta_left_endcap;
    TH1F* h_vertex_eta_right_endcap;
    TH1F* h_vertex_dBV_left_endcap;
    TH1F* h_vertex_dBV_right_endcap;
    TH1F* h_vertex_mass_left_endcap;
    TH1F* h_vertex_mass_right_endcap;
    TH2F* h_vertex_xy_left_endcap_global;
    TH2F* h_vertex_xy_right_endcap_global;
    TH2F* h_vertex_xy_left_endcap_beamspot;
    TH2F* h_vertex_xy_right_endcap_beamspot;

    // Histograms (canonical set)
    TH2F* h_vertex_xy_global;
    TH2F* h_vertex_xy_beamspot;
    TH1F* h_ntracks_global;
    TH1F* h_track_momenta_global;
    TH1F* h_radial_distance_global;
    TH1F* h_radial_distance_beamspot;
    TH1F* h_eta_distribution_global;
    TH1F* h_phi_distribution_global;
    TH2F* h_beamspot_global;
    TH2F* h_beamspot_vs_pmvtx;
    TH1F* h_vertex_pt;
    TH1F* h_vertex_eta;
    TH1F* h_vertex_phi;
    TH1F* h_vertex_mass;
    TH1F* h_vertex_eta_barrel;
    TH1F* h_vertex_eta_endcap;
    TH1F* h_vertex_dBV_barrel;
    TH1F* h_vertex_dBV_endcap;
    TH1F* h_vertex_mass_barrel;
    TH1F* h_vertex_mass_endcap;
    TH2F* h_vertex_xy_barrel_global;
    TH2F* h_vertex_xy_endcap_global;
    TH2F* h_vertex_xy_barrel_beamspot;
    TH2F* h_vertex_xy_endcap_beamspot;
    TH1F* h_vertex_dBV;
    TH1F* h_vertex_dBV_error;
    TH1F* h_nvertices_ntk;

    // Canonical per-track dxy histograms
    TH1F* h_track_dxy_00;
    TH1F* h_track_dxy_beamspot;
    TH1F* h_track_dxy_pmvtx;
    TH1F* h_track_dxyError;
    TH1F* h_track_dxySig_00;
    TH1F* h_track_dxySig_beamspot;
    TH1F* h_track_dxySig_pmvtx;
    TH1F* h_track_dxyError_barrel;
    TH1F* h_track_dxyError_endcap;

    typedef std::set<reco::TrackRef> track_set;
    typedef std::vector<reco::TrackRef> track_vec;
    track_set vertex_track_set(const reco::Vertex & v, const double min_weight = 0.5) const {
         track_set result;
         for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
             const double w = v.trackWeight(*it);
             const bool use = w >= min_weight;
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
    PVBoundary1(iConfig.getParameter<int>("PVBoundary1")),
    PVBoundary2(iConfig.getParameter<int>("PVBoundary2")),
    verticesToken(consumes<std::vector<reco::Vertex>>(iConfig.getParameter<edm::InputTag>("displacedVertices"))),
    beamspot_token(consumes<reco::BeamSpot>(iConfig.getParameter<edm::InputTag>("beamspot_src"))),
    tracksToken(consumes<std::vector<reco::Track>>(iConfig.getParameter<edm::InputTag>("tracks"))),
    primaryVerticesToken(consumes<std::vector<reco::Vertex>>(iConfig.getParameter<edm::InputTag>("primaryVertices")))
{
    usesResource("TFileService");
}

ScoutingTreeMakerRun3::~ScoutingTreeMakerRun3() {
    // no explicit cleanup required; histograms owned by TFileService
}

void ScoutingTreeMakerRun3::beginJob() {
    edm::Service<TFileService> fs;
    
    // Primary vertex histograms
    h_nPrimaryVertices = fs->make<TH1F>("nPrimaryVertices", "Number of Primary Vertices; Number of Primary Vertices; Events", 100, 0, 100);
    h_primaryVertices_xy_global = fs->make<TH2F>("primaryVertices_xy_global", "Primary Vertices XY Position (Global); X [cm]; Y [cm]", 400, -1, 1, 400, -1, 1);
    h_avg_primary_vertex_vs_beamspot = fs->make<TH2F>("avg_primary_vertex_vs_beamspot", "Average Primary Vertex - Beamspot; x_{avgPV}-x_{BS} [cm]; y_{avgPV}-y_{BS} [cm]", 400, -1, 1, 400, -1, 1);
    
    // avgPV-based histograms (equivalent to beamspot-based ones)
    h_vertex_xy_avgPV = fs->make<TH2F>("vertex_xy_avgPV", "Vertex XY Position (AvgPV); X-avgPV_x [cm]; Y-avgPV_y [cm]", 2000, -10, 10, 2000, -10, 10);
    h_radial_distance_avgPV = fs->make<TH1F>("radial_distance_avgPV", "Radial Distance (AvgPV); Distance [cm]; Vertices", 700, 0, 7);
    h_track_dxy_avgPV = fs->make<TH1F>("track_dxy_avgPV", "Track dxy w.r.t. avg primary vertex; dxy_{avgPV} [cm]; Tracks", 1000, -5, 5);
    h_track_dxySig_avgPV = fs->make<TH1F>("track_dxySig_avgPV", "Track dxy significance |dxy_{avgPV}/err|; |dxy_{avgPV}/err|; Tracks", 200, 0, 50);
    h_vertex_xy_barrel_avgPV = fs->make<TH2F>("vertex_xy_barrel_avgPV", "Vertex XY (Barrel, AvgPV-Centered); X-avgPV_x [cm]; Y-avgPV_y [cm]", 2000, -10, 10, 2000, -10, 10);
    h_vertex_xy_endcap_avgPV = fs->make<TH2F>("vertex_xy_endcap_avgPV", "Vertex XY (Endcap, AvgPV-Centered); X-avgPV_x [cm]; Y-avgPV_y [cm]", 2000, -10, 10, 2000, -10, 10);

    // PV region-based histograms
    if (PVBoundary1 != -1) {
        // Region A histograms
        h_vertex_xy_global_regionA = fs->make<TH2F>("vertex_xy_global_regionA", "Vertex XY Position (Global, Region A); X [cm]; Y [cm]", 2000, -10, 10, 2000, -10, 10);
        h_vertex_xy_beamspot_regionA = fs->make<TH2F>("vertex_xy_beamspot_regionA", "Vertex XY Position (Beamspot, Region A); X-bs_x0 [cm]; Y-bs_y0 [cm]", 2000, -10, 10, 2000, -10, 10);
        h_vertex_xy_avgPV_regionA = fs->make<TH2F>("vertex_xy_avgPV_regionA", "Vertex XY Position (AvgPV, Region A); X-avgPV_x [cm]; Y-avgPV_y [cm]", 2000, -10, 10, 2000, -10, 10);
        h_vertex_dBV_regionA = fs->make<TH1F>("vertex_dBV_regionA", "Vertex d_{BV} (Region A); d_{BV} [cm]; Vertices", 200, 0, 10);
        h_vertex_mass_regionA = fs->make<TH1F>("vertex_mass_regionA", "Vertex Mass (Region A); mass [GeV/c^{2}]; Vertices", 200, 0, 10);
        h_radial_distance_global_regionA = fs->make<TH1F>("radial_distance_global_regionA", "Radial Distance Global (Region A); Distance [cm]; Vertices", 700, 0, 7);
        h_radial_distance_beamspot_regionA = fs->make<TH1F>("radial_distance_beamspot_regionA", "Radial Distance Beamspot (Region A); Distance [cm]; Vertices", 700, 0, 7);
        h_radial_distance_avgPV_regionA = fs->make<TH1F>("radial_distance_avgPV_regionA", "Radial Distance AvgPV (Region A); Distance [cm]; Vertices", 700, 0, 7);
        
        // Region B histograms
        h_vertex_xy_global_regionB = fs->make<TH2F>("vertex_xy_global_regionB", "Vertex XY Position (Global, Region B); X [cm]; Y [cm]", 2000, -10, 10, 2000, -10, 10);
        h_vertex_xy_beamspot_regionB = fs->make<TH2F>("vertex_xy_beamspot_regionB", "Vertex XY Position (Beamspot, Region B); X-bs_x0 [cm]; Y-bs_y0 [cm]", 2000, -10, 10, 2000, -10, 10);
        h_vertex_xy_avgPV_regionB = fs->make<TH2F>("vertex_xy_avgPV_regionB", "Vertex XY Position (AvgPV, Region B); X-avgPV_x [cm]; Y-avgPV_y [cm]", 2000, -10, 10, 2000, -10, 10);
        h_vertex_dBV_regionB = fs->make<TH1F>("vertex_dBV_regionB", "Vertex d_{BV} (Region B); d_{BV} [cm]; Vertices", 200, 0, 10);
        h_vertex_mass_regionB = fs->make<TH1F>("vertex_mass_regionB", "Vertex Mass (Region B); mass [GeV/c^{2}]; Vertices", 200, 0, 10);
        h_radial_distance_global_regionB = fs->make<TH1F>("radial_distance_global_regionB", "Radial Distance Global (Region B); Distance [cm]; Vertices", 700, 0, 7);
        h_radial_distance_beamspot_regionB = fs->make<TH1F>("radial_distance_beamspot_regionB", "Radial Distance Beamspot (Region B); Distance [cm]; Vertices", 700, 0, 7);
        h_radial_distance_avgPV_regionB = fs->make<TH1F>("radial_distance_avgPV_regionB", "Radial Distance AvgPV (Region B); Distance [cm]; Vertices", 700, 0, 7);
        
        // Region C histograms
        h_vertex_xy_global_regionC = fs->make<TH2F>("vertex_xy_global_regionC", "Vertex XY Position (Global, Region C); X [cm]; Y [cm]", 2000, -10, 10, 2000, -10, 10);
        h_vertex_xy_beamspot_regionC = fs->make<TH2F>("vertex_xy_beamspot_regionC", "Vertex XY Position (Beamspot, Region C); X-bs_x0 [cm]; Y-bs_y0 [cm]", 2000, -10, 10, 2000, -10, 10);
        h_vertex_xy_avgPV_regionC = fs->make<TH2F>("vertex_xy_avgPV_regionC", "Vertex XY Position (AvgPV, Region C); X-avgPV_x [cm]; Y-avgPV_y [cm]", 2000, -10, 10, 2000, -10, 10);
        h_vertex_dBV_regionC = fs->make<TH1F>("vertex_dBV_regionC", "Vertex d_{BV} (Region C); d_{BV} [cm]; Vertices", 200, 0, 10);
        h_vertex_mass_regionC = fs->make<TH1F>("vertex_mass_regionC", "Vertex Mass (Region C); mass [GeV/c^{2}]; Vertices", 200, 0, 10);
        h_radial_distance_global_regionC = fs->make<TH1F>("radial_distance_global_regionC", "Radial Distance Global (Region C); Distance [cm]; Vertices", 700, 0, 7);
        h_radial_distance_beamspot_regionC = fs->make<TH1F>("radial_distance_beamspot_regionC", "Radial Distance Beamspot (Region C); Distance [cm]; Vertices", 700, 0, 7);
        h_radial_distance_avgPV_regionC = fs->make<TH1F>("radial_distance_avgPV_regionC", "Radial Distance AvgPV (Region C); Distance [cm]; Vertices", 700, 0, 7);
    }

    // Left and right endcap histograms
    h_vertex_eta_left_endcap = fs->make<TH1F>("vertex_eta_left_endcap", "Vertex Eta (Left Endcap); eta; Vertices", 200, -3.0, 3.0);
    h_vertex_eta_right_endcap = fs->make<TH1F>("vertex_eta_right_endcap", "Vertex Eta (Right Endcap); eta; Vertices", 200, -3.0, 3.0);
    h_vertex_dBV_left_endcap = fs->make<TH1F>("vertex_dBV_left_endcap", "Vertex d_{BV} (Left Endcap); d_{BV} [cm]; Vertices", 200, 0, 10);
    h_vertex_dBV_right_endcap = fs->make<TH1F>("vertex_dBV_right_endcap", "Vertex d_{BV} (Right Endcap); d_{BV} [cm]; Vertices", 200, 0, 10);
    h_vertex_mass_left_endcap = fs->make<TH1F>("vertex_mass_left_endcap", "Vertex Mass (Left Endcap); mass [GeV/c^{2}]; Vertices", 200, 0, 10);
    h_vertex_mass_right_endcap = fs->make<TH1F>("vertex_mass_right_endcap", "Vertex Mass (Right Endcap); mass [GeV/c^{2}]; Vertices", 200, 0, 10);
    h_vertex_xy_left_endcap_global = fs->make<TH2F>("vertex_xy_left_endcap_global", "Vertex XY (Left Endcap, Global); X [cm]; Y [cm]", 2000, -10, 10, 2000, -10, 10);
    h_vertex_xy_right_endcap_global = fs->make<TH2F>("vertex_xy_right_endcap_global", "Vertex XY (Right Endcap, Global); X [cm]; Y [cm]", 2000, -10, 10, 2000, -10, 10);
    h_vertex_xy_left_endcap_beamspot = fs->make<TH2F>("vertex_xy_left_endcap_beamspot", "Vertex XY (Left Endcap, Beamspot-Centered); X-bs_x0 [cm]; Y-bs_y0 [cm]", 2000, -10, 10, 2000, -10, 10);
    h_vertex_xy_right_endcap_beamspot = fs->make<TH2F>("vertex_xy_right_endcap_beamspot", "Vertex XY (Right Endcap, Beamspot-Centered); X-bs_x0 [cm]; Y-bs_y0 [cm]", 2000, -10, 10, 2000, -10, 10);

    // Overall histograms (kept high resolution where requested)
    h_vertex_xy_global = fs->make<TH2F>("vertex_xy_global", "Vertex XY Position (Global); X [cm]; Y [cm]", 2000, -10, 10, 2000, -10, 10);
    h_vertex_xy_beamspot = fs->make<TH2F>("vertex_xy_beamspot", "Vertex XY Position (Beamspot); X-bs_x0 [cm]; Y-bs_y0 [cm]", 2000, -10, 10, 2000, -10, 10);
    h_ntracks_global = fs->make<TH1F>("ntracks_global", "Number of Tracks; Number of Tracks; Vertices", 200, 0, 100);
    h_track_momenta_global = fs->make<TH1F>("track_momenta_global", "Track Momenta; Momentum [GeV/c]; Tracks", 400, 0, 100);
    h_radial_distance_global = fs->make<TH1F>("radial_distance_global", "Radial Distance (Global); Distance [cm]; Vertices", 700, 0, 7);
    h_radial_distance_beamspot= fs->make<TH1F>("radial_distance_beamspot", "Radial Distance (Beamspot); Distance [cm]; Vertices", 700, 0, 7);
    h_eta_distribution_global = fs->make<TH1F>("eta_distribution_global", "Eta Distribution; Eta; Tracks", 400, -3, 3);
    h_phi_distribution_global = fs->make<TH1F>("phi_distribution_global", "Phi Distribution; Phi; Tracks", 400, -3.142, 3.142);
    h_beamspot_global = fs->make<TH2F>("beamspot_global", "Beamspot Position (Global ref = (0,0)); x0 [cm]; y0 [cm]", 400, -1, 1, 400, -1, 1);
    h_beamspot_vs_pmvtx = fs->make<TH2F>("beamspot_vs_pmvtx", "Beamspot - PrimaryVertex; x_{BS}-x_{PV} [cm]; y_{BS}-y_{PV} [cm]", 400, -1, 1, 400, -1, 1);
    h_vertex_pt = fs->make<TH1F>("vertex_pt", "Vertex pT; pT [GeV/c]; Vertices", 200, 0., 100.);
    h_vertex_eta = fs->make<TH1F>("vertex_eta", "Vertex eta; eta; Vertices", 200, -5., 5.);
    h_vertex_phi = fs->make<TH1F>("vertex_phi", "Vertex phi; phi; Vertices", 200, -3.14, 3.14);
    h_vertex_mass = fs->make<TH1F>("vertex_mass", "Vertex mass; mass [GeV/c^2]; Vertices", 200, 0., 10.);
    h_vertex_dBV = fs->make<TH1F>("vertex_dBV", "Vertex d_{BV} [cm]; Vertices / 0.05 cm", 200, 0, 10);
    h_vertex_dBV_error = fs->make<TH1F>("vertex_dBV_error", "Vertex d_{BV} Uncertainty; d_{BV} Uncertainty [cm]; Entries", 1000, 0, 0.1);
    h_nvertices_ntk = fs->make<TH1F>("nvertices_ntk", "Number of Candidate Vertices (ntk within cut); Number of Vertices; Events", 1000, 0, 1000);

    // Canonical dxy histograms (high resolution)
    h_track_dxy_00 = fs->make<TH1F>("track_dxy_00", "Track dxy w.r.t. global origin (0,0); dxy_{00} [cm]; Tracks", 1000, -5, 5);
    h_track_dxy_beamspot = fs->make<TH1F>("track_dxy_beamspot", "Track dxy w.r.t. beamspot (x0,y0); dxy_{BS} [cm]; Tracks", 1000, -5, 5);
    h_track_dxy_pmvtx = fs->make<TH1F>("track_dxy_pmvtx", "Track dxy w.r.t. primary vertex (pmvtx); dxy_{PV} [cm]; Tracks", 1000, -5, 5);
    h_track_dxyError = fs->make<TH1F>("track_dxyError", "Track dxy Uncertainty (applies to dxy_00, dxy_BS, dxy_PV); dxy Error [cm]; Tracks", 1000, 0, 0.1);
    h_track_dxySig_00 = fs->make<TH1F>("track_dxySig_00", "Track dxy significance |dxy_{00}/err|; |dxy_{00}/err|; Tracks", 200, 0, 50);
    h_track_dxySig_beamspot = fs->make<TH1F>("track_dxySig_beamspot", "Track dxy significance |dxy_{BS}/err|; |dxy_{BS}/err|; Tracks", 200, 0, 50);
    h_track_dxySig_pmvtx = fs->make<TH1F>("track_dxySig_pmvtx", "Track dxy significance |dxy_{PV}/err|; |dxy_{PV}/err|; Tracks", 200, 0, 50);
    h_track_dxyError_barrel = fs->make<TH1F>("track_dxyError_barrel", "Track dxy Uncertainty (Barrel); dxy Error [cm]; Tracks", 1000, 0, 0.1);
    h_track_dxyError_endcap = fs->make<TH1F>("track_dxyError_endcap", "Track dxy Uncertainty (Endcap); dxy Error [cm]; Tracks", 1000, 0, 0.1);

    // Barrel / endcap vertex histograms (kept)
    h_vertex_eta_barrel = fs->make<TH1F>("vertex_eta_barrel", "Vertex Eta (Barrel); eta; Vertices", 200, -3.0, 3.0);
    h_vertex_dBV_barrel = fs->make<TH1F>("vertex_dBV_barrel", "Vertex d_{BV} (Barrel); d_{BV} [cm]; Vertices", 200, 0, 10);
    h_vertex_mass_barrel = fs->make<TH1F>("vertex_mass_barrel", "Vertex Mass (Barrel); mass [GeV/c^{2}]; Vertices", 200, 0, 10);
    h_vertex_xy_barrel_global = fs->make<TH2F>("vertex_xy_barrel_global", "Vertex XY (Barrel, Global); X [cm]; Y [cm]", 2000, -10, 10, 2000, -10, 10);
    h_vertex_xy_barrel_beamspot = fs->make<TH2F>("vertex_xy_barrel_beamspot", "Vertex XY (Barrel, Beamspot-Centered); X-bs_x0 [cm]; Y-bs_y0 [cm]", 2000, -10, 10, 2000, -10, 10);

    h_vertex_eta_endcap = fs->make<TH1F>("vertex_eta_endcap", "Vertex Eta (Endcap); eta; Vertices", 200, -3.0, 3.0);
    h_vertex_dBV_endcap = fs->make<TH1F>("vertex_dBV_endcap", "Vertex d_{BV} (Endcap); d_{BV} [cm]; Vertices", 200, 0, 10);
    h_vertex_mass_endcap = fs->make<TH1F>("vertex_mass_endcap", "Vertex Mass (Endcap); mass [GeV/c^{2}]; Vertices", 200, 0, 10);
    h_vertex_xy_endcap_global = fs->make<TH2F>("vertex_xy_endcap_global", "Vertex XY (Endcap, Global); X [cm]; Y [cm]", 2000, -10, 10, 2000, -10, 10);
    h_vertex_xy_endcap_beamspot = fs->make<TH2F>("vertex_xy_endcap_beamspot", "Vertex XY (Endcap, Beamspot-Centered); X-bs_x0 [cm]; Y-bs_y0 [cm]", 2000, -10, 10, 2000, -10, 10);
}

void ScoutingTreeMakerRun3::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
    using namespace edm; using namespace std; using namespace reco;

    Handle<BeamSpot> beamspot;
    iEvent.getByToken(beamspot_token, beamspot);
    if(!beamspot.isValid()){
        LogError("ScoutingTreeMakerRun3") << "Beamspot handle invalid!";
        return;
    }

    Handle<vector<Vertex>> verticesH;
    iEvent.getByToken(verticesToken, verticesH);
    if(!verticesH.isValid()){
        LogError("ScoutingTreeMakerRun3") << "Vertex handle invalid!";
        return;
    }

    // Get primary vertices - make this optional for scouting data
    Handle<vector<Vertex>> primaryVerticesH;
    iEvent.getByToken(primaryVerticesToken, primaryVerticesH);
    bool havePrimaryVertices = primaryVerticesH.isValid() && !primaryVerticesH->empty();
    
    // Analyze primary vertices only if available
    int nPV = 0;
    math::XYZPoint avgPV(0, 0, 0);
    if (havePrimaryVertices) {
        nPV = primaryVerticesH->size();
        h_nPrimaryVertices->Fill(nPV);
        
        // Calculate average primary vertex position
        double sumX = 0, sumY = 0, sumZ = 0;
        int validPVs = 0;
        for (const auto& pv : *primaryVerticesH) {
            if (!pv.isFake() && pv.ndof() > 4) {
                sumX += pv.x();
                sumY += pv.y(); 
                sumZ += pv.z();
                validPVs++;
                h_primaryVertices_xy_global->Fill(pv.x(), pv.y());
            }
        }
        if (validPVs > 0) {
            avgPV = math::XYZPoint(sumX/validPVs, sumY/validPVs, sumZ/validPVs);
        }
        
        // Fill avgPV vs beamspot
        h_avg_primary_vertex_vs_beamspot->Fill(avgPV.x() - beamspot->x0(), avgPV.y() - beamspot->y0());
    } else {
        LogInfo("ScoutingTreeMakerRun3") << "Primary vertices not available - continuing with displaced vertex analysis";
        // Use beamspot as avgPV fallback
        avgPV = math::XYZPoint(beamspot->x0(), beamspot->y0(), beamspot->z0());
    }

    // Determine PV region (only if we have primary vertices)
    int pvRegion = 0; // Default to region A
    if (havePrimaryVertices && PVBoundary1 != -1) {
        if (nPV < PVBoundary1) {
            pvRegion = 0; // Region A
        } else if (PVBoundary2 != -1 && nPV < PVBoundary2) {
            pvRegion = 1; // Region B
        } else {
            pvRegion = 2; // Region C
        }
    }

    // Choose event primary vertex (pmvtx): vertex with largest summed track pT^2
    Vertex pmvtx; // default constructed; if no good vertex we'll use it as origin approximation
    bool havePmvtx = false;
    if (!verticesH->empty()) {
        double bestSumPt = -1.0;
        size_t bestIdx = 0;
        for (size_t iv = 0; iv < verticesH->size(); ++iv) {
            const Vertex &v = verticesH->at(iv);
            double sumPt = 0.0;
            for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
                reco::TrackRef tr = it->castTo<reco::TrackRef>();
                if (tr.isNonnull()) sumPt += tr->pt() * tr->pt();
            }
            if (sumPt > bestSumPt) { bestSumPt = sumPt; bestIdx = iv; }
        }
        if (bestSumPt >= 0) {
            pmvtx = verticesH->at(bestIdx);
            havePmvtx = true;
        }
    }

    vector<Vertex> selVertices;

    // fill beamspot diagnostics: global and relative to pmvtx (if present)
    h_beamspot_global->Fill(beamspot->x0(), beamspot->y0());
    if (havePmvtx) h_beamspot_vs_pmvtx->Fill(beamspot->x0() - pmvtx.x(), beamspot->y0() - pmvtx.y());
    else h_beamspot_vs_pmvtx->Fill(beamspot->x0(), beamspot->y0());

    for (unsigned int t = 0; t < verticesH->size(); t++) {
        Vertex v = verticesH->at(t);
        vector<TrackRef> tks = vertex_track_vec(v);
        int ntk = static_cast<int>(tks.size());

        TLorentzVector sumVec(0,0,0,0);
        double sum_dxy = 0.0, sum_dxyErr = 0.0;
        double sum_abs_dxy = 0.0;

        for(auto track : tks) {
            if(!track.isNonnull()) continue; // Safety: skip null track references
            TLorentzVector trackVec;
            // assumption: charged pion mass
            constexpr double kPionMass = 0.13957;
            trackVec.SetPtEtaPhiM(track->pt(), track->eta(), track->phi(), kPionMass);
            sumVec += trackVec;

            // compute canonical dxy values
            const math::XYZPoint origin(0.,0.,0.);
            double dxy_00 = track->dxy(origin);
            double dxy_beam = track->dxy(beamspot->position());
            double dxy_avgPV = track->dxy(avgPV);
            double dxy_pv = 0.0;
            if (havePmvtx) dxy_pv = track->dxy(pmvtx.position());
            double dxyErr = track->dxyError();

            // keep sum_dxy used for selection exactly as before (beamspot-based)
            sum_dxy += dxy_beam;
            sum_dxyErr += dxyErr;
            sum_abs_dxy += std::fabs(dxy_beam);

            // Fill canonical histograms
            h_track_dxy_00->Fill(dxy_00);
            h_track_dxy_beamspot->Fill(dxy_beam);
            h_track_dxy_pmvtx->Fill(dxy_pv);
            h_track_dxy_avgPV->Fill(dxy_avgPV);
            h_track_dxyError->Fill(dxyErr);

            // per-region dxy error fills
            if (std::fabs(track->eta()) < 1.0) {
                 h_track_dxyError_barrel->Fill(dxyErr);
             } else {
                 h_track_dxyError_endcap->Fill(dxyErr);
             }
            // fill significance histograms for each dxy definition (absolute value)
            if (dxyErr > 0.0) {
                h_track_dxySig_00->Fill(std::fabs(dxy_00 / dxyErr));
                h_track_dxySig_beamspot->Fill(std::fabs(dxy_beam / dxyErr));
                h_track_dxySig_pmvtx->Fill(std::fabs(dxy_pv / dxyErr));
                h_track_dxySig_avgPV->Fill(std::fabs(dxy_avgPV / dxyErr));
            }
        }

        double invMass = sumVec.M();
        double avg_dxy = (ntk > 0 ? sum_dxy/ntk : 0.0);
        double avg_dxyErr = (ntk > 0 ? sum_dxyErr/ntk : 0.0);

        // compute dBV = XY distance vertex-to-beamspot (explicit, robust)
        double dx = v.x() - beamspot->x0();
        double dy = v.y() - beamspot->y0();
        double dBV = std::hypot(dx, dy);

        // approximate uncertainty on dBV from vertex x/y uncertainties
        double dBV_err = std::sqrt(v.xError()*v.xError() + v.yError()*v.yError());

        // Apply selection cuts (unchanged behavior by default)
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

        // fill general vertex eta (using summed 4-vector)
        h_vertex_eta->Fill(sumVec.Eta());

        // Fill vertex histograms
        h_ntracks_global->Fill(ntk);
        h_vertex_xy_global->Fill(v.x(), v.y());
        h_vertex_xy_beamspot->Fill(v.x() - beamspot->x0(), v.y() - beamspot->y0());
        h_vertex_xy_avgPV->Fill(v.x() - avgPV.x(), v.y() - avgPV.y());
        h_vertex_dBV->Fill(dBV);
        h_vertex_dBV_error->Fill(dBV_err);

        double rad_global = std::hypot(v.x(), v.y());
        double rad_beam = std::hypot(v.x()-beamspot->x0(), v.y()-beamspot->y0());
        double rad_avgPV = std::hypot(v.x()-avgPV.x(), v.y()-avgPV.y());
        h_radial_distance_global->Fill(rad_global);
        h_radial_distance_beamspot->Fill(rad_beam);
        h_radial_distance_avgPV->Fill(rad_avgPV);

        // Fill PV region histograms (only if we have primary vertices)
        if (havePrimaryVertices && PVBoundary1 != -1) {
            if (pvRegion == 0) { // Region A
                h_vertex_xy_global_regionA->Fill(v.x(), v.y());
                h_vertex_xy_beamspot_regionA->Fill(v.x() - beamspot->x0(), v.y() - beamspot->y0());
                h_vertex_xy_avgPV_regionA->Fill(v.x() - avgPV.x(), v.y() - avgPV.y());
                h_vertex_dBV_regionA->Fill(dBV);
                h_vertex_mass_regionA->Fill(invMass);
                h_radial_distance_global_regionA->Fill(rad_global);
                h_radial_distance_beamspot_regionA->Fill(rad_beam);
                h_radial_distance_avgPV_regionA->Fill(rad_avgPV);
            } else if (pvRegion == 1) { // Region B
                h_vertex_xy_global_regionB->Fill(v.x(), v.y());
                h_vertex_xy_beamspot_regionB->Fill(v.x() - beamspot->x0(), v.y() - beamspot->y0());
                h_vertex_xy_avgPV_regionB->Fill(v.x() - avgPV.x(), v.y() - avgPV.y());
                h_vertex_dBV_regionB->Fill(dBV);
                h_vertex_mass_regionB->Fill(invMass);
                h_radial_distance_global_regionB->Fill(rad_global);
                h_radial_distance_beamspot_regionB->Fill(rad_beam);
                h_radial_distance_avgPV_regionB->Fill(rad_avgPV);
            } else { // Region C
                h_vertex_xy_global_regionC->Fill(v.x(), v.y());
                h_vertex_xy_beamspot_regionC->Fill(v.x() - beamspot->x0(), v.y() - beamspot->y0());
                h_vertex_xy_avgPV_regionC->Fill(v.x() - avgPV.x(), v.y() - avgPV.y());
                h_vertex_dBV_regionC->Fill(dBV);
                h_vertex_mass_regionC->Fill(invMass);
                h_radial_distance_global_regionC->Fill(rad_global);
                h_radial_distance_beamspot_regionC->Fill(rad_beam);
                h_radial_distance_avgPV_regionC->Fill(rad_avgPV);
            }
        }

        // Fill barrel/endcap histograms with left/right separation
        if(std::fabs(sumVec.Eta()) < 1.0) {
            h_vertex_eta_barrel->Fill(sumVec.Eta());
            h_vertex_dBV_barrel->Fill(dBV);
            h_vertex_mass_barrel->Fill(invMass);
            h_vertex_xy_barrel_global->Fill(v.x(), v.y());
            h_vertex_xy_barrel_beamspot->Fill(v.x() - beamspot->x0(), v.y() - beamspot->y0());
            h_vertex_xy_barrel_avgPV->Fill(v.x() - avgPV.x(), v.y() - avgPV.y());
        } else {
            h_vertex_eta_endcap->Fill(sumVec.Eta());
            h_vertex_dBV_endcap->Fill(dBV);
            h_vertex_mass_endcap->Fill(invMass);
            h_vertex_xy_endcap_global->Fill(v.x(), v.y());
            h_vertex_xy_endcap_beamspot->Fill(v.x() - beamspot->x0(), v.y() - beamspot->y0());
            h_vertex_xy_endcap_avgPV->Fill(v.x() - avgPV.x(), v.y() - avgPV.y());
            
            // Left and right endcap separation
            if (sumVec.Eta() < -1.0) { // Left endcap
                h_vertex_eta_left_endcap->Fill(sumVec.Eta());
                h_vertex_dBV_left_endcap->Fill(dBV);
                h_vertex_mass_left_endcap->Fill(invMass);
                h_vertex_xy_left_endcap_global->Fill(v.x(), v.y());
                h_vertex_xy_left_endcap_beamspot->Fill(v.x() - beamspot->x0(), v.y() - beamspot->y0());
            } else if (sumVec.Eta() > 1.0) { // Right endcap
                h_vertex_eta_right_endcap->Fill(sumVec.Eta());
                h_vertex_dBV_right_endcap->Fill(dBV);
                h_vertex_mass_right_endcap->Fill(invMass);
                h_vertex_xy_right_endcap_global->Fill(v.x(), v.y());
                h_vertex_xy_right_endcap_beamspot->Fill(v.x() - beamspot->x0(), v.y() - beamspot->y0());
            }
        }
        
        h_vertex_pt->Fill(sumVec.Pt());
        h_vertex_phi->Fill(sumVec.Phi());
        h_vertex_mass->Fill(invMass);

        // Per-track legacy fills (momentum, eta, phi)
        for(auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
            reco::TrackRef track = it->castTo<reco::TrackRef>();
            if(!track.isNonnull()){
                LogWarning("ScoutingTreeMakerRun3") << "Null track reference in vertex " << t;
                continue;
            }
            h_track_momenta_global->Fill(track->p());
            h_eta_distribution_global->Fill(track->eta());
            h_phi_distribution_global->Fill(track->phi());
        }
    }

    h_nvertices_ntk->Fill(static_cast<double>(selVertices.size()));
}

void ScoutingTreeMakerRun3::endJob() {
    // With TFileService the histograms get written automatically.
    // Avoid calling Draw() or Write() here to be safe in multithreaded contexts.
}

void ScoutingTreeMakerRun3::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    desc.add<int>("required_ntk_min", -1);
    desc.add<int>("required_ntk_max", -1);
    desc.add<double>("required_invmass", -1.0);
    desc.add<double>("required_chi2", -1.0);
    desc.add<double>("required_dBV_min", -1.0);
    desc.add<double>("required_dBV_max", -1.0);
    desc.add<double>("required_dxy_min", -1.0);
    desc.add<double>("required_dxy_max", -1.0);
    desc.add<double>("required_dBV_error", -1.0);
    desc.add<double>("required_dxy_error", -1.0);
    desc.add<int>("PVBoundary1", -1);
    desc.add<int>("PVBoundary2", -1);
    desc.add<edm::InputTag>("displacedVertices", edm::InputTag("displacedVertices"));
    desc.add<edm::InputTag>("beamspot_src", edm::InputTag("offlineBeamSpot"));
    desc.add<edm::InputTag>("tracks", edm::InputTag("hltScoutingUnpackProducer", "Track"));
    desc.add<edm::InputTag>("primaryVertices", edm::InputTag("hltScoutingPrimaryVertexPacker", "primaryVtx"));
    descriptions.add("scoutingTreeMakerRun3", desc);
}

DEFINE_FWK_MODULE(ScoutingTreeMakerRun3);