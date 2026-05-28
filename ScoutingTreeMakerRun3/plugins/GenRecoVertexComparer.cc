// GenRecoVertexComparer.cc
#include <algorithm>
#include <cmath>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "DataFormats/BeamSpot/interface/BeamSpot.h"
#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/Math/interface/Point3D.h"
#include "DataFormats/TrackReco/interface/HitPattern.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"

#include "RecoVertex/VertexTools/interface/VertexDistance3D.h"
#include "RecoVertex/VertexTools/interface/VertexDistanceXY.h"
#include "TrackingTools/IPTools/interface/IPTools.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "TrackingTools/TransientTrack/interface/TransientTrack.h"
#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"

#include "TH1F.h"
#include "TH2F.h"
#include "TVector3.h"
#include "TLorentzVector.h"

class GenRecoVertexComparer : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit GenRecoVertexComparer(const edm::ParameterSet&);
  ~GenRecoVertexComparer() override = default;
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  void beginJob() override;
  void analyze(const edm::Event&, const edm::EventSetup&) override;
  void endJob() override {}

  struct RefVertex { reco::Vertex::Point p; reco::Vertex::Error e; bool valid = false; };
  struct GenTruth { reco::Vertex::Point p; bool valid = false; };

  struct Cuts {
    std::vector<int> cut_ntk;
    double required_invmass = -1.0;
    double required_chi2 = -1.0;
    double required_dBV_min = -1.0;
    double required_dBV_max = -1.0;
    double required_dxy_min = -1.0;
    double required_dxy_max = -1.0;
    double required_dBV_error = -1.0;
    double required_dxy_error = -1.0;
    int min_nTracks = 0;
    int max_nTracks = -1;
    double min_cosT = -1.0;
    double track_pt_min_cut = 0.9;
    double track_dxySig_min_cut = 4.0;
    double track_dxySig_max_cut = 100.0;
    bool applyHitCuts = false;
    int hit_minPixelHits = 3;
    int hit_minStripHits = 2;
    int hit_minTrackerLayers = 6;
    bool use_2d_track_dist = true;
    bool use_2d_vertex_dist = false;
    bool use_pdgId = true;
    int gen_pdgId = 1000006;
    int gen_status = 106;
    bool useGenDaughters = true;
  };

  edm::EDGetTokenT<std::vector<reco::Vertex>> displacedVerticesToken_;
  edm::EDGetTokenT<std::vector<reco::Track>> tracksToken_;
  edm::EDGetTokenT<std::vector<reco::Vertex>> primaryVerticesToken_;
  edm::EDGetTokenT<reco::BeamSpot> beamspotToken_;
  edm::EDGetTokenT<std::vector<reco::GenParticle>> genParticlesToken_;
  edm::ESGetToken<TransientTrackBuilder, TransientTrackRecord> ttBuilderToken_;

  const bool refPreferenceIsPV_;
  const Cuts cuts_;

  TH1F *h_dx_ = nullptr;
  TH1F *h_dy_ = nullptr;
  TH1F *h_dz_ = nullptr;
  TH1F *h_dxy_ = nullptr;
  TH1F *h_d3_ = nullptr;
  TH1F *h_cosT_ = nullptr;
  TH1F *h_nTracks_ = nullptr;
  TH1F *h_mass_ = nullptr;
  TH2F *h_xy_reco_vs_gen_ = nullptr;
  TH2F *h_z_reco_vs_gen_ = nullptr;

  static bool passAllowedNTracks(int n, const std::vector<int>& allowed, int minN, int maxN);
  static RefVertex makeReferenceVertex(const edm::Event& iEvent,
                                       const edm::EDGetTokenT<std::vector<reco::Vertex>>& pvToken,
                                       const edm::EDGetTokenT<reco::BeamSpot>& bsToken,
                                       bool preferPV);
  static bool isLLP(const reco::GenParticle& p, const Cuts& cuts);
  static GenTruth truthFromLLPSelf(const reco::GenParticle& llp);
  static GenTruth truthFromLLPChildren(const reco::GenParticle& llp);
  static GenTruth findTruthVertex(const std::vector<reco::GenParticle>& gens, const Cuts& cuts);
  static reco::Vertex buildGenRecoVertex(const GenTruth& truth);
  static const reco::Vertex* bestRecoMatch(const std::vector<reco::Vertex>& vtxs,
                                           const reco::Vertex& truthVtx,
                                           bool use2DVertexDist);
  bool passTrackCutsInVertex(const reco::Vertex& v, const edm::EventSetup& iSetup, const RefVertex& refVtx) const;
  bool passRecoVertexCuts(const reco::Vertex& v, const edm::EventSetup& iSetup, const RefVertex& refVtx) const;
  double meanAbsTrackDxy(const reco::Vertex& v, const edm::EventSetup& iSetup, const RefVertex& refVtx) const;
  double vertexCosT(const reco::Vertex& v, const reco::Vertex& truthVtx) const;
};

GenRecoVertexComparer::GenRecoVertexComparer(const edm::ParameterSet& ps)
  : displacedVerticesToken_(consumes<std::vector<reco::Vertex>>(ps.getParameter<edm::InputTag>("displacedVertices"))),
    tracksToken_(consumes<std::vector<reco::Track>>(ps.getParameter<edm::InputTag>("tracks"))),
    primaryVerticesToken_(consumes<std::vector<reco::Vertex>>(ps.getParameter<edm::InputTag>("primaryVertices"))),
    beamspotToken_(consumes<reco::BeamSpot>(ps.getParameter<edm::InputTag>("beamspot_src"))),
    genParticlesToken_(consumes<std::vector<reco::GenParticle>>(ps.getParameter<edm::InputTag>("genParticles"))),
    ttBuilderToken_(esConsumes<TransientTrackBuilder, TransientTrackRecord>(edm::ESInputTag("", "TransientTrackBuilder"))),
    refPreferenceIsPV_(ps.getUntrackedParameter<std::string>("refPreference", "BeamSpot") == "PV"),
    cuts_([&] {
      Cuts c;
      c.cut_ntk = ps.getParameter<std::vector<int>>("cut_ntk");
      c.required_invmass = ps.getParameter<double>("required_invmass");
      c.required_chi2 = ps.getParameter<double>("required_chi2");
      c.required_dBV_min = ps.getParameter<double>("required_dBV_min");
      c.required_dBV_max = ps.getParameter<double>("required_dBV_max");
      c.required_dxy_min = ps.getParameter<double>("required_dxy_min");
      c.required_dxy_max = ps.getParameter<double>("required_dxy_max");
      c.required_dBV_error = ps.getParameter<double>("required_dBV_error");
      c.required_dxy_error = ps.getParameter<double>("required_dxy_error");
      c.min_nTracks = ps.getParameter<int>("min_nTracks");
      c.max_nTracks = ps.getParameter<int>("max_nTracks");
      c.min_cosT = ps.getParameter<double>("min_cosT");
      c.track_pt_min_cut = ps.getUntrackedParameter<double>("track_pt_min_cut", 0.9);
      c.track_dxySig_min_cut = ps.getUntrackedParameter<double>("track_dxySig_min_cut", 4.0);
      c.track_dxySig_max_cut = ps.getUntrackedParameter<double>("track_dxySig_max_cut", 100.0);
      c.applyHitCuts = ps.getParameter<bool>("applyHitCuts");
      c.hit_minPixelHits = ps.getUntrackedParameter<int>("hit_minPixelHits", 3);
      c.hit_minStripHits = ps.getUntrackedParameter<int>("hit_minStripHits", 2);
      c.hit_minTrackerLayers = ps.getUntrackedParameter<int>("hit_minTrackerLayers", 6);
      c.use_2d_track_dist = ps.getParameter<bool>("use_2d_track_dist");
      c.use_2d_vertex_dist = ps.getParameter<bool>("use_2d_vertex_dist");
      c.use_pdgId = ps.getParameter<bool>("use_pdgId");
      c.gen_pdgId = ps.getUntrackedParameter<int>("gen_pdgId", 1000006);
      c.gen_status = ps.getUntrackedParameter<int>("gen_status", 106);
      c.useGenDaughters = ps.getParameter<bool>("useGenDaughters");
      return c;
    }()) {
  usesResource("TFileService");
}

void GenRecoVertexComparer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("displacedVertices", edm::InputTag("Vertexer"));
  desc.add<edm::InputTag>("tracks", edm::InputTag("hltScoutingUnpackProducer", "Track"));
  desc.add<edm::InputTag>("primaryVertices", edm::InputTag("hltScoutingUnpackProducer", "PrimaryVertex"));
  desc.add<edm::InputTag>("beamspot_src", edm::InputTag("offlineBeamSpot"));
  desc.add<edm::InputTag>("genParticles", edm::InputTag("prunedGenParticles"));
  desc.add<std::vector<int>>("cut_ntk", {});
  desc.add<double>("required_invmass", -1.0);
  desc.add<double>("required_chi2", -1.0);
  desc.add<double>("required_dBV_min", -1.0);
  desc.add<double>("required_dBV_max", -1.0);
  desc.add<double>("required_dxy_min", -1.0);
  desc.add<double>("required_dxy_max", -1.0);
  desc.add<double>("required_dBV_error", -1.0);
  desc.add<double>("required_dxy_error", -1.0);
  desc.add<int>("min_nTracks", 0);
  desc.add<int>("max_nTracks", -1);
  desc.add<double>("min_cosT", -1.0);
  desc.addUntracked<double>("track_pt_min_cut", 0.9);
  desc.addUntracked<double>("track_dxySig_min_cut", 4.0);
  desc.addUntracked<double>("track_dxySig_max_cut", 100.0);
  desc.add<bool>("applyHitCuts", false);
  desc.addUntracked<int>("hit_minPixelHits", 3);
  desc.addUntracked<int>("hit_minStripHits", 2);
  desc.addUntracked<int>("hit_minTrackerLayers", 6);
  desc.add<bool>("use_2d_track_dist", true);
  desc.add<bool>("use_2d_vertex_dist", false);
  desc.add<bool>("use_pdgId", true);
  desc.addUntracked<int>("gen_pdgId", 1000006);
  desc.addUntracked<int>("gen_status", 106);
  desc.add<bool>("useGenDaughters", true);
  desc.addUntracked<std::string>("refPreference", "BeamSpot");
  descriptions.add("GenRecoVertexComparer", desc);
}

void GenRecoVertexComparer::beginJob() {
  auto fs = edm::Service<TFileService>();
  h_dx_  = fs->make<TH1F>("dx",  "x_{reco}-x_{gen};#Delta x [cm];Entries", 200, -0.5, 0.5);
  h_dy_  = fs->make<TH1F>("dy",  "y_{reco}-y_{gen};#Delta y [cm];Entries", 200, -0.5, 0.5);
  h_dz_  = fs->make<TH1F>("dz",  "z_{reco}-z_{gen};#Delta z [cm];Entries", 200, -5.0, 5.0);
  h_dxy_ = fs->make<TH1F>("dxy", "r_{T,reco}-r_{T,gen};#Delta r_{T} [cm];Entries", 200, -0.5, 0.5);
  h_d3_  = fs->make<TH1F>("d3",   "|r_{reco}-r_{gen}|;3D distance [cm];Entries", 200, 0.0, 5.0);
  h_cosT_= fs->make<TH1F>("cosT", "cos(#theta);cosT;Entries", 120, -1.0, 1.0);
  h_nTracks_ = fs->make<TH1F>("nTracks", "nTracks;nTracks;Entries", 20, 0, 20);
  h_mass_ = fs->make<TH1F>("mass", "vertex mass;mass [GeV];Entries", 200, 0.0, 20.0);
  h_xy_reco_vs_gen_ = fs->make<TH2F>("xy_reco_vs_gen", "x/y; x_{gen} [cm]; y_{gen} [cm]", 200, -1, 1, 200, -1, 1);
  h_z_reco_vs_gen_ = fs->make<TH2F>("z_reco_vs_gen", "z; z_{gen} [cm]; z_{reco} [cm]", 200, -20, 20, 200, -20, 20);
}

GenRecoVertexComparer::RefVertex GenRecoVertexComparer::makeReferenceVertex(const edm::Event& iEvent,
                                                                            const edm::EDGetTokenT<std::vector<reco::Vertex>>& pvToken,
                                                                            const edm::EDGetTokenT<reco::BeamSpot>& bsToken,
                                                                            bool preferPV) {
  RefVertex out;
  edm::Handle<std::vector<reco::Vertex>> pvsH;
  edm::Handle<reco::BeamSpot> bsH;
  iEvent.getByToken(pvToken, pvsH);
  iEvent.getByToken(bsToken, bsH);

  if (preferPV && pvsH.isValid() && !pvsH->empty()) {
    double sx = 0, sy = 0, sz = 0;
    int n = 0;
    for (const auto& pv : *pvsH) {
      if (pv.isFake() || pv.ndof() <= 4) continue;
      sx += pv.x(); sy += pv.y(); sz += pv.z();
      ++n;
    }
    if (n > 0) {
      out.p = reco::Vertex::Point(sx/n, sy/n, sz/n);
      out.e = reco::Vertex::Error();
      out.e(0,0) = 0.0015*0.0015;
      out.e(1,1) = 0.0015*0.0015;
      out.e(2,2) = 0.0050*0.0050;
      out.valid = true;
      return out;
    }
  }

  if (bsH.isValid()) {
    out.p = bsH->position();
    out.e = bsH->covariance3D();
    out.valid = true;
    return out;
  }

  if (pvsH.isValid() && !pvsH->empty()) {
    const auto& pv = pvsH->front();
    out.p = reco::Vertex::Point(pv.x(), pv.y(), pv.z());
    out.e = pv.covariance();
    out.valid = true;
  }
  return out;
}

bool GenRecoVertexComparer::isLLP(const reco::GenParticle& p, const Cuts& cuts) {
  if (!cuts.use_pdgId) return true;
  if (std::abs(p.pdgId()) != std::abs(cuts.gen_pdgId)) return false;
  if (cuts.gen_status >= 0 && p.status() != cuts.gen_status) return false;
  return true;
}

GenRecoVertexComparer::GenTruth GenRecoVertexComparer::truthFromLLPSelf(const reco::GenParticle& llp) {
  GenTruth t;
  t.p = reco::Vertex::Point(llp.vx(), llp.vy(), llp.vz());
  t.valid = true;
  return t;
}

GenRecoVertexComparer::GenTruth GenRecoVertexComparer::truthFromLLPChildren(const reco::GenParticle& llp) {
  double sx = 0., sy = 0., sz = 0.;
  int n = 0;
  for (size_t i = 0; i < llp.numberOfDaughters(); ++i) {
    const auto* d = llp.daughter(i);
    if (!d) continue;
    sx += d->vx(); sy += d->vy(); sz += d->vz();
    ++n;
  }
  if (n == 0) return truthFromLLPSelf(llp);
  GenTruth t;
  t.p = reco::Vertex::Point(sx/n, sy/n, sz/n);
  t.valid = true;
  return t;
}

GenRecoVertexComparer::GenTruth GenRecoVertexComparer::findTruthVertex(const std::vector<reco::GenParticle>& gens, const Cuts& cuts) {
  for (const auto& p : gens) {
    if (!isLLP(p, cuts)) continue;
    return cuts.useGenDaughters ? truthFromLLPChildren(p) : truthFromLLPSelf(p);
  }
  return {};
}

reco::Vertex GenRecoVertexComparer::buildGenRecoVertex(const GenTruth& truth) {
  reco::Vertex::Error err;
  err(0,0) = 1e-6;
  err(1,1) = 1e-6;
  err(2,2) = 1e-6;
  return reco::Vertex(truth.p, err);
}

const reco::Vertex* GenRecoVertexComparer::bestRecoMatch(const std::vector<reco::Vertex>& vtxs,
                                                         const reco::Vertex& truthVtx,
                                                         bool use2DVertexDist) {
  if (vtxs.empty()) return nullptr;
  VertexDistanceXY dxy;
  VertexDistance3D d3;
  const reco::Vertex* best = nullptr;
  double bestDist = 1e99;
  for (const auto& v : vtxs) {
    const double d = use2DVertexDist ? dxy.distance(v, truthVtx).value() : d3.distance(v, truthVtx).value();
    if (d < bestDist) { bestDist = d; best = &v; }
  }
  return best;
}

bool GenRecoVertexComparer::passAllowedNTracks(int n, const std::vector<int>& allowed, int minN, int maxN) {
  if (!allowed.empty()) return std::find(allowed.begin(), allowed.end(), n) != allowed.end();
  if (n < minN) return false;
  if (maxN >= 0 && n > maxN) return false;
  return true;
}

double GenRecoVertexComparer::meanAbsTrackDxy(const reco::Vertex& v, const edm::EventSetup& iSetup, const RefVertex& refVtx) const {
  auto const& ttBuilder = iSetup.getData(ttBuilderToken_);
  reco::Vertex ref(refVtx.p, refVtx.e);
  double sum = 0.0;
  int n = 0;
  for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
    reco::TrackRef trRef = it->castTo<reco::TrackRef>();
    if (!trRef.isNonnull()) continue;
    reco::TransientTrack tt = ttBuilder.build(trRef);
    auto ip = cuts_.use_2d_track_dist ? IPTools::absoluteTransverseImpactParameter(tt, ref)
                                      : IPTools::absoluteImpactParameter3D(tt, ref);
    if (!ip.first) continue;
    sum += std::abs(ip.second.value());
    ++n;
  }
  return n > 0 ? sum / n : 0.0;
}

bool GenRecoVertexComparer::passTrackCutsInVertex(const reco::Vertex& v, const edm::EventSetup& iSetup, const RefVertex& refVtx) const {
  auto const& ttBuilder = iSetup.getData(ttBuilderToken_);
  reco::Vertex ref(refVtx.p, refVtx.e);

  for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
    reco::TrackRef trRef = it->castTo<reco::TrackRef>();
    if (!trRef.isNonnull()) return false;
    if (trRef->pt() < cuts_.track_pt_min_cut) return false;

    reco::TransientTrack tt = ttBuilder.build(trRef);
    if (!tt.isValid()) return false;
    auto ip = cuts_.use_2d_track_dist ? IPTools::absoluteTransverseImpactParameter(tt, ref)
                                      : IPTools::absoluteImpactParameter3D(tt, ref);
    if (!ip.first) return false;
    const double sig = std::abs(ip.second.significance());
    if (sig < cuts_.track_dxySig_min_cut || sig > cuts_.track_dxySig_max_cut) return false;

    if (cuts_.applyHitCuts) {
      const auto& hp = trRef->hitPattern();
      if (hp.numberOfValidPixelHits() < cuts_.hit_minPixelHits) return false;
      if (hp.numberOfValidStripHits() < cuts_.hit_minStripHits) return false;
      if (hp.trackerLayersWithMeasurement() < cuts_.hit_minTrackerLayers) return false;
    }
  }
  return true;
}

double GenRecoVertexComparer::vertexCosT(const reco::Vertex& v, const reco::Vertex& truthVtx) const {
  TVector3 disp(v.x() - truthVtx.x(), v.y() - truthVtx.y(), v.z() - truthVtx.z());
  TVector3 mom(v.p4().Px(), v.p4().Py(), v.p4().Pz());
  if (disp.Mag() <= 0 || mom.Mag() <= 0) return -2.0;
  return disp.Dot(mom) / (disp.Mag() * mom.Mag());
}

bool GenRecoVertexComparer::passRecoVertexCuts(const reco::Vertex& v, const edm::EventSetup& iSetup, const RefVertex& refVtx) const {
  if (!passAllowedNTracks(static_cast<int>(std::distance(v.tracks_begin(), v.tracks_end())), cuts_.cut_ntk, cuts_.min_nTracks, cuts_.max_nTracks))
    return false;

  if (cuts_.required_chi2 >= 0 && v.normalizedChi2() > cuts_.required_chi2) return false;

  reco::Vertex ref(refVtx.p, refVtx.e);
  VertexDistanceXY dxy;
  VertexDistance3D d3;
  const auto dv = cuts_.use_2d_vertex_dist ? dxy.distance(v, ref) : d3.distance(v, ref);
  const double dBV = dv.value();
  const double dBVerr = dv.error();
  const double dxyMean = meanAbsTrackDxy(v, iSetup, refVtx);

  if (cuts_.required_dBV_min >= 0 && dBV < cuts_.required_dBV_min) return false;
  if (cuts_.required_dBV_max >= 0 && dBV > cuts_.required_dBV_max) return false;
  if (cuts_.required_dxy_min >= 0 && dxyMean < cuts_.required_dxy_min) return false;
  if (cuts_.required_dxy_max >= 0 && dxyMean > cuts_.required_dxy_max) return false;
  if (cuts_.required_dBV_error >= 0 && dBVerr > cuts_.required_dBV_error) return false;

  if (cuts_.required_dxy_error >= 0) {
    auto const& ttBuilder = iSetup.getData(ttBuilderToken_);
    double errSum = 0.0;
    int n = 0;
    for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
      reco::TrackRef trRef = it->castTo<reco::TrackRef>();
      if (!trRef.isNonnull()) continue;
      reco::TransientTrack tt = ttBuilder.build(trRef);
      auto ip = cuts_.use_2d_track_dist ? IPTools::absoluteTransverseImpactParameter(tt, ref)
                                        : IPTools::absoluteImpactParameter3D(tt, ref);
      if (!ip.first) continue;
      errSum += ip.second.error();
      ++n;
    }
    if (n > 0 && (errSum / n) > cuts_.required_dxy_error) return false;
  }

  if (cuts_.required_invmass >= 0) {
    TLorentzVector sum(0,0,0,0);
    for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it) {
      reco::TrackRef trRef = it->castTo<reco::TrackRef>();
      if (!trRef.isNonnull()) continue;
      TLorentzVector p4;
      p4.SetPtEtaPhiM(trRef->pt(), trRef->eta(), trRef->phi(), 0.13957);
      sum += p4;
    }
    if (sum.M() < cuts_.required_invmass) return false;
  }

  if (cuts_.min_cosT >= -1.0) {
    if (vertexCosT(v, truthVtx) < cuts_.min_cosT) return false;
  }

  if (!passTrackCutsInVertex(v, iSetup, refVtx)) return false;
  return true;
}

void GenRecoVertexComparer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
  edm::Handle<std::vector<reco::Vertex>> vtxH;
  edm::Handle<std::vector<reco::GenParticle>> genH;
  iEvent.getByToken(displacedVerticesToken_, vtxH);
  iEvent.getByToken(genParticlesToken_, genH);
  if (!vtxH.isValid() || !genH.isValid()) return;

  const RefVertex ref = makeReferenceVertex(iEvent, primaryVerticesToken_, beamspotToken_, refPreferenceIsPV_);
  if (!ref.valid) return;

  const GenTruth truth = findTruthVertex(*genH, cuts_);
  if (!truth.valid) return;

  const reco::Vertex truthVtx = buildGenRecoVertex(truth);
  const reco::Vertex* bestReco = bestRecoMatch(*vtxH, truthVtx, cuts_.use_2d_vertex_dist);
  if (!bestReco) return;
  if (!passRecoVertexCuts(*bestReco, iSetup, ref)) return;

  VertexDistanceXY dxy;
  VertexDistance3D d3;
  const auto dv = cuts_.use_2d_vertex_dist ? dxy.distance(*bestReco, truthVtx) : d3.distance(*bestReco, truthVtx);

  h_dx_->Fill(bestReco->x() - truthVtx.x());
  h_dy_->Fill(bestReco->y() - truthVtx.y());
  h_dz_->Fill(bestReco->z() - truthVtx.z());
  h_dxy_->Fill(std::hypot(bestReco->x(), bestReco->y()) - std::hypot(truthVtx.x(), truthVtx.y()));
  h_d3_->Fill(dv.value());
  h_cosT_->Fill(vertexCosT(*bestReco, truthVtx));
  h_nTracks_->Fill(std::distance(bestReco->tracks_begin(), bestReco->tracks_end()));

  TLorentzVector sum(0,0,0,0);
  for (auto it = bestReco->tracks_begin(); it != bestReco->tracks_end(); ++it) {
    reco::TrackRef trRef = it->castTo<reco::TrackRef>();
    if (!trRef.isNonnull()) continue;
    TLorentzVector p4;
    p4.SetPtEtaPhiM(trRef->pt(), trRef->eta(), trRef->phi(), 0.13957);
    sum += p4;
  }
  h_mass_->Fill(sum.M());
  h_xy_reco_vs_gen_->Fill(truthVtx.x(), truthVtx.y());
  h_z_reco_vs_gen_->Fill(truthVtx.z(), bestReco->z());
}

DEFINE_FWK_MODULE(GenRecoVertexComparer);
