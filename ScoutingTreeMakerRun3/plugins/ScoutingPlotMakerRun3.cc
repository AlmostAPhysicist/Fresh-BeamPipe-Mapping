// -*- C++ -*-
// Package:    Vertexing/ScoutingTreeMakerRun3
// Class:      ScoutingPlotMakerRun3
//
// Refactor: Use maps of folders and histogram containers per fill-mode
//           ("Nominal" and optionally "WithHitCuts") to avoid duplicated
//           code paths and special-casing.  Created: assistant rewrite 2026

#include <memory>
#include <vector>
#include <set>
#include <string>
#include <cmath>
#include <fstream>
#include <sstream>
#include <map>
#include <iomanip>
#include <algorithm>

#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TTree.h"
#include "TLorentzVector.h"
#include "TMath.h"
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
#include "DataFormats/Scouting/interface/Run3ScoutingTrack.h"

#include "RecoVertex/VertexTools/interface/VertexDistanceXY.h"
#include "RecoVertex/VertexTools/interface/VertexDistance3D.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "DataFormats/Math/interface/Point3D.h"
#include "TrackingTools/TransientTrack/interface/TransientTrack.h"
#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "TrackingTools/IPTools/interface/IPTools.h"
#include "CondFormats/DataRecord/interface/BeamSpotOnlineHLTObjectsRcd.h"
#include "CondFormats/BeamSpotObjects/interface/BeamSpotOnlineObjects.h"

class ScoutingPlotMakerRun3 : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
    explicit ScoutingPlotMakerRun3(const edm::ParameterSet&);
    ~ScoutingPlotMakerRun3() override;

    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
    void beginJob() override;
    void analyze(const edm::Event&, const edm::EventSetup&) override;
    void endJob() override;

    // --- configuration members (same as before) ---
    const std::vector<std::vector<int>> cut_ntk_;
    const std::vector<double> cut_opening_angle_min_;
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
    const bool useOnlineBeamSpot_;

    // Input tokens
    const edm::EDGetTokenT<std::vector<reco::Vertex>> verticesToken;
    const edm::EDGetTokenT<reco::BeamSpot> beamspot_token;
    const edm::EDGetTokenT<std::vector<reco::Track>> tracksToken;
    const edm::EDGetTokenT<std::vector<reco::Vertex>> primaryVerticesToken;
    const edm::EDGetTokenT<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>> trackToScoutingRefToken_;
    const edm::ESGetToken<BeamSpotOnlineObjects, BeamSpotOnlineHLTObjectsRcd> bsOnlineToken_;

    // Histogram containers (same struct definitions as before)
    struct BranchHistos {
        TH1F* chi2norm = nullptr;
        TH1F* pt = nullptr;
        TH1F* eta = nullptr;
        TH1F* phi = nullptr;
        TH1F* mass = nullptr;
        TH1F* nTracks = nullptr;
        TH2F* xy_global = nullptr;
        TH2F* xy_ref = nullptr;
        struct {
            TH1F* dBV_origin = nullptr;
            TH1F* dBV_ref = nullptr;
            TH1F* dBV_beamspot = nullptr;
            TH1F* dBV_avgPV = nullptr;
            TH1F* dBV_error = nullptr;
        } distance;
        struct {
            TH1F* pairwise = nullptr;
            TH1F* mean = nullptr;
            TH1F* min = nullptr;
            TH1F* max = nullptr;
        } openingAngle;
        struct {
            TH1F* eta = nullptr;
            TH1F* mass = nullptr;
            TH1F* dBV = nullptr;
            TH2F* xy_global = nullptr;
            TH2F* xy_ref = nullptr;
        } barrel, endcap, leftEndcap, rightEndcap;
        struct {
            TH1F* mass = nullptr;
            TH1F* dBV = nullptr;
            TH2F* xy_global = nullptr;
            TH2F* xy_ref = nullptr;
        } regionA, regionB, regionC;
    };

    struct EventHistos {
        TH1F* nPrimaryVertices = nullptr;
        TH1F* nSelectedVertices = nullptr;
        TH2F* primaryVertices_xy = nullptr;
        TH2F* beamspot_xy = nullptr;
        TH2F* avgPV_vs_beamspot = nullptr;
        TH2F* offlineVSonlineBeamSpot = nullptr;
    };

    struct VertexHistos {
        struct {
            TH1F* chi2norm = nullptr;
            TH1F* nTracks = nullptr;
        } all;
        std::map<std::string, BranchHistos> branches;
    };

    struct TrackHistos {
        struct {
            TH1F* pt = nullptr;
            TH1F* eta = nullptr;
            TH1F* phi = nullptr;
            TH1F* momentum = nullptr;
            TH1F* nPixelHits = nullptr;
            TH1F* nStripHits = nullptr;
            TH1F* nTrackerLayers = nullptr;
            TH2F* nHits_vs_dxy = nullptr;
            TH2F* nHits_vs_dxyError = nullptr;
            struct {
                TH1F* IP_ref = nullptr;
                TH1F* IPSig_ref = nullptr;
                TH1F* dxy_origin = nullptr;
                TH1F* dxySig_origin = nullptr;
                TH1F* dxy_ref = nullptr;
                TH1F* dxySig_ref = nullptr;
                TH1F* dxy_beamspot = nullptr;
                TH1F* dxySig_beamspot = nullptr;
                TH1F* dxy_avgPV = nullptr;
                TH1F* dxySig_avgPV = nullptr;
                TH1F* dxy_primaryVtx = nullptr;
                TH1F* dxySig_primaryVtx = nullptr;
                TH1F* dxyError = nullptr;
                TH1F* dxyError_barrel = nullptr;
                TH1F* dxyError_endcap = nullptr;
            } ip;
        } all, seed, vertex;
    };

    // Maps keyed by fill-mode name ("Nominal" and optionally "WithHitCuts")
    std::vector<std::string> fillModes_; // active mode names
    std::map<std::string, EventHistos> event_map_;
    std::map<std::string, VertexHistos> vertices_map_;
    std::map<std::string, TrackHistos> tracks_map_;

    // transient track builder token
    edm::ESGetToken<TransientTrackBuilder, TransientTrackRecord> ttBuilderToken_;

    // seed-like selection controls
    const double seed_minIPSig_;
    const double seed_minPt_;
    const bool   seed_use2DTrackDist_;
    const bool   use_2d_vertex_dist_;
    const double seed_maxIPSig_;
    const int    seed_minPixelHits_;
    const int    seed_minStripHits_;
    const int    seed_minTrackerLayers_;

    // global hit-cut thresholds
    const int hit_minPixelHits_;
    const int hit_minStripHits_;
    const int hit_minTrackerLayers_;
    const bool applyHitCuts_;

    // debug histos (kept as singletons for quick checks)
    TH1F* h_allTracks_ipSig_ref = nullptr;
    TH1F* h_allTracks_simpleDxySig = nullptr;
    TH1F* h_allTracks_pt = nullptr;
    TH1F* h_vertexTracks_ipSig_ref = nullptr;
    TH1F* h_vertexTracks_ipSig_vtx = nullptr;
    TH1F* h_vertexTracks_pt = nullptr;
    TH1F* h_seedTracks_ipSig_ref = nullptr;
    TH1F* h_seedTracks_pt = nullptr;
    TH1F* h_seedTracks_eta = nullptr;
    TH1F* h_seedTracks_phi = nullptr;

    // utility: map mode name to FillMode-like boolean decision
    inline bool allowFillMode(const std::string &modeName, bool passesHitCuts) const {
        if (modeName == "Nominal") return true;
        if (modeName == "WithHitCuts") return passesHitCuts;
        return false;
    }

    // helper types
    typedef std::set<reco::TrackRef> track_set;
    typedef std::vector<reco::TrackRef> track_vec;
    track_set vertex_track_set(const reco::Vertex & v, const double min_weight = 0.5) const;
    track_vec vertex_track_vec(const reco::Vertex & v, const double min_weight = 0.5) const;

    std::pair<bool, std::string> determineReferenceVertex(
        const edm::Event& iEvent,
        const edm::EventSetup& iSetup,
        reco::Vertex& refVtx,
        bool& havePV,
        reco::Vertex& avgPVVtx,
        bool& haveBS,
        reco::Vertex& bsVtx);
};

// -----------------------------------------------------------------------------
// Constructor / destructor
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
    useOnlineBeamSpot_(iConfig.getUntrackedParameter<bool>("useOnlineBeamSpot", false)),
    verticesToken(consumes<std::vector<reco::Vertex>>(iConfig.getParameter<edm::InputTag>("displacedVertices"))),
    beamspot_token(consumes<reco::BeamSpot>(iConfig.getParameter<edm::InputTag>("beamspot_src"))),
    tracksToken(consumes<std::vector<reco::Track>>(iConfig.getParameter<edm::InputTag>("tracks"))),
    primaryVerticesToken(consumes<std::vector<reco::Vertex>>(iConfig.getParameter<edm::InputTag>("primaryVertices"))),
    trackToScoutingRefToken_(consumes<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>>(
        edm::InputTag("hltScoutingUnpackProducer", "Track-RefToOriginal"))),
    bsOnlineToken_(esConsumes<BeamSpotOnlineObjects, BeamSpotOnlineHLTObjectsRcd>()),
    ttBuilderToken_(esConsumes(edm::ESInputTag("", "TransientTrackBuilder"))),
    seed_minIPSig_( iConfig.existsAs<double>("minSeedIPSig", true) ?
                    iConfig.getUntrackedParameter<double>("minSeedIPSig") :
                    iConfig.getUntrackedParameter<double>("seed_minIPSig", 4.0) ),
    seed_minPt_(    iConfig.existsAs<double>("minSeedPt", true) ?
                    iConfig.getUntrackedParameter<double>("minSeedPt") :
                    iConfig.getUntrackedParameter<double>("seed_minPt", 0.9) ),
    seed_use2DTrackDist_( iConfig.existsAs<bool>("use_2d_track_dist", true) ?
                          iConfig.getParameter<bool>("use_2d_track_dist") :
                          iConfig.getUntrackedParameter<bool>("seed_use2DTrackDist", false) ),
    use_2d_vertex_dist_( iConfig.existsAs<bool>("use_2d_vertex_dist", true) ?
                         iConfig.getParameter<bool>("use_2d_vertex_dist") :
                         false ),
    seed_maxIPSig_(        iConfig.getUntrackedParameter<double>("seed_maxIPSig", 1e9) ),
    seed_minPixelHits_(    iConfig.getUntrackedParameter<int>("seed_minPixelHits", 0) ),
    seed_minStripHits_(    iConfig.getUntrackedParameter<int>("seed_minStripHits", 0) ),
    seed_minTrackerLayers_(iConfig.getUntrackedParameter<int>("seed_minTrackerLayers", 0) ),
    hit_minPixelHits_( iConfig.getUntrackedParameter<int>("hit_minPixelHits", 3) ),
    hit_minStripHits_( iConfig.getUntrackedParameter<int>("hit_minStripHits", 2) ),
    hit_minTrackerLayers_( iConfig.getUntrackedParameter<int>("hit_minTrackerLayers", 6) ),
    applyHitCuts_( iConfig.getParameter<bool>("applyHitCuts") )
{
    usesResource("TFileService");
}

ScoutingPlotMakerRun3::~ScoutingPlotMakerRun3() {
    // histograms owned by TFileService
}

// -----------------------------------------------------------------------------
// beginJob: create folders & histograms per fill mode using maps
void ScoutingPlotMakerRun3::beginJob() {
    edm::Service<TFileService> fs;

    // determine active fill modes
    fillModes_.clear();
    fillModes_.push_back("Nominal");
    if (applyHitCuts_) fillModes_.push_back("WithHitCuts");

    // For each mode create top-level directories and histograms stored in maps
    for (const auto &mode : fillModes_) {
        TFileDirectory root = fs->mkdir(mode.c_str());

        // EVENT-level histos
        TFileDirectory eventDir = root.mkdir("Event");
        EventHistos eh;
        eh.nPrimaryVertices = eventDir.make<TH1F>("nPrimaryVertices","Number of Primary Vertices; nPV; Events",100,0,100);
        eh.nSelectedVertices = eventDir.make<TH1F>("nSelectedVertices","Number of Selected Vertices; N_{vtx}; Events",100,0,100);
        eh.primaryVertices_xy = eventDir.make<TH2F>("primaryVertices_xy","Primary Vertices XY; X [cm]; Y [cm]",200,-1,1,200,-1,1);
        eh.beamspot_xy = eventDir.make<TH2F>("beamspot_xy","Beamspot Position; x_{BS} [cm]; y_{BS} [cm]",200,-1,1,200,-1,1);
        eh.avgPV_vs_beamspot = eventDir.make<TH2F>("avgPV_vs_beamspot","AvgPV - Beamspot; #Delta x [cm]; #Delta y [cm]",200,-1,1,200,-1,1);
        eh.offlineVSonlineBeamSpot = eventDir.make<TH2F>(
            "offlineVSonlineBeamSpot",
            "Offline - Online Beamspot; #Delta x [cm]; #Delta y [cm]",
            100,-0.2,0.2,100,-0.2,0.2);
        event_map_.emplace(mode, std::move(eh));

        // VERTICES: create Selected / ntk branches / angle sub-branches
        TFileDirectory verticesRoot = root.mkdir("Vertices");
        TFileDirectory selectedRoot = verticesRoot.mkdir("Selected");

        VertexHistos vhs;
        // create per-ntk × angle branches
        for (size_t i_ntk = 0; i_ntk < cut_ntk_.size(); ++i_ntk) {
            const auto& ntk_set = cut_ntk_[i_ntk];
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

            TFileDirectory ntkDir = selectedRoot.mkdir(ntkBranchName);

            for (size_t ia = 0; ia < cut_opening_angle_min_.size(); ++ia) {
                double angle_cut = cut_opening_angle_min_[ia];
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

                BranchHistos bh;
                TFileDirectory kinDir = branchDir.mkdir("Kinematics");
                bh.chi2norm = kinDir.make<TH1F>("chi2norm","Vertex #chi^{2}/ndof; #chi^{2}/ndof; Vertices",200,0,20);
                bh.pt = kinDir.make<TH1F>("pt","Vertex p_{T}; p_{T} [GeV]; Vertices",100,0,100);
                bh.eta = kinDir.make<TH1F>("eta","Vertex #eta; #eta; Vertices",100,-5,5);
                bh.phi = kinDir.make<TH1F>("phi","Vertex #phi; #phi; Vertices",100,-3.14,3.14);
                bh.mass = kinDir.make<TH1F>("mass","Vertex Mass; Mass [GeV]; Vertices",100,0,10);
                bh.nTracks = kinDir.make<TH1F>("nTracks","Number of Tracks; N_{tracks}; Vertices",50,0,50);

                TFileDirectory spatialDir = branchDir.mkdir("Spatial");
                bh.xy_global = spatialDir.make<TH2F>("xy_global","Vertex XY (Global); X [cm]; Y [cm]",800,-10,10,800,-10,10);
                bh.xy_ref = spatialDir.make<TH2F>("xy_ref","Vertex XY (ref); X-X_{ref} [cm]; Y-Y_{ref} [cm]",800,-10,10,800,-10,10);

                TFileDirectory distDir = branchDir.mkdir("Distance");
                bh.distance.dBV_origin = distDir.make<TH1F>("dBV_origin","d_{BV} wrt (0,0); d_{BV} [cm]; Vertices",200,0,10);
                bh.distance.dBV_ref = distDir.make<TH1F>("dBV_ref","d_{BV} wrt ref; d_{BV} [cm]; Vertices",200,0,10);
                bh.distance.dBV_beamspot = distDir.make<TH1F>("dBV_beamspot","d_{BV} wrt BS; d_{BV} [cm]; Vertices",200,0,10);
                bh.distance.dBV_avgPV = distDir.make<TH1F>("dBV_avgPV","d_{BV} wrt avgPV; d_{BV} [cm]; Vertices",200,0,10);
                bh.distance.dBV_error = distDir.make<TH1F>("dBV_error","d_{BV} Uncertainty; #sigma_{dBV} [cm]; Vertices",1000,0,0.1);

                TFileDirectory angleDir = branchDir.mkdir("OpeningAngles");
                bh.openingAngle.pairwise = angleDir.make<TH1F>("pairwise","Opening Angle (pairwise); Angle [rad]; Pairs",180,0,3.14159);
                bh.openingAngle.mean = angleDir.make<TH1F>("mean","Mean Opening Angle; <Angle> [rad]; Vertices",180,0,3.14159);
                bh.openingAngle.min = angleDir.make<TH1F>("min","Min Opening Angle; Min Angle [rad]; Vertices",180,0,3.14159);
                bh.openingAngle.max = angleDir.make<TH1F>("max","Max Opening Angle; Max Angle [rad]; Vertices",180,0,3.14159);

                TFileDirectory topoDir = branchDir.mkdir("Topology");
                TFileDirectory barrelDir = topoDir.mkdir("Barrel");
                bh.barrel.eta = barrelDir.make<TH1F>("eta","#eta (Barrel); #eta; Vertices",100,-3,3);
                bh.barrel.mass = barrelDir.make<TH1F>("mass","Mass (Barrel); Mass [GeV]; Vertices",100,0,10);
                bh.barrel.dBV = barrelDir.make<TH1F>("dBV","d_{BV} (Barrel); d_{BV} [cm]; Vertices",100,0,10);
                bh.barrel.xy_global = barrelDir.make<TH2F>("xy_global","XY (Barrel); X [cm]; Y [cm]",800,-10,10,800,-10,10);
                bh.barrel.xy_ref = barrelDir.make<TH2F>("xy_ref","XY (Barrel, ref); X-X_{ref} [cm]; Y-Y_{ref} [cm]",800,-10,10,800,-10,10);

                TFileDirectory endcapDir = topoDir.mkdir("Endcap");
                bh.endcap.eta = endcapDir.make<TH1F>("eta","#eta (Endcap); #eta; Vertices",100,-3,3);
                bh.endcap.mass = endcapDir.make<TH1F>("mass","Mass (Endcap); Mass [GeV]; Vertices",100,0,10);
                bh.endcap.dBV = endcapDir.make<TH1F>("dBV","d_{BV} (Endcap); d_{BV} [cm]; Vertices",100,0,10);
                bh.endcap.xy_global = endcapDir.make<TH2F>("xy_global","XY (Endcap); X [cm]; Y [cm]",800,-10,10,800,-10,10);
                bh.endcap.xy_ref = endcapDir.make<TH2F>("xy_ref","XY (Endcap, ref); X-X_{ref} [cm]; Y-Y_{ref} [cm]",800,-10,10,800,-10,10);

                TFileDirectory leftEndcapDir = topoDir.mkdir("LeftEndcap");
                bh.leftEndcap.eta = leftEndcapDir.make<TH1F>("eta","#eta (Left Endcap); #eta; Vertices",100,-3,3);
                bh.leftEndcap.mass = leftEndcapDir.make<TH1F>("mass","Mass (Left Endcap); Mass [GeV]; Vertices",100,0,10);
                bh.leftEndcap.dBV = leftEndcapDir.make<TH1F>("dBV","d_{BV} (Left Endcap); d_{BV} [cm]; Vertices",100,0,10);
                bh.leftEndcap.xy_global = leftEndcapDir.make<TH2F>("xy_global","XY (Left Endcap); X [cm]; Y [cm]",800,-10,10,800,-10,10);

                TFileDirectory rightEndcapDir = topoDir.mkdir("RightEndcap");
                bh.rightEndcap.eta = rightEndcapDir.make<TH1F>("eta","#eta (Right Endcap); #eta; Vertices",100,-3,3);
                bh.rightEndcap.mass = rightEndcapDir.make<TH1F>("mass","Mass (Right Endcap); Mass [GeV]; Vertices",100,0,10);
                bh.rightEndcap.dBV = rightEndcapDir.make<TH1F>("dBV","d_{BV} (Right Endcap); d_{BV} [cm]; Vertices",100,0,10);
                bh.rightEndcap.xy_global = rightEndcapDir.make<TH2F>("xy_global","XY (Right Endcap); X [cm]; Y [cm]",800,-10,10,800,-10,10);

                // PV regions (optional)
                if (PVBoundary1 != -1) {
                    TFileDirectory regionDir = branchDir.mkdir("PVRegions");
                    TFileDirectory regA = regionDir.mkdir("RegionA");
                    bh.regionA.mass = regA.make<TH1F>("mass","Mass (Region A); Mass [GeV]; Vertices",100,0,10);
                    bh.regionA.dBV = regA.make<TH1F>("dBV","d_{BV} (Region A); d_{BV} [cm]; Vertices",100,0,10);
                    bh.regionA.xy_global = regA.make<TH2F>("xy_global","XY Global (Region A); X [cm]; Y [cm]",800,-10,10,800,-10,10);
                    bh.regionA.xy_ref = regA.make<TH2F>("xy_ref","XY ref (Region A); X-X_{ref} [cm]; Y-Y_{ref} [cm]",800,-10,10,800,-10,10);

                    TFileDirectory regB = regionDir.mkdir("RegionB");
                    bh.regionB.mass = regB.make<TH1F>("mass","Mass (Region B); Mass [GeV]; Vertices",100,0,10);
                    bh.regionB.dBV = regB.make<TH1F>("dBV","d_{BV} (Region B); d_{BV} [cm]; Vertices",100,0,10);
                    bh.regionB.xy_global = regB.make<TH2F>("xy_global","XY Global (Region B); X [cm]; Y [cm]",800,-10,10,800,-10,10);
                    bh.regionB.xy_ref = regB.make<TH2F>("xy_ref","XY ref (Region B); X-X_{ref} [cm]; Y-Y_{ref} [cm]",800,-10,10,800,-10,10);

                    TFileDirectory regC = regionDir.mkdir("RegionC");
                    bh.regionC.mass = regC.make<TH1F>("mass","Mass (Region C); Mass [GeV]; Vertices",100,0,10);
                    bh.regionC.dBV = regC.make<TH1F>("dBV","d_{BV} (Region C); d_{BV} [cm]; Vertices",100,0,10);
                    bh.regionC.xy_global = regC.make<TH2F>("xy_global","XY Global (Region C); X [cm]; Y [cm]",800,-10,10,800,-10,10);
                    bh.regionC.xy_ref = regC.make<TH2F>("xy_ref","XY ref (Region C); X-X_{ref} [cm]; Y-Y_{ref} [cm]",800,-10,10,800,-10,10);
                }

                vhs.branches.emplace(branchKey, std::move(bh));
            } // angle loop
        } // ntk loop

        vertices_map_.emplace(mode, std::move(vhs));

        // TRACKS: All / SeedLike / VertexAssociated
        TFileDirectory tracksRoot = root.mkdir("Tracks");

        TrackHistos th;
        // All tracks
        TFileDirectory trkAll = tracksRoot.mkdir("All");
        TFileDirectory trkAllKin = trkAll.mkdir("Kinematics");
        th.all.pt = trkAllKin.make<TH1F>("pt","Track p_{T} (all); Transverse Momentum p_{T} [GeV]; Tracks",100,0,100);
        th.all.eta = trkAllKin.make<TH1F>("eta","Track #eta (all); #eta; Tracks",100,-3,3);
        th.all.phi = trkAllKin.make<TH1F>("phi","Track #phi (all); #phi; Tracks",100,-3.14,3.14);
        th.all.momentum = trkAllKin.make<TH1F>("momentum","Track Momentum (all); Total Momentum p [GeV]; Tracks",100,0,100);
        th.all.nPixelHits = trkAll.make<TH1F>("nPixelHits","Pixel hits per track (all);N pixel hits;Tracks",10,0,10);
        th.all.nStripHits = trkAll.make<TH1F>("nStripHits","Strip hits per track (all);N strip hits;Tracks",30,0,30);
        th.all.nTrackerLayers = trkAll.make<TH1F>("nTrackerLayers","Tracker layers with measurement (all);N layers;Tracks",30,0,30);
        th.all.nHits_vs_dxy = trkAll.make<TH2F>("nHits_vs_dxy","Hits vs dxy (all);d_{xy} [cm];N hits",200,-2.0,2.0,40,0,40);
        th.all.nHits_vs_dxyError = trkAll.make<TH2F>("nHits_vs_dxyError","Hits vs dxy error (all);#sigma_{dxy} [cm];N hits",200,0.0,0.05,40,0,40);

        TFileDirectory trkAllIP = trkAll.mkdir("ImpactParameter");
        th.all.ip.IP_ref = trkAllIP.make<TH1F>("IP_ref","Impact Parameter wrt ref (all); IP [cm]; Tracks",500,0,5);
        th.all.ip.IPSig_ref = trkAllIP.make<TH1F>("IPSig_ref","|IP|/#sigma wrt ref (all); |IP|/#sigma; Tracks",100,0,50);
        th.all.ip.dxy_origin = trkAllIP.make<TH1F>("dxy_origin","dxy wrt (0,0) (all); dxy [cm]; Tracks",200,-2,2);
        th.all.ip.dxySig_origin = trkAllIP.make<TH1F>("dxySig_origin","|dxy|/#sigma wrt (0,0) (all); |dxy|/#sigma; Tracks",100,0,50);
        th.all.ip.dxy_ref = trkAllIP.make<TH1F>("dxy_ref","dxy wrt ref (all); dxy [cm]; Tracks",500,-5,5);
        th.all.ip.dxySig_ref = trkAllIP.make<TH1F>("dxySig_ref","|dxy|/#sigma wrt ref (all); |dxy|/#sigma; Tracks",100,0,50);
        th.all.ip.dxy_beamspot = trkAllIP.make<TH1F>("dxy_beamspot","dxy wrt BS (all); dxy [cm]; Tracks",200,-2,2);
        th.all.ip.dxySig_beamspot = trkAllIP.make<TH1F>("dxySig_beamspot","|dxy|/#sigma wrt BS (all); |dxy|/#sigma; Tracks",100,0,50);
        th.all.ip.dxy_avgPV = trkAllIP.make<TH1F>("dxy_avgPV","dxy wrt avgPV (all); dxy [cm]; Tracks",200,-2,2);
        th.all.ip.dxySig_avgPV = trkAllIP.make<TH1F>("dxySig_avgPV","|dxy|/#sigma wrt avgPV (all); |dxy|/#sigma; Tracks",100,0,50);
        th.all.ip.dxyError = trkAllIP.make<TH1F>("dxyError","dxy Error (all); #sigma_{dxy} [cm]; Tracks",200,0,0.05);
        th.all.ip.dxyError_barrel = trkAllIP.make<TH1F>("dxyError_barrel","dxy Error Barrel (all); #sigma_{dxy} [cm]; Tracks",200,0,0.05);
        th.all.ip.dxyError_endcap = trkAllIP.make<TH1F>("dxyError_endcap","dxy Error Endcap (all); #sigma_{dxy} [cm]; Tracks",200,0,0.05);

        // Seed-like tracks
        TFileDirectory trkSeed = tracksRoot.mkdir("SeedLike");
        TFileDirectory trkSeedKin = trkSeed.mkdir("Kinematics");
        th.seed.pt = trkSeedKin.make<TH1F>("pt","Track p_{T} (seed-like); Transverse Momentum p_{T} [GeV]; Tracks",100,0,100);
        th.seed.eta = trkSeedKin.make<TH1F>("eta","Track #eta (seed-like); #eta; Tracks",100,-3,3);
        th.seed.phi = trkSeedKin.make<TH1F>("phi","Track #phi (seed-like); #phi; Tracks",100,-3.14,3.14);
        th.seed.momentum = trkSeedKin.make<TH1F>("momentum","Track Momentum (seed-like); Total Momentum p [GeV]; Tracks",100,0,100);
        th.seed.nPixelHits = trkSeed.make<TH1F>("nPixelHits","Pixel hits per track (seed-like);N pixel hits;Tracks",10,0,10);
        th.seed.nStripHits = trkSeed.make<TH1F>("nStripHits","Strip hits per track (seed-like);N strip hits;Tracks",30,0,30);
        th.seed.nTrackerLayers = trkSeed.make<TH1F>("nTrackerLayers","Tracker layers with measurement (seed-like);N layers;Tracks",30,0,30);
        th.seed.nHits_vs_dxy = trkSeed.make<TH2F>("nHits_vs_dxy","Hits vs dxy (seed-like);d_{xy} [cm];N hits",200,-2.0,2.0,40,0,40);
        th.seed.nHits_vs_dxyError = trkSeed.make<TH2F>("nHits_vs_dxyError","Hits vs dxy error (seed-like);#sigma_{dxy} [cm];N hits",200,0.0,0.05,40,0,40);

        TFileDirectory trkSeedIP = trkSeed.mkdir("ImpactParameter");
        th.seed.ip.IP_ref = trkSeedIP.make<TH1F>("IP_ref","Impact Parameter wrt ref (seed-like); IP [cm]; Tracks",500,0,5);
        th.seed.ip.IPSig_ref = trkSeedIP.make<TH1F>("IPSig_ref","|IP|/#sigma wrt ref (seed-like); |IP|/#sigma; Tracks",100,0,50);
        th.seed.ip.dxy_origin = trkSeedIP.make<TH1F>("dxy_origin","dxy wrt (0,0) (seed-like); dxy [cm]; Tracks",200,-2,2);
        th.seed.ip.dxySig_origin = trkSeedIP.make<TH1F>("dxySig_origin","|dxy|/#sigma wrt (0,0) (seed-like); |dxy|/#sigma; Tracks",100,0,50);
        th.seed.ip.dxy_ref = trkSeedIP.make<TH1F>("dxy_ref","dxy wrt ref (seed-like); dxy [cm]; Tracks",500,-5,5);
        th.seed.ip.dxySig_ref = trkSeedIP.make<TH1F>("dxySig_ref","|dxy|/#sigma wrt ref (seed-like); |dxy|/#sigma; Tracks",100,0,50);
        th.seed.ip.dxy_beamspot = trkSeedIP.make<TH1F>("dxy_beamspot","dxy wrt BS (seed-like); dxy [cm]; Tracks",200,-2,2);
        th.seed.ip.dxySig_beamspot = trkSeedIP.make<TH1F>("dxySig_beamspot","|dxy|/#sigma wrt BS (seed-like); |dxy|/#sigma; Tracks",100,0,50);
        th.seed.ip.dxy_avgPV = trkSeedIP.make<TH1F>("dxy_avgPV","dxy wrt avgPV (seed-like); dxy [cm]; Tracks",200,-2,2);
        th.seed.ip.dxySig_avgPV = trkSeedIP.make<TH1F>("dxySig_avgPV","|dxy|/#sigma wrt avgPV (seed-like); |dxy|/#sigma; Tracks",100,0,50);
        th.seed.ip.dxyError = trkSeedIP.make<TH1F>("dxyError","dxy Error (seed-like); #sigma_{dxy} [cm]; Tracks",200,0,0.05);
        th.seed.ip.dxyError_barrel = trkSeedIP.make<TH1F>("dxyError_barrel","dxy Error Barrel (seed-like); #sigma_{dxy} [cm]; Tracks",200,0,0.05);
        th.seed.ip.dxyError_endcap = trkSeedIP.make<TH1F>("dxyError_endcap","dxy Error Endcap (seed-like); #sigma_{dxy} [cm]; Tracks",200,0,0.05);

        // Vertex-associated tracks
        TFileDirectory trkVtx = tracksRoot.mkdir("VertexAssociated");
        TFileDirectory trkVtxKin = trkVtx.mkdir("Kinematics");
        th.vertex.pt = trkVtxKin.make<TH1F>("pt","Track p_{T} (vertex); Transverse Momentum p_{T} [GeV]; Tracks",100,0,100);
        th.vertex.eta = trkVtxKin.make<TH1F>("eta","Track #eta (vertex); #eta; Tracks",100,-3,3);
        th.vertex.phi = trkVtxKin.make<TH1F>("phi","Track #phi (vertex); #phi; Tracks",100,-3.14,3.14);
        th.vertex.momentum = trkVtxKin.make<TH1F>("momentum","Track Momentum (vertex); Total Momentum p [GeV]; Tracks",100,0,100);
        th.vertex.nPixelHits = trkVtx.make<TH1F>("nPixelHits","Pixel hits per track (vertex);N pixel hits;Tracks",10,0,10);
        th.vertex.nStripHits = trkVtx.make<TH1F>("nStripHits","Strip hits per track (vertex);N strip hits;Tracks",30,0,30);
        th.vertex.nTrackerLayers = trkVtx.make<TH1F>("nTrackerLayers","Tracker layers with measurement (vertex);N layers;Tracks",30,0,30);
        th.vertex.nHits_vs_dxy = trkVtx.make<TH2F>("nHits_vs_dxy","Hits vs dxy (vertex);d_{xy} [cm];N hits",200,-2.0,2.0,40,0,40);
        th.vertex.nHits_vs_dxyError = trkVtx.make<TH2F>("nHits_vs_dxyError","Hits vs dxy error (vertex);#sigma_{dxy} [cm];N hits",200,0.0,0.05,40,0,40);

        TFileDirectory trkVtxIP = trkVtx.mkdir("ImpactParameter");
        th.vertex.ip.IP_ref = trkVtxIP.make<TH1F>("IP_ref","Impact Parameter wrt ref (vertex); IP [cm]; Tracks",500,0,5);
        th.vertex.ip.IPSig_ref = trkVtxIP.make<TH1F>("IPSig_ref","|IP|/#sigma wrt ref (vertex); |IP|/#sigma; Tracks",100,0,50);
        th.vertex.ip.dxy_origin = trkVtxIP.make<TH1F>("dxy_origin","dxy wrt (0,0) (vertex); dxy [cm]; Tracks",200,-2,2);
        th.vertex.ip.dxySig_origin = trkVtxIP.make<TH1F>("dxySig_origin","|dxy|/#sigma wrt (0,0) (vertex); |dxy|/#sigma; Tracks",100,0,50);
        th.vertex.ip.dxy_ref = trkVtxIP.make<TH1F>("dxy_ref","dxy wrt ref (vertex); dxy [cm]; Tracks",500,-5,5);
        th.vertex.ip.dxySig_ref = trkVtxIP.make<TH1F>("dxySig_ref","|dxy|/#sigma wrt ref (vertex); |dxy|/#sigma; Tracks",100,0,50);
        th.vertex.ip.dxy_beamspot = trkVtxIP.make<TH1F>("dxy_beamspot","dxy wrt BS (vertex); dxy [cm]; Tracks",200,-2,2);
        th.vertex.ip.dxySig_beamspot = trkVtxIP.make<TH1F>("dxySig_beamspot","|dxy|/#sigma wrt BS (vertex); |dxy|/#sigma; Tracks",100,0,50);
        th.vertex.ip.dxy_avgPV = trkVtxIP.make<TH1F>("dxy_avgPV","dxy wrt avgPV (vertex); dxy [cm]; Tracks",200,-2,2);
        th.vertex.ip.dxySig_avgPV = trkVtxIP.make<TH1F>("dxySig_avgPV","|dxy|/#sigma wrt avgPV (vertex); |dxy|/#sigma; Tracks",100,0,50);
        th.vertex.ip.dxy_primaryVtx = trkVtxIP.make<TH1F>("dxy_primaryVtx","dxy wrt primary vertex; dxy [cm]; Tracks",200,-2,2);
        th.vertex.ip.dxySig_primaryVtx = trkVtxIP.make<TH1F>("dxySig_primaryVtx","|dxy|/#sigma wrt primary vertex; |dxy|/#sigma; Tracks",100,0,50);
        th.vertex.ip.dxyError = trkVtxIP.make<TH1F>("dxyError","dxy Error (vertex); #sigma_{dxy} [cm]; Tracks",200,0,0.05);
        th.vertex.ip.dxyError_barrel = trkVtxIP.make<TH1F>("dxyError_barrel","dxy Error Barrel (vertex); #sigma_{dxy} [cm]; Tracks",200,0,0.05);
        th.vertex.ip.dxyError_endcap = trkVtxIP.make<TH1F>("dxyError_endcap","dxy Error Endcap (vertex); #sigma_{dxy} [cm]; Tracks",200,0,0.05);

        tracks_map_.emplace(mode, std::move(th));
    } // end per-mode creation

    // Debug histograms (singletons, using global top-level Nominal directory)
    TFileDirectory dbg = fs->mkdir("Debug");
    h_allTracks_ipSig_ref = dbg.make<TH1F>("h_allTracks_ipSig_ref","ipSig ref all; |IP|/#sigma; Tracks",100,0,50);
    h_allTracks_simpleDxySig = dbg.make<TH1F>("h_allTracks_simpleDxySig","simple dxySig; |dxy|/#sigma; Tracks",100,0,50);
    h_allTracks_pt = dbg.make<TH1F>("h_allTracks_pt","all track pt; pT [GeV]; Tracks",100,0,100);
    h_vertexTracks_ipSig_ref = dbg.make<TH1F>("h_vertexTracks_ipSig_ref","vertex track ipSig ref; |IP|/#sigma; Tracks",100,0,50);
    h_vertexTracks_ipSig_vtx = dbg.make<TH1F>("h_vertexTracks_ipSig_vtx","vertex track ipSig vtx; |IP|/#sigma; Tracks",100,0,50);
    h_vertexTracks_pt = dbg.make<TH1F>("h_vertexTracks_pt","vertex track pT; pT [GeV]; Tracks",100,0,100);
    h_seedTracks_ipSig_ref = dbg.make<TH1F>("h_seedTracks_ipSig_ref","seed ipSig ref; |IP|/#sigma; Tracks",100,0,50);
    h_seedTracks_pt = dbg.make<TH1F>("h_seedTracks_pt","seed pT; pT [GeV]; Tracks",100,0,100);
    h_seedTracks_eta = dbg.make<TH1F>("h_seedTracks_eta","seed eta; #eta; Tracks",100,-3,3);
    h_seedTracks_phi = dbg.make<TH1F>("h_seedTracks_phi","seed phi; #phi; Tracks",100,-3.14,3.14);
}

// -----------------------------------------------------------------------------
// analyze: unified filling using mode maps
void ScoutingPlotMakerRun3::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
    using namespace edm;
    using namespace reco;
    using namespace std;

    // get track collection
    Handle<vector<Track>> tracksH;
    iEvent.getByToken(tracksToken, tracksH);
    if (!tracksH.isValid()) {
        LogWarning("ScoutingPlotMakerRun3") << "Track collection invalid (skip event).";
        return;
    }

    // displaced vertices
    Handle<vector<Vertex>> verticesH;
    iEvent.getByToken(verticesToken, verticesH);
    if (!verticesH.isValid()) {
        LogWarning("ScoutingPlotMakerRun3") << "Displaced vertex collection invalid (skip event).";
        return;
    }

    // transient track builder
    auto const& ttBuilder = iSetup.getData(ttBuilderToken_);

    // reference vertices
    reco::Vertex refVtx, avgPVVtx, bsVtx;
    bool havePV = false, haveBS = false;
    auto [haveRef, refType] = determineReferenceVertex(iEvent, iSetup, refVtx, havePV, avgPVVtx, haveBS, bsVtx);
    if (!haveRef) {
        LogWarning("ScoutingPlotMakerRun3") << "No valid reference (avgPV or BeamSpot). Skipping event.";
        return;
    }

    // Fill event-level beamspot comparison plots (offline vs online) when both are available.
    edm::Handle<reco::BeamSpot> bsOfflineH;
    iEvent.getByToken(beamspot_token, bsOfflineH);
    auto bsOnlineH = iSetup.getHandle(bsOnlineToken_);

    if (bsOfflineH.isValid() && bsOnlineH.isValid()) {
        const double dx = bsOfflineH->x0() - bsOnlineH->x();
        const double dy = bsOfflineH->y0() - bsOnlineH->y();
        for (const auto &mode : fillModes_) {
            event_map_.at(mode).offlineVSonlineBeamSpot->Fill(dx, dy);
        }
    }

    if (havePV) {
        for (const auto &mode : fillModes_) {
            event_map_.at(mode).primaryVertices_xy->Fill(avgPVVtx.x(), avgPVVtx.y());
        }
    }
    if (haveBS) {
        for (const auto &mode : fillModes_) {
            event_map_.at(mode).beamspot_xy->Fill(bsVtx.x(), bsVtx.y());
            if (havePV) {
                event_map_.at(mode).avgPV_vs_beamspot->Fill(avgPVVtx.x() - bsVtx.x(), avgPVVtx.y() - bsVtx.y());
            }
        }
    }

    VertexDistanceXY vertexDist2D;

    // track->scouting map (for hit counts)
    edm::Handle<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>> trackToScoutingRefH;
    iEvent.getByToken(trackToScoutingRefToken_, trackToScoutingRefH);
    const bool haveTrackToScoutingMap = trackToScoutingRefH.isValid();
    if (!haveTrackToScoutingMap) {
        edm::LogWarning("ScoutingPlotMakerRun3") << "Track->ScoutingTrack ValueMap not found; hit plots will be empty.";
    }

    auto passHitCuts = [&](int nPix, int nStrip, int nLayers) {
        if (!applyHitCuts_) return true;
        if (nPix    < hit_minPixelHits_) return false;
        if (nStrip  < hit_minStripHits_) return false;
        if (nLayers < hit_minTrackerLayers_) return false;
        return true;
    };

    // Helper: compute ip significance wrt chosen reference (2D or 3D depending on seed_use2DTrackDist_)
    auto ipSigWrtVertex = [&](const reco::TransientTrack& ttk, const reco::Vertex& vtx) -> pair<bool,double> {
        if (seed_use2DTrackDist_) {
            auto ip = IPTools::absoluteTransverseImpactParameter(ttk, vtx);
            return {ip.first, ip.first ? ip.second.significance() : 0.0};
        } else {
            auto ip = IPTools::absoluteImpactParameter3D(ttk, vtx);
            return {ip.first, ip.first ? ip.second.significance() : 0.0};
        }
    };

    // --- ALL TRACKS: fill into each mode's track containers if allowed ---
    {
        const math::XYZPoint origin(0.,0.,0.);
        for (size_t i = 0; i < tracksH->size(); ++i) {
            reco::TrackRef trRef(tracksH, i);
            if (!trRef.isNonnull()) continue;

            // some reused values
            const double dxyErr = trRef->dxyError();
            const double dxy0 = trRef->dxy(origin);

            // fill singleton debug histos
            h_allTracks_pt->Fill(trRef->pt());
            if (dxyErr > 0 && std::isfinite(dxyErr)) {
                h_allTracks_simpleDxySig->Fill(std::fabs(dxy0 / dxyErr));
            }

            // transient track for IP calculations
            reco::TransientTrack ttk = ttBuilder.build(trRef);
            GlobalVector direction(trRef->px(), trRef->py(), trRef->pz());

            // IP wrt ref
            auto ip_ref = ipSigWrtVertex(ttk, refVtx);
            if (ip_ref.first && std::isfinite(ip_ref.second)) {
                h_allTracks_ipSig_ref->Fill(std::fabs(ip_ref.second));
            }

            auto signed_ip_ref = IPTools::signedTransverseImpactParameter(ttk, direction, refVtx);

            // determine hit counts (if available)
            int nPixelHits = 0, nStripHits = 0, nTrackerLayers = 0;
            if (haveTrackToScoutingMap) {
                auto scoutingRef = (*trackToScoutingRefH)[trRef];
                if (scoutingRef.isNonnull()) {
                    nPixelHits     = scoutingRef->tk_nValidPixelHits();
                    nStripHits     = scoutingRef->tk_nValidStripHits();
                    nTrackerLayers = scoutingRef->tk_nTrackerLayersWithMeasurement();
                }
            }
            int nHits = nPixelHits + nStripHits;
            const bool trackPassesHitCuts = passHitCuts(nPixelHits, nStripHits, nTrackerLayers);

            // For each active fill mode, fill that mode's track histograms if allowed
            for (const auto &mode : fillModes_) {
                if (!allowFillMode(mode, trackPassesHitCuts)) continue;
                auto &T = tracks_map_.at(mode);

                // all-track kinematics
                T.all.pt->Fill(trRef->pt());
                T.all.eta->Fill(trRef->eta());
                T.all.phi->Fill(trRef->phi());
                T.all.momentum->Fill(trRef->p());

                // hits & nHits vs dxy
                T.all.nPixelHits->Fill(nPixelHits);
                T.all.nStripHits->Fill(nStripHits);
                T.all.nTrackerLayers->Fill(nTrackerLayers);
                T.all.nHits_vs_dxy->Fill(dxy0, nHits);
                if (dxyErr > 0 && std::isfinite(dxyErr)) T.all.nHits_vs_dxyError->Fill(dxyErr, nHits);

                // IP / dxy
                if (signed_ip_ref.first) {
                    T.all.ip.IP_ref->Fill(std::fabs(signed_ip_ref.second.value()));
                    T.all.ip.dxy_ref->Fill(signed_ip_ref.second.value());
                    if (signed_ip_ref.second.error() > 0) T.all.ip.dxySig_ref->Fill(std::fabs(signed_ip_ref.second.significance()));
                }
                T.all.ip.dxy_origin->Fill(dxy0);
                if (dxyErr > 0) T.all.ip.dxySig_origin->Fill(std::fabs(dxy0 / dxyErr));
                if (dxyErr > 0 && std::fabs(trRef->eta()) < 1.0) T.all.ip.dxyError_barrel->Fill(dxyErr);
                else if (dxyErr > 0) T.all.ip.dxyError_endcap->Fill(dxyErr);
                T.all.ip.dxyError->Fill(dxyErr);

                // beamspot, avgPV if available
                if (haveBS) {
                    auto ip_bs = IPTools::signedTransverseImpactParameter(ttk, direction, bsVtx);
                    if (ip_bs.first) {
                        T.all.ip.dxy_beamspot->Fill(ip_bs.second.value());
                        if (ip_bs.second.error() > 0) T.all.ip.dxySig_beamspot->Fill(std::fabs(ip_bs.second.significance()));
                    }
                }
                if (havePV) {
                    auto ip_avgPV = IPTools::signedTransverseImpactParameter(ttk, direction, avgPVVtx);
                    if (ip_avgPV.first) {
                        T.all.ip.dxy_avgPV->Fill(ip_avgPV.second.value());
                        if (ip_avgPV.second.error() > 0) T.all.ip.dxySig_avgPV->Fill(std::fabs(ip_avgPV.second.significance()));
                    }
                }
            } // mode loop

            // Apply seed-like selection (same policy as Vertexer-like seeds)
            if (trRef->pt() <= seed_minPt_) continue;
            if (!(ip_ref.first && std::isfinite(ip_ref.second))) continue;
            const double ipSigAbs = std::fabs(ip_ref.second);
            if (ipSigAbs <= seed_minIPSig_) continue;

            // debug fills for seeds
            h_seedTracks_ipSig_ref->Fill(ipSigAbs);
            h_seedTracks_pt->Fill(trRef->pt());
            h_seedTracks_eta->Fill(trRef->eta());
            h_seedTracks_phi->Fill(trRef->phi());

            // fill seed-like histos per mode
            for (const auto &mode : fillModes_) {
                if (!allowFillMode(mode, passHitCuts(nPixelHits, nStripHits, nTrackerLayers))) continue;
                auto &T = tracks_map_.at(mode);
                T.seed.pt->Fill(trRef->pt());
                T.seed.eta->Fill(trRef->eta());
                T.seed.phi->Fill(trRef->phi());
                T.seed.momentum->Fill(trRef->p());

                T.seed.nPixelHits->Fill(nPixelHits);
                T.seed.nStripHits->Fill(nStripHits);
                T.seed.nTrackerLayers->Fill(nTrackerLayers);
                T.seed.nHits_vs_dxy->Fill(dxy0, nHits);
                if (dxyErr > 0 && std::isfinite(dxyErr)) T.seed.nHits_vs_dxyError->Fill(dxyErr, nHits);

                T.seed.ip.IPSig_ref->Fill(ipSigAbs);
                if (signed_ip_ref.first) {
                    T.seed.ip.IP_ref->Fill(std::fabs(signed_ip_ref.second.value()));
                    T.seed.ip.dxy_ref->Fill(signed_ip_ref.second.value());
                    if (signed_ip_ref.second.error() > 0) T.seed.ip.dxySig_ref->Fill(std::fabs(signed_ip_ref.second.significance()));
                }
                T.seed.ip.dxy_origin->Fill(dxy0);
                if (dxyErr > 0) T.seed.ip.dxySig_origin->Fill(std::fabs(dxy0 / dxyErr));
                T.seed.ip.dxyError->Fill(dxyErr);
                if (std::fabs(trRef->eta()) < 1.0) T.seed.ip.dxyError_barrel->Fill(dxyErr);
                else T.seed.ip.dxyError_endcap->Fill(dxyErr);

                if (haveBS) {
                    auto ip_bs = IPTools::signedTransverseImpactParameter(ttk, direction, bsVtx);
                    if (ip_bs.first) {
                        T.seed.ip.dxy_beamspot->Fill(ip_bs.second.value());
                        if (ip_bs.second.error() > 0) T.seed.ip.dxySig_beamspot->Fill(std::fabs(ip_bs.second.significance()));
                    }
                }
                if (havePV) {
                    auto ip_avgPV = IPTools::signedTransverseImpactParameter(ttk, direction, avgPVVtx);
                    if (ip_avgPV.first) {
                        T.seed.ip.dxy_avgPV->Fill(ip_avgPV.second.value());
                        if (ip_avgPV.second.error() > 0) T.seed.ip.dxySig_avgPV->Fill(std::fabs(ip_avgPV.second.significance()));
                    }
                }
            } // seed mode loop
        }
    }

    // PV region determination and fill event-level nPrimaryVertices
    int pvRegion = 0;
    int nPV = 0;
    if (havePV) {
        edm::Handle<std::vector<reco::Vertex>> primaryVerticesH;
        iEvent.getByToken(primaryVerticesToken, primaryVerticesH);
        if (primaryVerticesH.isValid()) {
            nPV = primaryVerticesH->size();
            for (const auto &mode : fillModes_) {
                event_map_.at(mode).nPrimaryVertices->Fill(nPV);
                // primaryVertices_xy already filled when determineReferenceVertex called earlier
            }
        }
        if (PVBoundary1 != -1) {
            if (nPV < PVBoundary1) pvRegion = 0;
            else if (PVBoundary2 != -1 && nPV < PVBoundary2) pvRegion = 1;
            else pvRegion = 2;
        }
    }

    // Choose "primary" displaced vertex (highest sum pT^2)
    [[maybe_unused]] const Vertex* pmvtx = nullptr;
    if (!verticesH->empty()) {
        double bestSumPt = -1.0;
        size_t bestIdx = 0;
        for (size_t iv = 0; iv < verticesH->size(); ++iv) {
            const auto &vtx = verticesH->at(iv);
            double sumPt2 = 0.0;
            for (auto it = vtx.tracks_begin(); it != vtx.tracks_end(); ++it) {
                TrackRef tr = it->castTo<TrackRef>();
                if (tr.isNonnull()) sumPt2 += tr->pt() * tr->pt();
            }
            if (sumPt2 > bestSumPt) { bestSumPt = sumPt2; bestIdx = iv; }
        }
        if (bestSumPt >= 0) pmvtx = &verticesH->at(bestIdx);
    }

    int nSelVertices = 0;

    // Vertex loop: compute tks vector, opening angles, inv mass, distances, then fill per-mode branch histograms
    for (unsigned int t = 0; t < verticesH->size(); ++t) {
        const auto& v = verticesH->at(t);
        bool vertexSelected = false;

        // build vertex track vector (weights >= 0.5)
        track_vec tks = vertex_track_vec(v);
        int ntk = static_cast<int>(tks.size());

        // opening-angle computations
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

        // compute sum 4-vector (pion mass) and average dxy
        TLorentzVector sumVec(0,0,0,0);
        double sum_dxy = 0.0, sum_dxyErr = 0.0;
        for (auto &track : tks) {
            if (!track.isNonnull()) continue;
            constexpr double kPionMass = 0.13957;
            TLorentzVector tv; tv.SetPtEtaPhiM(track->pt(), track->eta(), track->phi(), kPionMass);
            sumVec += tv;

            auto transientTrack = ttBuilder.build(track);
            GlobalVector direction(track->px(), track->py(), track->pz());
            auto ip_ref = IPTools::signedTransverseImpactParameter(transientTrack, direction, refVtx);
            if (ip_ref.first) {
                sum_dxy += ip_ref.second.value();
                sum_dxyErr += ip_ref.second.error();
            }
        }
        double invMass = sumVec.M();
        double avg_dxy = (ntk > 0 ? sum_dxy / ntk : 0.0);
        double avg_dxyErr = (ntk > 0 ? sum_dxyErr / ntk : 0.0);

        // distances (always 2D to match Vertexer)
        Measurement1D dBVref_meas = vertexDist2D.distance(v, refVtx);
        double dBVref = dBVref_meas.value();
        double dBV_err = dBVref_meas.error();

        double dBV00 = std::hypot(v.x(), v.y());

        // vertex-level hit-cuts: all constituent tracks must pass
        bool vertexPassesHitCuts = true;
        if (applyHitCuts_) {
            for (auto &track : tks) {
                if (!track.isNonnull()) continue;
                int nPixelHits = 0, nStripHits = 0, nTrackerLayers = 0;
                if (haveTrackToScoutingMap) {
                    auto scoutingRef = (*trackToScoutingRefH)[track];
                    if (scoutingRef.isNonnull()) {
                        nPixelHits = scoutingRef->tk_nValidPixelHits();
                        nStripHits = scoutingRef->tk_nValidStripHits();
                        nTrackerLayers = scoutingRef->tk_nTrackerLayersWithMeasurement();
                    }
                }
                if (!passHitCuts(nPixelHits, nStripHits, nTrackerLayers)) { vertexPassesHitCuts = false; break; }
            }
        }

        // loop over ntk × angle branch definitions and apply selections then fill per-mode histos
        for (size_t i_ntk = 0; i_ntk < cut_ntk_.size(); ++i_ntk) {
            const auto &ntk_set = cut_ntk_[i_ntk];
            bool ntk_pass = ntk_set.empty();
            if (!ntk_pass) {
                for (int allowed : ntk_set) if (ntk == allowed) { ntk_pass = true; break; }
            }
            if (!ntk_pass) continue;

            std::string ntkBranchName;
            if (ntk_set.empty()) ntkBranchName = "ntk_any";
            else if (ntk_set.size() == 1) ntkBranchName = "ntk_" + std::to_string(ntk_set[0]);
            else {
                ntkBranchName = "ntk";
                for (size_t j = 0; j < ntk_set.size(); ++j) {
                    if (j > 0) ntkBranchName += "_or_";
                    ntkBranchName += std::to_string(ntk_set[j]);
                }
            }

            for (size_t ia = 0; ia < cut_opening_angle_min_.size(); ++ia) {
                double angle_cut = cut_opening_angle_min_[ia];
                if (angle_cut >= 0 && minAngle < angle_cut) continue;

                std::ostringstream oss;
                std::string angleBranchName;
                if (angle_cut < 0) angleBranchName = "angle_any";
                else { oss << "angle_gt_" << std::fixed << std::setprecision(2) << angle_cut; angleBranchName = oss.str(); std::replace(angleBranchName.begin(), angleBranchName.end(), '.', 'p'); }

                std::string branchKey = ntkBranchName + "/" + angleBranchName;

                // additional selection thresholds
                if (required_invmass  != -1 && invMass < required_invmass) continue;
                if (required_chi2     != -1 && v.normalizedChi2() > required_chi2) continue;
                if (required_dBV_min  != -1 && dBVref < required_dBV_min) continue;
                if (required_dBV_max  != -1 && dBVref > required_dBV_max) continue;
                if (required_dxy_min  != -1 && avg_dxy < required_dxy_min) continue;
                if (required_dxy_max  != -1 && avg_dxy > required_dxy_max) continue;
                if (required_dBV_error!= -1 && dBV_err > required_dBV_error) continue;
                if (required_dxy_error!= -1 && avg_dxyErr > required_dxy_error) continue;

                vertexSelected = true;

                // for each mode fill branch histograms if allowed
                for (const auto &mode : fillModes_) {
                    if (!allowFillMode(mode, vertexPassesHitCuts)) continue;

                    auto &vmap = vertices_map_.at(mode);
                    auto branchIt = vmap.branches.find(branchKey);
                    if (branchIt == vmap.branches.end()) continue;
                    BranchHistos &branch = branchIt->second;

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
                } // mode loop
            } // angle loop
        } // ntk loop

        if (vertexSelected) ++nSelVertices;
    } // end vertex loop

    // Fill event-level nSelectedVertices for all active modes
    for (const auto &mode : fillModes_) {
        event_map_.at(mode).nSelectedVertices->Fill(static_cast<double>(nSelVertices));
    }
}

// -----------------------------------------------------------------------------
// determineReferenceVertex (unchanged logic)
std::pair<bool, std::string> ScoutingPlotMakerRun3::determineReferenceVertex(
    const edm::Event& iEvent,
    const edm::EventSetup& iSetup,
    reco::Vertex& refVtx,
    bool& havePV,
    reco::Vertex& avgPVVtx,
    bool& haveBS,
    reco::Vertex& bsVtx) {

    std::string refType = "none";
    havePV = false; haveBS = false;

    edm::Handle<std::vector<reco::Vertex>> primaryVerticesH;
    iEvent.getByToken(primaryVerticesToken, primaryVerticesH);
    if (primaryVerticesH.isValid() && !primaryVerticesH->empty()) {
        double sumX=0, sumY=0, sumZ=0; int validPVs=0;
        for (const auto& pv : *primaryVerticesH) {
            if (!pv.isFake() && pv.ndof() > 4) {
                sumX += pv.x(); sumY += pv.y(); sumZ += pv.z();
                validPVs++;
            }
        }
        if (validPVs > 0) {
            reco::Vertex::Point avgPos(sumX/validPVs, sumY/validPVs, sumZ/validPVs);
            reco::Vertex::Error avgErr;
            for (int i=0;i<3;++i) for (int j=i;j<3;++j) avgErr(i,j)=0.0;
            avgErr(0,0)=0.0015*0.0015;
            avgErr(1,1)=0.0015*0.0015;
            avgErr(2,2)=0.0050*0.0050;
            avgPVVtx = reco::Vertex(avgPos, avgErr);
            havePV = true;
        }
    }

    bool haveOfflineBS = false;
    reco::Vertex bsOfflineVtx;
    edm::Handle<reco::BeamSpot> beamspot;
    iEvent.getByToken(beamspot_token, beamspot);
    if (beamspot.isValid()) {
        bsOfflineVtx = reco::Vertex(beamspot->position(), beamspot->covariance3D());
        haveOfflineBS = true;
    }

    bool haveOnlineBS = false;
    reco::Vertex bsOnlineVtx;
    if (useOnlineBeamSpot_) {
        auto bsOnlineHandle = iSetup.getHandle(bsOnlineToken_);
        if (bsOnlineHandle.isValid()) {
            reco::Vertex::Error bsErr;
            for (int i=0;i<3;++i) for (int j=i;j<3;++j) bsErr(i,j)=bsOnlineHandle->covariance(i,j);
            const reco::Vertex::Point onlinePos(bsOnlineHandle->x(), bsOnlineHandle->y(), bsOnlineHandle->z());
            bsOnlineVtx = reco::Vertex(onlinePos, bsErr);
            haveOnlineBS = true;
        } else {
            edm::LogError("ScoutingPlotMakerRun3")
                << "useOnlineBeamSpot=True but BeamSpotOnlineHLTObjectsRcd is unavailable. "
                << "Falling back to offline beamspot if present.";
        }
    }

    if (useOnlineBeamSpot_ && haveOnlineBS) {
        bsVtx = bsOnlineVtx;
        haveBS = true;
    } else if (haveOfflineBS) {
        bsVtx = bsOfflineVtx;
        haveBS = true;
    } else {
        haveBS = false;
        if (useOnlineBeamSpot_) {
            edm::LogError("ScoutingPlotMakerRun3")
                << "No usable beamspot found (online missing and offline beamspot product invalid). Continuing without beamspot reference.";
        }
    }

    if (refPreference_ == RefPreference::PreferPV && havePV) {
        refVtx = avgPVVtx; return {true, "avgPV"};
    } else if (refPreference_ == RefPreference::PreferBeamSpot && haveBS) {
        refVtx = bsVtx; return {true, "beamspot"};
    } else if (havePV) {
        refVtx = avgPVVtx; return {true, "avgPV (fallback)"};
    } else if (haveBS) {
        refVtx = bsVtx; return {true, "beamspot (fallback)"};
    }
    return {false, "none"};
}

// helper implementations
ScoutingPlotMakerRun3::track_set ScoutingPlotMakerRun3::vertex_track_set(const reco::Vertex& v, const double min_weight) const {
    track_set result;
    for (auto it = v.tracks_begin(), ite = v.tracks_end(); it != ite; ++it) {
        const double w = v.trackWeight(*it);
        if (w >= min_weight) result.insert(it->castTo<reco::TrackRef>());
    }
    return result;
}

ScoutingPlotMakerRun3::track_vec ScoutingPlotMakerRun3::vertex_track_vec(const reco::Vertex& v, const double min_weight) const {
    track_set s = vertex_track_set(v, min_weight);
    return track_vec(s.begin(), s.end());
}

void ScoutingPlotMakerRun3::endJob() {
    // TFileService handles writing.
}

// fillDescriptions (unchanged, keep parameters consistent with previous)
void ScoutingPlotMakerRun3::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    edm::ParameterSetDescription ntkPSet;
    ntkPSet.add<std::vector<int>>("values", {});
    desc.addVPSet("cut_ntk", ntkPSet, {});
    desc.add<std::vector<double>>("cut_opening_angle_min", {-1.0});
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
    desc.addUntracked<bool>("useOnlineBeamSpot", false);
    desc.addUntracked<std::string>("refPreference", "BeamSpot");
    desc.addUntracked<double>("seed_minIPSig", 4.0);
    desc.addUntracked<double>("seed_minPt", 0.9);
    desc.addUntracked<bool>("seed_use2DTrackDist", false);
    desc.add<bool>("use_2d_track_dist", false);
    desc.add<bool>("use_2d_vertex_dist", false);
    desc.addUntracked<double>("seed_maxIPSig", 1e9);
    desc.addUntracked<int>("seed_minPixelHits", 0);
    desc.addUntracked<int>("seed_minStripHits", 0);
    desc.addUntracked<int>("seed_minTrackerLayers", 0);
    desc.addUntracked<double>("track_pt_min_cut", 0.9);
    desc.addUntracked<double>("track_dxySig_min_cut", 4.0);
    desc.addUntracked<double>("track_dxySig_max_cut", 100.0);
    desc.addUntracked<int>("track_npixelHits_min_cut", 1);
    desc.addUntracked<int>("track_nstripHits_min_cut", 0);
    desc.addUntracked<int>("track_ntrackerLayers_min_cut", 5);
    desc.addUntracked<int>("hit_minPixelHits", 3);
    desc.addUntracked<int>("hit_minStripHits", 2);
    desc.addUntracked<int>("hit_minTrackerLayers", 6);
    desc.add<bool>("applyHitCuts", false);
    descriptions.add("scoutingPlotMakerRun3", desc);
}

DEFINE_FWK_MODULE(ScoutingPlotMakerRun3);