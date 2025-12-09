// -*- C++ -*-
//
// Package:    Vertexing/ScoutingTreeMakerRun3
// Class:      ScoutingTreeMakerRun3
//
// Original Author:  David Sperka
// Adapted/Fixed by: Aryan Malhotra
// Created:          Tue, 14 May 2024 14:23:11 GMT
// Last Modified:    Thu, 09 Jan 2025
//
// ============================================================================
// DESCRIPTION: TTree-based analyzer for Run-3 Scouting displaced vertex data
// ============================================================================
//
// PURPOSE:
//   Store comprehensive displaced vertex and track information for offline
//   analysis, applying minimal selection to preserve maximum flexibility.
//
// WHAT IS STORED:
//   1. EVENT-LEVEL INFO (per event):
//      - Run, luminosity section, event number
//      - Number of primary vertices (nPV)
//      - Beamspot position (x, y, z) + errors
//      - Average PV position (x, y, z) + errors # to be discarded
//      - Reference type used (beamspot or avgPV) # to be discarded
//
//   2. VERTEX-LEVEL INFO (vector per vertex passing cuts):
//      - Position (x, y, z) + errors
//      - Fit quality (χ², ndof, χ²/ndof) # χ²/ndof can be computed in the Tree2PlotsMaker
//      - Number of tracks
//
//   3. TRACK-LEVEL INFO (nested vector per vertex):
//      - Kinematics (pt, eta, phi)
//      - Impact parameters: dxy wrt origin, reference, beamspot, avgPV + errors # wrt origin and beamspot are enough
//      - IP significance wrt reference # since, mentioned above, we are saving the IP and its uncertainty, this can be discarded as it is just the ratio between the two
//      - Hit information: pixel hits, strip hits, tracker layers

// I am not saving dz right now, nor the 3D track distance
//
// SELECTION CUTS APPLIED (configurable via Python config):
//   - min_ntracks:     Minimum tracks per vertex (default: 3)
//   - max_ntracks:     Maximum tracks per vertex (default: -1 = no limit)
//   - max_chi2ndof:    Maximum χ²/ndof (default: 10.0)
//   - min_mass:        Minimum vertex mass [GeV] (default: 1.0)
//   - min_dBV:         Minimum displacement [cm] (default: 1.0 cm to exclude prompt)
//   - max_dBV:         Maximum displacement [cm] (default: -1 = no limit)
//   - max_dBV_error:   Maximum dBV uncertainty [cm] (default: 0.5)
//
// DATA STRUCTURE:
//   TTree "vertexTree" with:
//   - Event-level: scalar branches (run, lumi, event, nPV, beamspot_*, avgPV_*)
//   - Vertex-level: vector<float> branches (vtx_x, vtx_pt, vtx_mass, etc.)
//   - Track-level: vector<vector<float>> branches (trk_pt[ivtx][itrk], etc.)
//
// TYPICAL FILE SIZE ESTIMATE:
//   With min_dBV=1.0 cm and min_ntracks=3:
//   - ~1.0-1.5 KB per event with displaced vertices
//   - ~0.1-0.2 KB per event without (event structure still saved)
//   - For Run2024G (~7B events): expect 1-5 TB with ROOT compression
//
// USAGE:
//   Configured via UnifiedScoutingVertexingTreeMaker.py
//   Output: ScoutingTree_Output.root containing TTree "vertexTree"
//
// NOTES:
//   - All distances calculated in 2D (XY plane) for consistency with Vertexer
//   - Tracks stored only for selected vertices (not all event tracks)
//   - Even events with 0 selected vertices are saved (preserves event structure)
//   - Reference vertex preference: BeamSpot (default) or Average PV
//
// ============================================================================

#include <memory>
#include <vector>
#include <set>
#include <string>
#include <cmath>
#include <fstream>
#include <sstream>
#include <limits> // NEW: for std::numeric_limits

#include "TFile.h"
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

#include "RecoVertex/VertexTools/interface/VertexDistanceXY.h"
#include "RecoVertex/VertexTools/interface/VertexDistance3D.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "DataFormats/Math/interface/Point3D.h"
#include "TrackingTools/TransientTrack/interface/TransientTrack.h"
#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "TrackingTools/IPTools/interface/IPTools.h"

namespace {
	// Named sentinel for missing float values stored in the TTree.
	// Use a canonical NaN to indicate missing float values.
	const float kMissingFloat = std::numeric_limits<float>::quiet_NaN();
}

class ScoutingTreeMakerRun3 : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
    explicit ScoutingTreeMakerRun3(const edm::ParameterSet&);
    ~ScoutingTreeMakerRun3() override;

    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
    void beginJob() override;
    void analyze(const edm::Event&, const edm::EventSetup&) override;
    void endJob() override;

    // TTree-specific selection parameters (simpler than PlotMaker)
    const int min_ntracks_;
    const int max_ntracks_;
    const double max_chi2ndof_;
    const double min_mass_;
    const double min_dBV_;
    const double max_dBV_;
    const double max_dBV_error_;
    const bool store_all_vertex_tracks_;
    const int PVBoundary1_;
    const int PVBoundary2_;

    enum class RefPreference { PreferPV, PreferBeamSpot };
    const RefPreference refPreference_;

    // Input tokens
    const edm::EDGetTokenT<std::vector<reco::Vertex>> verticesToken;
    const edm::EDGetTokenT<reco::BeamSpot> beamspot_token;
    const edm::EDGetTokenT<std::vector<reco::Track>> tracksToken;
    const edm::EDGetTokenT<std::vector<reco::Vertex>> primaryVerticesToken;

    edm::ESGetToken<TransientTrackBuilder, TransientTrackRecord> ttBuilderToken_;

    const double seed_minIPSig_;
    const double seed_minPt_;
    const bool   seed_use2DTrackDist_;
    const bool   use_2d_vertex_dist_;
    const double seed_maxIPSig_;
    const int    seed_minPixelHits_;
    const int    seed_minStripHits_;
    const int    seed_minTrackerLayers_;

    // TTree and branches
    TTree* tree_;
    
    // Event-level branches
    UInt_t run_, lumi_, event_;
    Int_t nPV_;
    Float_t beamspot_x_, beamspot_y_, beamspot_z_;
    Float_t beamspot_xErr_, beamspot_yErr_, beamspot_zErr_;
    Float_t avgPV_x_, avgPV_y_, avgPV_z_;
    Float_t avgPV_xErr_, avgPV_yErr_, avgPV_zErr_;
    Int_t refType_; // 0=none, 1=BS, 2=avgPV
    
    // Vertex-level branches (KEEP only primitives: position, errors, fit quality, ntracks)
    std::vector<float> vtx_x_, vtx_y_, vtx_z_;
    std::vector<float> vtx_xErr_, vtx_yErr_, vtx_zErr_;
    std::vector<float> vtx_chi2_, vtx_ndof_, vtx_chi2norm_;
    std::vector<int> vtx_ntracks_;
    std::vector<int> vtx_pvRegion_; // 0=A, 1=B, 2=C
    
    // Track-level branches: per-vertex vectors of track primitives
    std::vector<std::vector<float>> trk_pt_, trk_eta_, trk_phi_;
    std::vector<std::vector<float>> trk_dxy_origin_;    // dxy wrt global origin
    std::vector<std::vector<float>> trk_dxyErr_;            // dxy uncertainty
    std::vector<std::vector<float>> trk_dxySig_ref_;    // optional placeholder (may be -999)
    std::vector<std::vector<int>> trk_nPixelHits_, trk_nStripHits_, trk_nTrackerLayers_;

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
    
    void clearVectors();
};

ScoutingTreeMakerRun3::ScoutingTreeMakerRun3(const edm::ParameterSet& iConfig):
    min_ntracks_(iConfig.getParameter<int>("min_ntracks")),
    max_ntracks_(iConfig.getParameter<int>("max_ntracks")),
    max_chi2ndof_(iConfig.getParameter<double>("max_chi2ndof")),
    min_mass_(iConfig.getParameter<double>("min_mass")),
    min_dBV_(iConfig.getParameter<double>("min_dBV")),
    max_dBV_(iConfig.getParameter<double>("max_dBV")),
    max_dBV_error_(iConfig.getParameter<double>("max_dBV_error")),
    store_all_vertex_tracks_(iConfig.getParameter<bool>("store_all_vertex_tracks")),
    PVBoundary1_(iConfig.getParameter<int>("PVBoundary1")),
    PVBoundary2_(iConfig.getParameter<int>("PVBoundary2")),
    refPreference_(iConfig.getUntrackedParameter<std::string>("refPreference", "BeamSpot") == "PV" ? 
                  RefPreference::PreferPV : RefPreference::PreferBeamSpot),
    verticesToken(consumes<std::vector<reco::Vertex>>(iConfig.getParameter<edm::InputTag>("displacedVertices"))),
    beamspot_token(consumes<reco::BeamSpot>(iConfig.getParameter<edm::InputTag>("beamspot_src"))),
    tracksToken(consumes<std::vector<reco::Track>>(iConfig.getParameter<edm::InputTag>("tracks"))),
    primaryVerticesToken(consumes<std::vector<reco::Vertex>>(iConfig.getParameter<edm::InputTag>("primaryVertices"))),
    ttBuilderToken_(esConsumes(edm::ESInputTag("", "TransientTrackBuilder"))),
    seed_minIPSig_(iConfig.getUntrackedParameter<double>("seed_minIPSig", 4.0)),
    seed_minPt_(iConfig.getUntrackedParameter<double>("seed_minPt", 0.9)),
    seed_use2DTrackDist_(iConfig.getParameter<bool>("use_2d_track_dist")),
    use_2d_vertex_dist_(iConfig.getParameter<bool>("use_2d_vertex_dist")),
    seed_maxIPSig_(iConfig.getUntrackedParameter<double>("seed_maxIPSig", 1e9)),
    seed_minPixelHits_(iConfig.getUntrackedParameter<int>("seed_minPixelHits", 0)),
    seed_minStripHits_(iConfig.getUntrackedParameter<int>("seed_minStripHits", 0)),
    seed_minTrackerLayers_(iConfig.getUntrackedParameter<int>("seed_minTrackerLayers", 0))
{
    usesResource("TFileService");
}

ScoutingTreeMakerRun3::~ScoutingTreeMakerRun3() {
}

void ScoutingTreeMakerRun3::beginJob() {
    edm::Service<TFileService> fs;
    tree_ = fs->make<TTree>("vertexTree", "Displaced Vertex Tree");

    // Event-level branches
    tree_->Branch("run", &run_);
    tree_->Branch("lumi", &lumi_);
    tree_->Branch("event", &event_);
    tree_->Branch("nPV", &nPV_);
    tree_->Branch("beamspot_x", &beamspot_x_);
    tree_->Branch("beamspot_y", &beamspot_y_);
    tree_->Branch("beamspot_z", &beamspot_z_);
    tree_->Branch("beamspot_xErr", &beamspot_xErr_);
    tree_->Branch("beamspot_yErr", &beamspot_yErr_);
    tree_->Branch("beamspot_zErr", &beamspot_zErr_);
    tree_->Branch("avgPV_x", &avgPV_x_);
    tree_->Branch("avgPV_y", &avgPV_y_);
    tree_->Branch("avgPV_z", &avgPV_z_);
    tree_->Branch("avgPV_xErr", &avgPV_xErr_);
    tree_->Branch("avgPV_yErr", &avgPV_yErr_);
    tree_->Branch("avgPV_zErr", &avgPV_zErr_);
    tree_->Branch("refType", &refType_);
    
    // Vertex-level branches (KEEP only primitives: position, errors, fit quality, ntracks)
    tree_->Branch("vtx_x", &vtx_x_);
    tree_->Branch("vtx_y", &vtx_y_);
    tree_->Branch("vtx_z", &vtx_z_);
    tree_->Branch("vtx_xErr", &vtx_xErr_);
    tree_->Branch("vtx_yErr", &vtx_yErr_);
    tree_->Branch("vtx_zErr", &vtx_zErr_);
    tree_->Branch("vtx_chi2", &vtx_chi2_);
    tree_->Branch("vtx_ndof", &vtx_ndof_);
    tree_->Branch("vtx_chi2norm", &vtx_chi2norm_);
    tree_->Branch("vtx_ntracks", &vtx_ntracks_);
    tree_->Branch("vtx_pvRegion", &vtx_pvRegion_);

    // Track-level branches: per-vertex vectors of track primitives
    tree_->Branch("trk_pt", &trk_pt_);
    tree_->Branch("trk_eta", &trk_eta_);
    tree_->Branch("trk_phi", &trk_phi_);
    tree_->Branch("trk_dxy_origin", &trk_dxy_origin_);    // dxy wrt global origin
    tree_->Branch("trk_dxyErr", &trk_dxyErr_);            // dxy uncertainty
    tree_->Branch("trk_dxySig_ref", &trk_dxySig_ref_);    // optional placeholder (may be -999)
    tree_->Branch("trk_nPixelHits", &trk_nPixelHits_);
    tree_->Branch("trk_nStripHits", &trk_nStripHits_);
    tree_->Branch("trk_nTrackerLayers", &trk_nTrackerLayers_);
}

void ScoutingTreeMakerRun3::clearVectors() {
    vtx_x_.clear(); vtx_y_.clear(); vtx_z_.clear();
    vtx_xErr_.clear(); vtx_yErr_.clear(); vtx_zErr_.clear();
    vtx_chi2_.clear(); vtx_ndof_.clear(); vtx_chi2norm_.clear();
    vtx_ntracks_.clear();
    vtx_pvRegion_.clear();
    
    trk_pt_.clear(); trk_eta_.clear(); trk_phi_.clear();
    trk_dxy_origin_.clear(); trk_dxyErr_.clear(); trk_dxySig_ref_.clear();
    trk_nPixelHits_.clear(); trk_nStripHits_.clear(); trk_nTrackerLayers_.clear();
}

void ScoutingTreeMakerRun3::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
    using namespace edm; using namespace std; using namespace reco;

    clearVectors();
    
    // Event info
    run_ = iEvent.id().run();
    lumi_ = iEvent.id().luminosityBlock();
    event_ = iEvent.id().event();

    // Ensure track collection is valid
    Handle<vector<Track>> tracksH;
    iEvent.getByToken(tracksToken, tracksH);
    if (!tracksH.isValid()) {
        LogWarning("ScoutingTreeMakerRun3") << "Track collection invalid (skip event).";
        return;
    }

    // Displaced vertices
    Handle<vector<Vertex>> verticesH;
    iEvent.getByToken(verticesToken, verticesH);
    if(!verticesH.isValid()){
        LogWarning("ScoutingTreeMakerRun3") << "Displaced vertex collection invalid (skip event).";
        return;
    }

    auto const& ttBuilder = iSetup.getData(ttBuilderToken_);
    (void)ttBuilder; // suppress unused-variable warning when not needed in this analyzer
    
    reco::Vertex refVtx, avgPVVtx, bsVtx;
    bool havePV = false, haveBS = false;
    auto [haveRef, refType] = determineReferenceVertex(iEvent, refVtx, havePV, avgPVVtx, haveBS, bsVtx);
    
    if (!haveRef) {
        LogWarning("ScoutingTreeMakerRun3") << "No valid reference. Skipping event.";
        return;
    }
    
    // Fill reference info
    refType_ = (refType == "beamspot" || refType == "beamspot (fallback)") ? 1 : 2;
    if (haveBS) {
        beamspot_x_ = bsVtx.x();
        beamspot_y_ = bsVtx.y();
        beamspot_z_ = bsVtx.z();
        beamspot_xErr_ = std::sqrt(bsVtx.covariance()(0,0));
        beamspot_yErr_ = std::sqrt(bsVtx.covariance()(1,1));
        beamspot_zErr_ = std::sqrt(bsVtx.covariance()(2,2));
    }
    if (havePV) {
        avgPV_x_ = avgPVVtx.x();
        avgPV_y_ = avgPVVtx.y();
        avgPV_z_ = avgPVVtx.z();
        avgPV_xErr_ = std::sqrt(avgPVVtx.covariance()(0,0));
        avgPV_yErr_ = std::sqrt(avgPVVtx.covariance()(1,1));
        avgPV_zErr_ = std::sqrt(avgPVVtx.covariance()(2,2));
    }
    
    // PV count
    Handle<vector<Vertex>> primaryVerticesH;
    iEvent.getByToken(primaryVerticesToken, primaryVerticesH);
    nPV_ = primaryVerticesH.isValid() ? primaryVerticesH->size() : 0;
    
    // PV region classification
    int pvRegion = 0;
    if (PVBoundary1_ != -1) {
        if (nPV_ < PVBoundary1_) pvRegion = 0;
        else if (PVBoundary2_ != -1 && nPV_ < PVBoundary2_) pvRegion = 1;
        else pvRegion = 2;
    }
    
    VertexDistanceXY vertexDist2D;
    Vertex originVtx(Vertex::Point(0,0,0), Vertex::Error());

    // Process vertices with SIMPLE CUTS for TTree storage
    for (unsigned int t = 0; t < verticesH->size(); ++t) {
        const auto& v = verticesH->at(t);
        vector<TrackRef> tks = vertex_track_vec(v);
        int ntk = static_cast<int>(tks.size());
        
        // Apply simple ntracks cut
        if (ntk < min_ntracks_) continue;
        if (max_ntracks_ > 0 && ntk > max_ntracks_) continue;
        
        // Apply simple quality cut
        if (max_chi2ndof_ > 0 && v.normalizedChi2() > max_chi2ndof_) continue;

        // --- NEW: compute vertex invariant mass from constituent tracks (pion mass assumption)
        TLorentzVector sumVec(0,0,0,0);
        constexpr double kPionMass = 0.13957;
        for (const auto& trRef : tks) {
            if (!trRef.isNonnull()) continue;
            TLorentzVector tv; tv.SetPtEtaPhiM(trRef->pt(), trRef->eta(), trRef->phi(), kPionMass);
            sumVec += tv;
        }
        double invMass = static_cast<double>(sumVec.M());
        
        // --- NEW: compute 2D dBV (vertex -> reference) and its uncertainty
        VertexDistanceXY vertexDist2D;
        Measurement1D dBVref_meas = vertexDist2D.distance(v, refVtx);
        double dBVref = dBVref_meas.value();
        double dBV_err = dBVref_meas.error();
        
        // --- NEW: apply configured mass / dBV / dBV_error storage cuts (use -1 to disable)
        if (min_mass_ > 0 && invMass < min_mass_) continue;
        if (min_dBV_ > -1 && dBVref < min_dBV_) continue;
        if (max_dBV_ > -1 && dBVref > max_dBV_) continue;
        if (max_dBV_error_ > -1 && dBV_err > max_dBV_error_) continue;

        // Store vertex primitives (position, errors, chi2, ndof, ntracks, pvRegion)
        vtx_x_.push_back(v.x());
        vtx_y_.push_back(v.y());
        vtx_z_.push_back(v.z());
        vtx_xErr_.push_back(std::sqrt(v.covariance()(0,0)));
        vtx_yErr_.push_back(std::sqrt(v.covariance()(1,1)));
        vtx_zErr_.push_back(std::sqrt(v.covariance()(2,2)));
        vtx_chi2_.push_back(v.chi2());
        vtx_ndof_.push_back(v.ndof());
        vtx_chi2norm_.push_back(v.normalizedChi2());
        vtx_ntracks_.push_back(ntk);
        vtx_pvRegion_.push_back(pvRegion);

        // Store per-vertex track primitive arrays
        std::vector<float> vtx_trk_pt, vtx_trk_eta, vtx_trk_phi;
        std::vector<float> vtx_trk_dxy_origin, vtx_trk_dxyErr, vtx_trk_dxySig_ref;
        std::vector<int> vtx_trk_nPixelHits, vtx_trk_nStripHits, vtx_trk_nTrackerLayers;

        const math::XYZPoint origin(0.,0.,0.);
        for(auto track : tks) {
            if(!track.isNonnull()) continue;

            vtx_trk_pt.push_back(track->pt());
            vtx_trk_eta.push_back(track->eta());
            vtx_trk_phi.push_back(track->phi());
            vtx_trk_dxy_origin.push_back(track->dxy(origin));         // store dxy wrt origin
            vtx_trk_dxyErr.push_back(track->dxyError());
            // we do not store dBV wrt ref here; Tree2Plots will compute it from vtx_x/y and ref
            vtx_trk_dxySig_ref.push_back(kMissingFloat); // placeholder sentinel (NaN) for "missing"

            vtx_trk_nPixelHits.push_back(track->hitPattern().numberOfValidPixelHits());
            vtx_trk_nStripHits.push_back(track->hitPattern().numberOfValidStripHits());
            vtx_trk_nTrackerLayers.push_back(track->hitPattern().trackerLayersWithMeasurement());
        }

        trk_pt_.push_back(std::move(vtx_trk_pt));
        trk_eta_.push_back(std::move(vtx_trk_eta));
        trk_phi_.push_back(std::move(vtx_trk_phi));
        trk_dxy_origin_.push_back(std::move(vtx_trk_dxy_origin));
        trk_dxyErr_.push_back(std::move(vtx_trk_dxyErr));
        trk_dxySig_ref_.push_back(std::move(vtx_trk_dxySig_ref));
        trk_nPixelHits_.push_back(std::move(vtx_trk_nPixelHits));
        trk_nStripHits_.push_back(std::move(vtx_trk_nStripHits));
        trk_nTrackerLayers_.push_back(std::move(vtx_trk_nTrackerLayers));
    }
    
    // Fill tree (even if no vertices passed)
    tree_->Fill();
}

std::pair<bool, std::string> ScoutingTreeMakerRun3::determineReferenceVertex(
    const edm::Event& iEvent, 
    reco::Vertex& refVtx,
    bool& havePV, 
    reco::Vertex& avgPVVtx,
    bool& haveBS,
    reco::Vertex& bsVtx) {
    
    havePV = false;
    haveBS = false;

    edm::Handle<std::vector<reco::Vertex>> primaryVerticesH;
    iEvent.getByToken(primaryVerticesToken, primaryVerticesH);
    if (primaryVerticesH.isValid() && !primaryVerticesH->empty()) {
        double sumX=0, sumY=0, sumZ=0; int validPVs=0;
        for (const auto& pv : *primaryVerticesH) {
            if (!pv.isFake() && pv.ndof() > 4) {
                sumX += pv.x(); sumY += pv.y(); sumZ += pv.z();
                ++validPVs;
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

    edm::Handle<reco::BeamSpot> beamspot;
    iEvent.getByToken(beamspot_token, beamspot);
    if (beamspot.isValid()) {
        reco::Vertex::Error bsErr;
        for (int i=0;i<3;++i) for (int j=i;j<3;++j) bsErr(i,j)=0.0;
        bsErr(0,0)=beamspot->covariance()(0,0);
        bsErr(1,1)=beamspot->covariance()(1,1);
        bsErr(2,2)=beamspot->covariance()(2,2);
        bsVtx = reco::Vertex(beamspot->position(), bsErr);
        haveBS = true;
    }

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
}

void ScoutingTreeMakerRun3::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    
    // TTree-specific parameters (simpler than PlotMaker)
    desc.add<int>("min_ntracks", 3);
    desc.add<int>("max_ntracks", -1);
    desc.add<double>("max_chi2ndof", 10.0);
    desc.add<double>("min_mass", 1.0);
    desc.add<double>("min_dBV", 0.3);
    desc.add<double>("max_dBV", -1);
    desc.add<double>("max_dBV_error", 0.5);
    desc.add<bool>("store_all_vertex_tracks", true);
    desc.add<int>("PVBoundary1", -1);
    desc.add<int>("PVBoundary2", -1);
    desc.add<edm::InputTag>("displacedVertices", edm::InputTag("displacedVertices"));
    desc.add<edm::InputTag>("beamspot_src", edm::InputTag("offlineBeamSpot"));
    desc.add<edm::InputTag>("tracks", edm::InputTag("hltScoutingUnpackProducer", "Track"));
    desc.add<edm::InputTag>("primaryVertices", edm::InputTag("hltScoutingPrimaryVertexPacker", "primaryVtx"));
    desc.addUntracked<std::string>("refPreference", "BeamSpot");
    desc.addUntracked<double>("seed_minIPSig", 4.0);
    desc.addUntracked<double>("seed_minPt", 0.9);
    desc.add<bool>("use_2d_track_dist", false);
    desc.add<bool>("use_2d_vertex_dist", false);
    desc.addUntracked<double>("seed_maxIPSig", 1e9);
    desc.addUntracked<int>("seed_minPixelHits", 0);
    desc.addUntracked<int>("seed_minStripHits", 0);
    desc.addUntracked<int>("seed_minTrackerLayers", 0);
    descriptions.add("scoutingTreeMakerRun3", desc);
}

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

ScoutingTreeMakerRun3::track_vec ScoutingTreeMakerRun3::vertex_track_vec(const reco::Vertex& v, const double min_weight) const {
    track_set s = vertex_track_set(v, min_weight);
    return track_vec(s.begin(), s.end());
}

DEFINE_FWK_MODULE(ScoutingTreeMakerRun3);
