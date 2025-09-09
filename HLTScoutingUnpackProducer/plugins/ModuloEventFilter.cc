#include "FWCore/Framework/interface/global/EDFilter.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

class ModuloEventFilter : public edm::global::EDFilter<> {
public:
  explicit ModuloEventFilter(const edm::ParameterSet& iConfig) :
    modulo_(iConfig.getParameter<unsigned int>("modulo")),
    remainder_(iConfig.getUntrackedParameter<unsigned int>("remainder", 0))
  {}
  // corrected signature: StreamID, Event& (non-const), EventSetup const&
  bool filter(edm::StreamID, edm::Event & iEvent, edm::EventSetup const&) const override {
    unsigned int ev = iEvent.id().event();
    if (modulo_ == 0) return true;
    return ((ev % modulo_) == remainder_);
  }
private:
  unsigned int modulo_;
  unsigned int remainder_;
};

DEFINE_FWK_MODULE(ModuloEventFilter);