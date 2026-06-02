
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"

#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "CondFormats/BeamSpotObjects/interface/BeamSpotOnlineObjects.h"
#include "CondFormats/DataRecord/interface/BeamSpotOnlineHLTObjectsRcd.h"

#include "DataFormats/BeamSpot/interface/BeamSpot.h"
#include "DataFormats/Candidate/interface/Candidate.h"
#include "DataFormats/Common/interface/ValueMap.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/Math/interface/Vector3D.h"
#include "DataFormats/Scouting/interface/Run3ScoutingPFJet.h"
#include "DataFormats/Scouting/interface/Run3ScoutingTrack.h"
#include "DataFormats/TrackReco/interface/HitPattern.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/VertexReco/interface/Vertex.h"

#include "TH1F.h"
#include "TH2F.h"
#include "TH3F.h"

class GenScoutingComparer : public edm::one::EDAnalyzer<edm::one::SharedResources> {
private:
    using LorentzVector = reco::Candidate::LorentzVector;

    static constexpr double kPionMass = 0.13957039;

    enum class MatchDistanceMode { k2D, k3D };

    struct SelectedVertexInfo {
        const reco::Vertex* vertex = nullptr;
        LorentzVector p4{0., 0., 0., 0.};
        math::XYZVector p3{0., 0., 0.};
        unsigned int rawTracks = 0;
        unsigned int selectedTracks = 0;
        unsigned int rawTrackRefs = 0;
        double chi2 = 0.0;
        double dBV = 0.0;
        double dBVErr = 0.0;
        double significance = 0.0;
        double rDistxyGlobal00 = 0.0;
        double rDistxyzGlobal00 = 0.0;
        double rDistxyBeamSpot = 0.0;
        double rDistxyzBeamSpot = 0.0;
        double cosT = -999.0;
    };

    struct MatchSolution {
        std::array<int, 2> recoForGen{{-1, -1}};
        std::array<double, 2> distance{{-1.0, -1.0}};
        double totalScore = std::numeric_limits<double>::max();
    };

    struct BucketPlots {
        TH2F* xyDiff = nullptr;
        TH3F* xyzDiff = nullptr;

        TH2F* xyGlobal00Gen = nullptr;
        TH2F* xyGlobal00Reco = nullptr;

        TH3F* xyzGlobal00Gen = nullptr;
        TH3F* xyzGlobal00Reco = nullptr;

        TH2F* xyBeamSpotGen = nullptr;
        TH2F* xyBeamSpotReco = nullptr;

        TH3F* xyzBeamSpotGen = nullptr;
        TH3F* xyzBeamSpotReco = nullptr;

        TH1F* rDistxyGlobal00Gen = nullptr;
        TH1F* rDistxyGlobal00Reco = nullptr;
        TH1F* rDistxyGlobal00Diff = nullptr;

        TH1F* rDistxyzGlobal00Gen = nullptr;
        TH1F* rDistxyzGlobal00Reco = nullptr;
        TH1F* rDistxyzGlobal00Diff = nullptr;

        TH1F* rDistxyBeamSpotGen = nullptr;
        TH1F* rDistxyBeamSpotReco = nullptr;
        TH1F* rDistxyBeamSpotDiff = nullptr;

        TH1F* rDistxyzBeamSpotGen = nullptr;
        TH1F* rDistxyzBeamSpotReco = nullptr;
        TH1F* rDistxyzBeamSpotDiff = nullptr;

        TH1F* ptGen = nullptr;
        TH1F* ptReco = nullptr;
        TH1F* ptDiff = nullptr;

        TH1F* etaGen = nullptr;
        TH1F* etaReco = nullptr;
        TH1F* etaDiff = nullptr;

        TH1F* phiGen = nullptr;
        TH1F* phiReco = nullptr;
        TH1F* phiDiff = nullptr;

        TH1F* massGen = nullptr;
        TH1F* massReco = nullptr;
        TH1F* massDiff = nullptr;

        TH1F* chi2Reco = nullptr;
        TH1F* significanceReco = nullptr;
        TH1F* dxyErrorReco = nullptr;

        TH1F* deltaR = nullptr;
        TH1F* matchDistance = nullptr;

        void book(edm::Service<TFileService>& fs,
                  const std::string& suffix,
                  const std::string& modeLabel,
                  double massHint,
                  double decayHint) {
            const auto makeName = [&](const std::string& base) {
                return base + "_" + suffix;
            };
            const double xyPositionMax = 5.0 * decayHint;
            const double xyzPositionMax = 100.0 * decayHint;
            const double xyDiffMax = 1.0 * decayHint;
            const double xyzDiffMax = 1.0 * decayHint;
            const double rDistxyMax = 10.0 * decayHint;
            const double rDistxyzMax = 100.0 * decayHint;
            const double massMax = 3.0 * massHint;
            const double massDiffMax = massHint;
            const double ptDiffMax = 1.5 * massHint;

            xyDiff = fs->make<TH2F>(makeName("xy_diff").c_str(),
                                    ";reco - gen x [cm];reco - gen y [cm];Events",
                                    220, -xyDiffMax, xyDiffMax, 220, -xyDiffMax, xyDiffMax);
            xyzDiff = fs->make<TH3F>(makeName("xyz_diff").c_str(),
                                     ";reco - gen x [cm];reco - gen y [cm];reco - gen z [cm];Events",
                                     160, -xyzDiffMax, xyzDiffMax, 160, -xyzDiffMax, xyzDiffMax, 160, -xyzDiffMax, xyzDiffMax);

            xyGlobal00Gen = fs->make<TH2F>(makeName("xy_position_global00_gen").c_str(), ";x gen wrt (0,0) [cm];y gen wrt (0,0) [cm];Events", 220, -xyPositionMax, xyPositionMax, 220, -xyPositionMax, xyPositionMax);
            xyGlobal00Reco = fs->make<TH2F>(makeName("xy_position_global00_reco").c_str(), ";x reco wrt (0,0) [cm];y reco wrt (0,0) [cm];Events", 220, -xyPositionMax, xyPositionMax, 220, -xyPositionMax, xyPositionMax);

            xyzGlobal00Gen = fs->make<TH3F>(makeName("xyz_position_global00_gen").c_str(), ";x gen wrt (0,0,0) [cm];y gen wrt (0,0,0) [cm];z gen wrt (0,0,0) [cm];Events", 160, -xyzPositionMax, xyzPositionMax, 160, -xyzPositionMax, xyzPositionMax, 160, -xyzPositionMax, xyzPositionMax);
            xyzGlobal00Reco = fs->make<TH3F>(makeName("xyz_position_global00_reco").c_str(), ";x reco wrt (0,0,0) [cm];y reco wrt (0,0,0) [cm];z reco wrt (0,0,0) [cm];Events", 160, -xyzPositionMax, xyzPositionMax, 160, -xyzPositionMax, xyzPositionMax, 160, -xyzPositionMax, xyzPositionMax);

            xyBeamSpotGen = fs->make<TH2F>(makeName("xy_position_beamspot_gen").c_str(), ";x gen wrt beamspot [cm];y gen wrt beamspot [cm];Events", 220, -xyPositionMax, xyPositionMax, 220, -xyPositionMax, xyPositionMax);
            xyBeamSpotReco = fs->make<TH2F>(makeName("xy_position_beamspot_reco").c_str(), ";x reco wrt beamspot [cm];y reco wrt beamspot [cm];Events", 220, -xyPositionMax, xyPositionMax, 220, -xyPositionMax, xyPositionMax);

            xyzBeamSpotGen = fs->make<TH3F>(makeName("xyz_position_beamspot_gen").c_str(), ";x gen wrt beamspot [cm];y gen wrt beamspot [cm];z gen wrt beamspot [cm];Events", 160, -xyzPositionMax, xyzPositionMax, 160, -xyzPositionMax, xyzPositionMax, 160, -xyzPositionMax, xyzPositionMax);
            xyzBeamSpotReco = fs->make<TH3F>(makeName("xyz_position_beamspot_reco").c_str(), ";x reco wrt beamspot [cm];y reco wrt beamspot [cm];z reco wrt beamspot [cm];Events", 160, -xyzPositionMax, xyzPositionMax, 160, -xyzPositionMax, xyzPositionMax, 160, -xyzPositionMax, xyzPositionMax);

            rDistxyGlobal00Gen = fs->make<TH1F>(makeName("rdist_xy_global00_gen").c_str(), ";|r_{xy}| wrt (0,0) gen [cm];Events", 160, 0.0, rDistxyMax);
            rDistxyGlobal00Reco = fs->make<TH1F>(makeName("rdist_xy_global00_reco").c_str(), ";|r_{xy}| wrt (0,0) reco [cm];Events", 160, 0.0, rDistxyMax);
            rDistxyGlobal00Diff = fs->make<TH1F>(makeName("rdist_xy_global00_diff").c_str(), ";reco - gen |r_{xy}| wrt (0,0) [cm];Events", 160, -rDistxyMax, rDistxyMax);

            rDistxyzGlobal00Gen = fs->make<TH1F>(makeName("rdist_xyz_global00_gen").c_str(), ";|r| wrt (0,0,0) gen [cm];Events", 160, 0.0, rDistxyzMax);
            rDistxyzGlobal00Reco = fs->make<TH1F>(makeName("rdist_xyz_global00_reco").c_str(), ";|r| wrt (0,0,0) reco [cm];Events", 160, 0.0, rDistxyzMax);
            rDistxyzGlobal00Diff = fs->make<TH1F>(makeName("rdist_xyz_global00_diff").c_str(), ";reco - gen |r| wrt (0,0,0) [cm];Events", 160, -rDistxyzMax, rDistxyzMax);

            rDistxyBeamSpotGen = fs->make<TH1F>(makeName("rdist_xy_beamspot_gen").c_str(), ";|r_{xy}| wrt beamspot gen [cm];Events", 160, 0.0, rDistxyMax);
            rDistxyBeamSpotReco = fs->make<TH1F>(makeName("rdist_xy_beamspot_reco").c_str(), ";|r_{xy}| wrt beamspot reco [cm];Events", 160, 0.0, rDistxyMax);
            rDistxyBeamSpotDiff = fs->make<TH1F>(makeName("rdist_xy_beamspot_diff").c_str(), ";reco - gen |r_{xy}| wrt beamspot [cm];Events", 160, -rDistxyMax, rDistxyMax);

            rDistxyzBeamSpotGen = fs->make<TH1F>(makeName("rdist_xyz_beamspot_gen").c_str(), ";|r| wrt beamspot gen [cm];Events", 160, 0.0, rDistxyzMax);
            rDistxyzBeamSpotReco = fs->make<TH1F>(makeName("rdist_xyz_beamspot_reco").c_str(), ";|r| wrt beamspot reco [cm];Events", 160, 0.0, rDistxyzMax);
            rDistxyzBeamSpotDiff = fs->make<TH1F>(makeName("rdist_xyz_beamspot_diff").c_str(), ";reco - gen |r| wrt beamspot [cm];Events", 160, -rDistxyzMax, rDistxyzMax);

            ptGen = fs->make<TH1F>(makeName("pt_gen").c_str(), ";p_{T} gen [GeV];Events", 180, 0.0, massMax);
            ptReco = fs->make<TH1F>(makeName("pt_reco").c_str(), ";p_{T} reco [GeV];Events", 180, 0.0, massMax);
            ptDiff = fs->make<TH1F>(makeName("pt_diff").c_str(), ";reco - gen p_{T} [GeV];Events", 180, -ptDiffMax, ptDiffMax);

            etaGen = fs->make<TH1F>(makeName("eta_gen").c_str(), ";#eta gen;Events", 120, -6.0, 6.0);
            etaReco = fs->make<TH1F>(makeName("eta_reco").c_str(), ";#eta reco;Events", 120, -6.0, 6.0);
            etaDiff = fs->make<TH1F>(makeName("eta_diff").c_str(), ";reco - gen #eta;Events", 120, -6.0, 6.0);

            phiGen = fs->make<TH1F>(makeName("phi_gen").c_str(), ";#phi gen;Events", 128, -3.2, 3.2);
            phiReco = fs->make<TH1F>(makeName("phi_reco").c_str(), ";#phi reco;Events", 128, -3.2, 3.2);
            phiDiff = fs->make<TH1F>(makeName("phi_diff").c_str(), ";reco - gen #phi;Events", 128, -3.2, 3.2);

            massGen = fs->make<TH1F>(makeName("mass_gen").c_str(), ";mass gen [GeV];Events", 180, 0.0, massMax);
            massReco = fs->make<TH1F>(makeName("mass_reco").c_str(), ";mass reco [GeV];Events", 180, 0.0, massMax);
            massDiff = fs->make<TH1F>(makeName("mass_diff").c_str(), ";reco - gen mass [GeV];Events", 180, -massDiffMax, massDiffMax);

            chi2Reco = fs->make<TH1F>(makeName("chi2_reco").c_str(), ";reduced #chi^{2} reco;Events", 120, 0.0, 10.0);
            significanceReco = fs->make<TH1F>(makeName("significance_reco").c_str(), ";d_{BV}/#sigma(d_{BV}) reco;Events", 200, 0.0, 200.0);
            dxyErrorReco = fs->make<TH1F>(makeName("dxy_error_reco").c_str(), ";#sigma(d_{BV}) reco [cm];Events", 200, 0.0, 0.02);

            deltaR = fs->make<TH1F>(makeName("deltaR").c_str(), ";#DeltaR(reco, gen);Events", 140, 0.0, 5.0);
            matchDistance = fs->make<TH1F>(makeName("match_distance").c_str(),
                                           (std::string(";pairing metric for ") + modeLabel + " match [cm];Events").c_str(),
                                           140, 0.0, 0.1);
        }
    };

    struct CounterSet {
        uint64_t events = 0;
        uint64_t truthEvents = 0;
        uint64_t eventsWithMatches = 0;
        uint64_t selectedVertices = 0;
    };

    edm::EDGetTokenT<std::vector<reco::GenParticle>> genParticlesToken_;
    edm::EDGetTokenT<std::vector<reco::Vertex>> scoutingVerticesToken_;
    edm::EDGetTokenT<reco::BeamSpot> offlineBeamspotToken_;
    edm::EDGetTokenT<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>> trackToScoutingMapToken_;
    edm::EDGetTokenT<std::vector<Run3ScoutingPFJet>> scoutingJetsToken_;
    edm::ESGetToken<BeamSpotOnlineObjects, BeamSpotOnlineHLTObjectsRcd> bsOnlineToken_;

    bool useOnlineBeamSpot_ = false;

    double cut_vtx_chi2_max_ = 3.0;
    double cut_vtx_dbv_min_ = 0.01;
    double cut_vtx_dbv_max_ = 2.0;
    unsigned int cut_vtx_tracks_min_ = 8;
    double cut_vtx_ddbv_max_ = 0.005;
    double cut_vtx_cosT_min_ = 0.0;

    int cut_track_pixelHits_min_ = 2;
    int cut_track_stripHits_min_ = 1;
    int cut_track_trackerLayers_min_ = 5;

    bool verbose_ = false;
    bool verbose_unselected_ = false;
    bool verbose_selected_only_ = false;
    bool ntracks_raw_ = false;
    bool cost_raw_ = false;

    double jet_pt_min_ = 30.0;
    double jet_eta_max_ = 2.5;
    unsigned int min_selected_jets_ = 3;
    bool require_jet_selection_ = true;

    MatchDistanceMode matchMode_ = MatchDistanceMode::k2D;
    std::string matchModeLabel_ = "2D";
    double massHint_ = 200.0;
    double decayHint_ = 0.1;

    std::array<BucketPlots, 2> plots_;
    CounterSet counters_;

    static double wrapDeltaPhi(double a, double b) {
        constexpr double kPi = 3.14159265358979323846;
        double d = a - b;
        while (d > kPi) d -= 2.0 * kPi;
        while (d <= -kPi) d += 2.0 * kPi;
        return d;
    }

    static LorentzVector trackP4(const reco::Track& trk) {
        const double p = trk.p();
        const double e = std::sqrt(p * p + kPionMass * kPionMass);
        return LorentzVector(trk.px(), trk.py(), trk.pz(), e);
    }

    static double deltaR(const LorentzVector& a, const LorentzVector& b) {
        const double dEta = a.eta() - b.eta();
        const double dPhi = wrapDeltaPhi(a.phi(), b.phi());
        return std::hypot(dEta, dPhi);
    }

    static double dist2D(double x1, double y1, double x2, double y2) {
        return std::hypot(x1 - x2, y1 - y2);
    }

    static double dist3D(double x1, double y1, double z1, double x2, double y2, double z2) {
        const double dx = x1 - x2;
        const double dy = y1 - y2;
        const double dz = z1 - z2;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    static double vertexDistxy(double x, double y) {
        return std::hypot(x, y);
    }

    static double vertexDistxyz(double x, double y, double z) {
        return std::sqrt(x * x + y * y + z * z);
    }

    double matchDistance(const reco::Vertex& v, const reco::GenParticle& g) const {
        if (matchMode_ == MatchDistanceMode::k3D) {
            return dist3D(v.x(), v.y(), v.z(), g.vx(), g.vy(), g.vz());
        }
        return dist2D(v.x(), v.y(), g.vx(), g.vy());
    }

    double matchDistanceValue(const reco::Vertex& v, const reco::GenParticle& g) const {
        return matchDistance(v, g);
    }

    LorentzVector makeVertexP4(const SelectedVertexInfo& info) const {
        return info.p4;
    }

    std::array<const reco::GenParticle*, 2> findStops(const std::vector<reco::GenParticle>& genParticles, std::ostringstream* log) const {
        std::array<const reco::GenParticle*, 2> stops{{nullptr, nullptr}};
        for (const auto& p : genParticles) {
            if (std::abs(p.pdgId()) != 1000006) continue;
            int downCount = 0;
            for (size_t d = 0; d < p.numberOfDaughters(); ++d) {
                const reco::Candidate* dau = p.daughter(d);
                if (dau && std::abs(dau->pdgId()) == 1) ++downCount;
            }
            if (downCount != 2) continue;
            if (!stops[0]) stops[0] = &p;
            else if (!stops[1]) {
                stops[1] = &p;
                break;
            }
        }
        if (log && stops[0] && stops[1]) {
            *log << "========== 2 STOPs found ==========\n";
            *log << "Stop1 -> x=" << stops[0]->vx() << " y=" << stops[0]->vy() << " z=" << stops[0]->vz()
                 << " pt=" << stops[0]->pt() << " mass=" << stops[0]->mass()
                 << " eta=" << stops[0]->eta() << " phi=" << stops[0]->phi()
                 << " | vxy: " << std::hypot(stops[0]->vx(), stops[0]->vy()) << "\n";
            *log << "Stop2 -> x=" << stops[1]->vx() << " y=" << stops[1]->vy() << " z=" << stops[1]->vz()
                 << " pt=" << stops[1]->pt() << " mass=" << stops[1]->mass()
                 << " eta=" << stops[1]->eta() << " phi=" << stops[1]->phi()
                 << " | vxy: " << std::hypot(stops[1]->vx(), stops[1]->vy()) << "\n";
        }
        return stops;
    }

    std::vector<SelectedVertexInfo> buildSelectedVertices(const edm::Handle<std::vector<reco::Vertex>>& scoutingVertices,
                                                          const reco::BeamSpot* beamspot,
                                                          const edm::Handle<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>>& trackToScoutingMap,
                                                          bool haveTrackToScoutingMap,
                                                          bool eventPassesJetGate,
                                                          std::ostringstream* log) const {
        std::vector<SelectedVertexInfo> selected;
        if (!scoutingVertices.isValid()) return selected;
        selected.reserve(scoutingVertices->size());

        const double bsX = beamspot ? beamspot->x0() : 0.0;
        const double bsY = beamspot ? beamspot->y0() : 0.0;
        const double bsZ = beamspot ? beamspot->z0() : 0.0;

        for (size_t i = 0; i < scoutingVertices->size(); ++i) {
            const auto& v = scoutingVertices->at(i);
            SelectedVertexInfo info;
            info.vertex = &v;
            info.rawTracks = v.tracksSize();
            info.chi2 = v.normalizedChi2();
            info.dBV = std::hypot(v.x() - bsX, v.y() - bsY);
            info.dBVErr = std::hypot(v.xError(), v.yError());
            info.rDistxyGlobal00 = vertexDistxy(v.x(), v.y());
            info.rDistxyzGlobal00 = vertexDistxyz(v.x(), v.y(), v.z());
            info.rDistxyBeamSpot = std::hypot(v.x() - bsX, v.y() - bsY);
            info.rDistxyzBeamSpot = dist3D(v.x(), v.y(), v.z(), bsX, bsY, bsZ);

            math::XYZVector rawPSum(0., 0., 0.);
            math::XYZVector selectedPSum(0., 0., 0.);
            double selectedEnergySum = 0.0;
            unsigned int trackIndex = 0;

            std::ostringstream trackLog;
            for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it, ++trackIndex) {
                reco::TrackRef trk = it->castTo<reco::TrackRef>();
                if (trk.isNonnull()) {
                    ++info.rawTrackRefs;
                    rawPSum += trk->momentum();

                    int nPixelHits = 0;
                    int nStripHits = 0;
                    int nTrackerLayers = 0;
                    if (haveTrackToScoutingMap && trackToScoutingMap.isValid()) {
                        const auto scoutingRef = (*trackToScoutingMap)[trk];
                        if (scoutingRef.isNonnull()) {
                            nPixelHits = scoutingRef->tk_nValidPixelHits();
                            nStripHits = scoutingRef->tk_nValidStripHits();
                            nTrackerLayers = scoutingRef->tk_nTrackerLayersWithMeasurement();
                        }
                    }

                    const bool passPixelHits = (nPixelHits >= cut_track_pixelHits_min_);
                    const bool passStripHits = (nStripHits >= cut_track_stripHits_min_);
                    const bool passTrackerLayers = (nTrackerLayers >= cut_track_trackerLayers_min_);
                    const bool passTrack = passPixelHits && passStripHits && passTrackerLayers;

                    if (passTrack) {
                        ++info.selectedTracks;
                        selectedPSum += trk->momentum();
                        selectedEnergySum += trackP4(*trk).E();
                    }

                    if (verbose_unselected_) {
                        trackLog << "track " << trackIndex << ": pT=" << trk->pt()
                                 << " | pixelHits=" << nPixelHits
                                 << " | stripHits=" << nStripHits
                                 << " | trackerLayers=" << nTrackerLayers
                                 << " -> " << (passTrack ? "SELECTED" : "REJECTED") << "\n";
                    }
                }
            }

            const math::XYZVector disp(v.x() - bsX, v.y() - bsY, v.z() - bsZ);
            const math::XYZVector& pSum = cost_raw_ ? rawPSum : selectedPSum;
            if (disp.R() > 0.0 && pSum.R() > 0.0) {
                info.cosT = disp.Dot(pSum) / (disp.R() * pSum.R());
            }

            const unsigned int nTracksForCut = ntracks_raw_ ? info.rawTracks : info.selectedTracks;
            const bool passChi2 = info.chi2 < cut_vtx_chi2_max_;
            const bool passDbvMin = info.dBV >= cut_vtx_dbv_min_;
            const bool passDbvMax = info.dBV < cut_vtx_dbv_max_;
            const bool passDdbv = info.dBVErr < cut_vtx_ddbv_max_;
            const bool passCosT = info.cosT > cut_vtx_cosT_min_;
            const bool passNtk = nTracksForCut >= cut_vtx_tracks_min_;
            const bool passAll = passChi2 && passDbvMin && passDbvMax && passDdbv && passCosT && passNtk && eventPassesJetGate;

            if (verbose_unselected_) {
                *log << "Vertex " << i << ":\n"
                     << "  chi2/dof : " << info.chi2 << (passChi2 ? " (< " : " (!< ") << cut_vtx_chi2_max_ << ")\n"
                     << "  dBV      : " << info.dBV << (passDbvMin ? " (>= " : " (!>= ") << cut_vtx_dbv_min_ << ") and "
                     << (passDbvMax ? "(< " : "(!< ") << cut_vtx_dbv_max_ << "\n"
                     << "  ddBV     : " << info.dBVErr << (passDdbv ? " (< " : " (!< ") << cut_vtx_ddbv_max_ << ")\n"
                     << "  cosT     : " << info.cosT << (passCosT ? " (> " : " (!> ") << cut_vtx_cosT_min_ << ") ["
                     << (cost_raw_ ? "raw" : "selected") << " track sum]\n"
                     << "  nTracks  : " << nTracksForCut << (passNtk ? " (>= " : " (!>= ") << cut_vtx_tracks_min_ << ") [raw="
                     << info.rawTracks << ", selected=" << info.selectedTracks << ", rawRefs=" << info.rawTrackRefs << "]\n";
                *log << trackLog.str();
                *log << "  -> STATUS: " << (passAll ? "PASSED ALL CUTS" : "FAILED") << "\n";
            }

            if (passAll) {
                info.p3 = selectedPSum;
                const LorentzVector p4(selectedPSum.x(), selectedPSum.y(), selectedPSum.z(), selectedEnergySum);
                info.p4 = p4;
                selected.push_back(info);
            }
        }

        return selected;
    }

    MatchSolution matchVertices(const std::vector<SelectedVertexInfo>& recoVertices,
                                const std::array<const reco::GenParticle*, 2>& genStops) const {
        MatchSolution out;
        if (!genStops[0] || !genStops[1] || recoVertices.empty()) return out;

        const auto dist = [&](const SelectedVertexInfo& v, const reco::GenParticle& g) {
            return matchDistanceValue(*v.vertex, g);
        };

        if (recoVertices.size() == 1) {
            const double d0 = dist(recoVertices[0], *genStops[0]);
            const double d1 = dist(recoVertices[0], *genStops[1]);
            if (d0 <= d1) {
                out.recoForGen[0] = 0;
                out.distance[0] = d0;
                out.totalScore = d0;
            } else {
                out.recoForGen[1] = 0;
                out.distance[1] = d1;
                out.totalScore = d1;
            }
            return out;
        }

        for (size_t i = 0; i < recoVertices.size(); ++i) {
            for (size_t j = i + 1; j < recoVertices.size(); ++j) {
                const double d00 = dist(recoVertices[i], *genStops[0]);
                const double d01 = dist(recoVertices[i], *genStops[1]);
                const double d10 = dist(recoVertices[j], *genStops[0]);
                const double d11 = dist(recoVertices[j], *genStops[1]);

                const double scoreA = d00 + d11;
                if (scoreA < out.totalScore) {
                    out.totalScore = scoreA;
                    out.recoForGen[0] = static_cast<int>(i);
                    out.recoForGen[1] = static_cast<int>(j);
                    out.distance[0] = d00;
                    out.distance[1] = d11;
                }

                const double scoreB = d10 + d01;
                if (scoreB < out.totalScore) {
                    out.totalScore = scoreB;
                    out.recoForGen[0] = static_cast<int>(j);
                    out.recoForGen[1] = static_cast<int>(i);
                    out.distance[0] = d10;
                    out.distance[1] = d01;
                }
            }
        }

        return out;
    }

    void fillBucket(size_t bucketIndex,
                    const SelectedVertexInfo& reco,
                    const reco::GenParticle& gen,
                    const reco::BeamSpot* beamspot,
                    double matchMetric) const {
        if (bucketIndex >= plots_.size()) return;
        const auto& h = plots_[bucketIndex];
        const auto& v = *reco.vertex;

        const double dx = v.x() - gen.vx();
        const double dy = v.y() - gen.vy();
        const double dz = v.z() - gen.vz();

        const LorentzVector genP4(gen.px(), gen.py(), gen.pz(), gen.energy());
        const LorentzVector recoP4 = reco.p4;

        const double recoXGlobal00 = v.x();
        const double recoYGlobal00 = v.y();
        const double recoZGlobal00 = v.z();
        const double genXGlobal00 = gen.vx();
        const double genYGlobal00 = gen.vy();
        const double genZGlobal00 = gen.vz();

        const double bsX = beamspot ? beamspot->x0() : 0.0;
        const double bsY = beamspot ? beamspot->y0() : 0.0;
        const double bsZ = beamspot ? beamspot->z0() : 0.0;
        const double recoXBeamSpot = v.x() - bsX;
        const double recoYBeamSpot = v.y() - bsY;
        const double recoZBeamSpot = v.z() - bsZ;
        const double genXBeamSpot = gen.vx() - bsX;
        const double genYBeamSpot = gen.vy() - bsY;
        const double genZBeamSpot = gen.vz() - bsZ;

        const double recoDistxy00 = reco.rDistxyGlobal00;
        const double genDistxy00 = vertexDistxy(gen.vx(), gen.vy());
        const double recoDistxyz00 = reco.rDistxyzGlobal00;
        const double genDistxyz00 = vertexDistxyz(gen.vx(), gen.vy(), gen.vz());

        const double recoDistxyBS = reco.rDistxyBeamSpot;
        const double genDistxyBS = std::hypot(gen.vx() - bsX, gen.vy() - bsY);
        const double recoDistxyzBS = reco.rDistxyzBeamSpot;
        const double genDistxyzBS = dist3D(gen.vx(), gen.vy(), gen.vz(), bsX, bsY, bsZ);

        const double ptGen = genP4.pt();
        const double ptReco = recoP4.pt();
        const double etaGen = genP4.eta();
        const double etaReco = recoP4.eta();
        const double phiGen = genP4.phi();
        const double phiReco = recoP4.phi();
        const double massGen = genP4.mass();
        const double massReco = recoP4.mass();
        const double dR = deltaR(recoP4, genP4);

        if (h.xyDiff) h.xyDiff->Fill(dx, dy);
        if (h.xyzDiff) h.xyzDiff->Fill(dx, dy, dz);

        if (h.xyGlobal00Gen) h.xyGlobal00Gen->Fill(genXGlobal00, genYGlobal00);
        if (h.xyGlobal00Reco) h.xyGlobal00Reco->Fill(recoXGlobal00, recoYGlobal00);
        if (h.xyzGlobal00Gen) h.xyzGlobal00Gen->Fill(genXGlobal00, genYGlobal00, genZGlobal00);
        if (h.xyzGlobal00Reco) h.xyzGlobal00Reco->Fill(recoXGlobal00, recoYGlobal00, recoZGlobal00);

        if (h.xyBeamSpotGen) h.xyBeamSpotGen->Fill(genXBeamSpot, genYBeamSpot);
        if (h.xyBeamSpotReco) h.xyBeamSpotReco->Fill(recoXBeamSpot, recoYBeamSpot);

        if (h.xyzBeamSpotGen) h.xyzBeamSpotGen->Fill(genXBeamSpot, genYBeamSpot, genZBeamSpot);
        if (h.xyzBeamSpotReco) h.xyzBeamSpotReco->Fill(recoXBeamSpot, recoYBeamSpot, recoZBeamSpot);

        if (h.rDistxyGlobal00Gen) h.rDistxyGlobal00Gen->Fill(genDistxy00);
        if (h.rDistxyGlobal00Reco) h.rDistxyGlobal00Reco->Fill(recoDistxy00);
        if (h.rDistxyGlobal00Diff) h.rDistxyGlobal00Diff->Fill(recoDistxy00 - genDistxy00);

        if (h.rDistxyzGlobal00Gen) h.rDistxyzGlobal00Gen->Fill(genDistxyz00);
        if (h.rDistxyzGlobal00Reco) h.rDistxyzGlobal00Reco->Fill(recoDistxyz00);
        if (h.rDistxyzGlobal00Diff) h.rDistxyzGlobal00Diff->Fill(recoDistxyz00 - genDistxyz00);

        if (h.rDistxyBeamSpotGen) h.rDistxyBeamSpotGen->Fill(genDistxyBS);
        if (h.rDistxyBeamSpotReco) h.rDistxyBeamSpotReco->Fill(recoDistxyBS);
        if (h.rDistxyBeamSpotDiff) h.rDistxyBeamSpotDiff->Fill(recoDistxyBS - genDistxyBS);

        if (h.rDistxyzBeamSpotGen) h.rDistxyzBeamSpotGen->Fill(genDistxyzBS);
        if (h.rDistxyzBeamSpotReco) h.rDistxyzBeamSpotReco->Fill(recoDistxyzBS);
        if (h.rDistxyzBeamSpotDiff) h.rDistxyzBeamSpotDiff->Fill(recoDistxyzBS - genDistxyzBS);

        if (h.ptGen) h.ptGen->Fill(ptGen);
        if (h.ptReco) h.ptReco->Fill(ptReco);
        if (h.ptDiff) h.ptDiff->Fill(ptReco - ptGen);

        if (h.etaGen) h.etaGen->Fill(etaGen);
        if (h.etaReco) h.etaReco->Fill(etaReco);
        if (h.etaDiff) h.etaDiff->Fill(etaReco - etaGen);

        if (h.phiGen) h.phiGen->Fill(phiGen);
        if (h.phiReco) h.phiReco->Fill(phiReco);
        if (h.phiDiff) h.phiDiff->Fill(wrapDeltaPhi(phiReco, phiGen));

        if (h.massGen) h.massGen->Fill(massGen);
        if (h.massReco) h.massReco->Fill(massReco);
        if (h.massDiff) h.massDiff->Fill(massReco - massGen);

        if (h.chi2Reco) h.chi2Reco->Fill(reco.chi2);
        if (h.significanceReco) h.significanceReco->Fill((reco.dBVErr > 0.0) ? (reco.dBV / reco.dBVErr) : -1.0);
        if (h.dxyErrorReco) h.dxyErrorReco->Fill(reco.dBVErr);

        if (h.deltaR) h.deltaR->Fill(dR);
        if (h.matchDistance) h.matchDistance->Fill(matchMetric);
    }

    void fillCounters(const std::vector<SelectedVertexInfo>& recoVertices,
                      const std::array<const reco::GenParticle*, 2>& genStops,
                      const MatchSolution& match) const {
        (void)recoVertices;
        (void)genStops;
        (void)match;
    }

public:
    explicit GenScoutingComparer(const edm::ParameterSet& config) {
        usesResource("TFileService");

        genParticlesToken_ = consumes<std::vector<reco::GenParticle>>(config.getParameter<edm::InputTag>("genParticles"));
        scoutingVerticesToken_ = consumes<std::vector<reco::Vertex>>(config.getParameter<edm::InputTag>("scoutingVertices"));
        offlineBeamspotToken_ = consumes<reco::BeamSpot>(config.getParameter<edm::InputTag>("beamspot"));
        trackToScoutingMapToken_ = consumes<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>>(config.getParameter<edm::InputTag>("trackToScoutingMap"));
        scoutingJetsToken_ = consumes<std::vector<Run3ScoutingPFJet>>(config.getParameter<edm::InputTag>("scoutingJets"));
        bsOnlineToken_ = esConsumes<BeamSpotOnlineObjects, BeamSpotOnlineHLTObjectsRcd>();

        cut_vtx_chi2_max_ = config.getParameter<double>("vtx_chi2_max");
        cut_vtx_dbv_min_ = config.getParameter<double>("vtx_dbv_min");
        cut_vtx_dbv_max_ = config.getParameter<double>("vtx_dbv_max");
        cut_vtx_tracks_min_ = config.getParameter<unsigned int>("vtx_tracks_min");
        cut_vtx_ddbv_max_ = config.getParameter<double>("vtx_ddbv_max");
        cut_vtx_cosT_min_ = config.getParameter<double>("vtx_cosT_min");

        cut_track_pixelHits_min_ = config.getParameter<int>("track_pixelHits_min");
        cut_track_stripHits_min_ = config.getParameter<int>("track_stripHits_min");
        cut_track_trackerLayers_min_ = config.getParameter<int>("track_trackerLayers_min");

        verbose_ = config.getUntrackedParameter<bool>("verbose", false);
        verbose_unselected_ = config.getUntrackedParameter<bool>("verbose_unselected", false);
        verbose_selected_only_ = config.getUntrackedParameter<bool>("verbose_selected_only", false);
        ntracks_raw_ = config.getUntrackedParameter<bool>("ntracks_raw", false);
        cost_raw_ = config.getUntrackedParameter<bool>("cost_raw", false);

        jet_pt_min_ = config.getParameter<double>("jet_pt_min");
        jet_eta_max_ = config.getParameter<double>("jet_eta_max");
        min_selected_jets_ = config.getParameter<unsigned int>("min_selected_jets");
        require_jet_selection_ = config.getUntrackedParameter<bool>("require_jet_selection", true);
        massHint_ = config.getUntrackedParameter<double>("masshint", 200.0);
        decayHint_ = config.getUntrackedParameter<double>("decayhint", 0.1);
        useOnlineBeamSpot_ = config.getUntrackedParameter<bool>("useOnlineBeamSpot", false);

        matchModeLabel_ = config.getUntrackedParameter<std::string>("match_distance_mode", "2D");
        if (matchModeLabel_ == "3D" || matchModeLabel_ == "3d") {
            matchMode_ = MatchDistanceMode::k3D;
            matchModeLabel_ = "3D";
        } else {
            matchMode_ = MatchDistanceMode::k2D;
            matchModeLabel_ = "2D";
        }
    }

    void beginJob() override {
        edm::Service<TFileService> fs;
        if (!fs) {
            edm::LogWarning("GenScoutingComparer") << "TFileService missing";
            return;
        }

        plots_[0].book(fs, "1", matchModeLabel_, massHint_, decayHint_);
        plots_[1].book(fs, "2", matchModeLabel_, massHint_, decayHint_);

        edm::LogInfo("GenScoutingComparer") << "setup done";
        edm::LogInfo("GenScoutingComparer") << "match mode: " << matchModeLabel_;
    }

    void analyze(const edm::Event& event, const edm::EventSetup& eventSetup) override {
        ++counters_.events;

        std::ostringstream log;
        log << "========== Event: " << event.id().event() << " ==========\n";

        edm::Handle<std::vector<reco::GenParticle>> genParticles;
        if (!event.getByToken(genParticlesToken_, genParticles) || !genParticles.isValid()) {
            return;
        }

        edm::Handle<std::vector<reco::Vertex>> scoutingVertices;
        if (!event.getByToken(scoutingVerticesToken_, scoutingVertices) || !scoutingVertices.isValid()) {
            return;
        }

        edm::Handle<reco::BeamSpot> beamspot;
        const bool haveOfflineBeamSpot = event.getByToken(offlineBeamspotToken_, beamspot) && beamspot.isValid();
        const reco::BeamSpot* bs = haveOfflineBeamSpot ? &(*beamspot) : nullptr;

        if (useOnlineBeamSpot_) {
            auto bsOnlineHandle = eventSetup.getHandle(bsOnlineToken_);
            if (bsOnlineHandle.isValid()) {
                static reco::BeamSpot onlineBeamSpot;
                reco::BeamSpot::CovarianceMatrix onlineError;
                for (int i = 0; i < 7; ++i) {
                    for (int j = i; j < 7; ++j) {
                        onlineError(i, j) = bsOnlineHandle->covariance(i, j);
                    }
                }
                const reco::BeamSpot::Point onlinePos(bsOnlineHandle->x(), bsOnlineHandle->y(), bsOnlineHandle->z());
                onlineBeamSpot = reco::BeamSpot(onlinePos,
                                                bsOnlineHandle->sigmaZ(),
                                                bsOnlineHandle->dxdz(),
                                                bsOnlineHandle->dydz(),
                                                bsOnlineHandle->beamWidthX(),
                                                onlineError);
                bs = &onlineBeamSpot;
            } else if (bs) {
                edm::LogWarning("GenScoutingComparer")
                    << "Online beamspot unavailable; proceeding with offline beamspot as fallback.";
            } else {
                edm::LogWarning("GenScoutingComparer")
                    << "Online beamspot unavailable and offline beamspot missing; proceeding with a null beamspot.";
            }
        }

        edm::Handle<std::vector<Run3ScoutingPFJet>> scoutingJets;
        event.getByToken(scoutingJetsToken_, scoutingJets);

        edm::Handle<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>> trackToScoutingMap;
        event.getByToken(trackToScoutingMapToken_, trackToScoutingMap);
        const bool haveTrackToScoutingMap = trackToScoutingMap.isValid();

        unsigned int selectedJets = 0;
        if (scoutingJets.isValid()) {
            for (const auto& jet : *scoutingJets) {
                if (jet.pt() > jet_pt_min_ && std::abs(jet.eta()) < jet_eta_max_) {
                    ++selectedJets;
                }
            }
        }
        const bool passJetSelection = (selectedJets >= min_selected_jets_);
        const bool eventPassesJetGate = (!require_jet_selection_ || passJetSelection);

        if (scoutingJets.isValid()) {
            // jet gate
        }

        auto stops = findStops(*genParticles, (verbose_ || verbose_unselected_) ? &log : nullptr);
        if (stops[0] && stops[1]) ++counters_.truthEvents;

        if ((verbose_ || verbose_unselected_) && stops[0] && stops[1]) {
            log << "========== 2 STOPs found ==========\n";
        }

        std::vector<SelectedVertexInfo> selectedVertices = buildSelectedVertices(
            scoutingVertices, bs, trackToScoutingMap, haveTrackToScoutingMap, eventPassesJetGate,
            (verbose_ || verbose_unselected_) ? &log : nullptr);

        counters_.selectedVertices += selectedVertices.size();
        if (selectedVertices.empty() && verbose_selected_only_) {
            if (verbose_ || verbose_unselected_) {
                edm::LogVerbatim("GenScoutingComparer") << log.str();
            }
            return;
        }

        if (verbose_ || verbose_unselected_) {
            log << "========== SCOUTING JETS (" << (scoutingJets.isValid() ? scoutingJets->size() : 0) << ") ==========\n";
            log << "Selected jets passing pT > " << jet_pt_min_ << " and |eta| < " << jet_eta_max_ << ": "
                << selectedJets << " / " << (scoutingJets.isValid() ? scoutingJets->size() : 0) << "\n";
            if (scoutingJets.isValid() && !scoutingJets->empty()) {
                log << "Leading jet -> Pt: " << scoutingJets->front().pt()
                    << " | Eta: " << scoutingJets->front().eta()
                    << " | Phi: " << scoutingJets->front().phi() << "\n";
            }
        }

        if (stops[0] && stops[1] && !selectedVertices.empty()) {
            const MatchSolution match = matchVertices(selectedVertices, stops);

            for (int genIndex = 0; genIndex < 2; ++genIndex) {
                const int recoIndex = match.recoForGen[genIndex];
                if (recoIndex < 0) continue;

                const int otherGenIndex = 1 - genIndex;
                const SelectedVertexInfo* otherReco = nullptr;
                if (otherGenIndex >= 0 && otherGenIndex < 2) {
                    const int otherRecoIndex = match.recoForGen[otherGenIndex];
                    if (otherRecoIndex >= 0 && otherRecoIndex != recoIndex) {
                        otherReco = &selectedVertices.at(static_cast<size_t>(otherRecoIndex));
                    }
                }
                (void)otherReco;
                fillBucket(static_cast<size_t>(genIndex), selectedVertices.at(static_cast<size_t>(recoIndex)), *stops[genIndex], bs, match.distance[genIndex]);
            }

            if (match.recoForGen[0] >= 0 || match.recoForGen[1] >= 0) {
                ++counters_.eventsWithMatches;
            }

            if (verbose_) {
                log << "========== SELECTED RECO SCOUTING VERTICES (" << selectedVertices.size() << ") ==========\n";
                log << "Jet selection: " << (passJetSelection ? "PASSED" : "FAILED")
                    << " (need >= " << min_selected_jets_ << " selected jets)\n";
                for (size_t i = 0; i < selectedVertices.size(); ++i) {
                    const auto& info = selectedVertices[i];
                    log << "  Vertex " << i
                        << " x=" << info.vertex->x()
                        << " y=" << info.vertex->y()
                        << " z=" << info.vertex->z()
                        << " pt=" << info.p4.pt()
                        << " mass=" << info.p4.mass()
                        << " eta=" << info.p4.eta()
                        << " phi=" << info.p4.phi()
                        << " [rawTracks=" << info.rawTracks
                        << ", selectedTracks=" << info.selectedTracks
                        << ", rawRefs=" << info.rawTrackRefs << "]\n";
                }
            }
        }

        if (verbose_ || verbose_unselected_) {
            edm::LogVerbatim("GenScoutingComparer") << log.str();
        }
    }

    void endJob() override {
        edm::LogInfo("GenScoutingComparer")
            << "events=" << counters_.events
            << " truthEvents=" << counters_.truthEvents
            << " matchedEvents=" << counters_.eventsWithMatches
            << " selectedVertices=" << counters_.selectedVertices;
    }
};

DEFINE_FWK_MODULE(GenScoutingComparer);
