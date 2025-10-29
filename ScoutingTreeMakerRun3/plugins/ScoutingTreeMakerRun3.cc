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
#include "DataFormats/TrackReco/interface/HitPattern.h"

#include "RecoVertex/VertexTools/interface/VertexDistanceXY.h"
#include "RecoVertex/VertexTools/interface/VertexDistance3D.h"
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

    // ----- Reorganized Histogram Structure -----
    
    // Event-level histograms
    struct EventHistos {
        TH1F* nPrimaryVertices;
        TH1F* nSelectedVertices;
        TH2F* primaryVertices_xy;
        TH2F* beamspot_xy;
        TH2F* avgPV_vs_beamspot;
    } event_;

    // Vertex histograms organized by category
    struct VertexHistos {
        // All vertices (before cuts)
        struct {
            TH1F* chi2norm;
            TH1F* nTracks;
        } all;
        
        // Selected vertices (after cuts)
        struct {
            TH1F* chi2norm;
            TH1F* pt;
            TH1F* eta;
            TH1F* phi;
            TH1F* mass;
            TH1F* nTracks;
            TH2F* xy_global;
            TH2F* xy_ref;
            
            // Distance measurements
            struct {
                TH1F* dBV_origin;
                TH1F* dBV_ref;
                TH1F* dBV_beamspot;
                TH1F* dBV_avgPV;
                TH1F* dBV_error;
            } distance;
            
            // Opening angles
            struct {
                TH1F* pairwise;
                TH1F* mean;
                TH1F* min;
                TH1F* max;
            } openingAngle;
            
            // Topology-based
            struct {
                TH1F* eta;
                TH1F* mass;
                TH1F* dBV;
                TH2F* xy_global;
                TH2F* xy_ref;
            } barrel, endcap, leftEndcap, rightEndcap;
            
            // PV-region based
            struct {
                TH1F* mass;
                TH1F* dBV;
                TH2F* xy_global;
                TH2F* xy_ref;
            } regionA, regionB, regionC;
        } selected;
    } vertices_;

    // Track histograms organized by category
    struct TrackHistos {
        // All tracks
        struct {
            TH1F* pt;
            TH1F* eta;
            TH1F* phi;
            TH1F* momentum;
            
            struct {
                TH1F* ipSig_ref;
                TH1F* dxy_origin;
                TH1F* dxySig_origin;
                TH1F* dxy_ref;
                TH1F* dxySig_ref;
                TH1F* dxy_beamspot;
                TH1F* dxySig_beamspot;
                TH1F* dxy_avgPV;
                TH1F* dxySig_avgPV;
                TH1F* dxyError;
                TH1F* dxyError_barrel;
                TH1F* dxyError_endcap;
            } ip;
        } all;
        
        // Seed-like tracks (passing Vertexer cuts)
        struct {
            TH1F* pt;
            TH1F* eta;
            TH1F* phi;
            TH1F* ipSig_ref;
        } seed;
        
        // Vertex-associated tracks
        struct {
            TH1F* pt;
            TH1F* eta;
            TH1F* phi;
            TH1F* momentum;
            TH1F* ipSig_ref;
            TH1F* ipSig_vtx;
            TH1F* dxy_primaryVtx;
            TH1F* dxySig_primaryVtx;
        } vertex;
    } tracks_;

    // Fix: ESGetTokenT -> ESGetToken
    edm::ESGetToken<TransientTrackBuilder, TransientTrackRecord> ttBuilderToken_;

    // --- NEW: seed-like selection controls (to mirror Vertexer) ---
    const double seed_minIPSig_;
    const double seed_minPt_;
    const bool   seed_use2DTrackDist_;
    // NEW: mirror Vertexer’s vertex-distance choice (2D vs 3D) for dBV plots
    const bool   use_2d_vertex_dist_;
    // NEW: complete Vertexer-like cuts (max IP and hit/layer thresholds)
    const double seed_maxIPSig_;
    const int    seed_minPixelHits_;
    const int    seed_minStripHits_;
    const int    seed_minTrackerLayers_;

    // --- NEW: debug histograms for IP significance populations ---
    // A) All tracks
    TH1F* h_allTracks_ipSig_ref;
    TH1F* h_allTracks_simpleDxySig;
    TH1F* h_allTracks_pt;            // NEW
    // B) Tracks associated to vertices
    TH1F* h_vertexTracks_ipSig_ref;
    TH1F* h_vertexTracks_ipSig_vtx;
    TH1F* h_vertexTracks_pt;         // NEW
    // C) Tracks that pass Vertexer cuts (aka “seed-like”)
    TH1F* h_seedTracks_ipSig_ref;
    TH1F* h_seedTracks_pt;
    TH1F* h_seedTracks_eta;          // NEW
    TH1F* h_seedTracks_phi;          // NEW

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
    // Fix member init order: ttBuilderToken_ is declared before seed_* in the class
    ttBuilderToken_(esConsumes(edm::ESInputTag("", "TransientTrackBuilder"))),
    // --- Harmonize seed/IP thresholds with Vertexer naming; fall back to legacy analyzer names ---
    seed_minIPSig_( iConfig.existsAs<double>("minSeedIPSig", true) ? 
                    iConfig.getUntrackedParameter<double>("minSeedIPSig") :
                    iConfig.getUntrackedParameter<double>("seed_minIPSig", 4.0) ),
    seed_minPt_(    iConfig.existsAs<double>("minSeedPt", true) ? 
                    iConfig.getUntrackedParameter<double>("minSeedPt") :
                    iConfig.getUntrackedParameter<double>("seed_minPt", 0.9) ),
    // Use the same 2D/3D toggle as Vertexer if provided; otherwise fall back to the analyzer param
    seed_use2DTrackDist_( iConfig.existsAs<bool>("use_2d_track_dist", true) ?
                          iConfig.getParameter<bool>("use_2d_track_dist") :
                          iConfig.getUntrackedParameter<bool>("seed_use2DTrackDist", false) ),
    use_2d_vertex_dist_( iConfig.existsAs<bool>("use_2d_vertex_dist", true) ?
                         iConfig.getParameter<bool>("use_2d_vertex_dist") :
                         false ),
    // NEW: thresholds to fully mirror Vertexer’s seed selection
    seed_maxIPSig_(        iConfig.getUntrackedParameter<double>("seed_maxIPSig", 1e9) ),
    seed_minPixelHits_(    iConfig.getUntrackedParameter<int>("seed_minPixelHits", 0) ),
    seed_minStripHits_(    iConfig.getUntrackedParameter<int>("seed_minStripHits", 0) ),
    seed_minTrackerLayers_(iConfig.getUntrackedParameter<int>("seed_minTrackerLayers", 0) )
{
    usesResource("TFileService");
}

ScoutingTreeMakerRun3::~ScoutingTreeMakerRun3() {
    // no explicit cleanup required; histograms owned by TFileService
}

void ScoutingTreeMakerRun3::beginJob() {
    edm::Service<TFileService> fs;
    
    // ==================== EVENT LEVEL ====================
    TFileDirectory eventDir = fs->mkdir("Event");
    event_.nPrimaryVertices = eventDir.make<TH1F>("nPrimaryVertices","Number of Primary Vertices; nPV; Events",100,0,100);
    event_.nSelectedVertices = eventDir.make<TH1F>("nSelectedVertices","Number of Selected Vertices; N_{vtx}; Events",1000,0,1000);
    event_.primaryVertices_xy = eventDir.make<TH2F>("primaryVertices_xy","Primary Vertices XY; X [cm]; Y [cm]",400,-1,1,400,-1,1);
    event_.beamspot_xy = eventDir.make<TH2F>("beamspot_xy","Beamspot Position; x_{BS} [cm]; y_{BS} [cm]",400,-1,1,400,-1,1);
    event_.avgPV_vs_beamspot = eventDir.make<TH2F>("avgPV_vs_beamspot","AvgPV - Beamspot; #Delta x [cm]; #Delta y [cm]",400,-1,1,400,-1,1);

    // ==================== VERTICES ====================
    TFileDirectory verticesDir = fs->mkdir("Vertices");
    
    // All vertices (before selection)
    TFileDirectory vtxAllDir = verticesDir.mkdir("All");
    vertices_.all.chi2norm = vtxAllDir.make<TH1F>("chi2norm","Vertex #chi^{2}/ndof (all); #chi^{2}/ndof; Vertices",200,0,20);
    vertices_.all.nTracks = vtxAllDir.make<TH1F>("nTracks","Number of Tracks (all); N_{tracks}; Vertices",200,0,100);
    
    // Selected vertices
    TFileDirectory vtxSelDir = verticesDir.mkdir("Selected");
    
    // Basic kinematics
    TFileDirectory vtxKinDir = vtxSelDir.mkdir("Kinematics");
    vertices_.selected.chi2norm = vtxKinDir.make<TH1F>("chi2norm","Vertex #chi^{2}/ndof; #chi^{2}/ndof; Vertices",200,0,20);
    vertices_.selected.pt = vtxKinDir.make<TH1F>("pt","Vertex p_{T}; p_{T} [GeV]; Vertices",200,0,100);
    vertices_.selected.eta = vtxKinDir.make<TH1F>("eta","Vertex #eta; #eta; Vertices",200,-5,5);
    vertices_.selected.phi = vtxKinDir.make<TH1F>("phi","Vertex #phi; #phi; Vertices",200,-3.14,3.14);
    vertices_.selected.mass = vtxKinDir.make<TH1F>("mass","Vertex Mass; Mass [GeV]; Vertices",200,0,10);
    vertices_.selected.nTracks = vtxKinDir.make<TH1F>("nTracks","Number of Tracks; N_{tracks}; Vertices",200,0,100);
    
    // Spatial
    TFileDirectory vtxSpatialDir = vtxSelDir.mkdir("Spatial");
    vertices_.selected.xy_global = vtxSpatialDir.make<TH2F>("xy_global","Vertex XY (Global); X [cm]; Y [cm]",2000,-10,10,2000,-10,10);
    vertices_.selected.xy_ref = vtxSpatialDir.make<TH2F>("xy_ref","Vertex XY (ref-centered); X-X_{ref} [cm]; Y-Y_{ref} [cm]",2000,-10,10,2000,-10,10);
    
    // Distance measurements
    TFileDirectory vtxDistDir = vtxSelDir.mkdir("Distance");
    vertices_.selected.distance.dBV_origin = vtxDistDir.make<TH1F>("dBV_origin","d_{BV} wrt (0,0); d_{BV} [cm]; Vertices",200,0,10);
    vertices_.selected.distance.dBV_ref = vtxDistDir.make<TH1F>("dBV_ref","d_{BV} wrt reference; d_{BV} [cm]; Vertices",200,0,10);
    vertices_.selected.distance.dBV_beamspot = vtxDistDir.make<TH1F>("dBV_beamspot","d_{BV} wrt beamspot; d_{BV} [cm]; Vertices",200,0,10);
    vertices_.selected.distance.dBV_avgPV = vtxDistDir.make<TH1F>("dBV_avgPV","d_{BV} wrt avgPV; d_{BV} [cm]; Vertices",200,0,10);
    vertices_.selected.distance.dBV_error = vtxDistDir.make<TH1F>("dBV_error","d_{BV} Uncertainty; #sigma_{dBV} [cm]; Vertices",1000,0,0.1);
    
    // Opening angles
    TFileDirectory vtxAngleDir = vtxSelDir.mkdir("OpeningAngles");
    vertices_.selected.openingAngle.pairwise = vtxAngleDir.make<TH1F>("pairwise","Track Opening Angle (pairwise); Angle [rad]; Pairs",180,0,3.14159);
    vertices_.selected.openingAngle.mean = vtxAngleDir.make<TH1F>("mean","Mean Opening Angle; <Angle> [rad]; Vertices",180,0,3.14159);
    vertices_.selected.openingAngle.min = vtxAngleDir.make<TH1F>("min","Min Opening Angle; Min Angle [rad]; Vertices",180,0,3.14159);
    vertices_.selected.openingAngle.max = vtxAngleDir.make<TH1F>("max","Max Opening Angle; Max Angle [rad]; Vertices",180,0,3.14159);
    
    // Topology subdivisions
    TFileDirectory vtxTopoDir = vtxSelDir.mkdir("Topology");
    
    TFileDirectory barrelDir = vtxTopoDir.mkdir("Barrel");
    vertices_.selected.barrel.eta = barrelDir.make<TH1F>("eta","#eta (Barrel); #eta; Vertices",200,-3,3);
    vertices_.selected.barrel.mass = barrelDir.make<TH1F>("mass","Mass (Barrel); Mass [GeV]; Vertices",200,0,10);
    vertices_.selected.barrel.dBV = barrelDir.make<TH1F>("dBV","d_{BV} (Barrel); d_{BV} [cm]; Vertices",200,0,10);
    vertices_.selected.barrel.xy_global = barrelDir.make<TH2F>("xy_global","XY (Barrel, Global); X [cm]; Y [cm]",2000,-10,10,2000,-10,10);
    vertices_.selected.barrel.xy_ref = barrelDir.make<TH2F>("xy_ref","XY (Barrel, ref-centered); X-X_{ref} [cm]; Y-Y_{ref} [cm]",2000,-10,10,2000,-10,10);
    
    TFileDirectory endcapDir = vtxTopoDir.mkdir("Endcap");
    vertices_.selected.endcap.eta = endcapDir.make<TH1F>("eta","#eta (Endcap); #eta; Vertices",200,-3,3);
    vertices_.selected.endcap.mass = endcapDir.make<TH1F>("mass","Mass (Endcap); Mass [GeV]; Vertices",200,0,10);
    vertices_.selected.endcap.dBV = endcapDir.make<TH1F>("dBV","d_{BV} (Endcap); d_{BV} [cm]; Vertices",200,0,10);
    vertices_.selected.endcap.xy_global = endcapDir.make<TH2F>("xy_global","XY (Endcap, Global); X [cm]; Y [cm]",2000,-10,10,2000,-10,10);
    vertices_.selected.endcap.xy_ref = endcapDir.make<TH2F>("xy_ref","XY (Endcap, ref-centered); X-X_{ref} [cm]; Y-Y_{ref} [cm]",2000,-10,10,2000,-10,10);
    
    TFileDirectory leftEndcapDir = vtxTopoDir.mkdir("LeftEndcap");
    vertices_.selected.leftEndcap.eta = leftEndcapDir.make<TH1F>("eta","#eta (Left Endcap); #eta; Vertices",200,-3,3);
    vertices_.selected.leftEndcap.mass = leftEndcapDir.make<TH1F>("mass","Mass (Left Endcap); Mass [GeV]; Vertices",200,0,10);
    vertices_.selected.leftEndcap.dBV = leftEndcapDir.make<TH1F>("dBV","d_{BV} (Left Endcap); d_{BV} [cm]; Vertices",200,0,10);
    vertices_.selected.leftEndcap.xy_global = leftEndcapDir.make<TH2F>("xy_global","XY (Left Endcap); X [cm]; Y [cm]",2000,-10,10,2000,-10,10);
    
    TFileDirectory rightEndcapDir = vtxTopoDir.mkdir("RightEndcap");
    vertices_.selected.rightEndcap.eta = rightEndcapDir.make<TH1F>("eta","#eta (Right Endcap); #eta; Vertices",200,-3,3);
    vertices_.selected.rightEndcap.mass = rightEndcapDir.make<TH1F>("mass","Mass (Right Endcap); Mass [GeV]; Vertices",200,0,10);
    vertices_.selected.rightEndcap.dBV = rightEndcapDir.make<TH1F>("dBV","d_{BV} (Right Endcap); d_{BV} [cm]; Vertices",200,0,10);
    vertices_.selected.rightEndcap.xy_global = rightEndcapDir.make<TH2F>("xy_global","XY (Right Endcap); X [cm]; Y [cm]",2000,-10,10,2000,-10,10);
    
    // PV regions
    if (PVBoundary1 != -1) {
        TFileDirectory vtxRegionDir = vtxSelDir.mkdir("PVRegions");
        
        std::ostringstream regAlabel, regBlabel, regClabel;
        regAlabel << "Region A (0 <= nPV < " << PVBoundary1 << ")";
        if (PVBoundary2 != -1) {
            regBlabel << "Region B (" << PVBoundary1 << " <= nPV < " << PVBoundary2 << ")";
            regClabel << "Region C (nPV >= " << PVBoundary2 << ")";
        } else {
            regBlabel << "Region B (nPV >= " << PVBoundary1 << ")";
            regClabel << "Region C (nPV >= " << PVBoundary1 << ")";
        }
        
        TFileDirectory regADir = vtxRegionDir.mkdir("RegionA");
        vertices_.selected.regionA.mass = regADir.make<TH1F>("mass",("Mass " + regAlabel.str() + "; Mass [GeV]; Vertices").c_str(),200,0,10);
        vertices_.selected.regionA.dBV = regADir.make<TH1F>("dBV",("d_{BV} " + regAlabel.str() + "; d_{BV} [cm]; Vertices").c_str(),200,0,10);
        vertices_.selected.regionA.xy_global = regADir.make<TH2F>("xy_global",("XY Global " + regAlabel.str() + "; X [cm]; Y [cm]").c_str(),2000,-10,10,2000,-10,10);
        vertices_.selected.regionA.xy_ref = regADir.make<TH2F>("xy_ref",("XY ref-centered " + regAlabel.str() + "; X-X_{ref} [cm]; Y-Y_{ref} [cm]").c_str(),2000,-10,10,2000,-10,10);
        
        TFileDirectory regBDir = vtxRegionDir.mkdir("RegionB");
        vertices_.selected.regionB.mass = regBDir.make<TH1F>("mass",("Mass " + regBlabel.str() + "; Mass [GeV]; Vertices").c_str(),200,0,10);
        vertices_.selected.regionB.dBV = regBDir.make<TH1F>("dBV",("d_{BV} " + regBlabel.str() + "; d_{BV} [cm]; Vertices").c_str(),200,0,10);
        vertices_.selected.regionB.xy_global = regBDir.make<TH2F>("xy_global",("XY Global " + regBlabel.str() + "; X [cm]; Y [cm]").c_str(),2000,-10,10,2000,-10,10);
        vertices_.selected.regionB.xy_ref = regBDir.make<TH2F>("xy_ref",("XY ref-centered " + regBlabel.str() + "; X-X_{ref} [cm]; Y-Y_{ref} [cm]").c_str(),2000,-10,10,2000,-10,10);
        
        TFileDirectory regCDir = vtxRegionDir.mkdir("RegionC");
        vertices_.selected.regionC.mass = regCDir.make<TH1F>("mass",("Mass " + regClabel.str() + "; Mass [GeV]; Vertices").c_str(),200,0,10);
        vertices_.selected.regionC.dBV = regCDir.make<TH1F>("dBV",("d_{BV} " + regClabel.str() + "; d_{BV} [cm]; Vertices").c_str(),200,0,10);
        vertices_.selected.regionC.xy_global = regCDir.make<TH2F>("xy_global",("XY Global " + regClabel.str() + "; X [cm]; Y [cm]").c_str(),2000,-10,10,2000,-10,10);
        vertices_.selected.regionC.xy_ref = regCDir.make<TH2F>("xy_ref",("XY ref-centered " + regClabel.str() + "; X-X_{ref} [cm]; Y-Y_{ref} [cm]").c_str(),2000,-10,10,2000,-10,10);
    }

    // ==================== TRACKS ====================
    TFileDirectory tracksDir = fs->mkdir("Tracks");
    
    // All tracks
    TFileDirectory trkAllDir = tracksDir.mkdir("All");
    TFileDirectory trkAllKinDir = trkAllDir.mkdir("Kinematics");
    tracks_.all.pt = trkAllKinDir.make<TH1F>("pt","Track p_{T} (all); p_{T} [GeV]; Tracks",200,0,100);
    tracks_.all.eta = trkAllKinDir.make<TH1F>("eta","Track #eta (all); #eta; Tracks",400,-3,3);
    tracks_.all.phi = trkAllKinDir.make<TH1F>("phi","Track #phi (all); #phi; Tracks",400,-3.14,3.14);
    tracks_.all.momentum = trkAllKinDir.make<TH1F>("momentum","Track Momentum (all); p [GeV]; Tracks",400,0,100);
    
    TFileDirectory trkAllIPDir = trkAllDir.mkdir("ImpactParameter");
    tracks_.all.ip.ipSig_ref = trkAllIPDir.make<TH1F>("ipSig_ref","|IP|/err wrt ref (all); |IP|/err; Tracks",200,0,50);
    tracks_.all.ip.dxy_origin = trkAllIPDir.make<TH1F>("dxy_origin","dxy wrt (0,0) (all); dxy [cm]; Tracks",1000,-5,5);
    tracks_.all.ip.dxySig_origin = trkAllIPDir.make<TH1F>("dxySig_origin","|dxy|/err wrt (0,0) (all); |dxy|/err; Tracks",200,0,50);
    tracks_.all.ip.dxy_ref = trkAllIPDir.make<TH1F>("dxy_ref","dxy wrt ref (all); dxy [cm]; Tracks",1000,-5,5);
    tracks_.all.ip.dxySig_ref = trkAllIPDir.make<TH1F>("dxySig_ref","|dxy|/err wrt ref (all); |dxy|/err; Tracks",200,0,50);
    tracks_.all.ip.dxy_beamspot = trkAllIPDir.make<TH1F>("dxy_beamspot","dxy wrt BS (all); dxy [cm]; Tracks",1000,-5,5);
    tracks_.all.ip.dxySig_beamspot = trkAllIPDir.make<TH1F>("dxySig_beamspot","|dxy|/err wrt BS (all); |dxy|/err; Tracks",200,0,50);
    tracks_.all.ip.dxy_avgPV = trkAllIPDir.make<TH1F>("dxy_avgPV","dxy wrt avgPV (all); dxy [cm]; Tracks",1000,-5,5);
    tracks_.all.ip.dxySig_avgPV = trkAllIPDir.make<TH1F>("dxySig_avgPV","|dxy|/err wrt avgPV (all); |dxy|/err; Tracks",200,0,50);
    tracks_.all.ip.dxyError = trkAllIPDir.make<TH1F>("dxyError","dxy Error (all); #sigma_{dxy} [cm]; Tracks",1000,0,0.1);
    tracks_.all.ip.dxyError_barrel = trkAllIPDir.make<TH1F>("dxyError_barrel","dxy Error Barrel (all); #sigma_{dxy} [cm]; Tracks",1000,0,0.1);
    tracks_.all.ip.dxyError_endcap = trkAllIPDir.make<TH1F>("dxyError_endcap","dxy Error Endcap (all); #sigma_{dxy} [cm]; Tracks",1000,0,0.1);
    
    // Seed-like tracks
    TFileDirectory trkSeedDir = tracksDir.mkdir("SeedLike");
    tracks_.seed.pt = trkSeedDir.make<TH1F>("pt","Track p_{T} (seed-like); p_{T} [GeV]; Tracks",200,0,100);
    tracks_.seed.eta = trkSeedDir.make<TH1F>("eta","Track #eta (seed-like); #eta; Tracks",200,-3,3);
    tracks_.seed.phi = trkSeedDir.make<TH1F>("phi","Track #phi (seed-like); #phi; Tracks",200,-3.14,3.14);
    tracks_.seed.ipSig_ref = trkSeedDir.make<TH1F>("ipSig_ref","|IP|/err wrt ref (seed-like); |IP|/err; Tracks",200,0,50);
    
    // Vertex-associated tracks
    TFileDirectory trkVtxDir = tracksDir.mkdir("VertexAssociated");
    TFileDirectory trkVtxKinDir = trkVtxDir.mkdir("Kinematics");
    tracks_.vertex.pt = trkVtxKinDir.make<TH1F>("pt","Track p_{T} (vertex); p_{T} [GeV]; Tracks",200,0,100);
    tracks_.vertex.eta = trkVtxKinDir.make<TH1F>("eta","Track #eta (vertex); #eta; Tracks",400,-3,3);
    tracks_.vertex.phi = trkVtxKinDir.make<TH1F>("phi","Track #phi (vertex); #phi; Tracks",400,-3.14,3.14);
    tracks_.vertex.momentum = trkVtxKinDir.make<TH1F>("momentum","Track Momentum (vertex); p [GeV]; Tracks",400,0,100);
    
    TFileDirectory trkVtxIPDir = trkVtxDir.mkdir("ImpactParameter");
    tracks_.vertex.ipSig_ref = trkVtxIPDir.make<TH1F>("ipSig_ref","|IP|/err wrt ref (vertex); |IP|/err; Tracks",200,0,50);
    tracks_.vertex.ipSig_vtx = trkVtxIPDir.make<TH1F>("ipSig_vtx","|IP|/err wrt vertex (vertex); |IP|/err; Tracks",200,0,50);
    tracks_.vertex.dxy_primaryVtx = trkVtxIPDir.make<TH1F>("dxy_primaryVtx","dxy wrt primary vertex; dxy [cm]; Tracks",1000,-5,5);
    tracks_.vertex.dxySig_primaryVtx = trkVtxIPDir.make<TH1F>("dxySig_primaryVtx","|dxy|/err wrt primary vertex; |dxy|/err; Tracks",200,0,50);
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
    
    // Use proper instances for vertex distance calculations (2D/3D match Vertexer)
    VertexDistanceXY vertexDist2D;
    VertexDistance3D vertexDist3D;

    // Update histogram titles with actual reference type
    std::string refName = (refType == "avgPV" || refType == "avgPV (fallback)") ? "avgPV" : "BS";
    vertices_.selected.xy_ref->SetTitle(("Vertex XY (wrt " + refName + "); X-" + refName + " [cm]; Y-" + refName + " [cm]").c_str());
    vertices_.selected.distance.dBV_ref->SetTitle(("d_{BV} wrt " + refName + "; d_{BV} [cm]; Vertices").c_str());
    tracks_.all.ip.dxy_ref->SetTitle(("dxy wrt " + refName + " (all); dxy [cm]; Tracks").c_str());
    tracks_.all.ip.dxySig_ref->SetTitle(("|dxy|/err wrt " + refName + " (all); |dxy|/err; Tracks").c_str());
    tracks_.all.ip.ipSig_ref->SetTitle(("|IP|/err wrt " + refName + " (all); |IP|/err; Tracks").c_str());
    tracks_.seed.ipSig_ref->SetTitle(("|IP|/err wrt " + refName + " (seed-like); |IP|/err; Tracks").c_str());
    tracks_.vertex.ipSig_ref->SetTitle(("|IP|/err wrt " + refName + " (vertex); |IP|/err; Tracks").c_str());

    // Update region plot titles too
    if (PVBoundary1 != -1) {
        vertices_.selected.regionA.xy_ref->SetTitle(("Vertex XY (" + refName + "-centered, Region A); X-" + refName + "_x [cm]; Y-" + refName + "_y [cm]").c_str());
        vertices_.selected.regionB.xy_ref->SetTitle(("Vertex XY (" + refName + "-centered, Region B); X-" + refName + "_x [cm]; Y-" + refName + "_y [cm]").c_str());
        vertices_.selected.regionC.xy_ref->SetTitle(("Vertex XY (" + refName + "-centered, Region C); X-" + refName + "_x [cm]; Y-" + refName + "_y [cm]").c_str());
        
        vertices_.selected.regionA.dBV->SetTitle(("Vertex d_{BV}^{" + refName + "} (Region A); d_{BV}^{" + refName + "} [cm]; Vertices").c_str());
        vertices_.selected.regionB.dBV->SetTitle(("Vertex d_{BV}^{" + refName + "} (Region B); d_{BV}^{" + refName + "} [cm]; Vertices").c_str());
        vertices_.selected.regionC.dBV->SetTitle(("Vertex d_{BV}^{" + refName + "} (Region C); d_{BV}^{" + refName + "} [cm]; Vertices").c_str());
    }

    // --- NEW: helper to compute |IP| significance wrt a vertex in 2D or 3D (mirrors seed setting) ---
    auto ipSigWrtVertex = [&](const reco::TransientTrack& ttk, const reco::Vertex& vtx) -> std::pair<bool,double> {
        if (seed_use2DTrackDist_) {
            auto ip = IPTools::absoluteTransverseImpactParameter(ttk, vtx);
            return {ip.first, ip.first ? ip.second.significance() : 0.0};
        } else {
            auto ip = IPTools::absoluteImpactParameter3D(ttk, vtx);
            return {ip.first, ip.first ? ip.second.significance() : 0.0};
        }
    };

    // Fill beamspot comparisons
    if (havePV && haveBS) {
        event_.avgPV_vs_beamspot->Fill(avgPVVtx.x() - bsVtx.x(), avgPVVtx.y() - bsVtx.y());
    }

    // --- ALL TRACKS + SEED-LIKE TRACKS ---
    {
        const math::XYZPoint origin(0.,0.,0.);
        for (size_t i = 0; i < tracksH->size(); ++i) {
            reco::TrackRef trRef(tracksH, i);
            if (!trRef.isNonnull()) continue;

            reco::TransientTrack ttk = ttBuilder.build(trRef);
            auto ip_ref = ipSigWrtVertex(ttk, refVtx);

            // All tracks
            tracks_.all.pt->Fill(trRef->pt());
            tracks_.all.eta->Fill(trRef->eta());
            tracks_.all.phi->Fill(trRef->phi());
            tracks_.all.momentum->Fill(trRef->p());
            
            if (ip_ref.first && std::isfinite(ip_ref.second)) {
                tracks_.all.ip.ipSig_ref->Fill(std::fabs(ip_ref.second));
            }
            
            const double dxyErr = trRef->dxyError();
            const double dxy0 = trRef->dxy(origin);
            tracks_.all.ip.dxy_origin->Fill(dxy0);
            if (dxyErr > 0) {
                tracks_.all.ip.dxySig_origin->Fill(std::fabs(dxy0 / dxyErr));
            }
            tracks_.all.ip.dxyError->Fill(dxyErr);
            if (std::fabs(trRef->eta()) < 1.0) tracks_.all.ip.dxyError_barrel->Fill(dxyErr);
            else tracks_.all.ip.dxyError_endcap->Fill(dxyErr);

            // Seed-like tracks
            if (trRef->pt() <= seed_minPt_) continue;
            if (!(ip_ref.first && std::isfinite(ip_ref.second))) continue;
            const double ipSigAbs = std::fabs(ip_ref.second);
            if (ipSigAbs <= seed_minIPSig_) continue;
            
            tracks_.seed.ipSig_ref->Fill(ipSigAbs);
            tracks_.seed.pt->Fill(trRef->pt());
            tracks_.seed.eta->Fill(trRef->eta());
            tracks_.seed.phi->Fill(trRef->phi());
        }
    }

    // PV region classification
    int pvRegion = 0;
    int nPV = 0;
    if (PVBoundary1 != -1 && havePV) {
        Handle<vector<Vertex>> primaryVerticesH;
        iEvent.getByToken(primaryVerticesToken, primaryVerticesH);
        nPV = primaryVerticesH->size();
        event_.nPrimaryVertices->Fill(nPV);
        
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

    int nSelVertices = 0;

    // Process each displaced vertex
    for (unsigned int t = 0; t < verticesH->size(); ++t) {
        const auto& v = verticesH->at(t);

        // --- VERTEX-ASSOCIATED TRACKS (before selection) ---
        for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
            reco::TrackRef tr = it->castTo<reco::TrackRef>();
            if (!tr.isNonnull()) continue;
            if (v.trackWeight(*it) < 0.5) continue;

            reco::TransientTrack ttk = ttBuilder.build(tr);

            auto ip_ref = ipSigWrtVertex(ttk, refVtx);
            if (ip_ref.first && std::isfinite(ip_ref.second)) {
                tracks_.vertex.ipSig_ref->Fill(std::fabs(ip_ref.second));
            }

            auto ip_v = ipSigWrtVertex(ttk, v);
            if (ip_v.first && std::isfinite(ip_v.second)) {
                tracks_.vertex.ipSig_vtx->Fill(std::fabs(ip_v.second));
            }

            tracks_.vertex.pt->Fill(tr->pt());
            tracks_.vertex.eta->Fill(tr->eta());
            tracks_.vertex.phi->Fill(tr->phi());
            tracks_.vertex.momentum->Fill(tr->p());
        }

        vector<TrackRef> tks = vertex_track_vec(v);
        int ntk = static_cast<int>(tks.size());

        // --- Opening angles ---
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
                    const double angle = vi.Angle(vj);
                    vertices_.selected.openingAngle.pairwise->Fill(angle);
                    sumAngles += angle;
                    ++npairs;
                    if (angle < minAngle) minAngle = angle;
                    if (angle > maxAngle) maxAngle = angle;
                }
            }
            meanAngle = (npairs > 0) ? (sumAngles / npairs) : 0.0;
            if (npairs == 0) { minAngle = 0.0; maxAngle = 0.0; }
        }

        // Create 4-vector for vertex mass calculation
        TLorentzVector sumVec(0,0,0,0);
        double sum_dxy = 0.0, sum_dxyErr = 0.0;

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
            
            // Define direction for signed impact parameter (track momentum direction)
            GlobalVector direction(track->px(), track->py(), track->pz());
            
            // Use IPTools::signedTransverseImpactParameter with correct arguments
            std::pair<bool, Measurement1D> ip_ref = IPTools::signedTransverseImpactParameter(transientTrack, direction, refVtx);
            
            // Track metrics for the main reference
            if (ip_ref.first) {
                sum_dxy += ip_ref.second.value();
                sum_dxyErr += ip_ref.second.error();
            }
        }

        // Calculate vertex properties
        double invMass = sumVec.M();  // Using correct relativistic mass from 4-vector
        double avg_dxy = (ntk > 0 ? sum_dxy / ntk : 0.0);
        double avg_dxyErr = (ntk > 0 ? sum_dxyErr / ntk : 0.0);

        vertices_.all.chi2norm->Fill(v.normalizedChi2());
        vertices_.all.nTracks->Fill(ntk);

        // --- Distance calculations ---
        // Use proper instances for vertex distance calculations (2D/3D match Vertexer)
        Measurement1D dBVref_meas = use_2d_vertex_dist_ ? vertexDist2D.distance(v, refVtx)
                                                         : vertexDist3D.distance(v, refVtx);
        double dBVref = dBVref_meas.value();
        double dBV_err = dBVref_meas.error();
        
        // Calculate origin distance using same method for consistency
        Vertex originVtx(Vertex::Point(0,0,0), Vertex::Error());
        Measurement1D dBV00_meas = use_2d_vertex_dist_ ? vertexDist2D.distance(v, originVtx)
                                                        : vertexDist3D.distance(v, originVtx);
        double dBV00 = dBV00_meas.value();

        // Calculate distances to all reference points with proper error propagation
        if (havePV) {
            Measurement1D dBVavgPV_meas = use_2d_vertex_dist_ ? vertexDist2D.distance(v, avgPVVtx)
                                                               : vertexDist3D.distance(v, avgPVVtx);
            vertices_.selected.distance.dBV_avgPV->Fill(dBVavgPV_meas.value());
        }
        
        if (haveBS) {
            Measurement1D dBVbs_meas = use_2d_vertex_dist_ ? vertexDist2D.distance(v, bsVtx)
                                                            : vertexDist3D.distance(v, bsVtx);
            vertices_.selected.distance.dBV_beamspot->Fill(dBVbs_meas.value());
        }

        // Apply selection criteria
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

        // Vertex accepted
        ++nSelVertices;

        vertices_.selected.chi2norm->Fill(v.normalizedChi2());
        vertices_.selected.nTracks->Fill(ntk);
        vertices_.selected.xy_global->Fill(v.x(), v.y());
        vertices_.selected.xy_ref->Fill(v.x() - refVtx.x(), v.y() - refVtx.y());

        vertices_.selected.distance.dBV_ref->Fill(dBVref);
        vertices_.selected.distance.dBV_origin->Fill(dBV00);
        vertices_.selected.distance.dBV_error->Fill(dBV_err);

        vertices_.selected.openingAngle.mean->Fill(meanAngle);
        vertices_.selected.openingAngle.min->Fill(minAngle);
        vertices_.selected.openingAngle.max->Fill(maxAngle);

        if (havePV) {
            Measurement1D dBVavgPV_meas = use_2d_vertex_dist_ ? vertexDist2D.distance(v, avgPVVtx)
                                                               : vertexDist3D.distance(v, avgPVVtx);
            vertices_.selected.distance.dBV_avgPV->Fill(dBVavgPV_meas.value());
        }
        
        if (haveBS) {
            Measurement1D dBVbs_meas = use_2d_vertex_dist_ ? vertexDist2D.distance(v, bsVtx)
                                                            : vertexDist3D.distance(v, bsVtx);
            vertices_.selected.distance.dBV_beamspot->Fill(dBVbs_meas.value());
        }

        // Region histograms
        if (PVBoundary1 != -1) {
            if (pvRegion == 0) {
                vertices_.selected.regionA.xy_global->Fill(v.x(), v.y());
                vertices_.selected.regionA.xy_ref->Fill(v.x() - refVtx.x(), v.y() - refVtx.y());
                vertices_.selected.regionA.dBV->Fill(dBVref);
                vertices_.selected.regionA.mass->Fill(invMass);
            } else if (pvRegion == 1) {
                vertices_.selected.regionB.xy_global->Fill(v.x(), v.y());
                vertices_.selected.regionB.xy_ref->Fill(v.x() - refVtx.x(), v.y() - refVtx.y());
                vertices_.selected.regionB.dBV->Fill(dBVref);
                vertices_.selected.regionB.mass->Fill(invMass);
            } else {
                vertices_.selected.regionC.xy_global->Fill(v.x(), v.y());
                vertices_.selected.regionC.xy_ref->Fill(v.x() - refVtx.x(), v.y() - refVtx.y());
                vertices_.selected.regionC.dBV->Fill(dBVref);
                vertices_.selected.regionC.mass->Fill(invMass);
            }
        }

        // Barrel / Endcap
        if (std::fabs(sumVec.Eta()) < 1.0) {
            vertices_.selected.barrel.eta->Fill(sumVec.Eta());
            vertices_.selected.barrel.dBV->Fill(dBVref);
            vertices_.selected.barrel.mass->Fill(invMass);
            vertices_.selected.barrel.xy_global->Fill(v.x(), v.y());
            vertices_.selected.barrel.xy_ref->Fill(v.x() - refVtx.x(), v.y() - refVtx.y());
        } else {
            vertices_.selected.endcap.eta->Fill(sumVec.Eta());
            vertices_.selected.endcap.dBV->Fill(dBVref);
            vertices_.selected.endcap.mass->Fill(invMass);
            vertices_.selected.endcap.xy_global->Fill(v.x(), v.y());
            vertices_.selected.endcap.xy_ref->Fill(v.x() - refVtx.x(), v.y() - refVtx.y());

            if (sumVec.Eta() < -1.0) {
                vertices_.selected.leftEndcap.eta->Fill(sumVec.Eta());
                vertices_.selected.leftEndcap.dBV->Fill(dBVref);
                vertices_.selected.leftEndcap.mass->Fill(invMass);
                vertices_.selected.leftEndcap.xy_global->Fill(v.x(), v.y());
            } else if (sumVec.Eta() > 1.0) {
                vertices_.selected.rightEndcap.eta->Fill(sumVec.Eta());
                vertices_.selected.rightEndcap.dBV->Fill(dBVref);
                vertices_.selected.rightEndcap.mass->Fill(invMass);
                vertices_.selected.rightEndcap.xy_global->Fill(v.x(), v.y());
            }
        }

        vertices_.selected.pt->Fill(sumVec.Pt());
        vertices_.selected.eta->Fill(sumVec.Eta());
        vertices_.selected.phi->Fill(sumVec.Phi());
        vertices_.selected.mass->Fill(invMass);

        // Track histograms (impact parameters for vertex-associated tracks)
        for(auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
            TrackRef track = it->castTo<TrackRef>();
            if(!track.isNonnull()) continue;
            
            reco::TransientTrack transientTrack = ttBuilder.build(track);
            GlobalVector direction(track->px(), track->py(), track->pz());
            
            std::pair<bool, Measurement1D> ip_ref = IPTools::signedTransverseImpactParameter(transientTrack, direction, refVtx);
            
            if (ip_ref.first) {
                tracks_.all.ip.dxy_ref->Fill(ip_ref.second.value());
                if (ip_ref.second.error() > 0) {
                    tracks_.all.ip.dxySig_ref->Fill(std::fabs(ip_ref.second.significance()));
                }
            }
            
            if (havePV) {
                std::pair<bool, Measurement1D> ip_avgPV = IPTools::signedTransverseImpactParameter(transientTrack, direction, avgPVVtx);
                if (ip_avgPV.first) {
                    tracks_.all.ip.dxy_avgPV->Fill(ip_avgPV.second.value());
                    tracks_.all.ip.dxySig_avgPV->Fill(std::fabs(ip_avgPV.second.significance()));
                }
            }
            
            if (haveBS) {
                std::pair<bool, Measurement1D> ip_bs = IPTools::signedTransverseImpactParameter(transientTrack, direction, bsVtx);
                if (ip_bs.first) {
                    tracks_.all.ip.dxy_beamspot->Fill(ip_bs.second.value());
                    tracks_.all.ip.dxySig_beamspot->Fill(std::fabs(ip_bs.second.significance()));
                }
            }
            
            if (pmvtx) {
                std::pair<bool, Measurement1D> ip_pmvtx = IPTools::signedTransverseImpactParameter(transientTrack, direction, *pmvtx);
                if (ip_pmvtx.first) {
                    tracks_.vertex.dxy_primaryVtx->Fill(ip_pmvtx.second.value());
                    tracks_.vertex.dxySig_primaryVtx->Fill(std::fabs(ip_pmvtx.second.significance()));
                }
            }
        }
    }

    event_.nSelectedVertices->Fill(static_cast<double>(nSelVertices));
}

// Helper method to determine the reference vertex based on preference and availability
std::pair<bool, std::string> ScoutingTreeMakerRun3::determineReferenceVertex(
    const edm::Event& iEvent, 
    reco::Vertex& refVtx,
    bool& havePV, 
    reco::Vertex& avgPVVtx,
    bool& haveBS,
    reco::Vertex& bsVtx) {
    
    std::string refType = "none";
    havePV = false;
    haveBS = false;

    // Compute average PV position; covariance is set to match Vertexer (fixed diag; off-diagonals = 0)
    edm::Handle<std::vector<reco::Vertex>> primaryVerticesH;
    iEvent.getByToken(primaryVerticesToken, primaryVerticesH);
    if (primaryVerticesH.isValid() && !primaryVerticesH->empty()) {
        double sumX=0, sumY=0, sumZ=0; int validPVs=0;
        for (const auto& pv : *primaryVerticesH) {
            if (!pv.isFake() && pv.ndof() > 4) {
                sumX += pv.x(); sumY += pv.y(); sumZ += pv.z();
                event_.primaryVertices_xy->Fill(pv.x(), pv.y());
                ++validPVs;
            }
        }
        if (validPVs > 0) {
            reco::Vertex::Point avgPos(sumX/validPVs, sumY/validPVs, sumZ/validPVs);
            reco::Vertex::Error avgErr; // zero-initialized off-diagonals
            for (int i=0;i<3;++i) for (int j=i;j<3;++j) avgErr(i,j)=0.0;
            // Match Vertexer fixed uncertainties
            avgErr(0,0)=0.0015*0.0015; // x
            avgErr(1,1)=0.0015*0.0015; // y
            avgErr(2,2)=0.0050*0.0050; // z
            avgPVVtx = reco::Vertex(avgPos, avgErr);
            havePV = true;
        }
    }

    // Build beamspot fake vertex with diagonal covariances only (match Vertexer; zero off-diagonals)
    edm::Handle<reco::BeamSpot> beamspot;
    iEvent.getByToken(beamspot_token, beamspot);
    if (beamspot.isValid()) {
        reco::Vertex::Error bsErr;
        for (int i=0;i<3;++i) for (int j=i;j<3;++j) bsErr(i,j)=0.0;
        bsErr(0,0)=beamspot->covariance()(0,0);
        bsErr(1,1)=beamspot->covariance()(1,1);
        bsErr(2,2)=beamspot->covariance()(2,2);
        bsVtx = reco::Vertex(beamspot->position(), bsErr);
        event_.beamspot_xy->Fill(beamspot->x0(), beamspot->y0());
        haveBS = true;
    }

    // Match Vertexer preference/fallback logic
    if (refPreference_ == RefPreference::PreferPV && havePV) {
        refVtx = avgPVVtx; 
        return {true, "avgPV"};
    } else if (refPreference_ == RefPreference::PreferBeamSpot && haveBS) {
        refVtx = bsVtx;    
        return {true, "beamspot"};
    } else if (havePV) {
        refVtx = avgPVVtx; 
        return {true, "avgPV (fallback)"};
    } else if (haveBS) {
        refVtx = bsVtx;    
        return {true, "beamspot (fallback)"};
    }

    return {false, "none"};
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
    // --- NEW: seed-like selection controls for analyzer-side reproduction ---
    desc.addUntracked<double>("seed_minIPSig", 4.0);
    desc.addUntracked<double>("seed_minPt", 0.9);
    desc.addUntracked<bool>("seed_use2DTrackDist", false);
    // Allow toggles to match Vertexer; these are tracked to accept cms.bool in the config
    desc.add<bool>("use_2d_track_dist", false);
    desc.add<bool>("use_2d_vertex_dist", false);
    // --- NEW: full Vertexer-like thresholds for seed-like plots (optional) ---
    desc.addUntracked<double>("seed_maxIPSig", 1e9);
    desc.addUntracked<int>("seed_minPixelHits", 0);
    desc.addUntracked<int>("seed_minStripHits", 0);
    desc.addUntracked<int>("seed_minTrackerLayers", 0);
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