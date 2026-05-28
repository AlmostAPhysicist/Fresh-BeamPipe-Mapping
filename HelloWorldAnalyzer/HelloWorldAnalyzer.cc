#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include <iostream>

class HelloWorldAnalyzer : public edm::one::EDAnalyzer<> {
public:
  explicit HelloWorldAnalyzer(const edm::ParameterSet&) {}
  void analyze(const edm::Event&, const edm::EventSetup&) override {
    std::cout << "Hello world from analyze()" << std::endl;
  }
};

DEFINE_FWK_MODULE(HelloWorldAnalyzer);