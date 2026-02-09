#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"

class PackedCandidateToTrackProducer : public edm::stream::EDProducer<> {
public:
  explicit PackedCandidateToTrackProducer(const edm::ParameterSet& iConfig);
  ~PackedCandidateToTrackProducer() override = default;

  void produce(edm::Event&, const edm::EventSetup&) override;

private:
  const edm::EDGetTokenT<pat::PackedCandidateCollection> packedCandToken_;
};

PackedCandidateToTrackProducer::PackedCandidateToTrackProducer(
    const edm::ParameterSet& iConfig)
  : packedCandToken_(
        consumes<pat::PackedCandidateCollection>(
            iConfig.getParameter<edm::InputTag>("src"))) {

  produces<reco::TrackCollection>("Track");
}

void PackedCandidateToTrackProducer::produce(
    edm::Event& iEvent, const edm::EventSetup&) {

  edm::Handle<pat::PackedCandidateCollection> packedCands;
  iEvent.getByToken(packedCandToken_, packedCands);

  auto outTracks = std::make_unique<reco::TrackCollection>();

  if (!packedCands.isValid()) {
    iEvent.put(std::move(outTracks), "Track");
    return;
  }

  for (const auto& cand : *packedCands) {
    if (cand.charge() == 0) continue;
    if (!cand.hasTrackDetails()) continue;

    // THIS is the crucial type conversion
    reco::Track tk = cand.pseudoTrack();

    // Minimal sanity guard
    if (!std::isfinite(tk.pt())) continue;

    outTracks->push_back(tk);
  }

  iEvent.put(std::move(outTracks), "Track");
}

DEFINE_FWK_MODULE(PackedCandidateToTrackProducer);
