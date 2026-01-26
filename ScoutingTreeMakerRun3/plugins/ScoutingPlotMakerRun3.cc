// -*- C++ -*-
// Package:    Vertexing/ScoutingTreeMakerRun3
// Class:      ScoutingTreeMakerRun3
//
// Author:     Adapted / fixed by an assistant (based on original by David Sperka)
// Created:    Tue, 14 May 2024 14:23:11 GMT
// Revised:    2025-09-09


/*
===============================================================================
ScoutingPlotMakerRun3
===============================================================================
Description:
  CMSSW EDAnalyzer that builds histograms from scouting unpacked data and
  reconstructed displaced vertices produced by the Vertexer. This module
  creates the canonical set of histograms used for monitoring and analysis:
  vertex kinematics, spatial distributions, distances, opening angles,
  event-level summaries, and comprehensive track-level IP distributions.

Purpose:
  - Produce final histograms at CMSSW runtime from unpacked scouting data.
  - Provide the reference histogram set that Tree2PlotsRun3 can reproduce
    using TTree input (for re-analysis).
  - Compute seed-like track collections (Vertexer-like selection) and
    produce seed vs all vs vertex-associated track plots.

What is read (input):
  - Displaced vertex collection (Vertexer output)
  - BeamSpot
  - Full track collection (unpacked scouting Track objects)
  - Primary vertices (for avgPV computation)

What is produced (output):
  - Event-level histograms (nPrimaryVertices, nSelectedVertices, beamspot vs avgPV)
  - Vertex histograms in branches defined by ntk × opening-angle categories
  - Track histograms for: all tracks, seed-like tracks, vertex-associated tracks

Selection & computation policy:
  - Vertex selection and seed-like criteria mirror the Vertexer/TreeMaker
    configuration where possible (pt thresholds, IP significance, hit counts).
  - Impact parameters and IPSignificance are computed with TransientTrack
    and IPTools for exact signed IP values (3D or 2D depending on toggles).
  - Vertex mass computed by summing track four-vectors (pion mass assumption).
  - Opening angles computed with TVector3 from track momentum vectors.

Configuration:
  - Configurable parameters include cut_ntk (VPSet), cut_opening_angle_min,
    required_invmass, required_chi2, required_dBV_min/max, PVBoundary1/2,
    and seed-like thresholds (seed_minIPSig, seed_minPt, seed_use2DTrackDist).
  - Use process.maxEvents in the python config to limit the number of processed events.

Notes / Caveats:
  - This module depends on having the unpacked Track collection available;
    Tree-based re-analysis (Tree2Plots) cannot reproduce "all-tracks" histograms
    unless TreeMaker stored per-event track collections. Consider storing
    event-level track arrays in the TreeMaker if you need full parity.
  - Binning and naming in this module are authoritative for downstream
    comparisons; Tree2Plots reproduces these when given the necessary primitives.
===============================================================================
*/

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

class ScoutingPlotMakerRun3 : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
    explicit ScoutingPlotMakerRun3(const edm::ParameterSet&);
    ~ScoutingPlotMakerRun3() override;

    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
    void beginJob() override;
    void analyze(const edm::Event&, const edm::EventSetup&) override;
    void endJob() override;

    // Parameter declarations with cut values
    const std::vector<std::vector<int>> cut_ntk_;       // NEW: vector of discrete ntk sets
    const std::vector<double> cut_opening_angle_min_;   // NEW: vector of min angle cuts
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

    // Define branch histogram structure as a named type
    struct BranchHistos {
        TH1F* chi2norm;
        TH1F* pt;
        TH1F* eta;
        TH1F* phi;
        TH1F* mass;
        TH1F* nTracks;
        TH2F* xy_global;
        TH2F* xy_ref;
        
        struct {
            TH1F* dBV_origin;
            TH1F* dBV_ref;
            TH1F* dBV_beamspot;
            TH1F* dBV_avgPV;
            TH1F* dBV_error;
        } distance;
        
        struct {
            TH1F* pairwise;
            TH1F* mean;
            TH1F* min;
            TH1F* max;
        } openingAngle;
        
        struct {
            TH1F* eta;
            TH1F* mass;
            TH1F* dBV;
            TH2F* xy_global;
            TH2F* xy_ref;
        } barrel, endcap, leftEndcap, rightEndcap;
        
        struct {
            TH1F* mass;
            TH1F* dBV;
            TH2F* xy_global;
            TH2F* xy_ref;
        } regionA, regionB, regionC;
    };

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
        struct {
            TH1F* chi2norm;
            TH1F* nTracks;
        } all;
        
        std::map<std::string, BranchHistos> branches;  // NOW THIS WORKS!
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
                TH1F* IP_ref;
                TH1F* IPSig_ref;
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
            TH1F* momentum;
            
            struct {
                TH1F* IP_ref;
                TH1F* IPSig_ref;
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
        } seed;
        
        // Vertex-associated tracks
        struct {
            TH1F* pt;
            TH1F* eta;
            TH1F* phi;
            TH1F* momentum;
            
            struct {
                TH1F* IP_ref;
                TH1F* IPSig_ref;
                TH1F* dxy_origin;
                TH1F* dxySig_origin;
                TH1F* dxy_ref;
                TH1F* dxySig_ref;
                TH1F* dxy_beamspot;
                TH1F* dxySig_beamspot;
                TH1F* dxy_avgPV;
                TH1F* dxySig_avgPV;
                TH1F* dxy_primaryVtx;       // ADD these two
                TH1F* dxySig_primaryVtx;    // ADD these two
                TH1F* dxyError;
                TH1F* dxyError_barrel;
                TH1F* dxyError_endcap;
            } ip;
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

ScoutingPlotMakerRun3::ScoutingPlotMakerRun3(const edm::ParameterSet& iConfig):
    cut_ntk_([&iConfig]() {
        std::vector<std::vector<int>> result;
        auto vpset = iConfig.getParameter<std::vector<edm::ParameterSet>>("cut_ntk");
        for (const auto& pset : vpset) {
            result.push_back(pset.getParameter<std::vector<int>>("values"));
        }
        return result;
    }()),
    cut_opening_angle_min_(iConfig.getParameter<std::vector<double>>("cut_opening_angle_min")),
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

ScoutingPlotMakerRun3::~ScoutingPlotMakerRun3() {
    // no explicit cleanup required; histograms owned by TFileService
}

void ScoutingPlotMakerRun3::beginJob() {
    edm::Service<TFileService> fs;
    
    // ==================== EVENT LEVEL ====================
    TFileDirectory eventDir = fs->mkdir("Event");
    event_.nPrimaryVertices = eventDir.make<TH1F>("nPrimaryVertices","Number of Primary Vertices; nPV; Events",100,0,100);
    event_.nSelectedVertices = eventDir.make<TH1F>("nSelectedVertices","Number of Selected Vertices; N_{vtx}; Events",200,0,1000);
    event_.primaryVertices_xy = eventDir.make<TH2F>("primaryVertices_xy","Primary Vertices XY; X [cm]; Y [cm]",200,-1,1,200,-1,1);
    event_.beamspot_xy = eventDir.make<TH2F>("beamspot_xy","Beamspot Position; x_{BS} [cm]; y_{BS} [cm]",200,-1,1,200,-1,1);
    event_.avgPV_vs_beamspot = eventDir.make<TH2F>("avgPV_vs_beamspot","AvgPV - Beamspot; #Delta x [cm]; #Delta y [cm]",200,-1,1,200,-1,1);

    // ==================== VERTICES ====================
    TFileDirectory verticesDir = fs->mkdir("Vertices");
    
    // Create branches for each ntk × angle combination
    TFileDirectory vtxSelDir = verticesDir.mkdir("Selected");
    
    for (size_t i_ntk = 0; i_ntk < cut_ntk_.size(); ++i_ntk) {
        const auto& ntk_set = cut_ntk_[i_ntk];
        
        // Build ntk branch name
        std::string ntkBranchName;
        if (ntk_set.empty()) {
            ntkBranchName = "ntk_any";
        } else if (ntk_set.size() == 1) {
            ntkBranchName = "ntk_" + std::to_string(ntk_set[0]);
        } else {
            ntkBranchName = "ntk";
            for (size_t j = 0; j < ntk_set.size(); ++j) {
                if (j > 0) ntkBranchName += "_or_";
                ntkBranchName += std::to_string(ntk_set[j]);
            }
        }
        
        TFileDirectory ntkDir = vtxSelDir.mkdir(ntkBranchName);
        
        for (size_t i_angle = 0; i_angle < cut_opening_angle_min_.size(); ++i_angle) {
            double angle_cut = cut_opening_angle_min_[i_angle];
            
            // Build angle branch name
            std::string angleBranchName;
            if (angle_cut < 0) {
                angleBranchName = "angle_any";
            } else {
                std::ostringstream oss;
                oss << "angle_gt_" << std::fixed << std::setprecision(2) << angle_cut;
                angleBranchName = oss.str();
                std::replace(angleBranchName.begin(), angleBranchName.end(), '.', 'p');
            }
            
            std::string branchKey = ntkBranchName + "/" + angleBranchName;
            TFileDirectory branchDir = ntkDir.mkdir(angleBranchName);
            
            auto& branch = vertices_.branches[branchKey];
            
            // Create all histograms for this branch (REDUCED BINNING)
            TFileDirectory kinDir = branchDir.mkdir("Kinematics");
            branch.chi2norm = kinDir.make<TH1F>("chi2norm","Vertex #chi^{2}/ndof; #chi^{2}/ndof; Vertices",200,0,20);
            branch.pt = kinDir.make<TH1F>("pt","Vertex p_{T}; p_{T} [GeV]; Vertices",100,0,100); // reduced from 200
            branch.eta = kinDir.make<TH1F>("eta","Vertex #eta; #eta; Vertices",100,-5,5); // reduced from 200
            branch.phi = kinDir.make<TH1F>("phi","Vertex #phi; #phi; Vertices",100,-3.14,3.14); // reduced from 200
            branch.mass = kinDir.make<TH1F>("mass","Vertex Mass; Mass [GeV]; Vertices",100,0,10); // reduced from 200
            branch.nTracks = kinDir.make<TH1F>("nTracks","Number of Tracks; N_{tracks}; Vertices",50,0,50); // reduced from 200
            
            TFileDirectory spatialDir = branchDir.mkdir("Spatial");
            branch.xy_global = spatialDir.make<TH2F>("xy_global","Vertex XY (Global); X [cm]; Y [cm]",800,-10,10,800,-10,10); // reduced from 2000x2000
            branch.xy_ref = spatialDir.make<TH2F>("xy_ref","Vertex XY (ref); X-X_{ref} [cm]; Y-Y_{ref} [cm]",800,-10,10,800,-10,10); // reduced from 2000x2000
            
            TFileDirectory distDir = branchDir.mkdir("Distance");
            branch.distance.dBV_origin = distDir.make<TH1F>("dBV_origin","d_{BV} wrt (0,0); d_{BV} [cm]; Vertices",200,0,10);
            branch.distance.dBV_ref = distDir.make<TH1F>("dBV_ref","d_{BV} wrt ref; d_{BV} [cm]; Vertices",200,0,10);
            branch.distance.dBV_beamspot = distDir.make<TH1F>("dBV_beamspot","d_{BV} wrt BS; d_{BV} [cm]; Vertices",200,0,10);
            branch.distance.dBV_avgPV = distDir.make<TH1F>("dBV_avgPV","d_{BV} wrt avgPV; d_{BV} [cm]; Vertices",200,0,10);
            branch.distance.dBV_error = distDir.make<TH1F>("dBV_error","d_{BV} Uncertainty; #sigma_{dBV} [cm]; Vertices",1000,0,0.1);
            
            TFileDirectory angleDir = branchDir.mkdir("OpeningAngles");
            branch.openingAngle.pairwise = angleDir.make<TH1F>("pairwise","Opening Angle (pairwise); Angle [rad]; Pairs",180,0,3.14159);
            branch.openingAngle.mean = angleDir.make<TH1F>("mean","Mean Opening Angle; <Angle> [rad]; Vertices",180,0,3.14159);
            branch.openingAngle.min = angleDir.make<TH1F>("min","Min Opening Angle; Min Angle [rad]; Vertices",180,0,3.14159);
            branch.openingAngle.max = angleDir.make<TH1F>("max","Max Opening Angle; Max Angle [rad]; Vertices",180,0,3.14159);
            
            // Topology (REDUCED BINNING)
            TFileDirectory topoDir = branchDir.mkdir("Topology");
            TFileDirectory barrelDir = topoDir.mkdir("Barrel");
            branch.barrel.eta = barrelDir.make<TH1F>("eta","#eta (Barrel); #eta; Vertices",100,-3,3); // reduced from 200
            branch.barrel.mass = barrelDir.make<TH1F>("mass","Mass (Barrel); Mass [GeV]; Vertices",100,0,10); // reduced from 200
            branch.barrel.dBV = barrelDir.make<TH1F>("dBV","d_{BV} (Barrel); d_{BV} [cm]; Vertices",100,0,10); // reduced from 200
            branch.barrel.xy_global = barrelDir.make<TH2F>("xy_global","XY (Barrel); X [cm]; Y [cm]",800,-10,10,800,-10,10); // reduced from 2000x2000
            branch.barrel.xy_ref = barrelDir.make<TH2F>("xy_ref","XY (Barrel, ref); X-X_{ref} [cm]; Y-Y_{ref} [cm]",800,-10,10,800,-10,10); // reduced from 2000x2000
            
            TFileDirectory endcapDir = topoDir.mkdir("Endcap");
            branch.endcap.eta = endcapDir.make<TH1F>("eta","#eta (Endcap); #eta; Vertices",100,-3,3); // reduced from 200
            branch.endcap.mass = endcapDir.make<TH1F>("mass","Mass (Endcap); Mass [GeV]; Vertices",100,0,10); // reduced from 200
            branch.endcap.dBV = endcapDir.make<TH1F>("dBV","d_{BV} (Endcap); d_{BV} [cm]; Vertices",100,0,10); // reduced from 200
            branch.endcap.xy_global = endcapDir.make<TH2F>("xy_global","XY (Endcap); X [cm]; Y [cm]",800,-10,10,800,-10,10); // reduced from 2000x2000
            branch.endcap.xy_ref = endcapDir.make<TH2F>("xy_ref","XY (Endcap, ref); X-X_{ref} [cm]; Y-Y_{ref} [cm]",800,-10,10,800,-10,10); // reduced from 2000x2000

            TFileDirectory leftEndcapDir = topoDir.mkdir("LeftEndcap");
            branch.leftEndcap.eta = leftEndcapDir.make<TH1F>("eta","#eta (Left Endcap); #eta; Vertices",100,-3,3); // reduced from 200
            branch.leftEndcap.mass = leftEndcapDir.make<TH1F>("mass","Mass (Left Endcap); Mass [GeV]; Vertices",100,0,10); // reduced from 200
            branch.leftEndcap.dBV = leftEndcapDir.make<TH1F>("dBV","d_{BV} (Left Endcap); d_{BV} [cm]; Vertices",100,0,10); // reduced from 200
            branch.leftEndcap.xy_global = leftEndcapDir.make<TH2F>("xy_global","XY (Left Endcap); X [cm]; Y [cm]",800,-10,10,800,-10,10); // reduced from 2000x2000
            
            TFileDirectory rightEndcapDir = topoDir.mkdir("RightEndcap");
            branch.rightEndcap.eta = rightEndcapDir.make<TH1F>("eta","#eta (Right Endcap); #eta; Vertices",100,-3,3); // reduced from 200
            branch.rightEndcap.mass = rightEndcapDir.make<TH1F>("mass","Mass (Right Endcap); Mass [GeV]; Vertices",100,0,10); // reduced from 200
            branch.rightEndcap.dBV = rightEndcapDir.make<TH1F>("dBV","d_{BV} (Right Endcap); d_{BV} [cm]; Vertices",100,0,10); // reduced from 200
            branch.rightEndcap.xy_global = rightEndcapDir.make<TH2F>("xy_global","XY (Right Endcap); X [cm]; Y [cm]",800,-10,10,800,-10,10); // reduced from 2000x2000

            // PV regions (REDUCED BINNING)
            if (PVBoundary1 != -1) {
                TFileDirectory regionDir = branchDir.mkdir("PVRegions");
                
                TFileDirectory regADir = regionDir.mkdir("RegionA");
                branch.regionA.mass = regADir.make<TH1F>("mass","Mass (Region A); Mass [GeV]; Vertices",100,0,10); // reduced from 200
                branch.regionA.dBV = regADir.make<TH1F>("dBV","d_{BV} (Region A); d_{BV} [cm]; Vertices",100,0,10); // reduced from 200
                branch.regionA.xy_global = regADir.make<TH2F>("xy_global","XY Global (Region A); X [cm]; Y [cm]",800,-10,10,800,-10,10); // reduced from 2000x2000
                branch.regionA.xy_ref = regADir.make<TH2F>("xy_ref","XY ref (Region A); X-X_{ref} [cm]; Y-Y_{ref} [cm]",800,-10,10,800,-10,10); // reduced from 2000x2000

                TFileDirectory regBDir = regionDir.mkdir("RegionB");
                branch.regionB.mass = regBDir.make<TH1F>("mass","Mass (Region B); Mass [GeV]; Vertices",100,0,10); // reduced from 200
                branch.regionB.dBV = regBDir.make<TH1F>("dBV","d_{BV} (Region B); d_{BV} [cm]; Vertices",100,0,10); // reduced from 200
                branch.regionB.xy_global = regBDir.make<TH2F>("xy_global","XY Global (Region B); X [cm]; Y [cm]",800,-10,10,800,-10,10); // reduced from 2000x2000
                branch.regionB.xy_ref = regBDir.make<TH2F>("xy_ref","XY ref (Region B); X-X_{ref} [cm]; Y-Y_{ref} [cm]",800,-10,10,800,-10,10); // reduced from 2000x2000
                
                TFileDirectory regCDir = regionDir.mkdir("RegionC");
                branch.regionC.mass = regCDir.make<TH1F>("mass","Mass (Region C); Mass [GeV]; Vertices",100,0,10); // reduced from 200
                branch.regionC.dBV = regCDir.make<TH1F>("dBV","d_{BV} (Region C); d_{BV} [cm]; Vertices",100,0,10); // reduced from 200
                branch.regionC.xy_global = regCDir.make<TH2F>("xy_global","XY Global (Region C); X [cm]; Y [cm]",800,-10,10,800,-10,10); // reduced from 2000x2000
                branch.regionC.xy_ref = regCDir.make<TH2F>("xy_ref","XY ref (Region C); X-X_{ref} [cm]; Y-Y_{ref} [cm]",800,-10,10,800,-10,10); // reduced from 2000x2000
            }
        }
    }

    // ==================== TRACKS ====================
    TFileDirectory tracksDir = fs->mkdir("Tracks");
    
    // All tracks
    TFileDirectory trkAllDir = tracksDir.mkdir("All");
    TFileDirectory trkAllKinDir = trkAllDir.mkdir("Kinematics");
    tracks_.all.pt = trkAllKinDir.make<TH1F>("pt","Track p_{T} (all); Transverse Momentum p_{T} [GeV]; Tracks",100,0,100);
    tracks_.all.eta = trkAllKinDir.make<TH1F>("eta","Track #eta (all); #eta; Tracks",100,-3,3);
    tracks_.all.phi = trkAllKinDir.make<TH1F>("phi","Track #phi (all); #phi; Tracks",100,-3.14,3.14);
    tracks_.all.momentum = trkAllKinDir.make<TH1F>("momentum","Track Momentum (all); Total Momentum p [GeV]; Tracks",100,0,100);
    
    TFileDirectory trkAllIPDir = trkAllDir.mkdir("ImpactParameter");
    tracks_.all.ip.IP_ref = trkAllIPDir.make<TH1F>("IP_ref","Impact Parameter wrt ref (all); IP [cm]; Tracks",500,0,5);
    tracks_.all.ip.IPSig_ref = trkAllIPDir.make<TH1F>("IPSig_ref","|IP|/#sigma wrt ref (all); |IP|/#sigma; Tracks",100,0,50);
    tracks_.all.ip.dxy_origin = trkAllIPDir.make<TH1F>("dxy_origin","dxy wrt (0,0) (all); dxy [cm]; Tracks",200,-2,2);
    tracks_.all.ip.dxySig_origin = trkAllIPDir.make<TH1F>("dxySig_origin","|dxy|/#sigma wrt (0,0) (all); |dxy|/#sigma; Tracks",100,0,50);
    tracks_.all.ip.dxy_ref = trkAllIPDir.make<TH1F>("dxy_ref","dxy wrt ref (all); dxy [cm]; Tracks",500,-5,5);
    tracks_.all.ip.dxySig_ref = trkAllIPDir.make<TH1F>("dxySig_ref","|dxy|/#sigma wrt ref (all); |dxy|/#sigma; Tracks",100,0,50);
    tracks_.all.ip.dxy_beamspot = trkAllIPDir.make<TH1F>("dxy_beamspot","dxy wrt BS (all); dxy [cm]; Tracks",200,-2,2);
    tracks_.all.ip.dxySig_beamspot = trkAllIPDir.make<TH1F>("dxySig_beamspot","|dxy|/#sigma wrt BS (all); |dxy|/#sigma; Tracks",100,0,50);
    tracks_.all.ip.dxy_avgPV = trkAllIPDir.make<TH1F>("dxy_avgPV","dxy wrt avgPV (all); dxy [cm]; Tracks",200,-2,2);
    tracks_.all.ip.dxySig_avgPV = trkAllIPDir.make<TH1F>("dxySig_avgPV","|dxy|/#sigma wrt avgPV (all); |dxy|/#sigma; Tracks",100,0,50);
    tracks_.all.ip.dxyError = trkAllIPDir.make<TH1F>("dxyError","dxy Error (all); #sigma_{dxy} [cm]; Tracks",200,0,0.05);
    tracks_.all.ip.dxyError_barrel = trkAllIPDir.make<TH1F>("dxyError_barrel","dxy Error Barrel (all); #sigma_{dxy} [cm]; Tracks",200,0,0.05);
    tracks_.all.ip.dxyError_endcap = trkAllIPDir.make<TH1F>("dxyError_endcap","dxy Error Endcap (all); #sigma_{dxy} [cm]; Tracks",200,0,0.05);
    
    // Seed-like tracks
    TFileDirectory trkSeedDir = tracksDir.mkdir("SeedLike");
    TFileDirectory trkSeedKinDir = trkSeedDir.mkdir("Kinematics");
    tracks_.seed.pt = trkSeedKinDir.make<TH1F>("pt","Track p_{T} (seed-like); Transverse Momentum p_{T} [GeV]; Tracks",100,0,100);
    tracks_.seed.eta = trkSeedKinDir.make<TH1F>("eta","Track #eta (seed-like); #eta; Tracks",100,-3,3);
    tracks_.seed.phi = trkSeedKinDir.make<TH1F>("phi","Track #phi (seed-like); #phi; Tracks",100,-3.14,3.14);
    tracks_.seed.momentum = trkSeedKinDir.make<TH1F>("momentum","Track Momentum (seed-like); Total Momentum p [GeV]; Tracks",100,0,100);
    
    TFileDirectory trkSeedIPDir = trkSeedDir.mkdir("ImpactParameter");
    tracks_.seed.ip.IP_ref = trkSeedIPDir.make<TH1F>("IP_ref","Impact Parameter wrt ref (seed-like); IP [cm]; Tracks",500,0,5);
    tracks_.seed.ip.IPSig_ref = trkSeedIPDir.make<TH1F>("IPSig_ref","|IP|/#sigma wrt ref (seed-like); |IP|/#sigma; Tracks",100,0,50);
    tracks_.seed.ip.dxy_origin = trkSeedIPDir.make<TH1F>("dxy_origin","dxy wrt (0,0) (seed-like); dxy [cm]; Tracks",200,-2,2);
    tracks_.seed.ip.dxySig_origin = trkSeedIPDir.make<TH1F>("dxySig_origin","|dxy|/#sigma wrt (0,0) (seed-like); |dxy|/#sigma; Tracks",100,0,50);
    tracks_.seed.ip.dxy_ref = trkSeedIPDir.make<TH1F>("dxy_ref","dxy wrt ref (seed-like); dxy [cm]; Tracks",500,-5,5);
    tracks_.seed.ip.dxySig_ref = trkSeedIPDir.make<TH1F>("dxySig_ref","|dxy|/#sigma wrt ref (seed-like); |dxy|/#sigma; Tracks",100,0,50);
    tracks_.seed.ip.dxy_beamspot = trkSeedIPDir.make<TH1F>("dxy_beamspot","dxy wrt BS (seed-like); dxy [cm]; Tracks",200,-2,2);
    tracks_.seed.ip.dxySig_beamspot = trkSeedIPDir.make<TH1F>("dxySig_beamspot","|dxy|/#sigma wrt BS (seed-like); |dxy|/#sigma; Tracks",100,0,50);
    tracks_.seed.ip.dxy_avgPV = trkSeedIPDir.make<TH1F>("dxy_avgPV","dxy wrt avgPV (seed-like); dxy [cm]; Tracks",200,-2,2);
    tracks_.seed.ip.dxySig_avgPV = trkSeedIPDir.make<TH1F>("dxySig_avgPV","|dxy|/#sigma wrt avgPV (seed-like); |dxy|/#sigma; Tracks",100,0,50);
    tracks_.seed.ip.dxyError = trkSeedIPDir.make<TH1F>("dxyError","dxy Error (seed-like); #sigma_{dxy} [cm]; Tracks",200,0,0.05);
    tracks_.seed.ip.dxyError_barrel = trkSeedIPDir.make<TH1F>("dxyError_barrel","dxy Error Barrel (seed-like); #sigma_{dxy} [cm]; Tracks",200,0,0.05);
    tracks_.seed.ip.dxyError_endcap = trkSeedIPDir.make<TH1F>("dxyError_endcap","dxy Error Endcap (seed-like); #sigma_{dxy} [cm]; Tracks",200,0,0.05);
    
    // Vertex-associated tracks
    TFileDirectory trkVtxDir = tracksDir.mkdir("VertexAssociated");
    TFileDirectory trkVtxKinDir = trkVtxDir.mkdir("Kinematics");
    tracks_.vertex.pt = trkVtxKinDir.make<TH1F>("pt","Track p_{T} (vertex); Transverse Momentum p_{T} [GeV]; Tracks",100,0,100);
    tracks_.vertex.eta = trkVtxKinDir.make<TH1F>("eta","Track #eta (vertex); #eta; Tracks",100,-3,3);
    tracks_.vertex.phi = trkVtxKinDir.make<TH1F>("phi","Track #phi (vertex); #phi; Tracks",100,-3.14,3.14);
    tracks_.vertex.momentum = trkVtxKinDir.make<TH1F>("momentum","Track Momentum (vertex); Total Momentum p [GeV]; Tracks",100,0,100);
    
    TFileDirectory trkVtxIPDir = trkVtxDir.mkdir("ImpactParameter");
    tracks_.vertex.ip.IP_ref = trkVtxIPDir.make<TH1F>("IP_ref","Impact Parameter wrt ref (vertex); IP [cm]; Tracks",500,0,5);
    tracks_.vertex.ip.IPSig_ref = trkVtxIPDir.make<TH1F>("IPSig_ref","|IP|/#sigma wrt ref (vertex); |IP|/#sigma; Tracks",100,0,50);
    tracks_.vertex.ip.dxy_origin = trkVtxIPDir.make<TH1F>("dxy_origin","dxy wrt (0,0) (vertex); dxy [cm]; Tracks",200,-2,2);
    tracks_.vertex.ip.dxySig_origin = trkVtxIPDir.make<TH1F>("dxySig_origin","|dxy|/#sigma wrt (0,0) (vertex); |dxy|/#sigma; Tracks",100,0,50);
    tracks_.vertex.ip.dxy_ref = trkVtxIPDir.make<TH1F>("dxy_ref","dxy wrt ref (vertex); dxy [cm]; Tracks",500,-5,5);
    tracks_.vertex.ip.dxySig_ref = trkVtxIPDir.make<TH1F>("dxySig_ref","|dxy|/#sigma wrt ref (vertex); |dxy|/#sigma; Tracks",100,0,50);
    tracks_.vertex.ip.dxy_beamspot = trkVtxIPDir.make<TH1F>("dxy_beamspot","dxy wrt BS (vertex); dxy [cm]; Tracks",200,-2,2);
    tracks_.vertex.ip.dxySig_beamspot = trkVtxIPDir.make<TH1F>("dxySig_beamspot","|dxy|/#sigma wrt BS (vertex); |dxy|/#sigma; Tracks",100,0,50);
    tracks_.vertex.ip.dxy_avgPV = trkVtxIPDir.make<TH1F>("dxy_avgPV","dxy wrt avgPV (vertex); dxy [cm]; Tracks",200,-2,2);
    tracks_.vertex.ip.dxySig_avgPV = trkVtxIPDir.make<TH1F>("dxySig_avgPV","|dxy|/#sigma wrt avgPV (vertex); |dxy|/#sigma; Tracks",100,0,50);
    tracks_.vertex.ip.dxy_primaryVtx = trkVtxIPDir.make<TH1F>("dxy_primaryVtx","dxy wrt primary vertex; dxy [cm]; Tracks",200,-2,2);
    tracks_.vertex.ip.dxySig_primaryVtx = trkVtxIPDir.make<TH1F>("dxySig_primaryVtx","|dxy|/#sigma wrt primary vertex; |dxy|/#sigma; Tracks",100,0,50);
    tracks_.vertex.ip.dxyError = trkVtxIPDir.make<TH1F>("dxyError","dxy Error (vertex); #sigma_{dxy} [cm]; Tracks",200,0,0.05);
    tracks_.vertex.ip.dxyError_barrel = trkVtxIPDir.make<TH1F>("dxyError_barrel","dxy Error Barrel (vertex); #sigma_{dxy} [cm]; Tracks",200,0,0.05);
    tracks_.vertex.ip.dxyError_endcap = trkVtxIPDir.make<TH1F>("dxyError_endcap","dxy Error Endcap (vertex); #sigma_{dxy} [cm]; Tracks",200,0,0.05);
}

void ScoutingPlotMakerRun3::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
    using namespace edm; using namespace std; using namespace reco;

    // NEW: ensure track collection (referenced by Vertex TrackRefs) is present to avoid segfault
    Handle<vector<Track>> tracksH;
    iEvent.getByToken(tracksToken, tracksH);
    if (!tracksH.isValid()) {
        LogWarning("ScoutingPlotMakerRun3") << "Track collection invalid (skip event to avoid dangling TrackRefs).";
        return;
    }

    // Displaced vertices
    Handle<vector<Vertex>> verticesH;
    iEvent.getByToken(verticesToken, verticesH);
    if(!verticesH.isValid()){
        LogWarning("ScoutingPlotMakerRun3") << "Displaced vertex collection invalid (skip event).";
        return;
    }

    // Get builder for transient tracks
    auto const& ttBuilder = iSetup.getData(ttBuilderToken_);
    
    // Create proper reference vertices using CMSSW methods
    reco::Vertex refVtx, avgPVVtx, bsVtx;
    bool havePV = false, haveBS = false;
    auto [haveRef, refType] = determineReferenceVertex(iEvent, refVtx, havePV, avgPVVtx, haveBS, bsVtx);
    
    if (!haveRef) {
        LogWarning("ScoutingPlotMakerRun3") << "No valid reference (avgPV or BeamSpot). Skipping event.";
        return;
    }
    
    // Use proper instance for 2D vertex distance calculations
    VertexDistanceXY vertexDist2D;

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

    // --- ALL TRACKS (from unpacker) ---
    {
        const math::XYZPoint origin(0.,0.,0.);
        for (size_t i = 0; i < tracksH->size(); ++i) {
            reco::TrackRef trRef(tracksH, i);
            if (!trRef.isNonnull()) continue;

            // Kinematics
            tracks_.all.pt->Fill(trRef->pt());
            tracks_.all.eta->Fill(trRef->eta());
            tracks_.all.phi->Fill(trRef->phi());
            tracks_.all.momentum->Fill(trRef->p());
            
            // Build transient track for IP calculations
            reco::TransientTrack ttk = ttBuilder.build(trRef);
            GlobalVector direction(trRef->px(), trRef->py(), trRef->pz());
            
            // IP wrt reference
            auto ip_ref = ipSigWrtVertex(ttk, refVtx);
            if (ip_ref.first && std::isfinite(ip_ref.second)) {
                tracks_.all.ip.IPSig_ref->Fill(std::fabs(ip_ref.second));
            }
            
            // dxy wrt reference
            auto signed_ip_ref = IPTools::signedTransverseImpactParameter(ttk, direction, refVtx);
            if (signed_ip_ref.first) {
                tracks_.all.ip.IP_ref->Fill(std::fabs(signed_ip_ref.second.value()));
                tracks_.all.ip.dxy_ref->Fill(signed_ip_ref.second.value());
                if (signed_ip_ref.second.error() > 0) {
                    tracks_.all.ip.dxySig_ref->Fill(std::fabs(signed_ip_ref.second.significance()));
                }
            }
            
            // dxy wrt origin
            const double dxyErr = trRef->dxyError();
            const double dxy0 = trRef->dxy(origin);
            tracks_.all.ip.dxy_origin->Fill(dxy0);
            if (dxyErr > 0) {
                tracks_.all.ip.dxySig_origin->Fill(std::fabs(dxy0 / dxyErr));
            }
            
            // dxy errors
            tracks_.all.ip.dxyError->Fill(dxyErr);
            if (std::fabs(trRef->eta()) < 1.0) {
                tracks_.all.ip.dxyError_barrel->Fill(dxyErr);
            } else {
                tracks_.all.ip.dxyError_endcap->Fill(dxyErr);
            }
            
            // dxy wrt beamspot
            if (haveBS) {
                auto ip_bs = IPTools::signedTransverseImpactParameter(ttk, direction, bsVtx);
                if (ip_bs.first) {
                    tracks_.all.ip.dxy_beamspot->Fill(ip_bs.second.value());
                    if (ip_bs.second.error() > 0) {
                        tracks_.all.ip.dxySig_beamspot->Fill(std::fabs(ip_bs.second.significance()));
                    }
                }
            }
            
            // dxy wrt avgPV
            if (havePV) {
                auto ip_avgPV = IPTools::signedTransverseImpactParameter(ttk, direction, avgPVVtx);
                if (ip_avgPV.first) {
                    tracks_.all.ip.dxy_avgPV->Fill(ip_avgPV.second.value());
                    if (ip_avgPV.second.error() > 0) {
                        tracks_.all.ip.dxySig_avgPV->Fill(std::fabs(ip_avgPV.second.significance()));
                    }
                }
            }

            // --- SEED-LIKE TRACKS (apply Vertexer cuts) ---
            if (trRef->pt() <= seed_minPt_) continue;
            if (!(ip_ref.first && std::isfinite(ip_ref.second))) continue;
            const double ipSigAbs = std::fabs(ip_ref.second);
            if (ipSigAbs <= seed_minIPSig_) continue;
            
            // Kinematics
            tracks_.seed.pt->Fill(trRef->pt());
            tracks_.seed.eta->Fill(trRef->eta());
            tracks_.seed.phi->Fill(trRef->phi());
            tracks_.seed.momentum->Fill(trRef->p());
            
            // Impact parameters (reuse variables from all-tracks section)
            tracks_.seed.ip.IPSig_ref->Fill(ipSigAbs);
            
            if (signed_ip_ref.first) {
                tracks_.seed.ip.IP_ref->Fill(std::fabs(signed_ip_ref.second.value()));
                tracks_.seed.ip.dxy_ref->Fill(signed_ip_ref.second.value());
                if (signed_ip_ref.second.error() > 0) {
                    tracks_.seed.ip.dxySig_ref->Fill(std::fabs(signed_ip_ref.second.significance()));
                }
            }
            
            // dxy wrt origin (reuse dxy0 and dxyErr already computed above)
            tracks_.seed.ip.dxy_origin->Fill(dxy0);
            if (dxyErr > 0) {
                tracks_.seed.ip.dxySig_origin->Fill(std::fabs(dxy0 / dxyErr));
            }
            
            tracks_.seed.ip.dxyError->Fill(dxyErr);
            if (std::fabs(trRef->eta()) < 1.0) {
                tracks_.seed.ip.dxyError_barrel->Fill(dxyErr);
            } else {
                tracks_.seed.ip.dxyError_endcap->Fill(dxyErr);
            }
            
            if (haveBS) {
                auto ip_bs = IPTools::signedTransverseImpactParameter(ttk, direction, bsVtx);
                if (ip_bs.first) {
                    tracks_.seed.ip.dxy_beamspot->Fill(ip_bs.second.value());
                    if (ip_bs.second.error() > 0) {
                        tracks_.seed.ip.dxySig_beamspot->Fill(std::fabs(ip_bs.second.significance()));
                    }
                }
            }
            
            if (havePV) {
                auto ip_avgPV = IPTools::signedTransverseImpactParameter(ttk, direction, avgPVVtx);
                if (ip_avgPV.first) {
                    tracks_.seed.ip.dxy_avgPV->Fill(ip_avgPV.second.value());
                    if (ip_avgPV.second.error() > 0) {
                        tracks_.seed.ip.dxySig_avgPV->Fill(std::fabs(ip_avgPV.second.significance()));
                    }
                }
            }
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
    // Vertex Loop
    for (unsigned int t = 0; t < verticesH->size(); ++t) {
        const auto& v = verticesH->at(t);

        bool vertexSelected = false; 

        
        // --- VERTEX-ASSOCIATED TRACKS (from vertex.tracks()) ---
        // Loop over tracks associated to vertex
        for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
            reco::TrackRef tr = it->castTo<reco::TrackRef>();
            if (!tr.isNonnull()) continue;
            if (v.trackWeight(*it) < 0.5) continue;

            // Kinematics
            tracks_.vertex.pt->Fill(tr->pt());
            tracks_.vertex.eta->Fill(tr->eta());
            tracks_.vertex.phi->Fill(tr->phi());
            tracks_.vertex.momentum->Fill(tr->p());

            // Build transient track
            reco::TransientTrack ttk = ttBuilder.build(tr);
            GlobalVector direction(tr->px(), tr->py(), tr->pz());
            
            // IP wrt reference
            auto ip_ref = ipSigWrtVertex(ttk, refVtx);
            if (ip_ref.first && std::isfinite(ip_ref.second)) {
                tracks_.vertex.ip.IPSig_ref->Fill(std::fabs(ip_ref.second));
            }
            
            // dxy wrt reference
            auto signed_ip_ref = IPTools::signedTransverseImpactParameter(ttk, direction, refVtx);
            if (signed_ip_ref.first) {
                tracks_.vertex.ip.IP_ref->Fill(std::fabs(signed_ip_ref.second.value()));
                tracks_.vertex.ip.dxy_ref->Fill(signed_ip_ref.second.value());
                if (signed_ip_ref.second.error() > 0) {
                    tracks_.vertex.ip.dxySig_ref->Fill(std::fabs(signed_ip_ref.second.significance()));
                }
            }
            
            // dxy wrt origin
            const math::XYZPoint origin(0.,0.,0.);
            const double dxyErr = tr->dxyError();
            const double dxy0 = tr->dxy(origin);
            tracks_.vertex.ip.dxy_origin->Fill(dxy0);
            if (dxyErr > 0) {
                tracks_.vertex.ip.dxySig_origin->Fill(std::fabs(dxy0 / dxyErr));
            }
            
            // dxy errors
            tracks_.vertex.ip.dxyError->Fill(dxyErr);
            if (std::fabs(tr->eta()) < 1.0) {
                tracks_.vertex.ip.dxyError_barrel->Fill(dxyErr);
            } else {
                tracks_.vertex.ip.dxyError_endcap->Fill(dxyErr);
            }
            
            // dxy wrt beamspot
            if (haveBS) {
                auto ip_bs = IPTools::signedTransverseImpactParameter(ttk, direction, bsVtx);
                if (ip_bs.first) {
                    tracks_.vertex.ip.dxy_beamspot->Fill(ip_bs.second.value());
                    if (ip_bs.second.error() > 0) {
                        tracks_.vertex.ip.dxySig_beamspot->Fill(std::fabs(ip_bs.second.significance()));
                    }
                }
            }
            
            // dxy wrt avgPV
            if (havePV) {
                auto ip_avgPV = IPTools::signedTransverseImpactParameter(ttk, direction, avgPVVtx);
                if (ip_avgPV.first) {
                    tracks_.vertex.ip.dxy_avgPV->Fill(ip_avgPV.second.value());
                    if (ip_avgPV.second.error() > 0) {
                        tracks_.vertex.ip.dxySig_avgPV->Fill(std::fabs(ip_avgPV.second.significance()));
                    }
                }
            }
            
            // dxy wrt primary displaced vertex (highest sum pT^2)
            if (pmvtx) {
                auto ip_pmvtx = IPTools::signedTransverseImpactParameter(ttk, direction, *pmvtx);
                if (ip_pmvtx.first) {
                    tracks_.vertex.ip.dxy_primaryVtx->Fill(ip_pmvtx.second.value());
                    if (ip_pmvtx.second.error() > 0) {
                        tracks_.vertex.ip.dxySig_primaryVtx->Fill(std::fabs(ip_pmvtx.second.significance()));
                    }
                }
            }
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
        double invMass = sumVec.M();
        double avg_dxy = (ntk > 0 ? sum_dxy / ntk : 0.0);
        double avg_dxyErr = (ntk > 0 ? sum_dxyErr / ntk : 0.0);

        // --- Distance calculations (ALWAYS 2D for consistency with Vertexer) ---
        // Reference distance
        Measurement1D dBVref_meas = vertexDist2D.distance(v, refVtx);
        double dBVref = dBVref_meas.value();
        double dBV_err = dBVref_meas.error();
        
        // Origin distance - use default-constructed Error (all zeros)
        Vertex originVtx(Vertex::Point(0,0,0), Vertex::Error());
        Measurement1D dBV00_meas = vertexDist2D.distance(v, originVtx);
        double dBV00 = dBV00_meas.value();

        // REMOVE the duplicate calculation block that was here!
        // The old code had a second calculation using use_2d_vertex_dist_ toggle
        // which was overwriting these correct 2D values with 3D values.

        // Loop over all ntk × angle branches
        for (size_t i_ntk = 0; i_ntk < cut_ntk_.size(); ++i_ntk) {
            const auto& ntk_set = cut_ntk_[i_ntk];
            
            // Check ntk condition
            bool ntk_pass = ntk_set.empty();
            if (!ntk_pass) {
                for (int allowed_ntk : ntk_set) {
                    if (ntk == allowed_ntk) {
                        ntk_pass = true;
                        break;
                    }
                }
            }
            if (!ntk_pass) continue;
            
            // Build ntk branch name (same logic as beginJob)
            std::string ntkBranchName;
            if (ntk_set.empty()) {
                ntkBranchName = "ntk_any";
            } else if (ntk_set.size() == 1) {
                ntkBranchName = "ntk_" + std::to_string(ntk_set[0]);
            } else {
                ntkBranchName = "ntk";
                for (size_t j = 0; j < ntk_set.size(); ++j) {
                    if (j > 0) ntkBranchName += "_or_";
                    ntkBranchName += std::to_string(ntk_set[j]);
                }
            }
            
            for (size_t i_angle = 0; i_angle < cut_opening_angle_min_.size(); ++i_angle) {
                double angle_cut = cut_opening_angle_min_[i_angle];
                
                // Check angle condition
                if (angle_cut >= 0 && minAngle < angle_cut) continue;
                
                // Build angle branch name (same logic as beginJob)
                std::string angleBranchName;
                if (angle_cut < 0) {
                    angleBranchName = "angle_any";
                } else {
                    std::ostringstream oss;
                    oss << "angle_gt_" << std::fixed << std::setprecision(2) << angle_cut;
                    angleBranchName = oss.str();
                    std::replace(angleBranchName.begin(), angleBranchName.end(), '.', 'p');
                }
                
                std::string branchKey = ntkBranchName + "/" + angleBranchName;
                
                // Apply other selection criteria
                if(required_invmass  != -1 && invMass < required_invmass) continue;
                if(required_chi2     != -1 && v.normalizedChi2() > required_chi2) continue;
                if(required_dBV_min  != -1 && dBVref < required_dBV_min) continue;
                if(required_dBV_max  != -1 && dBVref > required_dBV_max) continue;
                if(required_dxy_min  != -1 && avg_dxy < required_dxy_min) continue;
                if(required_dxy_max  != -1 && avg_dxy > required_dxy_max) continue;
                if(required_dBV_error!= -1 && dBV_err > required_dBV_error) continue;
                if(required_dxy_error!= -1 && avg_dxyErr > required_dxy_error) continue;

                vertexSelected = true;  // Vertex succeeded selection for at least one branch

                // Fill histograms for this branch
                auto& branch = vertices_.branches[branchKey];

                
                branch.chi2norm->Fill(v.normalizedChi2());
                branch.nTracks->Fill(ntk);
                branch.xy_global->Fill(v.x(), v.y());
                branch.xy_ref->Fill(v.x() - refVtx.x(), v.y() - refVtx.y());
                branch.distance.dBV_ref->Fill(dBVref);
                branch.distance.dBV_origin->Fill(dBV00);
                branch.distance.dBV_error->Fill(dBV_err);
                branch.openingAngle.mean->Fill(meanAngle);
                branch.openingAngle.min->Fill(minAngle);
                branch.openingAngle.max->Fill(maxAngle);
                
                // Additional reference distances (always 2D)
                if (havePV) {
                    Measurement1D dBVavgPV = vertexDist2D.distance(v, avgPVVtx);
                    branch.distance.dBV_avgPV->Fill(dBVavgPV.value());
                }
                if (haveBS) {
                    Measurement1D dBVbs = vertexDist2D.distance(v, bsVtx);
                    branch.distance.dBV_beamspot->Fill(dBVbs.value());
                }
                
                branch.pt->Fill(sumVec.Pt());
                branch.eta->Fill(sumVec.Eta());
                branch.phi->Fill(sumVec.Phi());
                branch.mass->Fill(invMass);
                
                // Topology
                if (std::fabs(sumVec.Eta()) < 1.0) {
                    branch.barrel.eta->Fill(sumVec.Eta());
                    branch.barrel.dBV->Fill(dBVref);
                    branch.barrel.mass->Fill(invMass);
                    branch.barrel.xy_global->Fill(v.x(), v.y());
                    branch.barrel.xy_ref->Fill(v.x() - refVtx.x(), v.y() - refVtx.y());
                } else {
                    branch.endcap.eta->Fill(sumVec.Eta());
                    branch.endcap.dBV->Fill(dBVref);
                    branch.endcap.mass->Fill(invMass);
                    branch.endcap.xy_global->Fill(v.x(), v.y());
                    branch.endcap.xy_ref->Fill(v.x() - refVtx.x(), v.y() - refVtx.y());
                    
                    if (sumVec.Eta() < -1.0) {
                        branch.leftEndcap.eta->Fill(sumVec.Eta());
                        branch.leftEndcap.dBV->Fill(dBVref);
                        branch.leftEndcap.mass->Fill(invMass);
                        branch.leftEndcap.xy_global->Fill(v.x(), v.y());
                    } else if (sumVec.Eta() > 1.0) {
                        branch.rightEndcap.eta->Fill(sumVec.Eta());
                        branch.rightEndcap.dBV->Fill(dBVref);
                        branch.rightEndcap.mass->Fill(invMass);
                        branch.rightEndcap.xy_global->Fill(v.x(), v.y());
                    }
                }
                
                // PV regions
                if (PVBoundary1 != -1) {
                    if (pvRegion == 0) {
                        branch.regionA.xy_global->Fill(v.x(), v.y());
                        branch.regionA.xy_ref->Fill(v.x() - refVtx.x(), v.y() - refVtx.y());
                        branch.regionA.dBV->Fill(dBVref);
                        branch.regionA.mass->Fill(invMass);
                    } else if (pvRegion == 1) {
                        branch.regionB.xy_global->Fill(v.x(), v.y());
                        branch.regionB.xy_ref->Fill(v.x() - refVtx.x(), v.y() - refVtx.y());
                        branch.regionB.dBV->Fill(dBVref);
                        branch.regionB.mass->Fill(invMass);
                    } else {
                        branch.regionC.xy_global->Fill(v.x(), v.y());
                        branch.regionC.xy_ref->Fill(v.x() - refVtx.x(), v.y() - refVtx.y());
                        branch.regionC.dBV->Fill(dBVref);
                        branch.regionC.mass->Fill(invMass);
                    }
                }
                

            }
        }
        if (vertexSelected) {
            ++nSelVertices;
        }
    }

    event_.nSelectedVertices->Fill(static_cast<double>(nSelVertices));
}

// Helper method to determine the reference vertex based on preference and availability
std::pair<bool, std::string> ScoutingPlotMakerRun3::determineReferenceVertex(
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

void ScoutingPlotMakerRun3::endJob() {
    // With TFileService the histograms get written automatically.
    // Avoid calling Draw() or Write() here to be safe in multithreaded contexts.
}

void ScoutingPlotMakerRun3::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    
    // Discrete ntk as VPSet
    edm::ParameterSetDescription ntkPSet;
    ntkPSet.add<std::vector<int>>("values", {});
    desc.addVPSet("cut_ntk", ntkPSet, {});
    
    desc.add<std::vector<double>>("cut_opening_angle_min", {-1.0});
    desc.add<double>("required_invmass", -1.0);
    desc.add<double>("required_chi2", -11.0);
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
    descriptions.add("scoutingPlotMakerRun3", desc);
}

// Add these helper method implementations after the constructor/destructor but before analyze()

// Implementation of vertex_track_set helper
ScoutingPlotMakerRun3::track_set ScoutingPlotMakerRun3::vertex_track_set(const reco::Vertex& v, const double min_weight) const {
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
ScoutingPlotMakerRun3::track_vec ScoutingPlotMakerRun3::vertex_track_vec(const reco::Vertex& v, const double min_weight) const {
    track_set s = vertex_track_set(v, min_weight);
    return track_vec(s.begin(), s.end());
}

// Make sure the module is registered with the framework correctly
// This should be at the bottom of the file after all class implementations
DEFINE_FWK_MODULE(ScoutingPlotMakerRun3);