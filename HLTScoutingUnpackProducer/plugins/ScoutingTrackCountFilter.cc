#include "FWCore/Framework/interface/global/EDFilter.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "DataFormats/Scouting/interface/Run3ScoutingTrack.h"
#include <vector>

class ScoutingTrackCountFilter : public edm::global::EDFilter<> {
public:
  explicit ScoutingTrackCountFilter(const edm::ParameterSet& iConfig) :
    src_(iConfig.getParameter<edm::InputTag>("src")),
    minNumber_(iConfig.getUntrackedParameter<unsigned int>("minNumber", 1))
  {
    token_ = consumes<std::vector<Run3ScoutingTrack>>(src_);
  }

  // corrected signature: StreamID, Event& (non-const), EventSetup const&
  bool filter(edm::StreamID, edm::Event & iEvent, edm::EventSetup const&) const override {
    edm::Handle<std::vector<Run3ScoutingTrack>> h;
    iEvent.getByToken(token_, h);
    if (!h.isValid()) return false;
    return h->size() >= minNumber_;
  }

private:
  edm::InputTag src_;
  edm::EDGetTokenT<std::vector<Run3ScoutingTrack>> token_;
  unsigned int minNumber_;
};

DEFINE_FWK_MODULE(ScoutingTrackCountFilter);