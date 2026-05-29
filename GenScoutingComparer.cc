// GenScoutingComparer.cc
#include <iostream>
#include <sstream>
#include <vector>
#include <cmath>

#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/BeamSpot/interface/BeamSpot.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/HitPattern.h"
#include "DataFormats/Math/interface/Vector3D.h"
#include "DataFormats/Scouting/interface/Run3ScoutingPFJet.h"

class GenScoutingComparer : public edm::one::EDAnalyzer<> {
private:
    edm::EDGetTokenT<std::vector<reco::GenParticle>> genParticlesToken_;
    edm::EDGetTokenT<std::vector<reco::Vertex>> scoutingVerticesToken_;
    edm::EDGetTokenT<reco::BeamSpot> beamspotToken_;
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

public:
    explicit GenScoutingComparer(const edm::ParameterSet& config) {
        genParticlesToken_ = consumes<std::vector<reco::GenParticle>>(config.getParameter<edm::InputTag>("genParticles"));
        scoutingVerticesToken_ = consumes<std::vector<reco::Vertex>>(config.getParameter<edm::InputTag>("scoutingVertices"));
        beamspotToken_ = consumes<reco::BeamSpot>(config.getParameter<edm::InputTag>("beamspot"));
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
            verboseOut << "Stop1 -> Pt: " << stop1->pt() << " | vxy: " << vxy(stop1) << "\n";
            verboseOut << "Stop2 -> Pt: " << stop2->pt() << " | vxy: " << vxy(stop2) << "\n";
        }

        // ------------- Reco scouting vertices --------------
        std::vector<const reco::Vertex*> selectedRecoVertices;
        selectedRecoVertices.reserve(scoutingVertices->size());

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
            std::ostringstream trackOut;

            auto symbolFor = [](bool pass, const char* failSymbol, const char* passSymbol) {
                return pass ? passSymbol : failSymbol;
            };

            unsigned int trackIndex = 0;
            for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it, ++trackIndex) {
                reco::TrackRef trk = it->castTo<reco::TrackRef>();
                if (trk.isNonnull()) {
                    ++rawTrackRefs;
                    rawPSum += trk->momentum();

                    const reco::HitPattern& hp = trk->hitPattern();
                    const bool passPixelHits = (hp.numberOfValidPixelHits() >= cut_track_pixelHits_min_);
                    const bool passStripHits = (hp.numberOfValidStripHits() >= cut_track_stripHits_min_);
                    const bool passTrackerLayers = (hp.trackerLayersWithMeasurement() >= cut_track_trackerLayers_min_);
                    const bool passTrack = passPixelHits && passStripHits && passTrackerLayers;

                    if (verbose_unselected_) {
                        trackOut << "    track " << trackIndex << ": pT=" << trk->pt()
                                 << " | pixelHits=" << hp.numberOfValidPixelHits()
                                 << symbolFor(passPixelHits, " (!< ", " (>= ") << cut_track_pixelHits_min_ << ")"
                                 << " | stripHits=" << hp.numberOfValidStripHits()
                                 << symbolFor(passStripHits, " (!< ", " (>= ") << cut_track_stripHits_min_ << ")"
                                 << " | trackerLayers=" << hp.trackerLayersWithMeasurement()
                                 << symbolFor(passTrackerLayers, " (!< ", " (>= ") << cut_track_trackerLayers_min_ << ")"
                                 << " -> " << (passTrack ? "SELECTED" : "REJECTED") << "\n";
                    }

                    if (passTrack) {
                        validTracks++;
                        selectedPSum += trk->momentum();
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
        }

        if (verbose_selected_only_ && selectedRecoVertices.empty()) {
            return;
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
            verboseOut << "========== SELECTED RECO SCOUTING VERTICES (" << selectedRecoVertices.size() << ") ==========\n";
            verboseOut << "Jet selection: " << (passJetSelection ? "PASSED" : "FAILED")
                       << " (need >= " << min_selected_jets_ << " selected jets)\n";
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