#include "Run3ScoutingAnalysisTools/interface/VertexPlotter.h"
#include "FWCore/Framework/interface/MakerMacros.h"

VertexPlotter::VertexPlotter(const edm::ParameterSet& iConfig):
    verticesToken(consumes<std::vector<reco::Vertex>>(iConfig.getParameter<edm::InputTag>("vertices"))),
    beamspotToken(consumes<reco::BeamSpot>(iConfig.getParameter<edm::InputTag>("beamspot")))
{
    edm::Service<TFileService> fs;
    h_noFiltVert_x_y = fs->make<TH2F>("noFiltVert_x_y", "All Vertex Positions; X Position [cm]; Y Position [cm]", 100, -15, 15, 100, -15, 15);
}

void VertexPlotter::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
    edm::Handle<std::vector<reco::Vertex>> verticesH;
    iEvent.getByToken(verticesToken, verticesH);

    edm::Handle<reco::BeamSpot> beamspotH;
    iEvent.getByToken(beamspotToken, beamspotH);
    const reco::BeamSpot& beamspot = *beamspotH;

    for (const auto& vertex : *verticesH) {
        h_noFiltVert_x_y->Fill(vertex.x() - beamspot.x0(), vertex.y() - beamspot.y0());
    }
}

void VertexPlotter::beginJob() {}

void VertexPlotter::endJob() {}

void VertexPlotter::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    desc.add<edm::InputTag>("vertices", edm::InputTag("offlinePrimaryVertices"));
    desc.add<edm::InputTag>("beamspot", edm::InputTag("offlineBeamSpot"));
    descriptions.add("vertexPlotter", desc);
}

DEFINE_FWK_MODULE(VertexPlotter);