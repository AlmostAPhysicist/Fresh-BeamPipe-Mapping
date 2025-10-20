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
#include <sstream>  // added for dynamic region title construction

#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TTree.h"
#include "TLorentzVector.h"
#include "TMath.h"
// Use ROOT TVector3 for opening-angle computations
#include "TVector3.h"

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
#include "TrackingTools/TransientTrack/interface/TransientTrack.h"
#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "TrackingTools/IPTools/interface/IPTools.h"

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

    enum class RefPreference { PreferPV, PreferBeamSpot };
    const RefPreference refPreference_;

    // Input tokens
    const edm::EDGetTokenT<std::vector<reco::Vertex>> verticesToken;
    const edm::EDGetTokenT<reco::BeamSpot> beamspot_token;
    const edm::EDGetTokenT<std::vector<reco::Track>> tracksToken;
    const edm::EDGetTokenT<std::vector<reco::Vertex>> primaryVerticesToken;

    // ----- Histograms organized by category -----
    
    // Primary vertex and beamspot histograms
    TH1F* h_nPrimaryVertices;
    TH2F* h_primaryVertices_xy_global;
    TH2F* h_beamspot_global;
    TH2F* h_avg_primary_vertex_vs_beamspot;
    
    // Vertex position histograms
    TH2F* h_vertex_xy_global;
    TH2F* h_vertex_xy_ref; // Reference-centered
    
    // Vertex kinematic histograms
    TH1F* h_vertex_pt;
    TH1F* h_vertex_eta;
    TH1F* h_vertex_phi;
    TH1F* h_vertex_mass;
    TH1F* h_ntracks_global;

    // --- NEW: normalized chi^2 histograms (all vertices & selected vertices) ---
    TH1F* h_vertex_chi2norm_all;
    TH1F* h_vertex_chi2norm_selected;

    // --- NEW: Track opening-angle histograms ---
    TH1F* h_track_opening_angle_pair; // pairwise opening angles (all vertices)
    TH1F* h_vertex_openingAngle_mean; // per-vertex mean opening angle (selected vertices)
    TH1F* h_vertex_openingAngle_min;  // per-vertex min opening angle (selected vertices)
    TH1F* h_vertex_openingAngle_max;  // per-vertex max opening angle (selected vertices)
    
    // Vertex distance histograms (all reference frames)
    TH1F* h_vertex_dBV00;       // wrt (0,0)
    TH1F* h_vertex_dBVref;      // wrt selected reference
    TH1F* h_vertex_dBVbs;       // wrt beamspot
    TH1F* h_vertex_dBVavgPV;    // wrt avgPV
    TH1F* h_vertex_dBV_error;   // distance uncertainty
    
    // Track measurement histograms (organized by reference point)
    struct TrackMeasurements {
        TH1F* dxy;        // Impact parameter
        TH1F* dxySig;     // Impact parameter significance
    };
    
    // All track measurements by reference point
    TrackMeasurements track_origin;    // wrt (0,0)
    TrackMeasurements track_ref;       // wrt selected reference
    TrackMeasurements track_bs;        // wrt beamspot
    TrackMeasurements track_avgPV;     // wrt avgPV
    TrackMeasurements track_pmvtx;     // wrt primary vertex
    
    // Track error histograms
    TH1F* h_track_dxyError;
    TH1F* h_track_dxyError_barrel;
    TH1F* h_track_dxyError_endcap;
    
    // Track property histograms
    TH1F* h_track_momenta_global;
    TH1F* h_eta_distribution_global;
    TH1F* h_phi_distribution_global;
    
    // Barrel/Endcap histograms
    struct BarrelEndcapHistos {
        TH1F* eta;
        TH1F* dBV;
        TH1F* mass;
        TH2F* xy_global;
        TH2F* xy_ref;
    };
    
    BarrelEndcapHistos barrel;
    BarrelEndcapHistos endcap;
    
    // Endcap side-specific histograms
    struct EndcapSideHistos {
        TH1F* eta;
        TH1F* dBV;
        TH1F* mass;
        TH2F* xy_global;
    };
    
    EndcapSideHistos left_endcap;
    EndcapSideHistos right_endcap;
    
    // Region-specific histograms (PV count regions)
    struct RegionHistos {
        TH2F* xy_global;
        TH2F* xy_ref;
        TH1F* dBV;
        TH1F* mass;
    };
    
    RegionHistos region_A;
    RegionHistos region_B;
    RegionHistos region_C;
    
    // Summary histograms
    TH1F* h_nvertices_ntk;

    // Fix: ESGetTokenT -> ESGetToken
    edm::ESGetToken<TransientTrackBuilder, TransientTrackRecord> ttBuilderToken_;

    // Helper methods
    typedef std::set<reco::TrackRef> track_set;
    typedef std::vector<reco::TrackRef> track_vec;
    track_set vertex_track_set(const reco::Vertex & v, const double min_weight = 0.5) const;
    track_vec vertex_track_vec(const reco::Vertex & v, const double min_weight = 0.5) const;
    
    std::pair<bool, std::string> determineReferenceVertex(
        const edm::Event& iEvent, 
        reco::Vertex& refVtx,
        bool& havePV, 
        reco::Vertex& avgPVVtx,
        bool& haveBS,
        reco::Vertex& bsVtx);
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
    refPreference_(iConfig.getUntrackedParameter<std::string>("refPreference", "BeamSpot") == "PV" ? 
                  RefPreference::PreferPV : RefPreference::PreferBeamSpot),
    verticesToken(consumes<std::vector<reco::Vertex>>(iConfig.getParameter<edm::InputTag>("displacedVertices"))),
    beamspot_token(consumes<reco::BeamSpot>(iConfig.getParameter<edm::InputTag>("beamspot_src"))),
    tracksToken(consumes<std::vector<reco::Track>>(iConfig.getParameter<edm::InputTag>("tracks"))),
    primaryVerticesToken(consumes<std::vector<reco::Vertex>>(iConfig.getParameter<edm::InputTag>("primaryVertices"))),
    ttBuilderToken_(esConsumes(edm::ESInputTag("", "TransientTrackBuilder")))
{
    usesResource("TFileService");
}

ScoutingTreeMakerRun3::~ScoutingTreeMakerRun3() {
    // no explicit cleanup required; histograms owned by TFileService
}

void ScoutingTreeMakerRun3::beginJob() {
    edm::Service<TFileService> fs;
    
    // Primary vertex & beamspot histograms
    h_nPrimaryVertices = fs->make<TH1F>("nPrimaryVertices","Number of Primary Vertices; Number of Primary Vertices; Events",100,0,100);
    h_primaryVertices_xy_global = fs->make<TH2F>("primaryVertices_xy_global","Primary Vertices XY Position (Global); X [cm]; Y [cm]",400,-1,1,400,-1,1);
    h_beamspot_global = fs->make<TH2F>("beamspot_global","Beamspot Position (Global ref = (0,0)); x0 [cm]; y0 [cm]",400,-1,1,400,-1,1);
    h_avg_primary_vertex_vs_beamspot = fs->make<TH2F>("avg_primary_vertex_vs_beamspot","Average Primary Vertex - Beamspot; x_{avgPV}-x_{BS} [cm]; y_{avgPV}-y_{BS} [cm]",400,-1,1,400,-1,1);

    // Vertex position histograms
    h_vertex_xy_global = fs->make<TH2F>("vertex_xy_global","Vertex XY Position (Global); X [cm]; Y [cm]",2000,-10,10,2000,-10,10);
    h_vertex_xy_ref = fs->make<TH2F>("vertex_xy_ref","Vertex XY Position (ref-centered); X-ref_x [cm]; Y-ref_y [cm]",2000,-10,10,2000,-10,10);

    // Vertex kinematic histograms
    h_vertex_pt = fs->make<TH1F>("vertex_pt","Vertex pT; pT [GeV/c]; Vertices",200,0.,100.);
    h_vertex_eta = fs->make<TH1F>("vertex_eta","Vertex eta; eta; Vertices",200,-5.,5.);
    h_vertex_phi = fs->make<TH1F>("vertex_phi","Vertex phi; phi; Vertices",200,-3.14,3.14);
    h_vertex_mass = fs->make<TH1F>("vertex_mass","Vertex mass; mass [GeV/c^2]; Vertices",200,0.,10.);
    h_ntracks_global = fs->make<TH1F>("ntracks_global","Number of Tracks; Number of Tracks; Vertices",200,0,100);

    // --- NEW: create normalized chi^2 histograms (all vs selected) ---
    h_vertex_chi2norm_all = fs->make<TH1F>("vertex_chi2norm_all","Vertex normalized #chi^{2} (all); normalized #chi^{2}; Vertices",200,0.,20.);
    h_vertex_chi2norm_selected = fs->make<TH1F>("vertex_chi2norm_selected","Vertex normalized #chi^{2} (selected); normalized #chi^{2}; Selected Vertices",200,0.,20.);

    // --- NEW: Opening-angle histograms (angles in radians: 0..pi) ---
    h_track_opening_angle_pair = fs->make<TH1F>("track_opening_angle_pair","Track opening angle (pairwise); Opening angle [rad]; Pairs",180,0.,3.141592653589793);
    h_vertex_openingAngle_mean = fs->make<TH1F>("vertex_openingAngle_mean","Vertex mean opening angle; mean opening angle [rad]; Vertices",180,0.,3.141592653589793);
    h_vertex_openingAngle_min  = fs->make<TH1F>("vertex_openingAngle_min","Vertex min opening angle; min opening angle [rad]; Vertices",180,0.,3.141592653589793);
    h_vertex_openingAngle_max  = fs->make<TH1F>("vertex_openingAngle_max","Vertex max opening angle; max opening angle [rad]; Vertices",180,0.,3.141592653589793);

    // Distance histograms
    h_vertex_dBV00 = fs->make<TH1F>("vertex_dBV00","Vertex transverse distance d_{BV}^{00} (wrt (0,0)); d_{BV}^{00} [cm]; Vertices / 0.05 cm",200,0,10);
    h_vertex_dBVref = fs->make<TH1F>("vertex_dBVref","Vertex transverse distance (wrt reference); d_{BV} [cm]; Vertices / 0.05 cm",200,0,10);
    h_vertex_dBVbs = fs->make<TH1F>("vertex_dBVbs","Vertex transverse distance (wrt BeamSpot); d_{BV}^{BS} [cm]; Vertices / 0.05 cm",200,0,10);
    h_vertex_dBVavgPV = fs->make<TH1F>("vertex_dBVavgPV","Vertex transverse distance (wrt avgPV); d_{BV}^{avgPV} [cm]; Vertices / 0.05 cm",200,0,10);
    h_vertex_dBV_error = fs->make<TH1F>("vertex_dBV_error_avgPV","Vertex d_{BV}^{avgPV} Uncertainty; d_{BV}^{avgPV} Unc [cm]; Entries",1000,0,0.1);

    // Track measurements by reference point
    // Origin (0,0)
    track_origin.dxy = fs->make<TH1F>("track_dxy_00","Track dxy w.r.t. global origin (0,0); dxy_{00} [cm]; Tracks",1000,-5,5);
    track_origin.dxySig = fs->make<TH1F>("track_dxySig_00","Track dxy significance |dxy_{00}/err|; |dxy_{00}/err|; Tracks",200,0,50);
    
    // Reference (will be updated at runtime)
    track_ref.dxy = fs->make<TH1F>("track_dxy_ref","Track dxy w.r.t. reference; dxy [cm]; Tracks",1000,-5,5);
    track_ref.dxySig = fs->make<TH1F>("track_dxySig_ref","Track |dxy/err| w.r.t. reference; |dxy/err|; Tracks",200,0,50);
    
    // BeamSpot
    track_bs.dxy = fs->make<TH1F>("track_dxy_bs","Track dxy w.r.t. BeamSpot; dxy_{BS} [cm]; Tracks",1000,-5,5);
    track_bs.dxySig = fs->make<TH1F>("track_dxySig_bs","Track |dxy_{BS}/err|; |dxy_{BS}/err|; Tracks",200,0,50);
    
    // Average PV
    track_avgPV.dxy = fs->make<TH1F>("track_dxy_avgPV","Track dxy w.r.t. avgPV; dxy_{avgPV} [cm]; Tracks",1000,-5,5);
    track_avgPV.dxySig = fs->make<TH1F>("track_dxySig_avgPV","Track |dxy_{avgPV}/err|; |dxy_{avgPV}/err|; Tracks",200,0,50);
    
    // Primary vertex
    track_pmvtx.dxy = fs->make<TH1F>("track_dxy_pmvtx","Track dxy w.r.t. primary vertex (pmvtx); dxy_{PV} [cm]; Tracks",1000,-5,5);
    track_pmvtx.dxySig = fs->make<TH1F>("track_dxySig_pmvtx","Track dxy significance |dxy_{PV}/err|; |dxy_{PV}/err|; Tracks",200,0,50);

    // Track error histograms
    h_track_dxyError = fs->make<TH1F>("track_dxyError","Track dxy Uncertainty; dxy Error [cm]; Tracks",1000,0,0.1);
    h_track_dxyError_barrel = fs->make<TH1F>("track_dxyError_barrel","Track dxy Uncertainty (Barrel); dxy Error [cm]; Tracks",1000,0,0.1);
    h_track_dxyError_endcap = fs->make<TH1F>("track_dxyError_endcap","Track dxy Uncertainty (Endcap); dxy Error [cm]; Tracks",1000,0,0.1);
    
    // Track property histograms
    h_track_momenta_global = fs->make<TH1F>("track_momenta_global","Track Momenta; Momentum [GeV/c]; Tracks",400,0,100);
    h_eta_distribution_global = fs->make<TH1F>("eta_distribution_global","Eta Distribution; Eta; Tracks",400,-3,3);
    h_phi_distribution_global = fs->make<TH1F>("phi_distribution_global","Phi Distribution; Phi; Tracks",400,-3.142,3.142);

    // Barrel histograms
    barrel.eta = fs->make<TH1F>("vertex_eta_barrel","Vertex Eta (Barrel); eta; Vertices",200,-3.0,3.0);
    barrel.dBV = fs->make<TH1F>("vertex_dBV_barrel_avgPV","Vertex d_{BV}^{avgPV} (Barrel); d_{BV}^{avgPV} [cm]; Vertices",200,0,10);
    barrel.mass = fs->make<TH1F>("vertex_mass_barrel","Vertex Mass (Barrel); mass [GeV/c^{2}]; Vertices",200,0,10);
    barrel.xy_global = fs->make<TH2F>("vertex_xy_barrel_global","Vertex XY (Barrel, Global); X [cm]; Y [cm]",2000,-10,10,2000,-10,10);
    barrel.xy_ref = fs->make<TH2F>("vertex_xy_barrel_avgPV","Vertex XY (Barrel, AvgPV-Centered); X-avgPV_x [cm]; Y-avgPV_y [cm]",2000,-10,10,2000,-10,10);

    // Endcap histograms
    endcap.eta = fs->make<TH1F>("vertex_eta_endcap","Vertex Eta (Endcap); eta; Vertices",200,-3.0,3.0);
    endcap.dBV = fs->make<TH1F>("vertex_dBV_endcap_avgPV","Vertex d_{BV}^{avgPV} (Endcap); d_{BV}^{avgPV} [cm]; Vertices",200,0,10);
    endcap.mass = fs->make<TH1F>("vertex_mass_endcap","Vertex Mass (Endcap); mass [GeV/c^{2}]; Vertices",200,0,10);
    endcap.xy_global = fs->make<TH2F>("vertex_xy_endcap_global","Vertex XY (Endcap, Global); X [cm]; Y [cm]",2000,-10,10,2000,-10,10);
    endcap.xy_ref = fs->make<TH2F>("vertex_xy_endcap_avgPV","Vertex XY (Endcap, AvgPV-Centered); X-avgPV_x [cm]; Y-avgPV_y [cm]",2000,-10,10,2000,-10,10);

    // Left endcap histograms
    left_endcap.eta = fs->make<TH1F>("vertex_eta_left_endcap","Vertex Eta (Left Endcap); eta; Vertices",200,-3.0,3.0);
    left_endcap.dBV = fs->make<TH1F>("vertex_dBV_left_endcap_avgPV","Vertex d_{BV}^{avgPV} (Left Endcap); d_{BV}^{avgPV} [cm]; Vertices",200,0,10);
    left_endcap.mass = fs->make<TH1F>("vertex_mass_left_endcap","Vertex Mass (Left Endcap); mass [GeV/c^{2}]; Vertices",200,0,10);
    left_endcap.xy_global = fs->make<TH2F>("vertex_xy_left_endcap_global","Vertex XY (Left Endcap, Global); X [cm]; Y [cm]",2000,-10,10,2000,-10,10);

    // Right endcap histograms
    right_endcap.eta = fs->make<TH1F>("vertex_eta_right_endcap","Vertex Eta (Right Endcap); eta; Vertices",200,-3.0,3.0);
    right_endcap.dBV = fs->make<TH1F>("vertex_dBV_right_endcap_avgPV","Vertex d_{BV}^{avgPV} (Right Endcap); d_{BV}^{avgPV} [cm]; Vertices",200,0,10);
    right_endcap.mass = fs->make<TH1F>("vertex_mass_right_endcap","Vertex Mass (Right Endcap); mass [GeV/c^{2}]; Vertices",200,0,10);
    right_endcap.xy_global = fs->make<TH2F>("vertex_xy_right_endcap_global","Vertex XY (Right Endcap, Global); X [cm]; Y [cm]",2000,-10,10,2000,-10,10);

    // Region histograms (PV count regions)
    if (PVBoundary1 != -1) {
        std::ostringstream regAlabel, regBlabel, regClabel;
        regAlabel << "Region A (0 <= nPV < " << PVBoundary1 << ")";
        if (PVBoundary2 != -1) {
            regBlabel << "Region B (" << PVBoundary1 << " <= nPV < " << PVBoundary2 << ")";
            regClabel << "Region C (nPV >= " << PVBoundary2 << ")";
        } else {
            regBlabel << "Region B (nPV >= " << PVBoundary1 << ")";
            regClabel << "Region C (nPV >= " << PVBoundary1 << ")";
        }

        // Region A - use consistent naming with reference
        region_A.xy_global = fs->make<TH2F>("vertex_xy_global_regionA",
            ("Vertex XY (Global, " + regAlabel.str() + "); X [cm]; Y [cm]").c_str(), 2000,-10,10,2000,-10,10);
        region_A.xy_ref = fs->make<TH2F>("vertex_xy_ref_regionA",
            ("Vertex XY (ref-centered, " + regAlabel.str() + "); X-ref_x [cm]; Y-ref_y [cm]").c_str(), 2000,-10,10,2000,-10,10);
        region_A.dBV = fs->make<TH1F>("vertex_dBVref_regionA",
            ("Vertex d_{BV} (wrt ref, " + regAlabel.str() + "); d_{BV} [cm]; Vertices").c_str(), 200,0,10);
        region_A.mass = fs->make<TH1F>("vertex_mass_regionA",
            ("Vertex Mass (" + regAlabel.str() + "); mass [GeV/c^{2}]; Vertices").c_str(), 200,0,10);

        // Region B
        region_B.xy_global = fs->make<TH2F>("vertex_xy_global_regionB",
            ("Vertex XY (Global, " + regBlabel.str() + "); X [cm]; Y [cm]").c_str(), 2000,-10,10,2000,-10,10);
        region_B.xy_ref = fs->make<TH2F>("vertex_xy_ref_regionB",
            ("Vertex XY (ref-centered, " + regBlabel.str() + "); X-ref_x [cm]; Y-ref_y [cm]").c_str(), 2000,-10,10,2000,-10,10);
        region_B.dBV = fs->make<TH1F>("vertex_dBVref_regionB",
            ("Vertex d_{BV} (wrt ref, " + regBlabel.str() + "); d_{BV} [cm]; Vertices").c_str(), 200,0,10);
        region_B.mass = fs->make<TH1F>("vertex_mass_regionB",
            ("Vertex Mass (" + regBlabel.str() + "); mass [GeV/c^{2}]; Vertices").c_str(), 200,0,10);

        // Region C
        region_C.xy_global = fs->make<TH2F>("vertex_xy_global_regionC",
            ("Vertex XY (Global, " + regClabel.str() + "); X [cm]; Y [cm]").c_str(), 2000,-10,10,2000,-10,10);
        region_C.xy_ref = fs->make<TH2F>("vertex_xy_ref_regionC",
            ("Vertex XY (ref-centered, " + regClabel.str() + "); X-ref_x [cm]; Y-ref_y [cm]").c_str(), 2000,-10,10,2000,-10,10);
        region_C.dBV = fs->make<TH1F>("vertex_dBVref_regionC",
            ("Vertex d_{BV} (wrt ref, " + regClabel.str() + "); d_{BV} [cm]; Vertices").c_str(), 200,0,10);
        region_C.mass = fs->make<TH1F>("vertex_mass_regionC",
            ("Vertex Mass (" + regClabel.str() + "); mass [GeV/c^{2}]; Vertices").c_str(), 200,0,10);
    }

    // Summary histograms
    h_nvertices_ntk = fs->make<TH1F>("nvertices_ntk","Number of Candidate Vertices (ntk within cut); Number of Vertices; Events",1000,0,1000);
}

void ScoutingTreeMakerRun3::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
    using namespace edm; using namespace std; using namespace reco;

    // NEW: ensure track collection (referenced by Vertex TrackRefs) is present to avoid segfault
    Handle<vector<Track>> tracksH;
    iEvent.getByToken(tracksToken, tracksH);
    if (!tracksH.isValid()) {
        LogWarning("ScoutingTreeMakerRun3") << "Track collection invalid (skip event to avoid dangling TrackRefs).";
        return;
    }

    // Displaced vertices
    Handle<vector<Vertex>> verticesH;
    iEvent.getByToken(verticesToken, verticesH);
    if(!verticesH.isValid()){
        LogWarning("ScoutingTreeMakerRun3") << "Displaced vertex collection invalid (skip event).";
        return;
    }

    // Get builder for transient tracks
    auto const& ttBuilder = iSetup.getData(ttBuilderToken_);
    
    // Create proper reference vertices using CMSSW methods
    reco::Vertex refVtx, avgPVVtx, bsVtx;
    bool havePV = false, haveBS = false;
    auto [haveRef, refType] = determineReferenceVertex(iEvent, refVtx, havePV, avgPVVtx, haveBS, bsVtx);
    
    if (!haveRef) {
        LogWarning("ScoutingTreeMakerRun3") << "No valid reference (avgPV or BeamSpot). Skipping event.";
        return;
    }
    
    // Use proper instance for 2D vertex distance calculations
    VertexDistanceXY vertexDist2D;
    
    // Update histogram titles with actual reference type
    std::string refName = (refType == "avgPV" || refType == "avgPV (fallback)") ? "avgPV" : "BS";
    h_vertex_xy_ref->SetTitle(("Vertex XY Position (wrt " + refName + "); X-" + refName + "_x [cm]; Y-" + refName + "_y [cm]").c_str());
    track_ref.dxy->SetTitle(("Track dxy w.r.t. " + refName + "; dxy_{" + refName + "} [cm]; Tracks").c_str());
    track_ref.dxySig->SetTitle(("Track |dxy_{" + refName + "}/err|; |dxy_{" + refName + "}/err|; Tracks").c_str());
    h_vertex_dBVref->SetTitle(("Vertex transverse distance d_{BV}^{" + refName + "} (wrt " + refName + "); d_{BV}^{" + refName + "} [cm]; Vertices / 0.05 cm").c_str());
    
    // Update region plot titles too
    if (PVBoundary1 != -1) {
        region_A.xy_ref->SetTitle(("Vertex XY (" + refName + "-centered, Region A); X-" + refName + "_x [cm]; Y-" + refName + "_y [cm]").c_str());
        region_B.xy_ref->SetTitle(("Vertex XY (" + refName + "-centered, Region B); X-" + refName + "_x [cm]; Y-" + refName + "_y [cm]").c_str());
        region_C.xy_ref->SetTitle(("Vertex XY (" + refName + "-centered, Region C); X-" + refName + "_x [cm]; Y-" + refName + "_y [cm]").c_str());
        
        region_A.dBV->SetTitle(("Vertex d_{BV}^{" + refName + "} (Region A); d_{BV}^{" + refName + "} [cm]; Vertices").c_str());
        region_B.dBV->SetTitle(("Vertex d_{BV}^{" + refName + "} (Region B); d_{BV}^{" + refName + "} [cm]; Vertices").c_str());
        region_C.dBV->SetTitle(("Vertex d_{BV}^{" + refName + "} (Region C); d_{BV}^{" + refName + "} [cm]; Vertices").c_str());
    }

    // Fill beamspot comparisons if both available
    if (havePV && haveBS) {
        h_avg_primary_vertex_vs_beamspot->Fill(avgPVVtx.x() - bsVtx.x(), avgPVVtx.y() - bsVtx.y());
    }

    // PV region classification
    int pvRegion = 0;
    int nPV = 0;
    if (PVBoundary1 != -1 && havePV) {
        Handle<vector<Vertex>> primaryVerticesH;
        iEvent.getByToken(primaryVerticesToken, primaryVerticesH);
        nPV = primaryVerticesH->size();
        h_nPrimaryVertices->Fill(nPV);
        
        if (nPV < PVBoundary1) pvRegion = 0;
        else if (PVBoundary2 != -1 && nPV < PVBoundary2) pvRegion = 1;
        else pvRegion = 2;
    }

    // Event "primary" displaced vertex (sum pT^2)
    const Vertex* pmvtx = nullptr;
    if (!verticesH->empty()) {
        double bestSumPt = -1.0; 
        size_t bestIdx = 0;
        for (size_t iv = 0; iv < verticesH->size(); ++iv) {
            const auto& vtx = verticesH->at(iv);
            double sumPt = 0.0;
            for (auto it = vtx.tracks_begin(); it != vtx.tracks_end(); ++it) {
                TrackRef tr = it->castTo<TrackRef>();
                if (tr.isNonnull()) sumPt += tr->pt() * tr->pt();
            }
            if (sumPt > bestSumPt) { bestSumPt = sumPt; bestIdx = iv; }
        }
        if (bestSumPt >= 0) pmvtx = &verticesH->at(bestIdx);
    }

    // Count selected vertices
    int nSelVertices = 0;

    // Process each displaced vertex
    for (unsigned int t = 0; t < verticesH->size(); ++t) {
        const auto& v = verticesH->at(t);
        vector<TrackRef> tks = vertex_track_vec(v);
        int ntk = static_cast<int>(tks.size());

        // --- NEW: compute pairwise opening angles and per-vertex stats ---
        double meanAngle = 0.0, minAngle = 0.0, maxAngle = 0.0;
        {
            double sumAngles = 0.0;
            int npairs = 0;
            minAngle = 1e9; maxAngle = 0.0;
            for (int i = 0; i < ntk; ++i) {
                const auto& ti = tks[i];
                if (!ti.isNonnull()) continue;
                TVector3 vi(ti->px(), ti->py(), ti->pz());
                if (vi.Mag2() <= 0) continue;
                for (int j = i+1; j < ntk; ++j) {
                    const auto& tj = tks[j];
                    if (!tj.isNonnull()) continue;
                    TVector3 vj(tj->px(), tj->py(), tj->pz());
                    if (vj.Mag2() <= 0) continue;
                    const double angle = vi.Angle(vj); // ROOT TVector3 handles normalization and acos internally
                    h_track_opening_angle_pair->Fill(angle); // per-pair, all vertices
                    sumAngles += angle;
                    ++npairs;
                    if (angle < minAngle) minAngle = angle;
                    if (angle > maxAngle) maxAngle = angle;
                }
            }
            meanAngle = (npairs > 0) ? (sumAngles / npairs) : 0.0;
            if (npairs == 0) { minAngle = 0.0; maxAngle = 0.0; }
        }
        // --- END NEW ---

        // Create 4-vector for vertex mass calculation
        TLorentzVector sumVec(0,0,0,0);
        double sum_dxy = 0.0, sum_dxyErr = 0.0;

        // --- NEW: fill chi2 (all vertices) before selection ---
        h_vertex_chi2norm_all->Fill(v.normalizedChi2());
        // --- END NEW ---

        // Process tracks using proper methods
        for(auto track : tks) {
            if(!track.isNonnull()) continue;
            
            // Build track 4-vector with pion mass assumption (standard approach)
            TLorentzVector trackVec;
            constexpr double kPionMass = 0.13957;
            trackVec.SetPtEtaPhiM(track->pt(), track->eta(), track->phi(), kPionMass);
            sumVec += trackVec;  // Sum 4-vectors properly for mass

            // Create transient track for proper IP calculations
            reco::TransientTrack transientTrack = ttBuilder.build(track);
            
            // Calculate track distances to all reference points using IPTools
            const math::XYZPoint origin(0.,0.,0.);
            reco::Vertex originVtx(origin, reco::Vertex::Error());
            
            // Define direction for signed impact parameter (track momentum direction)
            GlobalVector direction(track->px(), track->py(), track->pz());
            
            // Use IPTools::signedTransverseImpactParameter with correct arguments
            std::pair<bool, Measurement1D> ip_00 = IPTools::signedTransverseImpactParameter(transientTrack, direction, originVtx);
            std::pair<bool, Measurement1D> ip_ref = IPTools::signedTransverseImpactParameter(transientTrack, direction, refVtx);
            double dxyErr = track->dxyError();
            
            // Track metrics for the main reference
            if (ip_ref.first) {
                sum_dxy += ip_ref.second.value();
                sum_dxyErr += ip_ref.second.error();
                
                // Fill histograms with signed dxy
                track_ref.dxy->Fill(ip_ref.second.value());
                if (ip_ref.second.error() > 0) {
                    track_ref.dxySig->Fill(std::fabs(ip_ref.second.significance()));
                }
            }
            
            // Fill simple track.dxy() for origin (historical comparison)
            track_origin.dxy->Fill(track->dxy(origin));  // This is already signed
            if (dxyErr > 0.0) {
                track_origin.dxySig->Fill(std::fabs(track->dxy(origin) / dxyErr));
            }
            
            // Additional explicit measurements for all reference points
            if (havePV) {
                std::pair<bool, Measurement1D> ip_avgPV = IPTools::signedTransverseImpactParameter(transientTrack, direction, avgPVVtx);
                if (ip_avgPV.first) {
                    track_avgPV.dxy->Fill(ip_avgPV.second.value());
                    track_avgPV.dxySig->Fill(std::fabs(ip_avgPV.second.significance()));
                }
            }
            
            if (haveBS) {
                std::pair<bool, Measurement1D> ip_bs = IPTools::signedTransverseImpactParameter(transientTrack, direction, bsVtx);
                if (ip_bs.first) {
                    track_bs.dxy->Fill(ip_bs.second.value());
                    track_bs.dxySig->Fill(std::fabs(ip_bs.second.significance()));
                }
            }
            
            if (pmvtx) {
                std::pair<bool, Measurement1D> ip_pmvtx = IPTools::signedTransverseImpactParameter(transientTrack, direction, *pmvtx);
                if (ip_pmvtx.first) {
                    track_pmvtx.dxy->Fill(ip_pmvtx.second.value());
                    track_pmvtx.dxySig->Fill(std::fabs(ip_pmvtx.second.significance()));
                }
            }
            
            h_track_dxyError->Fill(dxyErr);
            if (std::fabs(track->eta()) < 1.0) h_track_dxyError_barrel->Fill(dxyErr);
            else                               h_track_dxyError_endcap->Fill(dxyErr);
        }

        // Calculate vertex properties
        double invMass = sumVec.M();  // Using correct relativistic mass from 4-vector
        double avg_dxy = (ntk > 0 ? sum_dxy / ntk : 0.0);
        double avg_dxyErr = (ntk > 0 ? sum_dxyErr / ntk : 0.0);

        // Calculate vertex distances using VertexDistanceXY for proper error propagation
        Measurement1D dBVref_meas = vertexDist2D.distance(v, refVtx);
        double dBVref = dBVref_meas.value();
        double dBV_err = dBVref_meas.error();
        
        // Calculate origin distance using same method for consistency
        Vertex originVtx(Vertex::Point(0,0,0), Vertex::Error());
        Measurement1D dBV00_meas = vertexDist2D.distance(v, originVtx);
        double dBV00 = dBV00_meas.value();
        
        // Calculate distances to all reference points with proper error propagation
        if (havePV) {
            Measurement1D dBVavgPV_meas = vertexDist2D.distance(v, avgPVVtx);
            h_vertex_dBVavgPV->Fill(dBVavgPV_meas.value());
        }
        
        if (haveBS) {
            Measurement1D dBVbs_meas = vertexDist2D.distance(v, bsVtx);
            h_vertex_dBVbs->Fill(dBVbs_meas.value());
        }

        // Apply all selection criteria
        if(required_ntk_min != -1 && ntk < required_ntk_min) continue;
        if(required_ntk_max != -1 && ntk > required_ntk_max) continue;
        if(required_invmass  != -1 && invMass < required_invmass) continue;
        if(required_chi2     != -1 && v.normalizedChi2() > required_chi2) continue;
        if(required_dBV_min  != -1 && dBVref < required_dBV_min) continue;
        if(required_dBV_max  != -1 && dBVref > required_dBV_max) continue;
        if(required_dxy_min  != -1 && avg_dxy < required_dxy_min) continue;
        if(required_dxy_max  != -1 && avg_dxy > required_dxy_max) continue;
        if(required_dBV_error!= -1 && dBV_err > required_dBV_error) continue;
        if(required_dxy_error!= -1 && avg_dxyErr > required_dxy_error) continue;

        // Vertex accepted - fill histograms
        ++nSelVertices;

        // --- NEW: fill chi2 (selected vertices) ---
        h_vertex_chi2norm_selected->Fill(v.normalizedChi2());
        // --- END NEW ---

        // Fill basic vertex histograms
        h_ntracks_global->Fill(ntk);
        h_vertex_xy_global->Fill(v.x(), v.y());
        h_vertex_xy_ref->Fill(v.x() - refVtx.x(), v.y() - refVtx.y());

        // Fill distance histograms
        h_vertex_dBVref->Fill(dBVref);
        h_vertex_dBV00->Fill(dBV00);
        h_vertex_dBV_error->Fill(dBV_err);

        // --- NEW: fill per-vertex opening-angle summaries for accepted vertices ---
        h_vertex_openingAngle_mean->Fill(meanAngle);
        h_vertex_openingAngle_min->Fill(minAngle);
        h_vertex_openingAngle_max->Fill(maxAngle);
        // --- END NEW ---

        // Region histograms - use the correct reference names
        if (PVBoundary1 != -1) {
            if (pvRegion == 0) {
                region_A.xy_global->Fill(v.x(), v.y());
                region_A.xy_ref->Fill(v.x() - refVtx.x(), v.y() - refVtx.y());
                region_A.dBV->Fill(dBVref);
                region_A.mass->Fill(invMass);
            } else if (pvRegion == 1) {
                region_B.xy_global->Fill(v.x(), v.y());
                region_B.xy_ref->Fill(v.x() - refVtx.x(), v.y() - refVtx.y());
                region_B.dBV->Fill(dBVref);
                region_B.mass->Fill(invMass);
            } else {
                region_C.xy_global->Fill(v.x(), v.y());
                region_C.xy_ref->Fill(v.x() - refVtx.x(), v.y() - refVtx.y());
                region_C.dBV->Fill(dBVref);
                region_C.mass->Fill(invMass);
            }
        }

        // Barrel / Endcap (reference-based)
        if (std::fabs(sumVec.Eta()) < 1.0) {
            barrel.eta->Fill(sumVec.Eta());
            barrel.dBV->Fill(dBVref);
            barrel.mass->Fill(invMass);
            barrel.xy_global->Fill(v.x(), v.y());
            barrel.xy_ref->Fill(v.x() - refVtx.x(), v.y() - refVtx.y());
        } else {
            endcap.eta->Fill(sumVec.Eta());
            endcap.dBV->Fill(dBVref);
            endcap.mass->Fill(invMass);
            endcap.xy_global->Fill(v.x(), v.y());
            endcap.xy_ref->Fill(v.x() - refVtx.x(), v.y() - refVtx.y());

            if (sumVec.Eta() < -1.0) {
                left_endcap.eta->Fill(sumVec.Eta());
                left_endcap.dBV->Fill(dBVref);
                left_endcap.mass->Fill(invMass);
                left_endcap.xy_global->Fill(v.x(), v.y());
            } else if (sumVec.Eta() > 1.0) {
                right_endcap.eta->Fill(sumVec.Eta());
                right_endcap.dBV->Fill(dBVref);
                right_endcap.mass->Fill(invMass);
                right_endcap.xy_global->Fill(v.x(), v.y());
            }
        }

        // Basic kinematic histograms
        h_vertex_pt->Fill(sumVec.Pt());
        h_vertex_eta->Fill(sumVec.Eta());
        h_vertex_phi->Fill(sumVec.Phi());
        h_vertex_mass->Fill(invMass);

        // Track histograms
        for(auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
            TrackRef track = it->castTo<TrackRef>();
            if(!track.isNonnull()){
                LogWarning("ScoutingTreeMakerRun3") << "Null track reference in vertex " << t;
                continue;
            }
            h_track_momenta_global->Fill(track->p());
            h_eta_distribution_global->Fill(track->eta());
            h_phi_distribution_global->Fill(track->phi());
        }
    }

    h_nvertices_ntk->Fill(static_cast<double>(nSelVertices));
}

// Helper method to determine the reference vertex based on preference and availability
std::pair<bool, std::string> ScoutingTreeMakerRun3::determineReferenceVertex(
    const edm::Event& iEvent, 
    reco::Vertex& refVtx,
    bool& havePV, 
    reco::Vertex& avgPVVtx,
    bool& haveBS,
    reco::Vertex& bsVtx) {
    
    // Default return - reference not found
    bool foundRef = false;
    std::string refType = "none";
    havePV = false;
    haveBS = false;
    
    // Try to get primary vertices and compute average
    edm::Handle<std::vector<reco::Vertex>> primaryVerticesH;
    iEvent.getByToken(primaryVerticesToken, primaryVerticesH);
    if (primaryVerticesH.isValid() && !primaryVerticesH->empty()) {
        double sumX=0, sumY=0, sumZ=0;
        reco::Vertex::Error avgError = reco::Vertex::Error();
        int validPVs=0;
        
        for (const auto& pv : *primaryVerticesH) {
            if (!pv.isFake() && pv.ndof() > 4) {
                sumX += pv.x(); sumY += pv.y(); sumZ += pv.z();
                // Accumulate covariance matrices properly
                for (int i = 0; i < 3; ++i) {
                    for (int j = i; j < 3; ++j) {
                        avgError(i,j) += pv.covariance(i,j);
                    }
                }
                ++validPVs;
                h_primaryVertices_xy_global->Fill(pv.x(), pv.y());
            }
        }
        
        if (validPVs > 0) {
            reco::Vertex::Point avgPos(sumX/validPVs, sumY/validPVs, sumZ/validPVs);
            // Properly propagate errors for the average
            for (int i = 0; i < 3; ++i) {
                for (int j = i; j < 3; ++j) {
                    avgError(i,j) /= validPVs*validPVs; // Proper error propagation for average
                }
            }
            avgPVVtx = reco::Vertex(avgPos, avgError);
            havePV = true;
            edm::LogInfo("ScoutingTreeMakerRun3") << "Found average PV from " << validPVs << " valid vertices with propagated covariance";
        }
    }
    
    // Try to get beamspot
    edm::Handle<reco::BeamSpot> beamspot;
    iEvent.getByToken(beamspot_token, beamspot);
    if (beamspot.isValid()) {
        // Properly create a fake vertex with full beamspot covariance
        bsVtx = reco::Vertex(beamspot->position(), beamspot->covariance3D());
        h_beamspot_global->Fill(beamspot->x0(), beamspot->y0());
        haveBS = true;
        edm::LogInfo("ScoutingTreeMakerRun3") << "Found valid beamspot at (" 
            << beamspot->x0() << ", " << beamspot->y0() << ", " << beamspot->z0() << ")";
    }
    
    // Select reference based on preference and availability
    if (refPreference_ == RefPreference::PreferPV && havePV) {
        refVtx = avgPVVtx;
        foundRef = true;
        refType = "avgPV";
        edm::LogInfo("ScoutingTreeMakerRun3") << "Using avgPV as reference point";
    } else if (refPreference_ == RefPreference::PreferBeamSpot && haveBS) {
        refVtx = bsVtx;
        foundRef = true;
        refType = "beamspot";
        edm::LogInfo("ScoutingTreeMakerRun3") << "Using beamspot as reference point";
    } else if (havePV) {
        // Fallback to avgPV
        refVtx = avgPVVtx;
        foundRef = true;
        refType = "avgPV (fallback)";
        edm::LogInfo("ScoutingTreeMakerRun3") << "Using avgPV as fallback reference point";
    } else if (haveBS) {
        // Fallback to beamspot
        refVtx = bsVtx;
        foundRef = true;
        refType = "beamspot (fallback)";
        edm::LogInfo("ScoutingTreeMakerRun3") << "Using beamspot as fallback reference point";
    }
    
    return {foundRef, refType};
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
    desc.addUntracked<std::string>("refPreference", "BeamSpot"); // Default to BeamSpot
    descriptions.add("scoutingTreeMakerRun3", desc);
}

// Add these helper method implementations after the constructor/destructor but before analyze()

// Implementation of vertex_track_set helper
ScoutingTreeMakerRun3::track_set ScoutingTreeMakerRun3::vertex_track_set(const reco::Vertex& v, const double min_weight) const {
    track_set result;
    for (auto it = v.tracks_begin(), ite = v.tracks_end(); it != ite; ++it) {
        const double w = v.trackWeight(*it);
        if (w >= min_weight) {
            result.insert(it->castTo<reco::TrackRef>());
        }
    }
    return result;
}

// Implementation of vertex_track_vec helper
ScoutingTreeMakerRun3::track_vec ScoutingTreeMakerRun3::vertex_track_vec(const reco::Vertex& v, const double min_weight) const {
    track_set s = vertex_track_set(v, min_weight);
    return track_vec(s.begin(), s.end());
}

// Make sure the module is registered with the framework correctly
// This should be at the bottom of the file after all class implementations
DEFINE_FWK_MODULE(ScoutingTreeMakerRun3);