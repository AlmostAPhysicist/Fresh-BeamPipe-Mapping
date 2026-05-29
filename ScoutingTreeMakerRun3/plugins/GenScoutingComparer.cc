// GenScoutingComparer.cc
#include <iostream>
#include <sstream>
#include <array>
#include <vector>
#include <cmath>
#include <limits>
#include <string>

#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "TH1F.h"
#include "TH2F.h"

#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/Common/interface/ValueMap.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/BeamSpot/interface/BeamSpot.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/HitPattern.h"
#include "DataFormats/Math/interface/Vector3D.h"
#include "DataFormats/Scouting/interface/Run3ScoutingTrack.h"
#include "DataFormats/Scouting/interface/Run3ScoutingPFJet.h"

class GenScoutingComparer : public edm::one::EDAnalyzer<edm::one::SharedResources> {
private:
    struct SelectedVertexInfo {
        const reco::Vertex* vertex = nullptr;
        math::XYZVector trackMomentumSum{0, 0, 0};
        double trackEnergySum = 0.0;
        unsigned int rawTracks = 0;
        unsigned int validTracks = 0;
        unsigned int rawTrackRefs = 0;
    };

    struct VertexPairAssignment {
        int recoIndexForGen[2] = {-1, -1};
        double distanceForGen[2] = {-1.0, -1.0};
        double totalDistance = std::numeric_limits<double>::max();
    };

    struct VertexPairObservables {
        double relXYDx = 0.0;
        double relXYDy = 0.0;
        double avgXYPosGlobal00X = 0.0;
        double avgXYPosGlobal00Y = 0.0;
        double avgXYPosBeamSpotX = 0.0;
        double avgXYPosBeamSpotY = 0.0;
        double massDiff = 0.0;
        double avgMass = 0.0;
        double ptDiff = 0.0;
        double avgPt = 0.0;
        double phiDiff = 0.0;
        double avgPhi = 0.0;
        double etaDiff = 0.0;
        double avgEta = 0.0;
        double normChi2Diff = 0.0;
        double normChi2Avg = 0.0;
        double radialDistanceGlobal00Diff = 0.0;
        double radialDistanceGlobal00Avg = 0.0;
        double radialDistanceBeamSpotDiff = 0.0;
        double radialDistanceBeamSpotAvg = 0.0;
        double significanceDiff = 0.0;
        double significanceAvg = 0.0;
        double dxyErrorDiff = 0.0;
        double dxyErrorAvg = 0.0;
    };

    struct VertexPairPlots {
        std::array<TH1F*, 2> relXYDx{{nullptr, nullptr}};
        std::array<TH2F*, 2> relXYPos{{nullptr, nullptr}};
        std::array<TH2F*, 2> avgXYPosGlobal00{{nullptr, nullptr}};
        std::array<TH2F*, 2> avgXYPosBeamSpot{{nullptr, nullptr}};
        std::array<TH1F*, 2> massDiff{{nullptr, nullptr}};
        std::array<TH1F*, 2> avgMass{{nullptr, nullptr}};
        std::array<TH1F*, 2> ptDiff{{nullptr, nullptr}};
        std::array<TH1F*, 2> avgPt{{nullptr, nullptr}};
        std::array<TH1F*, 2> phiDiff{{nullptr, nullptr}};
        std::array<TH1F*, 2> avgPhi{{nullptr, nullptr}};
        std::array<TH1F*, 2> etaDiff{{nullptr, nullptr}};
        std::array<TH1F*, 2> avgEta{{nullptr, nullptr}};
        std::array<TH1F*, 2> normChi2Diff{{nullptr, nullptr}};
        std::array<TH1F*, 2> normChi2Avg{{nullptr, nullptr}};
        std::array<TH1F*, 2> radialDistanceGlobal00Diff{{nullptr, nullptr}};
        std::array<TH1F*, 2> radialDistanceGlobal00Avg{{nullptr, nullptr}};
        std::array<TH1F*, 2> radialDistanceBeamSpotDiff{{nullptr, nullptr}};
        std::array<TH1F*, 2> radialDistanceBeamSpotAvg{{nullptr, nullptr}};
        std::array<TH1F*, 2> significanceDiff{{nullptr, nullptr}};
        std::array<TH1F*, 2> significanceAvg{{nullptr, nullptr}};
        std::array<TH1F*, 2> dxyErrorDiff{{nullptr, nullptr}};
        std::array<TH1F*, 2> dxyErrorAvg{{nullptr, nullptr}};
        TH1F* nSelectedRecoVertices = nullptr;

        void book(edm::Service<TFileService>& fs) {
            if (!fs) {
                return;
            }

            auto bookPair1D = [&](std::array<TH1F*, 2>& target,
                                const std::string& baseName,
                                const std::string& title,
                                int bins,
                                double min,
                                double max) {
                for (int i = 0; i < 2; ++i) {
                    const std::string suffix = std::to_string(i + 1);
                    target[static_cast<size_t>(i)] = fs->make<TH1F>((baseName + "_" + suffix).c_str(), title.c_str(), bins, min, max);
                }
            };

            auto bookPair2D = [&](std::array<TH2F*, 2>& target,
                                const std::string& baseName,
                                const std::string& title,
                                int xBins,
                                double xMin,
                                double xMax,
                                int yBins,
                                double yMin,
                                double yMax) {
                for (int i = 0; i < 2; ++i) {
                    const std::string suffix = std::to_string(i + 1);
                    target[static_cast<size_t>(i)] = fs->make<TH2F>((baseName + "_" + suffix).c_str(), title.c_str(), xBins, xMin, xMax, yBins, yMin, yMax);
                }
            };

            bookPair1D(relXYDx, "rel_xy_dx", ";#Delta x [cm];Events", 500, -0.025, 0.025);
            bookPair2D(relXYPos, "rel_xy_pos", ";#Delta x [cm];#Delta y [cm]", 200, -0.05, 0.05, 200, -0.05, 0.05);
            bookPair2D(avgXYPosGlobal00, "avg_xy_pos_global00", ";Average x [cm];Average y [cm]", 200, -1.0, 1.0, 200, -1.0, 1.0);
            bookPair2D(avgXYPosBeamSpot, "avg_xy_pos_beamspot", ";Average x wrt beamspot [cm];Average y wrt beamspot [cm]", 200, -1.0, 1.0, 200, -1.0, 1.0);
            bookPair1D(massDiff, "mass_diff", ";Reco - Gen mass [GeV];Events", 140, -100.0, 100.0);
            bookPair1D(avgMass, "avg_mass", ";Average mass [GeV];Events", 140, 0.0, 400.0);
            bookPair1D(ptDiff, "pt_diff", ";Reco - Gen p_{T} [GeV];Events", 140, -200.0, 200.0);
            bookPair1D(avgPt, "avg_pt", ";Average p_{T} [GeV];Events", 140, 0.0, 400.0);
            bookPair1D(phiDiff, "phi_diff", ";Reco - Gen #phi;Events", 128, -3.2, 3.2);
            bookPair1D(avgPhi, "avg_phi", ";Average #phi;Events", 128, -3.2, 3.2);
            bookPair1D(etaDiff, "eta_diff", ";Reco - Gen #eta;Events", 120, -6.0, 6.0);
            bookPair1D(avgEta, "avg_eta", ";Average #eta;Events", 120, -6.0, 6.0);
            bookPair1D(normChi2Diff, "norm_chi2_diff", ";Reco - partner #chi^{2};Events", 120, -10.0, 10.0);
            bookPair1D(normChi2Avg, "norm_chi2_avg", ";Average #chi^{2};Events", 120, 0.0, 10.0);
            bookPair1D(radialDistanceGlobal00Diff, "radial_distance_global00_diff", ";Reco - partner radial distance wrt (0,0) [cm];Events", 120, -1.0, 1.0);
            bookPair1D(radialDistanceGlobal00Avg, "radial_distance_global00_avg", ";Average radial distance wrt (0,0) [cm];Events", 120, 0.0, 1.0);
            bookPair1D(radialDistanceBeamSpotDiff, "radial_distance_beamspot_diff", ";Reco - partner radial distance wrt beamspot [cm];Events", 120, -1.0, 1.0);
            bookPair1D(radialDistanceBeamSpotAvg, "radial_distance_beamspot_avg", ";Average radial distance wrt beamspot [cm];Events", 120, 0.0, 1.0);
            bookPair1D(significanceDiff, "significance_diff", ";Reco - partner significance;Events", 120, -100.0, 100.0);
            bookPair1D(significanceAvg, "significance_avg", ";Average significance;Events", 120, 0.0, 100.0);
            bookPair1D(dxyErrorDiff, "dxy_error_diff", ";Reco - partner d_{xy} error [cm];Events", 120, -0.1, 0.1);
            bookPair1D(dxyErrorAvg, "dxy_error_avg", ";Average d_{xy} error [cm];Events", 120, 0.0, 0.1);

            nSelectedRecoVertices = fs->make<TH1F>("n_selected_scouting_vertices", ";Selected scouting reco vertices;Events", 6, -0.5, 5.5);
        }

        void fill(size_t genIndex, const VertexPairObservables& obs) const {
            if (genIndex >= 2) {
                return;
            }

            const auto idx = static_cast<size_t>(genIndex);
            if (relXYDx[idx]) relXYDx[idx]->Fill(obs.relXYDx);
            if (relXYPos[idx]) relXYPos[idx]->Fill(obs.relXYDx, obs.relXYDy);
            if (avgXYPosGlobal00[idx]) avgXYPosGlobal00[idx]->Fill(obs.avgXYPosGlobal00X, obs.avgXYPosGlobal00Y);
            if (avgXYPosBeamSpot[idx]) avgXYPosBeamSpot[idx]->Fill(obs.avgXYPosBeamSpotX, obs.avgXYPosBeamSpotY);
            if (massDiff[idx]) massDiff[idx]->Fill(obs.massDiff);
            if (avgMass[idx]) avgMass[idx]->Fill(obs.avgMass);
            if (ptDiff[idx]) ptDiff[idx]->Fill(obs.ptDiff);
            if (avgPt[idx]) avgPt[idx]->Fill(obs.avgPt);
            if (phiDiff[idx]) phiDiff[idx]->Fill(obs.phiDiff);
            if (avgPhi[idx]) avgPhi[idx]->Fill(obs.avgPhi);
            if (etaDiff[idx]) etaDiff[idx]->Fill(obs.etaDiff);
            if (avgEta[idx]) avgEta[idx]->Fill(obs.avgEta);
            if (normChi2Diff[idx]) normChi2Diff[idx]->Fill(obs.normChi2Diff);
            if (normChi2Avg[idx]) normChi2Avg[idx]->Fill(obs.normChi2Avg);
            if (radialDistanceGlobal00Diff[idx]) radialDistanceGlobal00Diff[idx]->Fill(obs.radialDistanceGlobal00Diff);
            if (radialDistanceGlobal00Avg[idx]) radialDistanceGlobal00Avg[idx]->Fill(obs.radialDistanceGlobal00Avg);
            if (radialDistanceBeamSpotDiff[idx]) radialDistanceBeamSpotDiff[idx]->Fill(obs.radialDistanceBeamSpotDiff);
            if (radialDistanceBeamSpotAvg[idx]) radialDistanceBeamSpotAvg[idx]->Fill(obs.radialDistanceBeamSpotAvg);
            if (significanceDiff[idx]) significanceDiff[idx]->Fill(obs.significanceDiff);
            if (significanceAvg[idx]) significanceAvg[idx]->Fill(obs.significanceAvg);
            if (dxyErrorDiff[idx]) dxyErrorDiff[idx]->Fill(obs.dxyErrorDiff);
            if (dxyErrorAvg[idx]) dxyErrorAvg[idx]->Fill(obs.dxyErrorAvg);
        }

        void fillEventCount(size_t selectedRecoVertexCount) const {
            if (nSelectedRecoVertices) {
                const double cappedCount = std::min<size_t>(selectedRecoVertexCount, 5);
                nSelectedRecoVertices->Fill(cappedCount);
            }
        }
    };

    static double wrapDeltaPhi(double phi1, double phi2) {
        constexpr double pi = 3.14159265358979323846;
        double deltaPhi = phi1 - phi2;
        while (deltaPhi > pi) deltaPhi -= 2.0 * pi;
        while (deltaPhi <= -pi) deltaPhi += 2.0 * pi;
        return deltaPhi;
    }

    static double vertexDistance3D(const SelectedVertexInfo& recoInfo, const reco::GenParticle& genStop) {
        const auto& v = *recoInfo.vertex;
        return std::sqrt(std::pow(v.x() - genStop.vx(), 2) +
                         std::pow(v.y() - genStop.vy(), 2) +
                         std::pow(v.z() - genStop.vz(), 2));
    }

    static double vertexDistance3DSquared(const SelectedVertexInfo& recoInfo, const reco::GenParticle& genStop) {
        const auto& v = *recoInfo.vertex;
        return std::pow(v.x() - genStop.vx(), 2) +
               std::pow(v.y() - genStop.vy(), 2) +
               std::pow(v.z() - genStop.vz(), 2);
    }

    static double vertexRadialDistanceGlobal00(const SelectedVertexInfo& recoInfo) {
        const auto& v = *recoInfo.vertex;
        return std::hypot(v.x(), v.y());
    }

    static double vertexRadialDistanceBeamSpot(const SelectedVertexInfo& recoInfo, const reco::BeamSpot& beamspot) {
        const auto& v = *recoInfo.vertex;
        return std::hypot(v.x() - beamspot.x0(), v.y() - beamspot.y0());
    }

    static double vectorPt(const math::XYZVector& vec) {
        return std::hypot(vec.x(), vec.y());
    }

    static double vectorPhi(const math::XYZVector& vec) {
        return std::atan2(vec.y(), vec.x());
    }

    static double vectorEta(const math::XYZVector& vec) {
        const double pt = vectorPt(vec);
        return (pt > 0.0) ? std::asinh(vec.z() / pt) : 0.0;
    }

    static double vectorMass(const math::XYZVector& momentum, double energy) {
        const double mass2 = energy * energy - momentum.mag2();
        return mass2 > 0.0 ? std::sqrt(mass2) : 0.0;
    }

    static VertexPairAssignment matchRecoVerticesToStops(
        const std::vector<SelectedVertexInfo>& recoInfos,
        const std::array<const reco::GenParticle*, 2>& genStops) {
        VertexPairAssignment best;
        if (!genStops[0] || !genStops[1] || recoInfos.empty()) {
            return best;
        }

        auto considerAssignment = [&](int recoIndexForGen0, int recoIndexForGen1, double dist0, double dist1) {
            const double totalDistance = dist0 + dist1;
            if (totalDistance < best.totalDistance) {
                best.recoIndexForGen[0] = recoIndexForGen0;
                best.recoIndexForGen[1] = recoIndexForGen1;
                best.distanceForGen[0] = dist0;
                best.distanceForGen[1] = dist1;
                best.totalDistance = totalDistance;
            }
        };

        if (recoInfos.size() == 1) {
            const double d0 = vertexDistance3DSquared(recoInfos[0], *genStops[0]);
            const double d1 = vertexDistance3DSquared(recoInfos[0], *genStops[1]);
            if (d0 <= d1) {
                best.recoIndexForGen[0] = 0;
                best.distanceForGen[0] = d0;
                best.totalDistance = d0;
            } else {
                best.recoIndexForGen[1] = 0;
                best.distanceForGen[1] = d1;
                best.totalDistance = d1;
            }
            return best;
        }

        for (size_t i = 0; i < recoInfos.size(); ++i) {
            for (size_t j = i + 1; j < recoInfos.size(); ++j) {
                const double d00 = vertexDistance3DSquared(recoInfos[i], *genStops[0]);
                const double d01 = vertexDistance3DSquared(recoInfos[i], *genStops[1]);
                const double d10 = vertexDistance3DSquared(recoInfos[j], *genStops[0]);
                const double d11 = vertexDistance3DSquared(recoInfos[j], *genStops[1]);

                considerAssignment(static_cast<int>(i), static_cast<int>(j), d00, d11);
                considerAssignment(static_cast<int>(j), static_cast<int>(i), d10, d01);
            }
        }

        return best;
    }

    static VertexPairObservables buildVertexPairObservables(
        const SelectedVertexInfo& recoInfo,
        const SelectedVertexInfo* otherRecoInfo,
        const reco::GenParticle& genStop,
        const reco::BeamSpot& beamspot) {
        const auto& v = *recoInfo.vertex;

        const double recoPt = vectorPt(recoInfo.trackMomentumSum);
        const double recoPhi = vectorPhi(recoInfo.trackMomentumSum);
        const double recoEta = vectorEta(recoInfo.trackMomentumSum);
        const double recoMass = vectorMass(recoInfo.trackMomentumSum, recoInfo.trackEnergySum);

        const double genPt = genStop.pt();
        const double genPhi = genStop.phi();
        const double genEta = genStop.eta();
        const double genMass = genStop.mass();

        const double recoRadialGlobal00 = vertexRadialDistanceGlobal00(recoInfo);
        const double recoRadialBeamSpot = vertexRadialDistanceBeamSpot(recoInfo, beamspot);
        const SelectedVertexInfo& partnerInfo = otherRecoInfo ? *otherRecoInfo : recoInfo;
        const auto& partnerVertex = *partnerInfo.vertex;
        const double partnerNormChi2 = partnerVertex.normalizedChi2();
        const double partnerRadialGlobal00 = vertexRadialDistanceGlobal00(partnerInfo);
        const double partnerRadialBeamSpot = vertexRadialDistanceBeamSpot(partnerInfo, beamspot);
        const double partnerDxyError = std::hypot(partnerVertex.xError(), partnerVertex.yError());
        const double partnerSignificance = (partnerDxyError > 0.0) ? (partnerRadialBeamSpot / partnerDxyError) : -1.0;

        VertexPairObservables obs;
        obs.relXYDx = v.x() - genStop.vx();
        obs.relXYDy = v.y() - genStop.vy();
        obs.avgXYPosGlobal00X = 0.5 * (v.x() + genStop.vx());
        obs.avgXYPosGlobal00Y = 0.5 * (v.y() + genStop.vy());
        obs.avgXYPosBeamSpotX = 0.5 * ((v.x() - beamspot.x0()) + (genStop.vx() - beamspot.x0()));
        obs.avgXYPosBeamSpotY = 0.5 * ((v.y() - beamspot.y0()) + (genStop.vy() - beamspot.y0()));
        obs.massDiff = recoMass - genMass;
        obs.avgMass = 0.5 * (recoMass + genMass);
        obs.ptDiff = recoPt - genPt;
        obs.avgPt = 0.5 * (recoPt + genPt);
        obs.phiDiff = wrapDeltaPhi(recoPhi, genPhi);
        obs.avgPhi = 0.5 * (recoPhi + genPhi);
        obs.etaDiff = recoEta - genEta;
        obs.avgEta = 0.5 * (recoEta + genEta);
        obs.normChi2Diff = v.normalizedChi2() - partnerNormChi2;
        obs.normChi2Avg = 0.5 * (v.normalizedChi2() + partnerNormChi2);
        obs.radialDistanceGlobal00Diff = recoRadialGlobal00 - partnerRadialGlobal00;
        obs.radialDistanceGlobal00Avg = 0.5 * (recoRadialGlobal00 + partnerRadialGlobal00);
        obs.radialDistanceBeamSpotDiff = recoRadialBeamSpot - partnerRadialBeamSpot;
        obs.radialDistanceBeamSpotAvg = 0.5 * (recoRadialBeamSpot + partnerRadialBeamSpot);
        obs.dxyErrorDiff = std::hypot(v.xError(), v.yError()) - partnerDxyError;
        obs.dxyErrorAvg = 0.5 * (std::hypot(v.xError(), v.yError()) + partnerDxyError);
        obs.significanceDiff = ((obs.dxyErrorAvg != 0.0) ? (recoRadialBeamSpot / std::hypot(v.xError(), v.yError())) : -1.0) - partnerSignificance;
        obs.significanceAvg = 0.5 * (((std::hypot(v.xError(), v.yError()) > 0.0) ? (recoRadialBeamSpot / std::hypot(v.xError(), v.yError())) : -1.0) + partnerSignificance);
        return obs;
    }

    void fillMatchedVertexPlots(const VertexPairAssignment& match,
                                const std::vector<SelectedVertexInfo>& recoInfos,
                                const std::array<const reco::GenParticle*, 2>& genStops,
                                const reco::BeamSpot& beamspot) const {
        for (int genIndex = 0; genIndex < 2; ++genIndex) {
            const int recoIndex = match.recoIndexForGen[genIndex];
            if (recoIndex < 0 || !genStops[genIndex]) {
                continue;
            }

            const auto& recoInfo = recoInfos.at(static_cast<size_t>(recoIndex));
            const int otherGenIndex = 1 - genIndex;
            const SelectedVertexInfo* otherRecoInfo = nullptr;
            if (otherGenIndex >= 0 && otherGenIndex < 2) {
                const int otherRecoIndex = match.recoIndexForGen[otherGenIndex];
                if (otherRecoIndex >= 0 && otherRecoIndex != recoIndex) {
                    otherRecoInfo = &recoInfos.at(static_cast<size_t>(otherRecoIndex));
                }
            }

            const VertexPairObservables obs = buildVertexPairObservables(recoInfo, otherRecoInfo, *genStops[genIndex], beamspot);

            plots_.fill(static_cast<size_t>(genIndex), obs);
        }
    }

    edm::EDGetTokenT<std::vector<reco::GenParticle>> genParticlesToken_;
    edm::EDGetTokenT<std::vector<reco::Vertex>> scoutingVerticesToken_;
    edm::EDGetTokenT<reco::BeamSpot> beamspotToken_;
    edm::EDGetTokenT<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>> trackToScoutingMapToken_;
    edm::EDGetTokenT<std::vector<Run3ScoutingPFJet>> scoutingJetsToken_;

    // Configurable Cut Thresholds
    double cut_vtx_chi2_max_;
    double cut_vtx_dbv_min_;
    double cut_vtx_dbv_max_;
    unsigned int cut_vtx_tracks_min_;
    double cut_vtx_ddbv_max_;
    double cut_vtx_cosT_min_;
    
    // Track Hit Thresholds
    int cut_track_pixelHits_min_;
    int cut_track_stripHits_min_;
    int cut_track_trackerLayers_min_;

    // Verbosity flags
    bool verbose_;
    bool verbose_unselected_;
    bool verbose_selected_only_;
    bool ntracks_raw_;
    bool cost_raw_;

    // Jet selection thresholds
    double jet_pt_min_;
    double jet_eta_max_;
    unsigned int min_selected_jets_;
    bool require_jet_selection_;

    VertexPairPlots plots_;

public:
    explicit GenScoutingComparer(const edm::ParameterSet& config) {
        usesResource("TFileService");
        genParticlesToken_ = consumes<std::vector<reco::GenParticle>>(config.getParameter<edm::InputTag>("genParticles"));
        scoutingVerticesToken_ = consumes<std::vector<reco::Vertex>>(config.getParameter<edm::InputTag>("scoutingVertices"));
        beamspotToken_ = consumes<reco::BeamSpot>(config.getParameter<edm::InputTag>("beamspot"));
        trackToScoutingMapToken_ = consumes<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>>(config.getParameter<edm::InputTag>("trackToScoutingMap"));
        scoutingJetsToken_ = consumes<std::vector<Run3ScoutingPFJet>>(config.getParameter<edm::InputTag>("scoutingJets"));

        // Load configuration parameters
        cut_vtx_chi2_max_ = config.getParameter<double>("vtx_chi2_max");
        cut_vtx_dbv_min_  = config.getParameter<double>("vtx_dbv_min");
        cut_vtx_dbv_max_  = config.getParameter<double>("vtx_dbv_max");
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
        cost_raw_ = config.getUntrackedParameter<bool>("cost_raw", true);

        jet_pt_min_ = config.getParameter<double>("jet_pt_min");
        jet_eta_max_ = config.getParameter<double>("jet_eta_max");
        min_selected_jets_ = config.getParameter<unsigned int>("min_selected_jets");
        require_jet_selection_ = config.getUntrackedParameter<bool>("require_jet_selection", false);
    }

    void beginJob() override {
        edm::Service<TFileService> fs;
        plots_.book(fs);
        std::cout << "\n=== GenScoutingComparer Initialized ===\n";
    }

    void analyze(const edm::Event& event, const edm::EventSetup&) override {
        std::ostringstream verboseOut;

        edm::Handle<std::vector<reco::GenParticle>> genParticles;
        if (!event.getByToken(genParticlesToken_, genParticles)) return;

        edm::Handle<std::vector<reco::Vertex>> scoutingVertices;
        if (!event.getByToken(scoutingVerticesToken_, scoutingVertices)) return;

        edm::Handle<reco::BeamSpot> beamspot;
        event.getByToken(beamspotToken_, beamspot);
        double bsX = beamspot.isValid() ? beamspot->x0() : 0.0;
        double bsY = beamspot.isValid() ? beamspot->y0() : 0.0;
        double bsZ = beamspot.isValid() ? beamspot->z0() : 0.0;

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

        // -------------------- Gen level --------------------
        const reco::GenParticle* stop1 = nullptr;
        const reco::GenParticle* stop2 = nullptr;

        auto vxy = [](const reco::GenParticle* gp) {
            return std::hypot(gp->vx(), gp->vy());
        };

        for (const auto& p : *genParticles) {
            if (std::abs(p.pdgId()) != 1000006) continue;

            int downCount = 0;
            for (size_t d = 0; d < p.numberOfDaughters(); ++d) {
                const reco::Candidate* dau = p.daughter(d);
                if (dau && std::abs(dau->pdgId()) == 1) {
                    downCount++;
                }
            }

            if (downCount != 2) continue;

            if (!stop1) {
                stop1 = &p;
            } else if (!stop2) {
                stop2 = &p;
                break; 
            }
        }

        if ((verbose_ || verbose_unselected_) && stop1 && stop2) { 
            verboseOut << "========== Event: " << event.id().event() << " ==========\n";
            verboseOut << "========== 2 STOPs found ==========\n";
            verboseOut << "Stop1 -> x=" << stop1->vx()
                       << " y=" << stop1->vy()
                       << " z=" << stop1->vz()
                       << " pt=" << stop1->pt()
                       << " mass=" << stop1->mass()
                       << " eta=" << stop1->eta()
                       << " phi=" << stop1->phi()
                       << " | vxy: " << vxy(stop1) << "\n";
            verboseOut << "Stop2 -> x=" << stop2->vx()
                       << " y=" << stop2->vy()
                       << " z=" << stop2->vz()
                       << " pt=" << stop2->pt()
                       << " mass=" << stop2->mass()
                       << " eta=" << stop2->eta()
                       << " phi=" << stop2->phi()
                       << " | vxy: " << vxy(stop2) << "\n";
        }

        // ------------- Reco scouting vertices --------------
        std::vector<const reco::Vertex*> selectedRecoVertices;
        std::vector<SelectedVertexInfo> selectedVertexInfos;
        selectedRecoVertices.reserve(scoutingVertices->size());
        selectedVertexInfos.reserve(scoutingVertices->size());

        if (verbose_unselected_) {
            verboseOut << "========== UNSELECTED RECO VERTICES EVALUATION ==========\n";
        }

        for (size_t i = 0; i < scoutingVertices->size(); ++i) {
            const auto& v = scoutingVertices->at(i);
            
            // --- 1. Property Calculations ---
            double chi2 = v.normalizedChi2();
            double dBV = std::hypot(v.x() - bsX, v.y() - bsY);
            double ddBV = std::hypot(v.xError(), v.yError());
            const unsigned int rawTracks = v.tracksSize();

            unsigned int validTracks = 0;
            unsigned int rawTrackRefs = 0;
            math::XYZVector rawPSum(0, 0, 0);
            math::XYZVector selectedPSum(0, 0, 0);
            double selectedEnergySum = 0.0;
            std::ostringstream trackOut;

            unsigned int trackIndex = 0;
            for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it, ++trackIndex) {
                reco::TrackRef trk = it->castTo<reco::TrackRef>();
                if (trk.isNonnull()) {
                    ++rawTrackRefs;
                    rawPSum += trk->momentum();

                    int nPixelHits = 0;
                    int nStripHits = 0;
                    int nTrackerLayers = 0;
                    if (haveTrackToScoutingMap) {
                        auto scoutingRef = (*trackToScoutingMap)[trk];
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

                    if (verbose_unselected_) {
                        trackOut << "    track " << trackIndex << ": pT=" << trk->pt()
                                 << " | pixelHits=" << nPixelHits << (passPixelHits ? " (>= " : " (!>= ") << cut_track_pixelHits_min_ << ")"
                                 << " | stripHits=" << nStripHits << (passStripHits ? " (>= " : " (!>= ") << cut_track_stripHits_min_ << ")"
                                 << " | trackerLayers=" << nTrackerLayers << (passTrackerLayers ? " (>= " : " (!>= ") << cut_track_trackerLayers_min_ << ")"
                                 << " -> " << (passTrack ? "SELECTED" : "REJECTED") << "\n";
                    }

                    if (passTrack) {
                        validTracks++;
                        selectedPSum += trk->momentum();
                        constexpr double pionMass = 0.13957039;
                        const double trackMomentum = trk->p();
                        selectedEnergySum += std::sqrt(trackMomentum * trackMomentum + pionMass * pionMass);
                    }
                }
            }

            math::XYZVector disp(v.x() - bsX, v.y() - bsY, v.z() - bsZ);
            double cosT = -999.0;
            const math::XYZVector& cosTPsum = cost_raw_ ? rawPSum : selectedPSum;
            if (disp.R() > 0 && cosTPsum.R() > 0) {
                cosT = disp.Dot(cosTPsum) / (disp.R() * cosTPsum.R());
            }

            // --- 2. Cut Evaluations ---
            bool passChi2   = (chi2 < cut_vtx_chi2_max_);
            bool passDbvMin = (dBV >= cut_vtx_dbv_min_);
            bool passDbvMax = (dBV < cut_vtx_dbv_max_);
            bool passDdbv   = (ddBV < cut_vtx_ddbv_max_);
            bool passCosT   = (cosT > cut_vtx_cosT_min_);
            const unsigned int nTracksForCut = ntracks_raw_ ? rawTracks : validTracks;
            bool passNtk    = (nTracksForCut >= cut_vtx_tracks_min_);

            // --- 3. Verbose Unselected Printing ---
            if (verbose_unselected_) {
                verboseOut << "Vertex " << i << ":\n"
                           << "  chi2/dof : " << chi2 << (passChi2 ? " (< " : " (!< ") << cut_vtx_chi2_max_ << ")\n"
                           << "  dBV      : " << dBV << (passDbvMin ? " (>= " : " (!>= ") << cut_vtx_dbv_min_ << ") and "
                                                       << (passDbvMax ? "(< " : "(!< ") << cut_vtx_dbv_max_ << ")\n"
                           << "  ddBV     : " << ddBV << (passDdbv ? " (< " : " (!< ") << cut_vtx_ddbv_max_ << ")\n"
                           << "  cosT     : " << cosT << (passCosT ? " (> " : " (!> ") << cut_vtx_cosT_min_ << ") [using " << (cost_raw_ ? "raw" : "selected") << " track sum]\n"
                           << "  nTracks  : " << nTracksForCut << (passNtk ? " (>= " : " (!>= ") << cut_vtx_tracks_min_ << ") [raw=" << rawTracks << ", selected=" << validTracks << ", rawRefs=" << rawTrackRefs << "]\n";
                if (verbose_unselected_) {
                    verboseOut << trackOut.str();
                }
                
                if (passChi2 && passDbvMin && passDbvMax && passDdbv && passCosT && passNtk) {
                    verboseOut << "  -> STATUS: PASSED ALL CUTS\n";
                } else {
                    verboseOut << "  -> STATUS: FAILED\n";
                }
            }

            // --- 4. Filtering ---
            if (!passChi2 || !passDbvMin || !passDbvMax || !passDdbv || !passCosT || !passNtk || !eventPassesJetGate) continue;

            selectedRecoVertices.push_back(&v);
            selectedVertexInfos.push_back(SelectedVertexInfo{
                &v,
                selectedPSum,
                selectedEnergySum,
                rawTracks,
                validTracks,
                rawTrackRefs,
            });
        }

        if (verbose_selected_only_ && selectedRecoVertices.empty()) {
            plots_.fillEventCount(selectedRecoVertices.size());
            return;
        }

        plots_.fillEventCount(selectedRecoVertices.size());

        if (stop1 && stop2 && beamspot.isValid()) {
            const std::array<const reco::GenParticle*, 2> genStops{{stop1, stop2}};
            const VertexPairAssignment match = matchRecoVerticesToStops(selectedVertexInfos, genStops);
            fillMatchedVertexPlots(match, selectedVertexInfos, genStops, *beamspot);
        }

        if ((verbose_ || verbose_unselected_) && scoutingJets.isValid()) {
            verboseOut << "========== SCOUTING JETS (" << scoutingJets->size() << ") ==========\n";
            verboseOut << "Selected jets passing pT > " << jet_pt_min_ << " and |eta| < " << jet_eta_max_ << ": "
                       << selectedJets << " / " << scoutingJets->size() << "\n";
            if (!scoutingJets->empty()) {
                verboseOut << "Leading jet -> Pt: " << scoutingJets->front().pt()
                           << " | Eta: " << scoutingJets->front().eta()
                           << " | Phi: " << scoutingJets->front().phi() << "\n";
            }
        }

        if (verbose_) {
            if (verbose_ || verbose_selected_only_) {
                verboseOut << "========== SELECTED RECO SCOUTING VERTICES (" << selectedRecoVertices.size() << ") ==========\n";
                verboseOut << "Jet selection: " << (passJetSelection ? "PASSED" : "FAILED")
                           << " (need >= " << min_selected_jets_ << " selected jets)\n";
                for (size_t i = 0; i < selectedVertexInfos.size(); ++i) {
                    const auto& info = selectedVertexInfos[i];
                    const auto& v = *info.vertex;

                    const double px = info.trackMomentumSum.x();
                    const double py = info.trackMomentumSum.y();
                    const double pz = info.trackMomentumSum.z();
                    const double pt = std::hypot(px, py);
                    const double phi = std::atan2(py, px);
                    const double eta = (pt > 0.0) ? std::asinh(pz / pt) : 0.0;
                    const double p2 = info.trackMomentumSum.mag2();
                    const double mass2 = info.trackEnergySum * info.trackEnergySum - p2;
                    const double mass = mass2 > 0.0 ? std::sqrt(mass2) : 0.0;

                    verboseOut << "  Vertex " << i << ":"
                               << " x=" << v.x()
                               << " y=" << v.y()
                               << " z=" << v.z()
                               << " pt=" << pt
                               << " mass=" << mass
                               << " eta=" << eta
                               << " phi=" << phi
                               << " [rawTracks=" << info.rawTracks
                               << ", selectedTracks=" << info.validTracks
                               << ", rawRefs=" << info.rawTrackRefs
                               << "]\n";
                }
            }
        }

        if (verbose_ || verbose_unselected_) {
            std::cout << verboseOut.str();
        }
    }

    void endJob() override {
        std::cout << "\n=== GenScoutingComparer Finished ===\n";
    }
};

DEFINE_FWK_MODULE(GenScoutingComparer);