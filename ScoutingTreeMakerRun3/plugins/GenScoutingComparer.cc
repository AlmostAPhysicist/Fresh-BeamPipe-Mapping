// GenScoutingComparer.cc
#include <iostream>
#include <vector>
#include <cmath>

#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/VertexReco/interface/Vertex.h"

class GenScoutingComparer : public edm::one::EDAnalyzer<> {
private:
    edm::EDGetTokenT<std::vector<reco::GenParticle>> genParticlesToken_;
    edm::EDGetTokenT<std::vector<reco::Vertex>> scoutingVerticesToken_;

    bool verbose_;

public:
    explicit GenScoutingComparer(const edm::ParameterSet& config) {
        genParticlesToken_ = consumes<std::vector<reco::GenParticle>>(config.getParameter<edm::InputTag>("genParticles")); // prunedGenParticles
        scoutingVerticesToken_ = consumes<std::vector<reco::Vertex>>(config.getParameter<edm::InputTag>("scoutingVertices")); // Vertexer
        verbose_ = config.getUntrackedParameter<bool>("verbose", false);
    }

    void beginJob() override {
        std::cout << "\n=== GenScoutingComparer Initialized ===\n";
    }

    void analyze(const edm::Event& event, const edm::EventSetup&) override {

        if (verbose_) { // Event ID printout
            std::cout << "\n========== Event: " << event.id().event() << " ==========\n";
        }

    // Retrieve GenParticles and scouting vertices
        edm::Handle<std::vector<reco::GenParticle>> genParticles;
        if (!event.getByToken(genParticlesToken_, genParticles)) {
            std::cout << "[WARNING] GenParticle collection not found!" << std::endl;
            return;
        }

        edm::Handle<std::vector<reco::Vertex>> scoutingVertices;
        if (!event.getByToken(scoutingVerticesToken_, scoutingVertices)) {
            std::cout << "[WARNING] Scouting vertex collection not found!" << std::endl;
            return;
        }

    // -------------------- Gen level --------------------
        const reco::GenParticle* stop1 = nullptr;
        const reco::GenParticle* stop2 = nullptr;

        // Helper to compute transverse production radius.
        auto vxy = [](const reco::GenParticle* gp) {
            return std::hypot(gp->vx(), gp->vy());
        };

        for (const auto& p : *genParticles) {
            // 1. Must be stop/antistop (PDG ID 1000006 or -1000006)
            if (std::abs(p.pdgId()) != 1000006) continue;

            // 2. Must have 2 daughters both either down/antidown (PDG ID 1 or -1)
            int downCount = 0;
            for (size_t d = 0; d < p.numberOfDaughters(); ++d) {
                const reco::Candidate* dau = p.daughter(d);
                if (dau && std::abs(dau->pdgId()) == 1) {
                    downCount++;
                }
            }

            if (downCount != 2) continue;

            // Found a stop candidate, assign to stop1 or stop2
            if (!stop1) {
                stop1 = &p;
            } else if (!stop2) {
                stop2 = &p;
                break; // found both, exit loop
            }
        }

        
        if (verbose_){ // Print properties cleanly if both exist
            if (stop1 && stop2) {
                std::cout << "\n========== 2 STOPs found ==========\n";
                
                double vxy1 = vxy(stop1);
                std::cout << "Stop1 -> PDGID: " << stop1->pdgId() << " | Pt: " << stop1->pt() << " | Eta: " << stop1->eta()
                          << " | vxy: " << vxy1
                          << " | vx: " << stop1->vx() << " | vy: " << stop1->vy() << " | vz: " << stop1->vz() << "\n";

                double vxy2 = vxy(stop2);
                std::cout << "Stop2 -> PDGID: " << stop2->pdgId() << " | Pt: " << stop2->pt() << " | Eta: " << stop2->eta()
                          << " | vxy: " << vxy2
                          << " | vx: " << stop2->vx() << " | vy: " << stop2->vy() << " | vz: " << stop2->vz() << "\n";
            } else {
                std::cout << "\n========== " << (stop1 ? "1" : "0") << " STOPs found ==========\n";
            }
        }

    // ------------- Reco scouting vertices --------------
        std::vector<const reco::Vertex*> recoScoutingVertices; // vector of vertex pointers
        recoScoutingVertices.reserve(scoutingVertices->size());

        for (const auto& v : *scoutingVertices) {
            recoScoutingVertices.push_back(&v);
        }

        // CUTS AND FILTERS TO BE APPLIED HERE


        //-----

        if (verbose_) {
            std::cout << "\n========== RECO SCOUTING VERTICES ==========\n";
            auto vxy = [](const reco::Vertex* v) {
                return std::hypot(v->x(), v->y());
            };
            for (size_t i = 0; i < recoScoutingVertices.size(); ++i) {
                const auto* v = recoScoutingVertices[i];
                std::cout << "Vertex " << i
                          << " | vxy: " << vxy(v)
                          << " | vx: " << v->x() << " | vy: " << v->y() << " | vz: " << v->z()
                          << "\n";
            }
        }
    }

    void endJob() override {
        std::cout << "\n=== GenScoutingComparer Finished ===\n";
    }
};

DEFINE_FWK_MODULE(GenScoutingComparer);