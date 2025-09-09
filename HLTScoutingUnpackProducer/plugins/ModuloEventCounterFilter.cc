#include <atomic>
#include "FWCore/Framework/interface/global/EDFilter.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

class ModuloEventCounterFilter : public edm::global::EDFilter<> {
public:
  explicit ModuloEventCounterFilter(const edm::ParameterSet& iConfig) :
    modulo_(iConfig.getParameter<unsigned int>("modulo")),
    remainder_(iConfig.getUntrackedParameter<unsigned int>("remainder", 0))
  {}
  bool filter(edm::StreamID, edm::Event&, edm::EventSetup const&) const override {
    unsigned long long idx = counter_.fetch_add(1, std::memory_order_relaxed);
    if (modulo_ == 0) return true;
    return ((idx % modulo_) == remainder_);
  }
private:
  unsigned int modulo_;
  unsigned int remainder_;
  static std::atomic<unsigned long long> counter_;
};
std::atomic<unsigned long long> ModuloEventCounterFilter::counter_{0ull};
DEFINE_FWK_MODULE(ModuloEventCounterFilter);