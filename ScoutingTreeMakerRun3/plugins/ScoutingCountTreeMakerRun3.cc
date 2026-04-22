// ScoutingCountTreeMakerRun3.cc
// Tree-based replacement for ScoutingCountMakerRun3.
//
// Output idea:
//   Events tree with event-level metadata, plus flat branches that preserve
//   the same information that used to be printed / counted / organized in folders.
//
// Notes:
// - This version avoids custom ROOT-streamed classes by using only primitive
//   branches and std::vector branches.
// - It keeps the full selection logic, counters, approximate match bookkeeping,
//   and per-event metadata from ScoutingCountMakerRun3.
// - It also stores vertex/track hierarchies in a tree-friendly flattened form.
//
// If you later want the tree schema renamed to exactly match your preferred
// folder naming, that is just a branch-name cleanup.

#include <memory>
#include <algorithm>
#include <vector>
#include <string>
#include <cmath>
#include <limits>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <set>
#include <utility>

#include "TLorentzVector.h"
#include "TVector3.h"
#include "TString.h"
#include "TTree.h"

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/ServiceRegistry/interface/Service.h"

#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/BeamSpot/interface/BeamSpot.h"
#include "DataFormats/Scouting/interface/Run3ScoutingTrack.h"
#include "DataFormats/Common/interface/ValueMap.h"

#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "TrackingTools/IPTools/interface/IPTools.h"
#include "RecoVertex/VertexTools/interface/VertexDistanceXY.h"

#include "CondFormats/BeamSpotObjects/interface/BeamSpotOnlineObjects.h"
#include "CondFormats/DataRecord/interface/BeamSpotOnlineHLTObjectsRcd.h"

class ScoutingCountTreeMakerRun3 : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
    explicit ScoutingCountTreeMakerRun3(const edm::ParameterSet &);
    ~ScoutingCountTreeMakerRun3() override = default;

    static void fillDescriptions(edm::ConfigurationDescriptions &descriptions);

private:
    void beginJob() override;
    void analyze(const edm::Event &, const edm::EventSetup &) override;
    void endJob() override;

    // ------------------------ config ------------------------
    const edm::InputTag scoutingVerticesTag_;
    const edm::InputTag offlineVerticesTag_;
    const edm::InputTag primaryVerticesTag_;
    const edm::InputTag beamspotTag_;

    std::vector<int> cut_ntk_;
    const double cut_opening_angle_min_;
    const double required_invmass_;
    const double required_chi2_;
    const double required_dBV_min_;
    const double required_dBV_max_;
    const double required_dxy_min_;
    const double required_dxy_max_;
    const double required_dBV_error_;
    const double required_dxy_error_;

    const double track_pt_min_cut_;
    const double track_dxySig_min_cut_;
    const double track_dxySig_max_cut_;

    const bool applyHitCuts_;
    const int hit_minPixelHits_;
    const int hit_minStripHits_;
    const int hit_minTrackerLayers_;

    const double seed_minIPSig_;
    const double seed_minPt_;
    const double seed_maxIPSig_;
    const bool seed_use2DTrackDist_;
    const bool verbose_;
    const bool printSummary_;

    const bool useOnlineBeamSpot_;
    enum class RefPreference { PreferPV, PreferBeamSpot };
    const RefPreference refPreference_;

    // ------------------------ tokens ------------------------
    const edm::EDGetTokenT<std::vector<reco::Vertex>> scoutingVerticesToken_;
    const edm::EDGetTokenT<std::vector<reco::Vertex>> offlineVerticesToken_;
    const edm::EDGetTokenT<std::vector<reco::Vertex>> primaryVerticesToken_;
    const edm::EDGetTokenT<reco::BeamSpot> beamspotToken_;
    const edm::EDGetTokenT<std::vector<reco::Track>> tracksToken_;
    const edm::EDGetTokenT<std::vector<reco::Track>> offlineTracksToken_;
    const edm::EDGetTokenT<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>> trackToScoutingRefToken_;
    const edm::ESGetToken<TransientTrackBuilder, TransientTrackRecord> ttBuilderToken_;
    const edm::ESGetToken<BeamSpotOnlineObjects, BeamSpotOnlineHLTObjectsRcd> bsOnlineToken_;

    bool haveTrackToScoutingMap_ = false;

    // ------------------------ tree ------------------------
    TTree *tree_ = nullptr;

    // Event-level branches
    int evt_run_ = 0;
    int evt_lumi_ = 0;
    int evt_event_ = 0;
    bool evt_hasScouting_ = false;
    bool evt_hasOffline_ = false;
    bool evt_hasBoth_ = false;
    int evt_matchCategory_ = 0;
    bool evt_poolNonEmpty_ = false;
    float evt_beamspot_x_ = 0.0f;
    float evt_beamspot_y_ = 0.0f;
    float evt_beamspot_z_ = 0.0f;
    int evt_nPV_ = 0;

    int evt_scoutingSelected_ = 0;
    int evt_offlineSelected_ = 0;
    int evt_scoutingTrackPoolSize_ = 0;

    int evt_offlineMatchedTrackCount_ = 0;
    int evt_scoutingMatchedTrackCount_ = 0;
    int evt_offlineMatchBucket_ = 0;
    int evt_scoutingMatchBucket_ = 0;

    int evt_selectedEventIndex_ = 0;
    TString evt_eventTag_;

    // Counters from original analyzer
    unsigned long long nEvents_ = 0;
    unsigned long long nSelectedEvents_ = 0;
    unsigned long long totalScoutingSelected_ = 0;
    unsigned long long totalOfflineSelected_ = 0;

    unsigned long long nEventsWithOfflineSelected_ = 0;
    unsigned long long nEventsWithScoutingSelected_ = 0;
    unsigned long long nEventsWithBothSelected_ = 0;
    unsigned long long nEventsWithOfflineOnly_ = 0;
    unsigned long long nEventsWithScoutingOnly_ = 0;

    unsigned long long nOffSelAndScoSel_ = 0;
    unsigned long long nOffSelAndNoScoSel_ = 0;
    unsigned long long nOffSelNoScoNoMatch_ = 0;
    unsigned long long nOffSelNoScoMatch_ = 0;
    unsigned long long nOffSelNoScoMatchR_ = 0;
    unsigned long long nOffSelScoNoMatch_ = 0;
    unsigned long long nOffSelScoMatch_ = 0;
    unsigned long long nOffSelScoMatchR_ = 0;

    unsigned long long nScoSelAndOffSel_ = 0;
    unsigned long long nScoSelAndNoOffSel_ = 0;
    unsigned long long nScoSelNoOffNoMatch_ = 0;
    unsigned long long nScoSelNoOffMatch_ = 0;
    unsigned long long nScoSelNoOffMatchR_ = 0;
    unsigned long long nScoSelOffNoMatch_ = 0;
    unsigned long long nScoSelOffMatch_ = 0;
    unsigned long long nScoSelOffMatchR_ = 0;

    unsigned long long nEventsPoolNonEmpty_ = 0;
    unsigned long long nEventsPoolEmpty_ = 0;

    // ------------------------ flat tree blocks ------------------------
    struct TrackSummary {
        float pt = 0.0f;
        float eta = 0.0f;
        float phi = 0.0f;
        float dxy = 0.0f;
        float dxyErr = 0.0f;
        float ipSig = 0.0f;
    };

    struct TrackMatchRecord {
        TrackSummary source;
        bool hasMatch = false;
        TrackSummary match;
    };

    struct VertexSummary {
        float x = 0.0f;
        float y = 0.0f;
        float pt = 0.0f;
        float mass = 0.0f;
        float chi2 = 0.0f;
        float dBVerr = 0.0f;
        float dBV = 0.0f;
        float avgDxy = 0.0f;
        float avgDxyErr = 0.0f;
        float minOpeningAngle = 0.0f;
        int nPairs = 0;
        int nTracks = 0;
        std::vector<reco::TrackRef> tracks;
    };

    struct SelectionResult {
        int nSel = 0;
        std::vector<std::pair<float, float>> selectedXY;
        std::vector<VertexSummary> vertices;
    };

    struct VertexBlock {
        int n = 0;
        std::vector<int> vertexId;
        std::vector<float> x;
        std::vector<float> y;
        std::vector<float> pt;
        std::vector<float> mass;
        std::vector<float> chi2;
        std::vector<float> dBVerr;
        std::vector<float> dBV;
        std::vector<float> avgDxy;
        std::vector<float> avgDxyErr;
        std::vector<float> minOpeningAngle;
        std::vector<int> nPairs;
        std::vector<int> nTracks;
        std::vector<int> trackStart;
        std::vector<int> trackCount;

        void clear() {
            n = 0;
            vertexId.clear();
            x.clear(); y.clear(); pt.clear(); mass.clear();
            chi2.clear(); dBVerr.clear(); dBV.clear();
            avgDxy.clear(); avgDxyErr.clear();
            minOpeningAngle.clear();
            nPairs.clear(); nTracks.clear();
            trackStart.clear(); trackCount.clear();
        }
    };

    struct TrackBlock {
        int n = 0;
        std::vector<float> pt;
        std::vector<float> eta;
        std::vector<float> phi;
        std::vector<float> dxy;
        std::vector<float> dxyErr;
        std::vector<float> ipSig;
        std::vector<int> hasMatch;
        std::vector<float> match_pt;
        std::vector<float> match_eta;
        std::vector<float> match_phi;

        void clear() {
            n = 0;
            pt.clear(); eta.clear(); phi.clear();
            dxy.clear(); dxyErr.clear(); ipSig.clear();
            hasMatch.clear();
            match_pt.clear(); match_eta.clear(); match_phi.clear();
        }
    };

    // selected vertices / tracks
    VertexBlock scoSelVtx_;
    TrackBlock  scoSelTrk_;
    VertexBlock offSelVtx_;
    TrackBlock  offSelTrk_;

    // raw track collections
    TrackBlock scoRawTrk_;
    TrackBlock offRawTrk_;

    // scouting filtered pool used for matching/reporting
    TrackBlock scoCutTrk_;

    // approximate match blocks
    VertexBlock offApproxVtx_;   // source offline vertices with matched scouting tracks
    TrackBlock  offApproxTrk_;

    VertexBlock scoApproxVtx_;   // source scouting vertices with matched offline tracks
    TrackBlock  scoApproxTrk_;

    // ------------------------ helpers ------------------------
    void resetEventBlocks();
    void bookTree();
    void bookVertexBlock(const std::string &prefix, VertexBlock &blk);
    void bookTrackBlock(const std::string &prefix, TrackBlock &blk);

    static TrackSummary summarizeTrack(const reco::Track &track);

    static void fillTrackBlockFromTrackSummaries(TrackBlock &blk,
                                                 const std::vector<TrackSummary> &tracks,
                                                 bool useMatchFields = false);

    static void fillTrackBlockFromTrackMatchRecords(TrackBlock &blk,
                                                    const std::vector<TrackMatchRecord> &tracks);

    static void fillTrackBlockFromRecoTracks(TrackBlock &blk,
                                             const edm::Handle<std::vector<reco::Track>> &tracksH,
                                             const std::vector<size_t> *cutIndices = nullptr);

    static void fillVertexBlockFromSelection(VertexBlock &blk,
                                             const SelectionResult &result);

    static void fillVertexBlockFromApprox(VertexBlock &blk,
                                          const SelectionResult &sourceVertices,
                                          const std::vector<std::vector<TrackMatchRecord>> &approxTracks);

};

// ----------------------------------------------------------------------
// implementation
// ----------------------------------------------------------------------

ScoutingCountTreeMakerRun3::ScoutingCountTreeMakerRun3(const edm::ParameterSet &ps) :
    scoutingVerticesTag_( ps.getParameter<edm::InputTag>("scoutingVertices") ),
    offlineVerticesTag_( ps.getParameter<edm::InputTag>("offlineVertices") ),
    primaryVerticesTag_( ps.getParameter<edm::InputTag>("primaryVertices") ),
    beamspotTag_( ps.getParameter<edm::InputTag>("beamspot_src") ),

    cut_opening_angle_min_( ps.getParameter<double>("cut_opening_angle_min") ),
    required_invmass_( ps.getParameter<double>("required_invmass") ),
    required_chi2_( ps.getParameter<double>("required_chi2") ),
    required_dBV_min_( ps.getParameter<double>("required_dBV_min") ),
    required_dBV_max_( ps.getParameter<double>("required_dBV_max") ),
    required_dxy_min_( ps.getParameter<double>("required_dxy_min") ),
    required_dxy_max_( ps.getParameter<double>("required_dxy_max") ),
    required_dBV_error_( ps.getParameter<double>("required_dBV_error") ),
    required_dxy_error_( ps.getParameter<double>("required_dxy_error") ),

    track_pt_min_cut_( ps.getUntrackedParameter<double>("track_pt_min_cut", 0.9) ),
    track_dxySig_min_cut_( ps.getUntrackedParameter<double>("track_dxySig_min_cut", 4.0) ),
    track_dxySig_max_cut_( ps.getUntrackedParameter<double>("track_dxySig_max_cut", 100.0) ),

    applyHitCuts_( ps.getParameter<bool>("applyHitCuts") ),
    hit_minPixelHits_( ps.getUntrackedParameter<int>("hit_minPixelHits", 3) ),
    hit_minStripHits_( ps.getUntrackedParameter<int>("hit_minStripHits", 2) ),
    hit_minTrackerLayers_( ps.getUntrackedParameter<int>("hit_minTrackerLayers", 6) ),

    seed_minIPSig_( ps.getUntrackedParameter<double>("seed_minIPSig", 4.0) ),
    seed_minPt_( ps.getUntrackedParameter<double>("seed_minPt", 0.9) ),
    seed_maxIPSig_( ps.getUntrackedParameter<double>("seed_maxIPSig", 1e9) ),
    seed_use2DTrackDist_( ps.getUntrackedParameter<bool>("seed_use2DTrackDist", false) ),
    verbose_( ps.getUntrackedParameter<bool>("verbose", false) ),
    printSummary_( ps.getUntrackedParameter<bool>("printSummary", true) ),

    useOnlineBeamSpot_( ps.getUntrackedParameter<bool>("useOnlineBeamSpot", false) ),
    refPreference_( ps.getUntrackedParameter<std::string>("refPreference", "BeamSpot") == "PV"
                        ? RefPreference::PreferPV
                        : RefPreference::PreferBeamSpot ),

    scoutingVerticesToken_( consumes<std::vector<reco::Vertex>>(scoutingVerticesTag_) ),
    offlineVerticesToken_( consumes<std::vector<reco::Vertex>>(offlineVerticesTag_) ),
    primaryVerticesToken_( consumes<std::vector<reco::Vertex>>(primaryVerticesTag_) ),
    beamspotToken_( consumes<reco::BeamSpot>(beamspotTag_) ),
    tracksToken_( consumes<std::vector<reco::Track>>( ps.getParameter<edm::InputTag>("tracks") ) ),
    offlineTracksToken_( consumes<std::vector<reco::Track>>( ps.getParameter<edm::InputTag>("offlineTracks") ) ),
    trackToScoutingRefToken_( consumes<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>>(
        edm::InputTag("hltScoutingUnpackProducer", "Track-RefToOriginal") ) ),
    ttBuilderToken_( esConsumes<TransientTrackBuilder, TransientTrackRecord>(edm::ESInputTag("", "TransientTrackBuilder")) ),
    bsOnlineToken_( esConsumes<BeamSpotOnlineObjects, BeamSpotOnlineHLTObjectsRcd>() )
{
    usesResource("TFileService");

    const auto &ntkPSet = ps.getParameter<edm::ParameterSet>("cut_ntk");
    cut_ntk_ = ntkPSet.getParameter<std::vector<int>>("values");
}

void ScoutingCountTreeMakerRun3::beginJob() {
    edm::Service<TFileService> fs;
    if (!fs.isAvailable()) {
        throw cms::Exception("ScoutingCountTreeMakerRun3")
            << "TFileService is not available.";
    }

    tree_ = fs->make<TTree>("Events", "ScoutingCountTreeMakerRun3 event tree");

    // Event-level branches
    tree_->Branch("run", &evt_run_);
    tree_->Branch("lumi", &evt_lumi_);
    tree_->Branch("event", &evt_event_);
    tree_->Branch("hasScouting", &evt_hasScouting_);
    tree_->Branch("hasOffline", &evt_hasOffline_);
    tree_->Branch("hasBoth", &evt_hasBoth_);
    tree_->Branch("matchCategory", &evt_matchCategory_);
    tree_->Branch("poolNonEmpty", &evt_poolNonEmpty_);
    tree_->Branch("beamspot_x", &evt_beamspot_x_);
    tree_->Branch("beamspot_y", &evt_beamspot_y_);
    tree_->Branch("beamspot_z", &evt_beamspot_z_);
    tree_->Branch("nPV", &evt_nPV_);

    tree_->Branch("scoutingSelected", &evt_scoutingSelected_);
    tree_->Branch("offlineSelected", &evt_offlineSelected_);
    tree_->Branch("scoutingTrackPoolSize", &evt_scoutingTrackPoolSize_);

    tree_->Branch("offlineMatchedTrackCount", &evt_offlineMatchedTrackCount_);
    tree_->Branch("scoutingMatchedTrackCount", &evt_scoutingMatchedTrackCount_);
    tree_->Branch("offlineMatchBucket", &evt_offlineMatchBucket_);
    tree_->Branch("scoutingMatchBucket", &evt_scoutingMatchBucket_);

    tree_->Branch("selectedEventIndex", &evt_selectedEventIndex_);
    tree_->Branch("eventTag", &evt_eventTag_);

    // Collection blocks
    bookVertexBlock("scoutingSel_", scoSelVtx_);
    bookTrackBlock("scoutingSel_", scoSelTrk_);

    bookVertexBlock("offlineSel_", offSelVtx_);
    bookTrackBlock("offlineSel_", offSelTrk_);

    bookTrackBlock("scoutingRaw_", scoRawTrk_);
    bookTrackBlock("offlineRaw_", offRawTrk_);

    bookTrackBlock("scoutingCut_", scoCutTrk_);

    bookVertexBlock("offlineApprox_", offApproxVtx_);
    bookTrackBlock("offlineApprox_", offApproxTrk_);

    bookVertexBlock("scoutingApprox_", scoApproxVtx_);
    bookTrackBlock("scoutingApprox_", scoApproxTrk_);
}

void ScoutingCountTreeMakerRun3::bookVertexBlock(const std::string &prefix, VertexBlock &blk) {
    tree_->Branch((prefix + "n").c_str(), &blk.n);
    tree_->Branch((prefix + "vertexId").c_str(), &blk.vertexId);
    tree_->Branch((prefix + "x").c_str(), &blk.x);
    tree_->Branch((prefix + "y").c_str(), &blk.y);
    tree_->Branch((prefix + "pt").c_str(), &blk.pt);
    tree_->Branch((prefix + "mass").c_str(), &blk.mass);
    tree_->Branch((prefix + "chi2").c_str(), &blk.chi2);
    tree_->Branch((prefix + "dBVerr").c_str(), &blk.dBVerr);
    tree_->Branch((prefix + "dBV").c_str(), &blk.dBV);
    tree_->Branch((prefix + "avgDxy").c_str(), &blk.avgDxy);
    tree_->Branch((prefix + "avgDxyErr").c_str(), &blk.avgDxyErr);
    tree_->Branch((prefix + "minOpeningAngle").c_str(), &blk.minOpeningAngle);
    tree_->Branch((prefix + "nPairs").c_str(), &blk.nPairs);
    tree_->Branch((prefix + "nTracks").c_str(), &blk.nTracks);
    tree_->Branch((prefix + "trackStart").c_str(), &blk.trackStart);
    tree_->Branch((prefix + "trackCount").c_str(), &blk.trackCount);
}

void ScoutingCountTreeMakerRun3::bookTrackBlock(const std::string &prefix, TrackBlock &blk) {
    tree_->Branch((prefix + "n").c_str(), &blk.n);
    tree_->Branch((prefix + "pt").c_str(), &blk.pt);
    tree_->Branch((prefix + "eta").c_str(), &blk.eta);
    tree_->Branch((prefix + "phi").c_str(), &blk.phi);
    tree_->Branch((prefix + "dxy").c_str(), &blk.dxy);
    tree_->Branch((prefix + "dxyErr").c_str(), &blk.dxyErr);
    tree_->Branch((prefix + "ipSig").c_str(), &blk.ipSig);
    tree_->Branch((prefix + "hasMatch").c_str(), &blk.hasMatch);
    tree_->Branch((prefix + "match_pt").c_str(), &blk.match_pt);
    tree_->Branch((prefix + "match_eta").c_str(), &blk.match_eta);
    tree_->Branch((prefix + "match_phi").c_str(), &blk.match_phi);
}

void ScoutingCountTreeMakerRun3::resetEventBlocks() {
    scoSelVtx_.clear();
    scoSelTrk_.clear();
    offSelVtx_.clear();
    offSelTrk_.clear();

    scoRawTrk_.clear();
    offRawTrk_.clear();
    scoCutTrk_.clear();

    offApproxVtx_.clear();
    offApproxTrk_.clear();
    scoApproxVtx_.clear();
    scoApproxTrk_.clear();

    evt_eventTag_.Clear();
}

ScoutingCountTreeMakerRun3::TrackSummary ScoutingCountTreeMakerRun3::summarizeTrack(const reco::Track &track) {
    TrackSummary summary;
    summary.pt = static_cast<float>(track.pt());
    summary.eta = static_cast<float>(track.eta());
    summary.phi = static_cast<float>(track.phi());
    summary.dxy = static_cast<float>(track.d0());
    summary.dxyErr = static_cast<float>(track.d0Error());
    summary.ipSig = (summary.dxyErr > 0.0f) ? std::fabs(summary.dxy / summary.dxyErr) : 0.0f;
    return summary;
}

void ScoutingCountTreeMakerRun3::fillTrackBlockFromTrackSummaries(TrackBlock &blk,
                                                                  const std::vector<TrackSummary> &tracks,
                                                                  bool useMatchFields) {
    blk.clear();
    blk.n = static_cast<int>(tracks.size());
    blk.pt.reserve(tracks.size());
    blk.eta.reserve(tracks.size());
    blk.phi.reserve(tracks.size());
    blk.dxy.reserve(tracks.size());
    blk.dxyErr.reserve(tracks.size());
    blk.ipSig.reserve(tracks.size());
    blk.hasMatch.reserve(tracks.size());
    blk.match_pt.reserve(tracks.size());
    blk.match_eta.reserve(tracks.size());
    blk.match_phi.reserve(tracks.size());

    for (const auto &t : tracks) {
        blk.pt.push_back(t.pt);
        blk.eta.push_back(t.eta);
        blk.phi.push_back(t.phi);
        blk.dxy.push_back(t.dxy);
        blk.dxyErr.push_back(t.dxyErr);
        blk.ipSig.push_back(t.ipSig);
        blk.hasMatch.push_back(0);
        blk.match_pt.push_back(0.0f);
        blk.match_eta.push_back(0.0f);
        blk.match_phi.push_back(0.0f);
    }

    if (useMatchFields) {
        // left intentionally available for future extension;
        // caller should use fillTrackBlockFromTrackMatchRecords when actual matches exist.
    }
}

void ScoutingCountTreeMakerRun3::fillTrackBlockFromTrackMatchRecords(TrackBlock &blk,
                                                                    const std::vector<TrackMatchRecord> &tracks) {
    blk.clear();
    blk.n = static_cast<int>(tracks.size());
    blk.pt.reserve(tracks.size());
    blk.eta.reserve(tracks.size());
    blk.phi.reserve(tracks.size());
    blk.dxy.reserve(tracks.size());
    blk.dxyErr.reserve(tracks.size());
    blk.ipSig.reserve(tracks.size());
    blk.hasMatch.reserve(tracks.size());
    blk.match_pt.reserve(tracks.size());
    blk.match_eta.reserve(tracks.size());
    blk.match_phi.reserve(tracks.size());

    for (const auto &rec : tracks) {
        blk.pt.push_back(rec.source.pt);
        blk.eta.push_back(rec.source.eta);
        blk.phi.push_back(rec.source.phi);
        blk.dxy.push_back(rec.source.dxy);
        blk.dxyErr.push_back(rec.source.dxyErr);
        blk.ipSig.push_back(rec.source.ipSig);
        blk.hasMatch.push_back(rec.hasMatch ? 1 : 0);
        blk.match_pt.push_back(rec.hasMatch ? rec.match.pt : 0.0f);
        blk.match_eta.push_back(rec.hasMatch ? rec.match.eta : 0.0f);
        blk.match_phi.push_back(rec.hasMatch ? rec.match.phi : 0.0f);
    }
}

void ScoutingCountTreeMakerRun3::fillTrackBlockFromRecoTracks(TrackBlock &blk,
                                                              const edm::Handle<std::vector<reco::Track>> &tracksH,
                                                              const std::vector<size_t> *cutIndices) {
    blk.clear();

    if (!tracksH.isValid()) return;

    if (cutIndices) {
        blk.n = static_cast<int>(cutIndices->size());
        blk.pt.reserve(cutIndices->size());
        blk.eta.reserve(cutIndices->size());
        blk.phi.reserve(cutIndices->size());
        blk.dxy.reserve(cutIndices->size());
        blk.dxyErr.reserve(cutIndices->size());
        blk.ipSig.reserve(cutIndices->size());
        blk.hasMatch.reserve(cutIndices->size());
        blk.match_pt.reserve(cutIndices->size());
        blk.match_eta.reserve(cutIndices->size());
        blk.match_phi.reserve(cutIndices->size());

        for (const size_t idx : *cutIndices) {
            if (idx >= tracksH->size()) continue;
            const auto &track = tracksH->at(idx);
            const TrackSummary s = summarizeTrack(track);
            blk.pt.push_back(s.pt);
            blk.eta.push_back(s.eta);
            blk.phi.push_back(s.phi);
            blk.dxy.push_back(s.dxy);
            blk.dxyErr.push_back(s.dxyErr);
            blk.ipSig.push_back(s.ipSig);
            blk.hasMatch.push_back(0);
            blk.match_pt.push_back(0.0f);
            blk.match_eta.push_back(0.0f);
            blk.match_phi.push_back(0.0f);
        }
    } else {
        blk.n = static_cast<int>(tracksH->size());
        blk.pt.reserve(tracksH->size());
        blk.eta.reserve(tracksH->size());
        blk.phi.reserve(tracksH->size());
        blk.dxy.reserve(tracksH->size());
        blk.dxyErr.reserve(tracksH->size());
        blk.ipSig.reserve(tracksH->size());
        blk.hasMatch.reserve(tracksH->size());
        blk.match_pt.reserve(tracksH->size());
        blk.match_eta.reserve(tracksH->size());
        blk.match_phi.reserve(tracksH->size());

        for (const auto &track : *tracksH) {
            const TrackSummary s = summarizeTrack(track);
            blk.pt.push_back(s.pt);
            blk.eta.push_back(s.eta);
            blk.phi.push_back(s.phi);
            blk.dxy.push_back(s.dxy);
            blk.dxyErr.push_back(s.dxyErr);
            blk.ipSig.push_back(s.ipSig);
            blk.hasMatch.push_back(0);
            blk.match_pt.push_back(0.0f);
            blk.match_eta.push_back(0.0f);
            blk.match_phi.push_back(0.0f);
        }
    }
}

void ScoutingCountTreeMakerRun3::fillVertexBlockFromSelection(VertexBlock &blk,
                                                             const SelectionResult &result) {
    blk.clear();
    blk.n = static_cast<int>(result.vertices.size());

    blk.vertexId.reserve(result.vertices.size());
    blk.x.reserve(result.vertices.size());
    blk.y.reserve(result.vertices.size());
    blk.pt.reserve(result.vertices.size());
    blk.mass.reserve(result.vertices.size());
    blk.chi2.reserve(result.vertices.size());
    blk.dBVerr.reserve(result.vertices.size());
    blk.dBV.reserve(result.vertices.size());
    blk.avgDxy.reserve(result.vertices.size());
    blk.avgDxyErr.reserve(result.vertices.size());
    blk.minOpeningAngle.reserve(result.vertices.size());
    blk.nPairs.reserve(result.vertices.size());
    blk.nTracks.reserve(result.vertices.size());
    blk.trackStart.reserve(result.vertices.size());
    blk.trackCount.reserve(result.vertices.size());

    int trackOffset = 0;
    for (size_t iv = 0; iv < result.vertices.size(); ++iv) {
        const auto &v = result.vertices[iv];

        blk.vertexId.push_back(static_cast<int>(iv));
        blk.x.push_back(v.x);
        blk.y.push_back(v.y);
        blk.pt.push_back(v.pt);
        blk.mass.push_back(v.mass);
        blk.chi2.push_back(v.chi2);
        blk.dBVerr.push_back(v.dBVerr);
        blk.dBV.push_back(v.dBV);
        blk.avgDxy.push_back(v.avgDxy);
        blk.avgDxyErr.push_back(v.avgDxyErr);
        blk.minOpeningAngle.push_back(v.minOpeningAngle);
        blk.nPairs.push_back(v.nPairs);
        blk.nTracks.push_back(v.nTracks);
        blk.trackStart.push_back(trackOffset);

        std::set<unsigned int> seenTrackKeys;
        int addedTracks = 0;
        for (const auto &trRef : v.tracks) {
            if (!trRef.isNonnull()) continue;
            const unsigned int key = trRef.key();
            if (!seenTrackKeys.insert(key).second) continue;
            ++addedTracks;
        }
        blk.trackCount.push_back(addedTracks);
        trackOffset += addedTracks;
    }
}

void ScoutingCountTreeMakerRun3::fillVertexBlockFromApprox(VertexBlock &blk,
                                                          const SelectionResult &sourceVertices,
                                                          const std::vector<std::vector<TrackMatchRecord>> &approxTracks) {
    blk.clear();
    blk.n = static_cast<int>(sourceVertices.vertices.size());

    blk.vertexId.reserve(sourceVertices.vertices.size());
    blk.x.reserve(sourceVertices.vertices.size());
    blk.y.reserve(sourceVertices.vertices.size());
    blk.pt.reserve(sourceVertices.vertices.size());
    blk.mass.reserve(sourceVertices.vertices.size());
    blk.chi2.reserve(sourceVertices.vertices.size());
    blk.dBVerr.reserve(sourceVertices.vertices.size());
    blk.dBV.reserve(sourceVertices.vertices.size());
    blk.avgDxy.reserve(sourceVertices.vertices.size());
    blk.avgDxyErr.reserve(sourceVertices.vertices.size());
    blk.minOpeningAngle.reserve(sourceVertices.vertices.size());
    blk.nPairs.reserve(sourceVertices.vertices.size());
    blk.nTracks.reserve(sourceVertices.vertices.size());
    blk.trackStart.reserve(sourceVertices.vertices.size());
    blk.trackCount.reserve(sourceVertices.vertices.size());

    int trackOffset = 0;
    for (size_t iv = 0; iv < sourceVertices.vertices.size(); ++iv) {
        const auto &v = sourceVertices.vertices[iv];

        blk.vertexId.push_back(static_cast<int>(iv));
        blk.x.push_back(v.x);
        blk.y.push_back(v.y);
        blk.pt.push_back(v.pt);
        blk.mass.push_back(v.mass);
        blk.chi2.push_back(v.chi2);
        blk.dBVerr.push_back(v.dBVerr);
        blk.dBV.push_back(v.dBV);
        blk.avgDxy.push_back(v.avgDxy);
        blk.avgDxyErr.push_back(v.avgDxyErr);
        blk.minOpeningAngle.push_back(v.minOpeningAngle);
        blk.nPairs.push_back(v.nPairs);
        blk.nTracks.push_back(v.nTracks);
        blk.trackStart.push_back(trackOffset);

        int nThis = 0;
        if (iv < approxTracks.size()) {
            for (const auto &rec : approxTracks[iv]) {
                (void)rec;
                ++nThis;
            }
        }
        blk.trackCount.push_back(nThis);
        trackOffset += nThis;
    }
}

void ScoutingCountTreeMakerRun3::analyze(const edm::Event &iEvent, const edm::EventSetup &iSetup) {
    using namespace edm;
    using namespace reco;

    resetEventBlocks();

    evt_run_ = iEvent.id().run();
    evt_lumi_ = iEvent.id().luminosityBlock();
    evt_event_ = iEvent.id().event();

    // Check track->scouting map availability
    Handle<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>> trackToScoutingRefH;
    iEvent.getByToken(trackToScoutingRefToken_, trackToScoutingRefH);
    haveTrackToScoutingMap_ = trackToScoutingRefH.isValid();
    if (!haveTrackToScoutingMap_ && applyHitCuts_) {
        edm::LogWarning("ScoutingCountTreeMakerRun3")
            << "applyHitCuts=True but Run3ScoutingTrack ValueMap not found; hit-cuts will be skipped.";
    }

    auto const& ttBuilder = iSetup.getData(ttBuilderToken_);

    bool havePV = false;
    bool haveOfflineBS = false;
    bool haveOnlineBS = false;
    Vertex avgPVVtx, offlineBsVtx, onlineBsVtx;

    {
        Handle<std::vector<Vertex>> primaryVerticesH;
        iEvent.getByToken(primaryVerticesToken_, primaryVerticesH);
        if (primaryVerticesH.isValid() && !primaryVerticesH->empty()) {
            double sumX = 0.0, sumY = 0.0, sumZ = 0.0;
            int validPVs = 0;
            for (const auto &pv : *primaryVerticesH) {
                if (!pv.isFake() && pv.ndof() > 4) {
                    sumX += pv.x();
                    sumY += pv.y();
                    sumZ += pv.z();
                    ++validPVs;
                }
            }
            if (validPVs > 0) {
                reco::Vertex::Point avgPos(sumX / validPVs, sumY / validPVs, sumZ / validPVs);
                reco::Vertex::Error avgErr;
                for (int i = 0; i < 3; ++i) {
                    for (int j = i; j < 3; ++j) {
                        avgErr(i, j) = 0.0;
                    }
                }
                avgErr(0,0) = 0.0015 * 0.0015;
                avgErr(1,1) = 0.0015 * 0.0015;
                avgErr(2,2) = 0.0050 * 0.0050;
                avgPVVtx = reco::Vertex(avgPos, avgErr);
                havePV = true;
            }
        }

        Handle<reco::BeamSpot> bsOfflineH;
        iEvent.getByToken(beamspotToken_, bsOfflineH);
        if (bsOfflineH.isValid()) {
            offlineBsVtx = reco::Vertex(bsOfflineH->position(), bsOfflineH->covariance3D());
            haveOfflineBS = true;
        }

        if (useOnlineBeamSpot_) {
            auto bsOnlineHandle = iSetup.getHandle(bsOnlineToken_);
            if (bsOnlineHandle.isValid()) {
                reco::Vertex::Error bsErr;
                for (int i = 0; i < 3; ++i) {
                    for (int j = i; j < 3; ++j) {
                        bsErr(i, j) = bsOnlineHandle->covariance(i, j);
                    }
                }
                const reco::Vertex::Point onlinePos(bsOnlineHandle->x(), bsOnlineHandle->y(), bsOnlineHandle->z());
                onlineBsVtx = reco::Vertex(onlinePos, bsErr);
                haveOnlineBS = true;
            } else if (verbose_) {
                edm::LogInfo("ScoutingCountTreeMakerRun3")
                    << "Online beamspot unavailable; falling back to offline beamspot/PV.";
            }
        }
    }

    Vertex scoutingRefVtx;
    bool haveScoutingRef = false;
    if (useOnlineBeamSpot_ && haveOnlineBS) {
        scoutingRefVtx = onlineBsVtx;
        haveScoutingRef = true;
    } else if (haveOfflineBS) {
        scoutingRefVtx = offlineBsVtx;
        haveScoutingRef = true;
    } else if (havePV) {
        scoutingRefVtx = avgPVVtx;
        haveScoutingRef = true;
    }

    Vertex offlineRefVtx;
    bool haveOfflineRef = false;
    if (haveOfflineBS) {
        offlineRefVtx = offlineBsVtx;
        haveOfflineRef = true;
    } else if (havePV) {
        offlineRefVtx = avgPVVtx;
        haveOfflineRef = true;
    }

    if (!haveScoutingRef || !haveOfflineRef) {
        edm::LogWarning("ScoutingCountTreeMakerRun3")
            << "Missing reference(s): haveScoutingRef=" << haveScoutingRef
            << " haveOfflineRef=" << haveOfflineRef
            << ". Affected collection will yield zero selected vertices.";
    }

    auto vertex_track_vec = [&](const reco::Vertex &v) {
        std::vector<reco::TrackRef> out;
        for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
            const double w = v.trackWeight(*it);
            if (w >= 0.5) out.push_back(it->castTo<reco::TrackRef>());
        }
        return out;
    };

    VertexDistanceXY vdist2d;

    auto processVertices = [&](const edm::Handle<std::vector<reco::Vertex>> &vH,
                               const reco::Vertex &refVtx,
                               bool useHitCutsForCollection) -> SelectionResult {
        SelectionResult result;
        if (!vH.isValid()) return result;

        Handle<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>> trackToScoutingRefH_local;
        if (useHitCutsForCollection) {
            iEvent.getByToken(trackToScoutingRefToken_, trackToScoutingRefH_local);
        }
        const auto *trackToScoutingPtr = trackToScoutingRefH_local.isValid() ? &(*trackToScoutingRefH_local) : nullptr;

        for (size_t iv = 0; iv < vH->size(); ++iv) {
            const auto &v = vH->at(iv);
            std::vector<reco::TrackRef> tks = vertex_track_vec(v);
            const size_t ntk = tks.size();
            if (ntk < 2) continue;

            if (!cut_ntk_.empty()) {
                bool ntkAccepted = false;
                for (int allowed : cut_ntk_) {
                    if (static_cast<int>(ntk) == allowed) {
                        ntkAccepted = true;
                        break;
                    }
                }
                if (!ntkAccepted) continue;
            }

            double minAngle = 1e9;
            int npairs = 0;
            for (size_t i = 0; i < ntk; ++i) {
                if (!tks[i].isNonnull()) continue;
                TVector3 vi(tks[i]->px(), tks[i]->py(), tks[i]->pz());
                if (vi.Mag2() <= 0) continue;
                for (size_t j = i + 1; j < ntk; ++j) {
                    if (!tks[j].isNonnull()) continue;
                    TVector3 vj(tks[j]->px(), tks[j]->py(), tks[j]->pz());
                    if (vj.Mag2() <= 0) continue;
                    const double angle = vi.Angle(vj);
                    ++npairs;
                    if (angle < minAngle) minAngle = angle;
                }
            }
            if (npairs == 0) continue;
            if (cut_opening_angle_min_ > 0.0 && minAngle < cut_opening_angle_min_) continue;

            TLorentzVector sumVec(0,0,0,0);
            double sum_dxy = 0.0;
            double sum_dxyErr = 0.0;
            bool track_ip_ok = true;
            bool hitcuts_ok = true;
            int nGoodTracks = 0;

            for (const auto &trRef : tks) {
                if (!trRef.isNonnull()) continue;
                ++nGoodTracks;

                constexpr double kPionMass = 0.13957;
                TLorentzVector tv;
                tv.SetPtEtaPhiM(trRef->pt(), trRef->eta(), trRef->phi(), kPionMass);
                sumVec += tv;

                if (trRef->pt() < track_pt_min_cut_) {
                    track_ip_ok = false;
                    break;
                }

                reco::TransientTrack ttrack = ttBuilder.build(trRef);

                std::pair<bool,Measurement1D> ipres;
                if (seed_use2DTrackDist_) {
                    ipres = IPTools::absoluteTransverseImpactParameter(ttrack, refVtx);
                } else {
                    ipres = IPTools::absoluteImpactParameter3D(ttrack, refVtx);
                }
                if (!ipres.first) {
                    track_ip_ok = false;
                    break;
                }

                sum_dxy += ipres.second.value();
                sum_dxyErr += ipres.second.error();

                const double ipSig = ipres.second.significance();
                if (ipSig < track_dxySig_min_cut_ || ipSig > track_dxySig_max_cut_) {
                    track_ip_ok = false;
                    break;
                }

                if (applyHitCuts_ && useHitCutsForCollection) {
                    int nPixelHits = 0, nStripHits = 0, nTrackerLayers = 0;
                    if (trackToScoutingPtr) {
                        auto scoutingRef = (*trackToScoutingPtr)[trRef];
                        if (scoutingRef.isNonnull()) {
                            nPixelHits     = scoutingRef->tk_nValidPixelHits();
                            nStripHits     = scoutingRef->tk_nValidStripHits();
                            nTrackerLayers = scoutingRef->tk_nTrackerLayersWithMeasurement();
                        }
                    } else {
                        nPixelHits = 999;
                        nStripHits = 999;
                        nTrackerLayers = 999;
                    }

                    if (nPixelHits < hit_minPixelHits_ ||
                        nStripHits < hit_minStripHits_ ||
                        nTrackerLayers < hit_minTrackerLayers_) {
                        hitcuts_ok = false;
                        break;
                    }
                }
            }

            if (!track_ip_ok) continue;
            if (!hitcuts_ok) continue;

            const double invMass = sumVec.M();
            const double avg_dxy = (nGoodTracks > 0) ? (sum_dxy / double(nGoodTracks)) : 0.0;
            const double avg_dxyErr = (nGoodTracks > 0) ? (sum_dxyErr / double(nGoodTracks)) : 0.0;

            Measurement1D dBVref_meas = vdist2d.distance(v, refVtx);
            const double dBVref = dBVref_meas.value();
            const double dBV_err = dBVref_meas.error();

            if (required_invmass_ != -1 && invMass < required_invmass_) continue;
            if (required_chi2_ != -1 && v.normalizedChi2() > required_chi2_) continue;
            if (required_dBV_min_ != -1 && dBVref < required_dBV_min_) continue;
            if (required_dBV_max_ != -1 && dBVref > required_dBV_max_) continue;
            if (required_dxy_min_ != -1 && avg_dxy < required_dxy_min_) continue;
            if (required_dxy_max_ != -1 && avg_dxy > required_dxy_max_) continue;
            if (required_dBV_error_ != -1 && dBV_err > required_dBV_error_) continue;
            if (required_dxy_error_ != -1 && avg_dxyErr > required_dxy_error_) continue;

            ++result.nSel;

            VertexSummary vtxSummary;
            vtxSummary.x = static_cast<float>(v.x());
            vtxSummary.y = static_cast<float>(v.y());
            vtxSummary.pt = static_cast<float>(sumVec.Pt());
            vtxSummary.mass = static_cast<float>(invMass);
            vtxSummary.chi2 = static_cast<float>(v.normalizedChi2());
            vtxSummary.dBVerr = static_cast<float>(dBV_err);
            vtxSummary.dBV = static_cast<float>(dBVref);
            vtxSummary.avgDxy = static_cast<float>(avg_dxy);
            vtxSummary.avgDxyErr = static_cast<float>(avg_dxyErr);
            vtxSummary.minOpeningAngle = static_cast<float>(minAngle);
            vtxSummary.nPairs = npairs;
            vtxSummary.nTracks = nGoodTracks;

            for (const auto &trRef : tks) {
                if (trRef.isNonnull()) vtxSummary.tracks.push_back(trRef);
            }

            result.vertices.push_back(std::move(vtxSummary));
        }

        return result;
    };

    // -------------------- retrieve vertex collections --------------------
    Handle<std::vector<reco::Vertex>> scoutingVtxH;
    iEvent.getByToken(scoutingVerticesToken_, scoutingVtxH);
    if (!scoutingVtxH.isValid()) {
        edm::LogWarning("ScoutingCountTreeMakerRun3")
            << "Scouting vertex collection is invalid for event "
            << iEvent.id().run() << ":" << iEvent.id().luminosityBlock() << ":" << iEvent.id().event();
    }

    SelectionResult scoutingResult;
    if (haveScoutingRef) scoutingResult = processVertices(scoutingVtxH, scoutingRefVtx, true);
    evt_scoutingSelected_ = scoutingResult.nSel;

    Handle<std::vector<reco::Vertex>> offlineVtxH;
    iEvent.getByToken(offlineVerticesToken_, offlineVtxH);
    if (!offlineVtxH.isValid()) {
        edm::LogWarning("ScoutingCountTreeMakerRun3")
            << "Offline vertex collection is invalid for event "
            << iEvent.id().run() << ":" << iEvent.id().luminosityBlock() << ":" << iEvent.id().event();
    }

    SelectionResult offlineResult;
    if (haveOfflineRef) offlineResult = processVertices(offlineVtxH, offlineRefVtx, false);
    evt_offlineSelected_ = offlineResult.nSel;

    // -------------------- track collections --------------------
    Handle<std::vector<reco::Track>> scoutingTracksH;
    iEvent.getByToken(tracksToken_, scoutingTracksH);
    if (!scoutingTracksH.isValid()) {
        edm::LogWarning("ScoutingCountTreeMakerRun3")
            << "Scouting track collection is invalid for event "
            << iEvent.id().run() << ":" << iEvent.id().luminosityBlock() << ":" << iEvent.id().event();
    }

    Handle<std::vector<reco::Track>> offlineTracksH;
    iEvent.getByToken(offlineTracksToken_, offlineTracksH);
    if (!offlineTracksH.isValid()) {
        edm::LogWarning("ScoutingCountTreeMakerRun3")
            << "Offline track collection is invalid for event "
            << iEvent.id().run() << ":" << iEvent.id().luminosityBlock() << ":" << iEvent.id().event();
    }

    // Build scouting track pool used for matching/reporting.
    std::vector<size_t> scoutingTrackPoolIndices;
    if (scoutingTracksH.isValid()) {
        Handle<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>> trackToScoutingRefPoolH;
        if (applyHitCuts_) {
            iEvent.getByToken(trackToScoutingRefToken_, trackToScoutingRefPoolH);
        }
        const auto *trackToScoutingPoolPtr = trackToScoutingRefPoolH.isValid() ? &(*trackToScoutingRefPoolH) : nullptr;

        if (!haveScoutingRef) {
            edm::LogWarning("ScoutingCountTreeMakerRun3")
                << "Scouting reference unavailable: scouting track pool cannot be IP-filtered for event "
                << iEvent.id().run() << ":" << iEvent.id().luminosityBlock() << ":" << iEvent.id().event();
        } else {
            scoutingTrackPoolIndices.reserve(scoutingTracksH->size());
            for (size_t iscout = 0; iscout < scoutingTracksH->size(); ++iscout) {
                reco::TrackRef trackRef(scoutingTracksH, iscout);
                if (!trackRef.isNonnull()) continue;

                if (trackRef->pt() < track_pt_min_cut_) continue;

                reco::TransientTrack ttrack = ttBuilder.build(trackRef);
                std::pair<bool, Measurement1D> ipres;
                if (seed_use2DTrackDist_) {
                    ipres = IPTools::absoluteTransverseImpactParameter(ttrack, scoutingRefVtx);
                } else {
                    ipres = IPTools::absoluteImpactParameter3D(ttrack, scoutingRefVtx);
                }
                if (!ipres.first) continue;

                const double ipSig = ipres.second.significance();
                if (ipSig < track_dxySig_min_cut_ || ipSig > track_dxySig_max_cut_) continue;

                if (applyHitCuts_) {
                    int nPixelHits = 999;
                    int nStripHits = 999;
                    int nTrackerLayers = 999;
                    if (trackToScoutingPoolPtr) {
                        auto scoutingRef = (*trackToScoutingPoolPtr)[trackRef];
                        if (scoutingRef.isNonnull()) {
                            nPixelHits     = scoutingRef->tk_nValidPixelHits();
                            nStripHits     = scoutingRef->tk_nValidStripHits();
                            nTrackerLayers = scoutingRef->tk_nTrackerLayersWithMeasurement();
                        }
                    }
                    if (nPixelHits < hit_minPixelHits_ ||
                        nStripHits < hit_minStripHits_ ||
                        nTrackerLayers < hit_minTrackerLayers_) {
                        continue;
                    }
                }

                scoutingTrackPoolIndices.push_back(iscout);
            }
        }

        if (verbose_) {
            edm::LogInfo("ScoutingCountTreeMakerRun3")
                << "Scouting track pool size after filtering: " << scoutingTrackPoolIndices.size()
                << " / " << scoutingTracksH->size();
        }
    }

    evt_scoutingTrackPoolSize_ = static_cast<int>(scoutingTrackPoolIndices.size());

    if (scoutingTrackPoolIndices.empty()) {
        ++nEventsPoolEmpty_;
    } else {
        ++nEventsPoolNonEmpty_;
    }

    // -------------------- approximate matching --------------------
    constexpr double kApproximateMatchCut = 1.3;
    auto deltaPhiAbs = [](double phi1, double phi2) {
        return std::fabs(std::atan2(std::sin(phi1 - phi2), std::cos(phi1 - phi2)));
    };

    std::vector<std::vector<TrackMatchRecord>> approximateScoutingMatches; // offline source -> scouting match
    if (evt_offlineSelected_ > 0) {
        approximateScoutingMatches.resize(offlineResult.vertices.size());
        if (!scoutingTracksH.isValid()) {
            edm::LogWarning("ScoutingCountTreeMakerRun3")
                << "Cannot build approximate scouting matches because the scouting track collection is unavailable.";
        } else if (scoutingTrackPoolIndices.empty()) {
            if (verbose_) {
                edm::LogInfo("ScoutingCountTreeMakerRun3")
                    << "Scouting track pool is empty after filtering; approximate scouting matching is skipped.";
            }
            // still keep source tracks with hasMatch = false
            for (size_t ivtx = 0; ivtx < offlineResult.vertices.size(); ++ivtx) {
                const auto &vertex = offlineResult.vertices[ivtx];
                std::set<unsigned int> seenOfflineTrackKeys;
                for (const auto &offlineTrackRef : vertex.tracks) {
                    if (!offlineTrackRef.isNonnull()) continue;
                    if (!seenOfflineTrackKeys.insert(offlineTrackRef.key()).second) continue;
                    TrackMatchRecord rec;
                    rec.source = summarizeTrack(*offlineTrackRef);
                    approximateScoutingMatches[ivtx].push_back(rec);
                }
            }
        } else {
            for (size_t ivtx = 0; ivtx < offlineResult.vertices.size(); ++ivtx) {
                const auto &vertex = offlineResult.vertices[ivtx];
                std::set<unsigned int> seenOfflineTrackKeys;

                for (const auto &offlineTrackRef : vertex.tracks) {
                    if (!offlineTrackRef.isNonnull()) continue;
                    if (!seenOfflineTrackKeys.insert(offlineTrackRef.key()).second) continue;

                    TrackMatchRecord rec;
                    rec.source = summarizeTrack(*offlineTrackRef);

                    std::vector<std::pair<double, size_t>> rankedMatches;
                    rankedMatches.reserve(scoutingTrackPoolIndices.size());

                    for (const size_t iscout : scoutingTrackPoolIndices) {
                        const auto &scoutingTrack = scoutingTracksH->at(iscout);
                        const double dEta = std::fabs(offlineTrackRef->eta() - scoutingTrack.eta());
                        const double dPhi = deltaPhiAbs(offlineTrackRef->phi(), scoutingTrack.phi());
                        const double dPt = std::fabs(offlineTrackRef->pt() - scoutingTrack.pt());
                        const double cost = (1.0 + dEta) * (1.0 + dPhi) * (1.0 + dPt);
                        if (cost >= kApproximateMatchCut) continue;
                        rankedMatches.emplace_back(cost, iscout);
                    }

                    if (!rankedMatches.empty()) {
                        std::sort(rankedMatches.begin(), rankedMatches.end(),
                                  [](const auto &lhs, const auto &rhs) { return lhs.first < rhs.first; });

                        rec.hasMatch = true;
                        rec.match = summarizeTrack(scoutingTracksH->at(rankedMatches.front().second));
                    }

                    approximateScoutingMatches[ivtx].push_back(rec);
                }
            }
        }
    }

    std::vector<std::vector<TrackMatchRecord>> approximateOfflineMatches; // scouting source -> offline match
    if (evt_scoutingSelected_ > 0) {
        approximateOfflineMatches.resize(scoutingResult.vertices.size());
        if (!offlineTracksH.isValid()) {
            edm::LogWarning("ScoutingCountTreeMakerRun3")
                << "Cannot build approximate offline matches because the offline track collection is unavailable.";
        } else {
            for (size_t ivtx = 0; ivtx < scoutingResult.vertices.size(); ++ivtx) {
                const auto &vertex = scoutingResult.vertices[ivtx];
                std::set<unsigned int> seenScoutingTrackKeys;

                for (const auto &scoutingTrackRef : vertex.tracks) {
                    if (!scoutingTrackRef.isNonnull()) continue;
                    if (!seenScoutingTrackKeys.insert(scoutingTrackRef.key()).second) continue;

                    TrackMatchRecord rec;
                    rec.source = summarizeTrack(*scoutingTrackRef);

                    std::vector<std::pair<double, size_t>> rankedMatches;
                    rankedMatches.reserve(offlineTracksH->size());

                    for (size_t ioff = 0; ioff < offlineTracksH->size(); ++ioff) {
                        const auto &offlineTrack = offlineTracksH->at(ioff);
                        const double dEta = std::fabs(scoutingTrackRef->eta() - offlineTrack.eta());
                        const double dPhi = deltaPhiAbs(scoutingTrackRef->phi(), offlineTrack.phi());
                        const double dPt  = std::fabs(scoutingTrackRef->pt() - offlineTrack.pt());
                        const double cost = (1.0 + dEta) * (1.0 + dPhi) * (1.0 + dPt);
                        if (cost >= kApproximateMatchCut) continue;
                        rankedMatches.emplace_back(cost, ioff);
                    }

                    if (!rankedMatches.empty()) {
                        std::sort(rankedMatches.begin(), rankedMatches.end(),
                                  [](const auto &lhs, const auto &rhs) { return lhs.first < rhs.first; });

                        rec.hasMatch = true;
                        rec.match = summarizeTrack(offlineTracksH->at(rankedMatches.front().second));
                    }

                    approximateOfflineMatches[ivtx].push_back(rec);
                }
            }
        }
    }

    // Count matched tracks in the same spirit as the original code.
    auto countMatched = [](const std::vector<std::vector<TrackMatchRecord>> &vv) {
        int total = 0;
        for (const auto &v : vv) {
            for (const auto &r : v) {
                if (r.hasMatch) ++total;
            }
        }
        return total;
    };

    evt_offlineMatchedTrackCount_ = countMatched(approximateScoutingMatches);
    evt_scoutingMatchedTrackCount_ = countMatched(approximateOfflineMatches);

    auto classifyMatchBucket = [](int nMatchedTracks) {
        if (nMatchedTracks >= 2) return 2;
        if (nMatchedTracks == 1) return 1;
        return 0;
    };

    evt_offlineMatchBucket_ = classifyMatchBucket(evt_offlineMatchedTrackCount_);
    evt_scoutingMatchBucket_ = classifyMatchBucket(evt_scoutingMatchedTrackCount_);

    const bool hasOfflineSelected = (evt_offlineSelected_ > 0);
    const bool hasScoutingSelected = (evt_scoutingSelected_ > 0);

    if (hasOfflineSelected) ++nEventsWithOfflineSelected_;
    if (hasScoutingSelected) ++nEventsWithScoutingSelected_;
    if (hasOfflineSelected && hasScoutingSelected) {
        ++nEventsWithBothSelected_;
    } else if (hasOfflineSelected) {
        ++nEventsWithOfflineOnly_;
    } else if (hasScoutingSelected) {
        ++nEventsWithScoutingOnly_;
    }

    if (hasOfflineSelected) {
        if (hasScoutingSelected) {
            ++nOffSelAndScoSel_;
            if (evt_offlineMatchBucket_ == 2) ++nOffSelScoMatchR_;
            else if (evt_offlineMatchBucket_ == 1) ++nOffSelScoMatch_;
            else ++nOffSelScoNoMatch_;
        } else {
            ++nOffSelAndNoScoSel_;
            if (evt_offlineMatchBucket_ == 2) ++nOffSelNoScoMatchR_;
            else if (evt_offlineMatchBucket_ == 1) ++nOffSelNoScoMatch_;
            else ++nOffSelNoScoNoMatch_;
        }
    }

    if (hasScoutingSelected) {
        if (hasOfflineSelected) {
            ++nScoSelAndOffSel_;
            if (evt_scoutingMatchBucket_ == 2) ++nScoSelOffMatchR_;
            else if (evt_scoutingMatchBucket_ == 1) ++nScoSelOffMatch_;
            else ++nScoSelOffNoMatch_;
        } else {
            ++nScoSelAndNoOffSel_;
            if (evt_scoutingMatchBucket_ == 2) ++nScoSelNoOffMatchR_;
            else if (evt_scoutingMatchBucket_ == 1) ++nScoSelNoOffMatch_;
            else ++nScoSelNoOffNoMatch_;
        }
    }

    // -------------------- build event metadata --------------------
    evt_hasScouting_ = hasScoutingSelected;
    evt_hasOffline_ = hasOfflineSelected;
    evt_hasBoth_ = (hasScoutingSelected && hasOfflineSelected);
    evt_matchCategory_ = evt_hasBoth_ ? 2 : (evt_hasScouting_ || evt_hasOffline_ ? 1 : 0);
    evt_poolNonEmpty_ = !scoutingTrackPoolIndices.empty();

    // beamspot metadata
    evt_beamspot_x_ = 0.0f;
    evt_beamspot_y_ = 0.0f;
    evt_beamspot_z_ = 0.0f;
    if (haveOfflineBS) {
        evt_beamspot_x_ = static_cast<float>(offlineBsVtx.x());
        evt_beamspot_y_ = static_cast<float>(offlineBsVtx.y());
        evt_beamspot_z_ = static_cast<float>(offlineBsVtx.z());
    } else if (havePV) {
        evt_beamspot_x_ = static_cast<float>(avgPVVtx.x());
        evt_beamspot_y_ = static_cast<float>(avgPVVtx.y());
        evt_beamspot_z_ = static_cast<float>(avgPVVtx.z());
    }

    evt_nPV_ = havePV ? 1 : 0;

    // Same folder-style metadata as a string, but stored in the tree.
    if (evt_scoutingSelected_ > 0 || evt_offlineSelected_ > 0) {
        std::ostringstream dirName;
        ++nSelectedEvents_;
        evt_selectedEventIndex_ = static_cast<int>(nSelectedEvents_);

        const bool poolNonEmpty = !scoutingTrackPoolIndices.empty();
        dirName << (poolNonEmpty ? "EW" : "E") << nSelectedEvents_;

        if (evt_offlineSelected_ > 0) {
            dirName << "_O";
            std::set<int> uniqueNtksOff;
            for (const auto &v : offlineResult.vertices) uniqueNtksOff.insert(v.nTracks);
            for (int ntk : uniqueNtksOff) {
                int matchedTrackCount = 0;
                for (size_t ivtx = 0; ivtx < approximateScoutingMatches.size(); ++ivtx) {
                    if (ivtx < offlineResult.vertices.size() &&
                        offlineResult.vertices[ivtx].nTracks == ntk) {
                        for (const auto &rec : approximateScoutingMatches[ivtx]) {
                            if (rec.hasMatch) ++matchedTrackCount;
                        }
                    }
                }
                dirName << "_ntk" << ntk;
                if (matchedTrackCount > 0) {
                    dirName << "withMatch";
                    if (matchedTrackCount >= 2) dirName << "(R)";
                }
            }
        }

        if (evt_scoutingSelected_ > 0) {
            dirName << "_S";
            std::set<int> uniqueNtksSco;
            for (const auto &v : scoutingResult.vertices) uniqueNtksSco.insert(v.nTracks);
            for (int ntk : uniqueNtksSco) {
                int matchedTrackCount = 0;
                for (size_t ivtx = 0; ivtx < approximateOfflineMatches.size(); ++ivtx) {
                    if (ivtx < scoutingResult.vertices.size() &&
                        scoutingResult.vertices[ivtx].nTracks == ntk) {
                        for (const auto &rec : approximateOfflineMatches[ivtx]) {
                            if (rec.hasMatch) ++matchedTrackCount;
                        }
                    }
                }
                dirName << "_ntk" << ntk;
                if (matchedTrackCount > 0) {
                    dirName << "withMatch";
                    if (matchedTrackCount >= 2) dirName << "(R)";
                }
            }
        }

        evt_eventTag_ = dirName.str().c_str();
    } else {
        evt_selectedEventIndex_ = 0;
        evt_eventTag_ = "";
    }

    // -------------------- fill tree blocks --------------------
    fillVertexBlockFromSelection(scoSelVtx_, scoutingResult);
    fillVertexBlockFromSelection(offSelVtx_, offlineResult);

    // Selected-vertex track blocks: flat track lists from selected vertices
    {
        std::vector<TrackSummary> tracks;
        for (const auto &v : scoutingResult.vertices) {
            std::set<unsigned int> seen;
            for (const auto &trRef : v.tracks) {
                if (!trRef.isNonnull()) continue;
                if (!seen.insert(trRef.key()).second) continue;
                tracks.push_back(summarizeTrack(*trRef));
            }
        }
        fillTrackBlockFromTrackSummaries(scoSelTrk_, tracks, false);
    }

    {
        std::vector<TrackSummary> tracks;
        for (const auto &v : offlineResult.vertices) {
            std::set<unsigned int> seen;
            for (const auto &trRef : v.tracks) {
                if (!trRef.isNonnull()) continue;
                if (!seen.insert(trRef.key()).second) continue;
                tracks.push_back(summarizeTrack(*trRef));
            }
        }
        fillTrackBlockFromTrackSummaries(offSelTrk_, tracks, false);
    }

    fillTrackBlockFromRecoTracks(scoRawTrk_, scoutingTracksH, nullptr);
    fillTrackBlockFromRecoTracks(offRawTrk_, offlineTracksH, nullptr);
    fillTrackBlockFromRecoTracks(scoCutTrk_, scoutingTracksH, &scoutingTrackPoolIndices);

    // Approximate match blocks store the source track and the matched track.
    fillVertexBlockFromApprox(offApproxVtx_, offlineResult, approximateScoutingMatches);
    fillVertexBlockFromApprox(scoApproxVtx_, scoutingResult, approximateOfflineMatches);

    {
        std::vector<TrackMatchRecord> flat;
        for (const auto &v : approximateScoutingMatches) {
            for (const auto &rec : v) flat.push_back(rec);
        }
        fillTrackBlockFromTrackMatchRecords(offApproxTrk_, flat);
    }

    {
        std::vector<TrackMatchRecord> flat;
        for (const auto &v : approximateOfflineMatches) {
            for (const auto &rec : v) flat.push_back(rec);
        }
        fillTrackBlockFromTrackMatchRecords(scoApproxTrk_, flat);
    }

    // -------------------- event log --------------------
    if (evt_scoutingSelected_ > 0 || evt_offlineSelected_ > 0) {
        if (verbose_) {
            std::cout << "Selected vertices found in event " << iEvent.id().event()
                      << " (run:lumi:event=" << iEvent.id().run() << ":"
                      << iEvent.id().luminosityBlock() << ":" << iEvent.id().event() << ")"
                      << " scouting=" << evt_scoutingSelected_
                      << " offline=" << evt_offlineSelected_
                      << std::endl;
        }
    }

    ++nEvents_;
    totalScoutingSelected_ += static_cast<unsigned long long>(evt_scoutingSelected_);
    totalOfflineSelected_ += static_cast<unsigned long long>(evt_offlineSelected_);

    std::ostringstream oss;
    oss << "Event " << iEvent.id().run() << ":" << iEvent.id().luminosityBlock() << ":" << iEvent.id().event()
        << " scouting_selected_vertices=" << evt_scoutingSelected_
        << " offline_selected_vertices=" << evt_offlineSelected_
        << " poolNonEmpty=" << evt_poolNonEmpty_
        << " tag=" << evt_eventTag_;
    if (verbose_) {
        edm::LogInfo("ScoutingCountTreeMakerRun3") << oss.str();
    }

    if (tree_) tree_->Fill();
}

void ScoutingCountTreeMakerRun3::endJob() {
    if (!printSummary_) return;

    const auto fracPct = [](unsigned long long num, unsigned long long den) {
        if (den == 0) return 0.0;
        return 100.0 * static_cast<double>(num) / static_cast<double>(den);
    };

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "================ ScoutingCountTreeMakerRun3 Summary ================\n";
    std::cout << "Total events processed: " << nEvents_ << "\n";
    std::cout << "Events with offline selected vertices: "
              << nEventsWithOfflineSelected_ << "/" << nEvents_
              << " (" << fracPct(nEventsWithOfflineSelected_, nEvents_) << "%)\n";
    std::cout << "Events with scouting selected vertices: "
              << nEventsWithScoutingSelected_ << "/" << nEvents_
              << " (" << fracPct(nEventsWithScoutingSelected_, nEvents_) << "%)\n";
    std::cout << "Events with both selected: "
              << nEventsWithBothSelected_ << "/" << nEvents_
              << " (" << fracPct(nEventsWithBothSelected_, nEvents_) << "%)\n";
    std::cout << "Events with offline only: "
              << nEventsWithOfflineOnly_ << "/" << nEvents_
              << " (" << fracPct(nEventsWithOfflineOnly_, nEvents_) << "%)\n";
    std::cout << "Events with scouting only: "
              << nEventsWithScoutingOnly_ << "/" << nEvents_
              << " (" << fracPct(nEventsWithScoutingOnly_, nEvents_) << "%)\n";

    std::cout << "Offline-selected event breakdown:\n";
    std::cout << "Offline selected AND scouting selected: "
              << nOffSelAndScoSel_ << "/" << nEventsWithOfflineSelected_
              << " (" << fracPct(nOffSelAndScoSel_, nEventsWithOfflineSelected_) << "%)\n";
    std::cout << "Offline selected AND no scouting selected: "
              << nOffSelAndNoScoSel_ << "/" << nEventsWithOfflineSelected_
              << " (" << fracPct(nOffSelAndNoScoSel_, nEventsWithOfflineSelected_) << "%)\n";

    std::cout << "Offline selected, no scouting selected, no match: "
              << nOffSelNoScoNoMatch_ << "/" << nEventsWithOfflineSelected_
              << " (" << fracPct(nOffSelNoScoNoMatch_, nEventsWithOfflineSelected_) << "%)\n";
    std::cout << "Offline selected, no scouting selected, match: "
              << nOffSelNoScoMatch_ << "/" << nEventsWithOfflineSelected_
              << " (" << fracPct(nOffSelNoScoMatch_, nEventsWithOfflineSelected_) << "%)\n";
    std::cout << "Offline selected, no scouting selected, match(R): "
              << nOffSelNoScoMatchR_ << "/" << nEventsWithOfflineSelected_
              << " (" << fracPct(nOffSelNoScoMatchR_, nEventsWithOfflineSelected_) << "%)\n";

    std::cout << "Offline selected, scouting selected too, no match: "
              << nOffSelScoNoMatch_ << "/" << nEventsWithOfflineSelected_
              << " (" << fracPct(nOffSelScoNoMatch_, nEventsWithOfflineSelected_) << "%)\n";
    std::cout << "Offline selected, scouting selected too, match: "
              << nOffSelScoMatch_ << "/" << nEventsWithOfflineSelected_
              << " (" << fracPct(nOffSelScoMatch_, nEventsWithOfflineSelected_) << "%)\n";
    std::cout << "Offline selected, scouting selected too, match(R): "
              << nOffSelScoMatchR_ << "/" << nEventsWithOfflineSelected_
              << " (" << fracPct(nOffSelScoMatchR_, nEventsWithOfflineSelected_) << "%)\n";

    std::cout << "Scouting-selected event breakdown:\n";
    std::cout << "Scouting selected AND offline selected: "
              << nScoSelAndOffSel_ << "/" << nEventsWithScoutingSelected_
              << " (" << fracPct(nScoSelAndOffSel_, nEventsWithScoutingSelected_) << "%)\n";
    std::cout << "Scouting selected AND no offline selected: "
              << nScoSelAndNoOffSel_ << "/" << nEventsWithScoutingSelected_
              << " (" << fracPct(nScoSelAndNoOffSel_, nEventsWithScoutingSelected_) << "%)\n";

    std::cout << "Scouting selected, no offline selected, no match: "
              << nScoSelNoOffNoMatch_ << "/" << nEventsWithScoutingSelected_
              << " (" << fracPct(nScoSelNoOffNoMatch_, nEventsWithScoutingSelected_) << "%)\n";
    std::cout << "Scouting selected, no offline selected, match: "
              << nScoSelNoOffMatch_ << "/" << nEventsWithScoutingSelected_
              << " (" << fracPct(nScoSelNoOffMatch_, nEventsWithScoutingSelected_) << "%)\n";
    std::cout << "Scouting selected, no offline selected, match(R): "
              << nScoSelNoOffMatchR_ << "/" << nEventsWithScoutingSelected_
              << " (" << fracPct(nScoSelNoOffMatchR_, nEventsWithScoutingSelected_) << "%)\n";

    std::cout << "Scouting selected, offline selected too, no match: "
              << nScoSelOffNoMatch_ << "/" << nEventsWithScoutingSelected_
              << " (" << fracPct(nScoSelOffNoMatch_, nEventsWithScoutingSelected_) << "%)\n";
    std::cout << "Scouting selected, offline selected too, match: "
              << nScoSelOffMatch_ << "/" << nEventsWithScoutingSelected_
              << " (" << fracPct(nScoSelOffMatch_, nEventsWithScoutingSelected_) << "%)\n";
    std::cout << "Scouting selected, offline selected too, match(R): "
              << nScoSelOffMatchR_ << "/" << nEventsWithScoutingSelected_
              << " (" << fracPct(nScoSelOffMatchR_, nEventsWithScoutingSelected_) << "%)\n";

    std::cout << "Scouting track cut pool occupancy (all events):\n";
    std::cout << "Events with non-empty scouting cut pool (EW): "
              << nEventsPoolNonEmpty_ << "/" << nEvents_
              << " (" << fracPct(nEventsPoolNonEmpty_, nEvents_) << "%)\n";
    std::cout << "Events with empty scouting cut pool (E): "
              << nEventsPoolEmpty_ << "/" << nEvents_
              << " (" << fracPct(nEventsPoolEmpty_, nEvents_) << "%)\n";
    std::cout << "===============================================================\n";

    edm::LogInfo("ScoutingCountTreeMakerRun3")
        << "Summary: events=" << nEvents_
        << " total_scouting_selected_vertices=" << totalScoutingSelected_
        << " total_offline_selected_vertices=" << totalOfflineSelected_;
}

void ScoutingCountTreeMakerRun3::fillDescriptions(edm::ConfigurationDescriptions &descriptions) {
    edm::ParameterSetDescription desc;
    desc.add<edm::InputTag>("scoutingVertices", edm::InputTag("Vertexer"));
    desc.add<edm::InputTag>("offlineVertices", edm::InputTag("displacedVertices"));
    desc.add<edm::InputTag>("primaryVertices", edm::InputTag("hltScoutingPrimaryVertexPacker", "primaryVtx"));
    desc.add<edm::InputTag>("beamspot_src", edm::InputTag("offlineBeamSpot"));
    desc.add<edm::InputTag>("tracks", edm::InputTag("hltScoutingUnpackProducer", "Track"));
    desc.add<edm::InputTag>("offlineTracks", edm::InputTag("packedCandidateToTrack", "Track"));

    edm::ParameterSetDescription ntkDesc;
    ntkDesc.add<std::vector<int>>("values", {});
    desc.add("cut_ntk", ntkDesc);

    desc.add<double>("cut_opening_angle_min", 0.05);
    desc.add<double>("required_invmass", 2.0);
    desc.add<double>("required_chi2", -1.0);
    desc.add<double>("required_dBV_min", -1.0);
    desc.add<double>("required_dBV_max", -1.0);
    desc.add<double>("required_dxy_min", -1.0);
    desc.add<double>("required_dxy_max", -1.0);
    desc.add<double>("required_dBV_error", -1.0);
    desc.add<double>("required_dxy_error", -1.0);

    desc.addUntracked<double>("track_pt_min_cut", 0.9);
    desc.addUntracked<double>("track_dxySig_min_cut", 4.0);
    desc.addUntracked<double>("track_dxySig_max_cut", 100.0);

    desc.add<bool>("applyHitCuts", true);
    desc.addUntracked<int>("hit_minPixelHits", 3);
    desc.addUntracked<int>("hit_minStripHits", 2);
    desc.addUntracked<int>("hit_minTrackerLayers", 6);

    desc.addUntracked<double>("seed_minIPSig", 4.0);
    desc.addUntracked<double>("seed_minPt", 0.9);
    desc.addUntracked<double>("seed_maxIPSig", 100.0);
    desc.addUntracked<bool>("seed_use2DTrackDist", true);
    desc.addUntracked<bool>("verbose", false);
    desc.addUntracked<bool>("printSummary", true);

    desc.addUntracked<bool>("useOnlineBeamSpot", true);
    desc.addUntracked<std::string>("refPreference", "BeamSpot");

    descriptions.add("ScoutingCountTreeMakerRun3", desc);
}

DEFINE_FWK_MODULE(ScoutingCountTreeMakerRun3);