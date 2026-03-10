// ScoutingCountMakerRun3.cc
// Minimal EDAnalyzer: counts selected vertices (scouting & offline) per event and logs counts.
//
// Configure parameters in your cfg as shown after this file.

#include <memory>
#include <vector>
#include <string>
#include <cmath>
#include <sstream>
#include <iostream>
#include <set>

#include "TLorentzVector.h"
#include "TVector3.h"

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
#include "TH1F.h"
#include "TH2F.h"

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

class ScoutingCountMakerRun3 : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
    explicit ScoutingCountMakerRun3(const edm::ParameterSet &);
    ~ScoutingCountMakerRun3() override = default;

    static void fillDescriptions(edm::ConfigurationDescriptions &descriptions);

private:
    void beginJob() override {}
    void analyze(const edm::Event &, const edm::EventSetup &) override;
    void endJob() override;

    // config
    const edm::InputTag scoutingVerticesTag_;
    const edm::InputTag offlineVerticesTag_;
    const edm::InputTag primaryVerticesTag_;
    const edm::InputTag beamspotTag_;

    const double cut_opening_angle_min_;
    const double required_invmass_;
    const double required_chi2_;
    const double required_dBV_min_;
    const double required_dBV_max_;
    const double required_dxy_min_;
    const double required_dxy_max_;
    const double required_dBV_error_;
    const double required_dxy_error_;

    // track-level analyzer cuts
    const double track_pt_min_cut_;
    const double track_dxySig_min_cut_;
    const double track_dxySig_max_cut_;

    // hit cuts
    const bool applyHitCuts_;
    const int hit_minPixelHits_;
    const int hit_minStripHits_;
    const int hit_minTrackerLayers_;

    // seed thresholds (used to filter seed-like tracks if desired)
    const double seed_minIPSig_;
    const double seed_minPt_;
    const double seed_maxIPSig_;
    const bool seed_use2DTrackDist_;
    const bool verbose_;
    const bool printSummary_;

    // reference / beamspot
    const bool useOnlineBeamSpot_;
    enum class RefPreference { PreferPV, PreferBeamSpot };
    const RefPreference refPreference_;

    // tokens
    const edm::EDGetTokenT<std::vector<reco::Vertex>> scoutingVerticesToken_;
    const edm::EDGetTokenT<std::vector<reco::Vertex>> offlineVerticesToken_;
    const edm::EDGetTokenT<std::vector<reco::Vertex>> primaryVerticesToken_;
    const edm::EDGetTokenT<reco::BeamSpot> beamspotToken_;
    const edm::EDGetTokenT<std::vector<reco::Track>> tracksToken_;
    const edm::EDGetTokenT<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>> trackToScoutingRefToken_;
    const edm::ESGetToken<TransientTrackBuilder, TransientTrackRecord> ttBuilderToken_;
    const edm::ESGetToken<BeamSpotOnlineObjects, BeamSpotOnlineHLTObjectsRcd> bsOnlineToken_;

    // runtime flags
    bool haveTrackToScoutingMap_ = false;

    // simple run summary counters
    unsigned long long nEvents_ = 0;
    unsigned long long totalScoutingSelected_ = 0;
    unsigned long long totalOfflineSelected_ = 0;
};

// -------------------- Implementation --------------------

ScoutingCountMakerRun3::ScoutingCountMakerRun3(const edm::ParameterSet &ps) :
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
    refPreference_( ps.getUntrackedParameter<std::string>("refPreference", "BeamSpot") == "PV" ? RefPreference::PreferPV : RefPreference::PreferBeamSpot ),

    scoutingVerticesToken_( consumes<std::vector<reco::Vertex>>(scoutingVerticesTag_) ),
    offlineVerticesToken_( consumes<std::vector<reco::Vertex>>(offlineVerticesTag_) ),
    primaryVerticesToken_( consumes<std::vector<reco::Vertex>>(primaryVerticesTag_) ),
    beamspotToken_( consumes<reco::BeamSpot>(beamspotTag_) ),
    tracksToken_( consumes<std::vector<reco::Track>>( ps.getParameter<edm::InputTag>("tracks") ) ),
    trackToScoutingRefToken_( consumes<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>>( edm::InputTag("hltScoutingUnpackProducer","Track-RefToOriginal") ) ),
    ttBuilderToken_( esConsumes<TransientTrackBuilder, TransientTrackRecord>(edm::ESInputTag("", "TransientTrackBuilder")) ),
    bsOnlineToken_( esConsumes<BeamSpotOnlineObjects, BeamSpotOnlineHLTObjectsRcd>() )
{
    usesResource("TFileService"); // no histos but safe if other modules use TFileService
    // Note: trackToScoutingRefToken_ may be invalid in some workflows - we check at runtime
}

void ScoutingCountMakerRun3::analyze(const edm::Event &iEvent, const edm::EventSetup &iSetup) {
    using namespace edm;
    using namespace reco;

    // Check track->scouting map availability once per event (we do it each event but it's cheap)
    Handle<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>> trackToScoutingRefH;
    iEvent.getByToken(trackToScoutingRefToken_, trackToScoutingRefH);
    haveTrackToScoutingMap_ = trackToScoutingRefH.isValid();
    if (!haveTrackToScoutingMap_ && applyHitCuts_) {
        edm::LogWarning("ScoutingCountMakerRun3") << "applyHitCuts=True but Run3ScoutingTrack ValueMap not found; hit-cuts will be skipped.";
    }

    // Load transient track builder
    auto const& ttBuilder = iSetup.getData(ttBuilderToken_);

    // Build candidate references used by scouting/offline collections.
    bool havePV = false;
    bool haveOfflineBS = false;
    bool haveOnlineBS = false;
    Vertex avgPVVtx, offlineBsVtx, onlineBsVtx;
    {
        Handle<std::vector<Vertex>> primaryVerticesH;
        iEvent.getByToken(primaryVerticesToken_, primaryVerticesH);
        if (primaryVerticesH.isValid() && !primaryVerticesH->empty()) {
            double sumX=0, sumY=0, sumZ=0; int validPVs=0;
            for (const auto &pv : *primaryVerticesH) {
                if (!pv.isFake() && pv.ndof() > 4) {
                    sumX += pv.x(); sumY += pv.y(); sumZ += pv.z();
                    ++validPVs;
                }
            }
            if (validPVs>0) {
                reco::Vertex::Point avgPos(sumX/validPVs, sumY/validPVs, sumZ/validPVs);
                reco::Vertex::Error avgErr;
                for (int i=0;i<3;++i) for (int j=i;j<3;++j) avgErr(i,j)=0.0;
                avgErr(0,0)=0.0015*0.0015; avgErr(1,1)=0.0015*0.0015; avgErr(2,2)=0.0050*0.0050;
                avgPVVtx = reco::Vertex(avgPos, avgErr);
                havePV = true;
            }
        }

        // offline beamspot
        Handle<reco::BeamSpot> bsOfflineH;
        iEvent.getByToken(beamspotToken_, bsOfflineH);
        if (bsOfflineH.isValid()) {
            offlineBsVtx = reco::Vertex(bsOfflineH->position(), bsOfflineH->covariance3D());
            haveOfflineBS = true;
        }

        // online beamspot (if requested)
        if (useOnlineBeamSpot_) {
            auto bsOnlineHandle = iSetup.getHandle(bsOnlineToken_);
            if (bsOnlineHandle.isValid()) {
                reco::Vertex::Error bsErr;
                for (int i=0;i<3;++i) for (int j=i;j<3;++j) bsErr(i,j)=bsOnlineHandle->covariance(i,j);
                const reco::Vertex::Point onlinePos(bsOnlineHandle->x(), bsOnlineHandle->y(), bsOnlineHandle->z());
                onlineBsVtx = reco::Vertex(onlinePos, bsErr);
                haveOnlineBS = true;
            } else {
                if (verbose_) {
                    edm::LogInfo("ScoutingCountMakerRun3")
                        << "Online beamspot unavailable in BeamSpotOnlineHLTObjectsRcd; falling back to offline beamspot/PV as configured.";
                }
            }
        }
    }

    // Scouting reference: online beamspot if requested and available, then offline beamspot, then PV.
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

    // Offline reference: offline beamspot first, then PV.
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
        edm::LogWarning("ScoutingCountMakerRun3")
            << "Missing reference(s): haveScoutingRef=" << haveScoutingRef
            << " haveOfflineRef=" << haveOfflineRef
            << ". Affected collection will yield zero selected vertices.";
    }

    // We'll implement selection loops manually for scouting and offline collections,
    // filling per-event counters and applying checks similar to the prior analyzer.

    // Utility to extract track refs from a vertex (weight >= 0.5)
    auto vertex_track_vec = [&](const reco::Vertex &v) {
        std::vector<reco::TrackRef> out;
        for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
            const double w = v.trackWeight(*it);
            if (w >= 0.5) out.push_back(it->castTo<reco::TrackRef>());
        }
        return out;
    };

    VertexDistanceXY vdist2d;

    // --- Scouting collection handling ---
    struct VertexSummary {
        float x = 0.0f;
        float y = 0.0f;
        float pt = 0.0f;
        float mass = 0.0f;
        float chi2 = 0.0f;
        float dBVerr = 0.0f;
        int nTracks = 0;
        std::vector<reco::TrackRef> tracks;
    };

    struct SelectionResult {
        int nSel = 0;
        std::vector<std::pair<float, float>> selectedXY;
        std::vector<VertexSummary> vertices;
    };

    auto processVertices = [&](const edm::Handle<std::vector<reco::Vertex>> &vH,
                               const reco::Vertex &refVtx,
                               bool useHitCutsForCollection) -> SelectionResult {
        SelectionResult result;
        if (!vH.isValid()) return result;

        // get (optional) track->scouting map handle (used only when hit cuts are enabled for this collection)
        Handle<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>> trackToScoutingRefH_local;
        if (useHitCutsForCollection) {
            iEvent.getByToken(trackToScoutingRefToken_, trackToScoutingRefH_local);
        }
        const auto *trackToScoutingPtr = trackToScoutingRefH_local.isValid() ? &(*trackToScoutingRefH_local) : nullptr;

        // loop vertices
        for (size_t iv = 0; iv < vH->size(); ++iv) {
            const auto &v = vH->at(iv);
            // compose constituent tracks
            std::vector<reco::TrackRef> tks = vertex_track_vec(v);
            const size_t ntk = tks.size();
            if (ntk < 2) continue;

            // opening-angle
            double minAngle = 1e9;
            int npairs = 0;
            for (size_t i=0;i<ntk;++i) {
                if (!tks[i].isNonnull()) continue;
                TVector3 vi(tks[i]->px(), tks[i]->py(), tks[i]->pz());
                if (vi.Mag2() <= 0) continue;
                for (size_t j=i+1;j<ntk;++j) {
                    if (!tks[j].isNonnull()) continue;
                    TVector3 vj(tks[j]->px(), tks[j]->py(), tks[j]->pz());
                    if (vj.Mag2() <= 0) continue;
                    const double angle = vi.Angle(vj);
                    ++npairs;
                    if (angle < minAngle) minAngle = angle;
                }
            }
            if (npairs == 0) continue;
            if (cut_opening_angle_min_ > 0 && minAngle < cut_opening_angle_min_) continue;

            // sum 4-vector & average dxy & dxy error w.r.t ref
            TLorentzVector sumVec(0,0,0,0);
            double sum_dxy = 0.0, sum_dxyErr = 0.0;
            bool track_ip_ok = true;
            bool hitcuts_ok = true;
            int nGoodTracks = 0;

            for (const auto &trRef : tks) {
                if (!trRef.isNonnull()) continue;
                ++nGoodTracks;
                constexpr double kPionMass = 0.13957;
                TLorentzVector tv; tv.SetPtEtaPhiM(trRef->pt(), trRef->eta(), trRef->phi(), kPionMass);
                sumVec += tv;

                // track-level cuts
                if (trRef->pt() < track_pt_min_cut_) { track_ip_ok = false; break; }

                // transient track and IP
                reco::TransientTrack ttrack = ttBuilder.build(trRef);

                // compute IP (2D or 3D)
                std::pair<bool,Measurement1D> ipres;
                if (seed_use2DTrackDist_) {
                    ipres = IPTools::absoluteTransverseImpactParameter(ttrack, refVtx);
                } else {
                    ipres = IPTools::absoluteImpactParameter3D(ttrack, refVtx);
                }
                if (!ipres.first) { track_ip_ok = false; break; }
                sum_dxy += ipres.second.value();
                sum_dxyErr += ipres.second.error();
                const double ipSig = ipres.second.significance();
                if (ipSig < track_dxySig_min_cut_ || ipSig > track_dxySig_max_cut_) { track_ip_ok = false; break; }

                // hit counting (only if scouting map is available)
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
                        // We don't have scouting map: skip hit cut (already warned at top)
                        nPixelHits = 999; nStripHits = 999; nTrackerLayers = 999;
                    }
                    if (nPixelHits < hit_minPixelHits_ || nStripHits < hit_minStripHits_ || nTrackerLayers < hit_minTrackerLayers_) {
                        hitcuts_ok = false; break;
                    }
                }
            } // tracks loop

            if (!track_ip_ok) continue;
            if (!hitcuts_ok) continue;

            // compute inv mass and avg dxy metrics
            const double invMass = sumVec.M();
            const double avg_dxy = (nGoodTracks>0) ? (sum_dxy / double(nGoodTracks)) : 0.0;
            const double avg_dxyErr = (nGoodTracks>0) ? (sum_dxyErr / double(nGoodTracks)) : 0.0;

            // distances
            Measurement1D dBVref_meas = vdist2d.distance(v, refVtx);
            const double dBVref = dBVref_meas.value();
            const double dBV_err = dBVref_meas.error();
            // apply scalar thresholds
            if (required_invmass_ != -1 && invMass < required_invmass_) continue;
            if (required_chi2_ != -1 && v.normalizedChi2() > required_chi2_) continue;
            if (required_dBV_min_ != -1 && dBVref < required_dBV_min_) continue;
            if (required_dBV_max_ != -1 && dBVref > required_dBV_max_) continue;
            if (required_dxy_min_ != -1 && avg_dxy < required_dxy_min_) continue;
            if (required_dxy_max_ != -1 && avg_dxy > required_dxy_max_) continue;
            if (required_dBV_error_ != -1 && dBV_err > required_dBV_error_) continue;
            if (required_dxy_error_ != -1 && avg_dxyErr > required_dxy_error_) continue;

            // vertex passes
            ++result.nSel;
            result.selectedXY.emplace_back(static_cast<float>(v.x()), static_cast<float>(v.y()));

            VertexSummary vtxSummary;
            vtxSummary.x = static_cast<float>(v.x());
            vtxSummary.y = static_cast<float>(v.y());
            vtxSummary.pt = static_cast<float>(sumVec.Pt());
            vtxSummary.mass = static_cast<float>(invMass);
            vtxSummary.chi2 = static_cast<float>(v.normalizedChi2());
            vtxSummary.dBVerr = static_cast<float>(dBV_err);
            vtxSummary.nTracks = nGoodTracks;
            for (const auto &trRef : tks) {
                if (trRef.isNonnull()) vtxSummary.tracks.push_back(trRef);
            }
            result.vertices.push_back(std::move(vtxSummary));
        } // vertex loop

        return result;
    };

    // get scouting vertices
    Handle<std::vector<reco::Vertex>> scoutingVtxH;
    iEvent.getByToken(scoutingVerticesToken_, scoutingVtxH);
    if (!scoutingVtxH.isValid()) {
        edm::LogWarning("ScoutingCountMakerRun3")
            << "Scouting vertex collection is invalid for event "
            << iEvent.id().run() << ":" << iEvent.id().luminosityBlock() << ":" << iEvent.id().event();
    }
    SelectionResult scoutingResult;
    if (haveScoutingRef) scoutingResult = processVertices(scoutingVtxH, scoutingRefVtx, true);
    const int scoutingSelected = scoutingResult.nSel;

    // get offline vertices
    Handle<std::vector<reco::Vertex>> offlineVtxH;
    iEvent.getByToken(offlineVerticesToken_, offlineVtxH);
    if (!offlineVtxH.isValid()) {
        edm::LogWarning("ScoutingCountMakerRun3")
            << "Offline vertex collection is invalid for event "
            << iEvent.id().run() << ":" << iEvent.id().luminosityBlock() << ":" << iEvent.id().event();
    }
    SelectionResult offlineResult;
    if (haveOfflineRef) offlineResult = processVertices(offlineVtxH, offlineRefVtx, false);
    const int offlineSelected = offlineResult.nSel;

    // Write event folders only for events with at least one selected vertex in either collection.
    if (scoutingSelected > 0 || offlineSelected > 0) {
        std::cout << "Selected vertices found in event " << iEvent.id().event()
                  << " (run:lumi:event=" << iEvent.id().run() << ":"
                  << iEvent.id().luminosityBlock() << ":" << iEvent.id().event() << ")"
                  << " scouting=" << scoutingSelected
                  << " offline=" << offlineSelected
                  << std::endl;

        edm::Service<TFileService> fs;
        if (fs.isAvailable()) {
            std::ostringstream dirName;
            dirName << "Event" << iEvent.id().event();
            TFileDirectory eventDir = fs->mkdir(dirName.str().c_str());

            auto fillCollectionFolder = [&](const char *collectionName,
                                            const SelectionResult &result) {
                TFileDirectory collectionDir = eventDir.mkdir(collectionName);

                TFileDirectory verticesDir = collectionDir.mkdir("Vertices");
                TH1F *h_vtx_pt = verticesDir.make<TH1F>("pt", "Vertex p_{T};p_{T} [GeV];entries", 100, 0, 50);
                TH1F *h_vtx_mass = verticesDir.make<TH1F>("mass", "Vertex mass;m [GeV];entries", 100, 0, 10);
                TH1F *h_vtx_chi2 = verticesDir.make<TH1F>("chi2", "Vertex norm chi2;chi2_{norm};entries", 100, 0, 50);
                TH1F *h_vtx_ntrk = verticesDir.make<TH1F>("nTracks", "Vertex nTracks;n_{tracks};entries", 20, 0, 20);
                TH1F *h_vtx_dbverr = verticesDir.make<TH1F>("dBV_error", "Vertex d_{BV} error;#sigma_{dBV} [cm];entries", 200, 0, 0.2);
                TH2F *h_vtx_xy = verticesDir.make<TH2F>("xy", "Vertex XY;x [cm];y [cm]", 400, -10, 10, 400, -10, 10);
                TH2F *h_beamspot_xy = verticesDir.make<TH2F>("beamspot_xy", "Beamspot XY;x_{BS} [cm];y_{BS} [cm]", 400, -1, 1, 400, -1, 1);

                if (haveOfflineBS) {
                    h_beamspot_xy->Fill(offlineBsVtx.x(), offlineBsVtx.y());
                }

                std::vector<reco::TrackRef> allVertexTracks;
                for (const auto &v : result.vertices) {
                    h_vtx_pt->Fill(v.pt);
                    h_vtx_mass->Fill(v.mass);
                    h_vtx_chi2->Fill(v.chi2);
                    h_vtx_ntrk->Fill(v.nTracks);
                    h_vtx_dbverr->Fill(v.dBVerr);
                    h_vtx_xy->Fill(v.x, v.y);
                    allVertexTracks.insert(allVertexTracks.end(), v.tracks.begin(), v.tracks.end());
                }

                TFileDirectory tracksDir = collectionDir.mkdir("Tracks");
                TH1F *h_trk_pt = tracksDir.make<TH1F>("pt", "Track p_{T};p_{T} [GeV];entries", 100, 0, 50);
                TH1F *h_trk_eta = tracksDir.make<TH1F>("eta", "Track #eta;#eta;entries", 100, -3, 3);
                TH1F *h_trk_phi = tracksDir.make<TH1F>("phi", "Track #phi;#phi;entries", 100, -3.2, 3.2);
                TH1F *h_trk_dxy = tracksDir.make<TH1F>("dxy", "Track dxy;dxy [cm];entries", 100, -1.0, 1.0);
                TH1F *h_trk_dxyerr = tracksDir.make<TH1F>("dxy_error", "Track dxy error;#sigma_{dxy} [cm];entries", 200, 0, 0.05);
                TH1F *h_trk_ipsig = tracksDir.make<TH1F>("ipsig", "Track IPsig;|dxy|/err;entries", 100, 0, 50);

                TFileDirectory vertexAssocDir = tracksDir.mkdir("VertexAssociatedTracks");
                for (size_t ivtx = 0; ivtx < result.vertices.size(); ++ivtx) {
                    const auto &v = result.vertices[ivtx];
                    std::ostringstream vertexDirName;
                    vertexDirName << "Vertex" << (ivtx + 1);
                    TFileDirectory oneVertexDir = vertexAssocDir.mkdir(vertexDirName.str().c_str());

                    std::set<unsigned int> seenTrackKeys;
                    int trackCounter = 0;
                    for (const auto &trRef : v.tracks) {
                        if (!trRef.isNonnull()) continue;
                        const unsigned int key = trRef.key();
                        if (seenTrackKeys.find(key) != seenTrackKeys.end()) continue;
                        seenTrackKeys.insert(key);

                        const float pt = static_cast<float>(trRef->pt());
                        const float eta = static_cast<float>(trRef->eta());
                        const float phi = static_cast<float>(trRef->phi());
                        const float dxy = static_cast<float>(trRef->d0());
                        const float dxyErr = static_cast<float>(trRef->d0Error());
                        const float ipSig = (dxyErr > 0.0f) ? std::fabs(dxy / dxyErr) : 0.0f;

                        h_trk_pt->Fill(pt);
                        h_trk_eta->Fill(eta);
                        h_trk_phi->Fill(phi);
                        h_trk_dxy->Fill(dxy);
                        h_trk_dxyerr->Fill(dxyErr);
                        h_trk_ipsig->Fill(ipSig);

                        ++trackCounter;
                        std::ostringstream trackDirName;
                        trackDirName << "Track" << trackCounter;
                        TFileDirectory oneTrackDir = oneVertexDir.mkdir(trackDirName.str().c_str());

                        TH1F *h_one_pt = oneTrackDir.make<TH1F>("pt", "Track p_{T};p_{T} [GeV];entries", 100, 0, 50);
                        TH1F *h_one_eta = oneTrackDir.make<TH1F>("eta", "Track #eta;#eta;entries", 100, -3, 3);
                        TH1F *h_one_phi = oneTrackDir.make<TH1F>("phi", "Track #phi;#phi;entries", 100, -3.2, 3.2);
                        TH1F *h_one_dxy = oneTrackDir.make<TH1F>("dxy", "Track dxy;dxy [cm];entries", 100, -1.0, 1.0);
                        TH1F *h_one_dxyerr = oneTrackDir.make<TH1F>("dxy_error", "Track dxy error;#sigma_{dxy} [cm];entries", 200, 0, 0.05);
                        TH1F *h_one_ipsig = oneTrackDir.make<TH1F>("ipsig", "Track IPsig;|dxy|/err;entries", 100, 0, 50);

                        h_one_pt->Fill(pt);
                        h_one_eta->Fill(eta);
                        h_one_phi->Fill(phi);
                        h_one_dxy->Fill(dxy);
                        h_one_dxyerr->Fill(dxyErr);
                        h_one_ipsig->Fill(ipSig);
                    }
                }
            };

            if (scoutingSelected > 0) {
                fillCollectionFolder("Scouting", scoutingResult);
            }
            if (offlineSelected > 0) {
                fillCollectionFolder("Offline", offlineResult);
            }
        }
    }

    ++nEvents_;
    totalScoutingSelected_ += static_cast<unsigned long long>(scoutingSelected);
    totalOfflineSelected_ += static_cast<unsigned long long>(offlineSelected);

    // Log the per-event counts
    std::ostringstream oss;
    oss << "Event " << iEvent.id().run() << ":" << iEvent.id().luminosityBlock() << ":" << iEvent.id().event()
        << "  scouting_selected_vertices=" << scoutingSelected
        << "  offline_selected_vertices=" << offlineSelected;
    if (verbose_) {
        edm::LogInfo("ScoutingCountMakerRun3") << oss.str();
    }
}

void ScoutingCountMakerRun3::endJob() {
    if (printSummary_) {
        edm::LogInfo("ScoutingCountMakerRun3")
            << "Summary: events=" << nEvents_
            << " total_scouting_selected_vertices=" << totalScoutingSelected_
            << " total_offline_selected_vertices=" << totalOfflineSelected_;
    }
}

// fillDescriptions
void ScoutingCountMakerRun3::fillDescriptions(edm::ConfigurationDescriptions &descriptions) {
    edm::ParameterSetDescription desc;
    desc.add<edm::InputTag>("scoutingVertices", edm::InputTag("Vertexer"));
    desc.add<edm::InputTag>("offlineVertices", edm::InputTag("displacedVertices"));
    desc.add<edm::InputTag>("primaryVertices", edm::InputTag("hltScoutingPrimaryVertexPacker", "primaryVtx"));
    desc.add<edm::InputTag>("beamspot_src", edm::InputTag("offlineBeamSpot"));
    desc.add<edm::InputTag>("tracks", edm::InputTag("hltScoutingUnpackProducer", "Track"));

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

    descriptions.add("ScoutingCountMakerRun3", desc);
}

DEFINE_FWK_MODULE(ScoutingCountMakerRun3);