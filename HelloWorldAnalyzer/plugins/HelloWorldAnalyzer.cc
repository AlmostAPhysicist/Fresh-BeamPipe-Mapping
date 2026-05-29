// HelloWorldAnalyzer.cc
#include <algorithm>
#include <iostream>
#include <vector>
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

class HelloWorldAnalyzer : public edm::one::EDAnalyzer<> {
private:
    int eventIndex_ = 0;
    std::vector<int> selectedEvents_;

public:
    explicit HelloWorldAnalyzer(const edm::ParameterSet& config)
        : selectedEvents_(config.getParameter<std::vector<int>>("selectedEvents")) {}

    void beginJob() override {
        std::cout << "\n=== Event quantities commonly used ===\n"
                  << "event.id().run()\n"
                  << "event.id().event()\n"
                  << "event.luminosityBlock()\n"
                  << "event.bunchCrossing()\n"
                  << "event.orbitNumber()\n"
                  << "event.isRealData()\n"
                  << "======================================\n";
    }

    void analyze(const edm::Event& event, const edm::EventSetup&) override {
        ++eventIndex_;
        if (std::find(selectedEvents_.begin(), selectedEvents_.end(), eventIndex_) == selectedEvents_.end()) {
            return;
        }

        std::cout << "\n========== EVENT ==========\n"
                  << "File Event Index: " << eventIndex_ << "\n"
                  << "Run: " << event.id().run() << "\n"
                  << "Lumi: " << event.luminosityBlock() << "\n"
                  << "Event: " << event.id().event() << "\n"
                  << "===========================\n";
    }

    void endJob() override {
        std::cout << "\nFile ended\n";
    }
};

DEFINE_FWK_MODULE(HelloWorldAnalyzer);