#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/TrackReco/interface/HitPattern.h"
#include "DataFormats/SiPixelDetId/interface/PixelSubdetector.h"

#include <cmath>

class PackedCandidateToTrackProducer : public edm::stream::EDProducer<> {
public:
  explicit PackedCandidateToTrackProducer(const edm::ParameterSet& iConfig);
  ~PackedCandidateToTrackProducer() override = default;

  void produce(edm::Event&, const edm::EventSetup&) override;

private:
  const edm::EDGetTokenT<pat::PackedCandidateCollection> packedCandToken_;

  bool pass_tk(const reco::Track& tk) const;
};

PackedCandidateToTrackProducer::PackedCandidateToTrackProducer(
    const edm::ParameterSet& iConfig)
  : packedCandToken_(
        consumes<pat::PackedCandidateCollection>(
            iConfig.getParameter<edm::InputTag>("src"))) {

  produces<reco::TrackCollection>("Track");
}

bool PackedCandidateToTrackProducer::pass_tk(const reco::Track& tk) const {
  // pT and hit requirements
  if (tk.pt() < 1.0) return false;
  if (tk.hitPattern().pixelLayersWithMeasurement() < 2) return false;
  if (tk.hitPattern().stripLayersWithMeasurement() < 6) return false;

  // Innermost pixel layer logic
  const bool hasLayer1 =
    tk.hitPattern().hasValidHitInPixelLayer(PixelSubdetector::PixelBarrel, 1);

  const bool hasLayer2NoMissingInner =
    tk.hitPattern().hasValidHitInPixelLayer(PixelSubdetector::PixelBarrel, 2) &&
    tk.hitPattern().numberOfLostHits(reco::HitPattern::MISSING_INNER_HITS) == 0;

  if (!(hasLayer1 || hasLayer2NoMissingInner)) return false;

  // dxy significance
  if (tk.dxyError() <= 0) return false;
  if (std::abs(tk.dxy() / tk.dxyError()) <= 4.0) return false;

  return true;
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

    // PackedCandidate → reco::Track
    reco::Track tk = cand.pseudoTrack();

    // Safety guard
    if (!std::isfinite(tk.pt())) continue;

    // Apply Joey's cuts
    if (!pass_tk(tk)) continue;

    outTracks->push_back(tk);
  }

  iEvent.put(std::move(outTracks), "Track");
}

DEFINE_FWK_MODULE(PackedCandidateToTrackProducer);
