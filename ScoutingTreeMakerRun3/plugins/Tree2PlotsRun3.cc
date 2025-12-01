// CMSSW EDAnalyzer that reads TTree from ScoutingTreeMakerRun3 and makes plots

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TH2F.h"
#include <vector>
#include <map>
#include <string>
#include <algorithm> // for std::find

class Tree2PlotsRun3 : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
	explicit Tree2PlotsRun3(const edm::ParameterSet&);
	~Tree2PlotsRun3() override = default;

	static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
	void beginJob() override;
	void analyze(const edm::Event&, const edm::EventSetup&) override;
	void endJob() override;

	// Config parameters
	const std::string inputFile_;
	const std::string inputTree_;
	const std::vector<std::vector<int>> cut_ntk_;
	const std::vector<double> cut_opening_angle_min_;
	const double required_invmass_;
	const double required_chi2_;
	const double required_dBV_min_;
	const double required_dBV_max_;
	const int PVBoundary1_;
	const int PVBoundary2_;

	// Histogram structure (same as PlotMaker)
	struct BranchHistos {
		TH1F *chi2norm, *pt, *eta, *phi, *mass, *nTracks;
		TH2F *xy_global, *xy_ref;
		TH1F *dBV_origin, *dBV_ref, *dBV_beamspot, *dBV_avgPV, *dBV_error;
		TH1F *angleMin, *angleMean, *angleMax;
		TH1F *barrel_mass, *barrel_dBV, *endcap_mass, *endcap_dBV;
		TH1F *regionA_mass, *regionA_dBV, *regionB_mass, *regionB_dBV, *regionC_mass, *regionC_dBV;
	};

	std::map<std::string, BranchHistos> histMap_;

	// File/tree state for processing
	TFile* file_ = nullptr;
	TTree* treePtr_ = nullptr;
	Long64_t totalEntries_ = 0;
	Long64_t currentEntry_ = 0;
	bool openedTree_ = false;
	bool processed_ = false;

	// --- NEW: persistent branch variables (used for SetBranchAddress in beginJob)
	Int_t nPV_br_; Int_t refType_br_;
	Float_t beamspot_x_br_, beamspot_y_br_, avgPV_x_br_, avgPV_y_br_;
	std::vector<float>* vtx_x_br = nullptr; std::vector<float>* vtx_y_br = nullptr;
	std::vector<float>* vtx_chi2norm_br = nullptr; std::vector<int>* vtx_ntracks_br = nullptr;
	std::vector<float>* vtx_pt_br = nullptr; std::vector<float>* vtx_eta_br = nullptr; std::vector<float>* vtx_phi_br = nullptr;
	std::vector<float>* vtx_mass_br = nullptr;
	std::vector<float>* vtx_dBV_origin_br = nullptr; std::vector<float>* vtx_dBV_ref_br = nullptr; std::vector<float>* vtx_dBV_err_br = nullptr;
	std::vector<float>* vtx_angleMin_br = nullptr; std::vector<float>* vtx_angleMean_br = nullptr; std::vector<float>* vtx_angleMax_br = nullptr;
	std::vector<int>*   vtx_pvRegion_br = nullptr;

	// Event histograms
	TH1F* event_nPrimaryVertices = nullptr;
	TH1F* event_nSelectedVertices = nullptr;
	TH2F* event_avgPV_vs_beamspot = nullptr;

	// Track-level histograms (vertex-associated tracks; TTree stores per-vertex track arrays)
	TH1F* trk_pt = nullptr;
	TH1F* trk_eta = nullptr;
	TH1F* trk_phi = nullptr;
	TH1F* trk_dxy_ref = nullptr;
	TH1F* trk_dxySig_ref = nullptr;
	TH1F* trk_dxyErr = nullptr;
	TH1F* trk_dxyErr_barrel = nullptr;
	TH1F* trk_dxyErr_endcap = nullptr;

	// Branch pointers for track-level nested vectors
	std::vector<std::vector<float>>* trk_pt_br = nullptr;
	std::vector<std::vector<float>>* trk_eta_br = nullptr;
	std::vector<std::vector<float>>* trk_phi_br = nullptr;
	std::vector<std::vector<float>>* trk_dxy_ref_br = nullptr;
	std::vector<std::vector<float>>* trk_dxyErr_br = nullptr;
	std::vector<std::vector<float>>* trk_dxySig_ref_br = nullptr;

	void processEntry(Long64_t iEntry, Int_t refType, Float_t beamspot_x, Float_t beamspot_y, Float_t avgPV_x, Float_t avgPV_y,
		std::vector<float>* vtx_x, std::vector<float>* vtx_y, std::vector<float>* vtx_chi2norm,
		std::vector<int>* vtx_ntracks, std::vector<float>* vtx_pt, std::vector<float>* vtx_eta,
		std::vector<float>* vtx_phi, std::vector<float>* vtx_mass, std::vector<float>* vtx_dBV_origin,
		std::vector<float>* vtx_dBV_ref, std::vector<float>* vtx_dBV_err,
		std::vector<float>* vtx_angleMin, std::vector<float>* vtx_angleMean, std::vector<float>* vtx_angleMax,
		std::vector<int>* vtx_pvRegion);
};

Tree2PlotsRun3::Tree2PlotsRun3(const edm::ParameterSet& iConfig):
	inputFile_(iConfig.getParameter<std::string>("inputFile")),
	inputTree_(iConfig.getParameter<std::string>("inputTree")),
	cut_ntk_([&iConfig]() {
		std::vector<std::vector<int>> result;
		auto vpset = iConfig.getParameter<std::vector<edm::ParameterSet>>("cut_ntk");
		for (const auto& pset : vpset) result.push_back(pset.getParameter<std::vector<int>>("values"));
		return result;
	}()),
	cut_opening_angle_min_(iConfig.getParameter<std::vector<double>>("cut_opening_angle_min")),
	required_invmass_(iConfig.getParameter<double>("required_invmass")),
	required_chi2_(iConfig.getParameter<double>("required_chi2")),
	required_dBV_min_(iConfig.getParameter<double>("required_dBV_min")),
	required_dBV_max_(iConfig.getParameter<double>("required_dBV_max")),
	PVBoundary1_(iConfig.getParameter<int>("PVBoundary1")),
	PVBoundary2_(iConfig.getParameter<int>("PVBoundary2"))
{
	usesResource("TFileService");
}

void Tree2PlotsRun3::beginJob() {
	edm::Service<TFileService> fs;
	TFileDirectory verticesDir = fs->mkdir("Vertices");
	TFileDirectory vtxSelDir = verticesDir.mkdir("Selected");

	// Create histograms for each ntk × angle branch (EXACTLY matching PlotMaker naming)
	for (size_t i_ntk = 0; i_ntk < cut_ntk_.size(); ++i_ntk) {
		const auto& ntk_set = cut_ntk_[i_ntk];

		// Build ntk branch name EXACTLY like PlotMaker does
		std::string ntkName;
		if (ntk_set.empty()) {
			ntkName = "ntk_any";
		} else if (ntk_set.size() == 1) {
			ntkName = "ntk_" + std::to_string(ntk_set[0]);
		} else {
			// For multiple values like [3, 4]: create "ntk_3_or_4"
			ntkName = "ntk";
			for (size_t j = 0; j < ntk_set.size(); ++j) {
				if (j > 0) ntkName += "_or_";
				ntkName += std::to_string(ntk_set[j]);
			}
		}

		TFileDirectory ntkDir = vtxSelDir.mkdir(ntkName);

		for (size_t i_angle = 0; i_angle < cut_opening_angle_min_.size(); ++i_angle) {
			double angle_cut = cut_opening_angle_min_[i_angle];
			std::string angleName = angle_cut < 0 ? "angle_any" : "angle_gt_" + std::to_string(int(angle_cut*100));

			TFileDirectory branchDir = ntkDir.mkdir(angleName);
			std::string key = ntkName + "/" + angleName;
			histMap_[key] = BranchHistos(); // create map entry
			auto& h = histMap_[key];

			TFileDirectory kinDir = branchDir.mkdir("Kinematics");
			h.chi2norm = kinDir.make<TH1F>("chi2norm", "#chi^{2}/ndof; #chi^{2}/ndof; Vertices", 200, 0, 20);
			h.pt = kinDir.make<TH1F>("pt", "p_{T}; p_{T} [GeV]; Vertices", 100, 0, 100);
			h.eta = kinDir.make<TH1F>("eta", "#eta; #eta; Vertices", 100, -5, 5);
			h.phi = kinDir.make<TH1F>("phi", "#phi; #phi; Vertices", 100, -3.14, 3.14);
			h.mass = kinDir.make<TH1F>("mass", "Mass; Mass [GeV]; Vertices", 100, 0, 10);
			h.nTracks = kinDir.make<TH1F>("nTracks", "N_{tracks}; N_{tracks}; Vertices", 50, 0, 50);

			TFileDirectory spatialDir = branchDir.mkdir("Spatial");
			h.xy_global = spatialDir.make<TH2F>("xy_global", "XY; X [cm]; Y [cm]", 800, -10, 10, 800, -10, 10);
			h.xy_ref = spatialDir.make<TH2F>("xy_ref", "XY ref; X-X_{ref} [cm]; Y-Y_{ref} [cm]", 800, -10, 10, 800, -10, 10);

			TFileDirectory distDir = branchDir.mkdir("Distance");
			h.dBV_origin = distDir.make<TH1F>("dBV_origin", "d_{BV} (origin); d_{BV} [cm]; Vertices", 200, 0, 10);
			h.dBV_ref = distDir.make<TH1F>("dBV_ref", "d_{BV} (ref); d_{BV} [cm]; Vertices", 200, 0, 10);
			h.dBV_error = distDir.make<TH1F>("dBV_error", "#sigma_{dBV}; #sigma_{dBV} [cm]; Vertices", 1000, 0, 0.1);

			TFileDirectory angleDir = branchDir.mkdir("OpeningAngles");
			h.angleMin = angleDir.make<TH1F>("min", "Min Angle; Min Angle [rad]; Vertices", 180, 0, 3.14159);
			h.angleMean = angleDir.make<TH1F>("mean", "Mean Angle; <Angle> [rad]; Vertices", 180, 0, 3.14159);
			h.angleMax = angleDir.make<TH1F>("max", "Max Angle; Max Angle [rad]; Vertices", 180, 0, 3.14159);

			TFileDirectory topoDir = branchDir.mkdir("Topology");
			TFileDirectory barrelDir = topoDir.mkdir("Barrel");
			h.barrel_mass = barrelDir.make<TH1F>("mass", "Mass (Barrel); Mass [GeV]; Vertices", 100, 0, 10);
			h.barrel_dBV = barrelDir.make<TH1F>("dBV", "d_{BV} (Barrel); d_{BV} [cm]; Vertices", 100, 0, 10);

			TFileDirectory endcapDir = topoDir.mkdir("Endcap");
			h.endcap_mass = endcapDir.make<TH1F>("mass", "Mass (Endcap); Mass [GeV]; Vertices", 100, 0, 10);
			h.endcap_dBV = endcapDir.make<TH1F>("dBV", "d_{BV} (Endcap); d_{BV} [cm]; Vertices", 100, 0, 10);

			if (PVBoundary1_ != -1) {
				TFileDirectory regionDir = branchDir.mkdir("PVRegions");
				TFileDirectory regADir = regionDir.mkdir("RegionA");
				h.regionA_mass = regADir.make<TH1F>("mass", "Mass (Region A); Mass [GeV]; Vertices", 100, 0, 10);
				h.regionA_dBV = regADir.make<TH1F>("dBV", "d_{BV} (Region A); d_{BV} [cm]; Vertices", 100, 0, 10);

				TFileDirectory regBDir = regionDir.mkdir("RegionB");
				h.regionB_mass = regBDir.make<TH1F>("mass", "Mass (Region B); Mass [GeV]; Vertices", 100, 0, 10);
				h.regionB_dBV = regBDir.make<TH1F>("dBV", "d_{BV} (Region B); d_{BV} [cm]; Vertices", 100, 0, 10);

				TFileDirectory regCDir = regionDir.mkdir("RegionC");
				h.regionC_mass = regCDir.make<TH1F>("mass", "Mass (Region C); Mass [GeV]; Vertices", 100, 0, 10);
				h.regionC_dBV = regCDir.make<TH1F>("dBV", "d_{BV} (Region C); d_{BV} [cm]; Vertices", 100, 0, 10);
			}
		}
	}

	// === Event histograms ===
	TFileDirectory eventDir = fs->mkdir("Event");
	event_nPrimaryVertices = eventDir.make<TH1F>("nPrimaryVertices", "Number of Primary Vertices; nPV; Events", 100, 0, 100);
	event_nSelectedVertices = eventDir.make<TH1F>("nSelectedVertices", "Number of Selected Vertices; N_{vtx}; Events", 200, 0, 200);
	event_avgPV_vs_beamspot = eventDir.make<TH2F>("avgPV_vs_beamspot", "AvgPV - Beamspot; #Delta x [cm]; #Delta y [cm]", 200, -0.5, 0.5, 200, -0.5, 0.5);

	// === Track histograms (vertex-associated tracks only, from the TTree) ===
	TFileDirectory tracksDir = fs->mkdir("Tracks");
	trk_pt = tracksDir.make<TH1F>("pt", "Track p_{T} (vertex-associated); p_{T} [GeV]; Tracks", 100, 0, 100);
	trk_eta = tracksDir.make<TH1F>("eta", "Track #eta (vertex-associated); #eta; Tracks", 100, -3, 3);
	trk_phi = tracksDir.make<TH1F>("phi", "Track #phi (vertex-associated); #phi; Tracks", 100, -3.14, 3.14);
	trk_dxy_ref = tracksDir.make<TH1F>("dxy_ref", "Track dxy wrt ref (vertex-associated); dxy [cm]; Tracks", 500, -5, 5);
	trk_dxySig_ref = tracksDir.make<TH1F>("dxySig_ref", "|dxy|/#sigma wrt ref (vertex-associated); |dxy|/#sigma; Tracks", 100, 0, 50);
	trk_dxyErr = tracksDir.make<TH1F>("dxyErr", "Track dxy Error (vertex-associated); #sigma_{dxy} [cm]; Tracks", 200, 0, 0.05);
	trk_dxyErr_barrel = tracksDir.make<TH1F>("dxyErr_barrel", "dxy Error Barrel; #sigma_{dxy} [cm]; Tracks", 200, 0, 0.05);
	trk_dxyErr_endcap = tracksDir.make<TH1F>("dxyErr_endcap", "dxy Error Endcap; #sigma_{dxy} [cm]; Tracks", 200, 0, 0.05);

	// Open ROOT file and locate TTree here (setup only; do not loop)
	file_ = TFile::Open(inputFile_.c_str(), "READ");
	if (!file_ || file_->IsZombie()) {
		edm::LogError("Tree2PlotsRun3") << "Cannot open input file: " << inputFile_;
		return;
	}

	// Try direct path first, then fallback
	treePtr_ = dynamic_cast<TTree*>( file_->Get(inputTree_.c_str()) );
	if (!treePtr_) {
		TDirectory* dir = file_->GetDirectory("scoutingTree");
		if (dir) treePtr_ = dynamic_cast<TTree*>(dir->Get("vertexTree"));
	}
	if (!treePtr_) {
		edm::LogError("Tree2PlotsRun3") << "Cannot find tree: " << inputTree_ << " (fallback tried scoutingTree/vertexTree)";
		file_->ls();
		openedTree_ = false;
		return;
	}

	// Set branch addresses ONCE and store pointers in member variables
	treePtr_->SetBranchAddress("nPV", &nPV_br_);
	treePtr_->SetBranchAddress("refType", &refType_br_);
	treePtr_->SetBranchAddress("beamspot_x", &beamspot_x_br_);
	treePtr_->SetBranchAddress("beamspot_y", &beamspot_y_br_);
	treePtr_->SetBranchAddress("avgPV_x", &avgPV_x_br_);
	treePtr_->SetBranchAddress("avgPV_y", &avgPV_y_br_);
	treePtr_->SetBranchAddress("vtx_x", &vtx_x_br);
	treePtr_->SetBranchAddress("vtx_y", &vtx_y_br);
	treePtr_->SetBranchAddress("vtx_chi2norm", &vtx_chi2norm_br);
	treePtr_->SetBranchAddress("vtx_ntracks", &vtx_ntracks_br);
	treePtr_->SetBranchAddress("vtx_pt", &vtx_pt_br);
	treePtr_->SetBranchAddress("vtx_eta", &vtx_eta_br);
	treePtr_->SetBranchAddress("vtx_phi", &vtx_phi_br);
	treePtr_->SetBranchAddress("vtx_mass", &vtx_mass_br);
	treePtr_->SetBranchAddress("vtx_dBV_origin", &vtx_dBV_origin_br);
	treePtr_->SetBranchAddress("vtx_dBV_ref", &vtx_dBV_ref_br);
	treePtr_->SetBranchAddress("vtx_dBV_err", &vtx_dBV_err_br);
	treePtr_->SetBranchAddress("vtx_angleMin", &vtx_angleMin_br);
	treePtr_->SetBranchAddress("vtx_angleMean", &vtx_angleMean_br);
	treePtr_->SetBranchAddress("vtx_angleMax", &vtx_angleMax_br);
	treePtr_->SetBranchAddress("vtx_pvRegion", &vtx_pvRegion_br);

	// Add track-level branch addresses (nested vectors created by ScoutingTreeMakerRun3)
	// these branches are vector<vector<float>> in the TTree
	treePtr_->SetBranchAddress("trk_pt", &trk_pt_br);
	treePtr_->SetBranchAddress("trk_eta", &trk_eta_br);
	treePtr_->SetBranchAddress("trk_phi", &trk_phi_br);
	treePtr_->SetBranchAddress("trk_dxy_ref", &trk_dxy_ref_br);
	treePtr_->SetBranchAddress("trk_dxyErr", &trk_dxyErr_br);
	treePtr_->SetBranchAddress("trk_dxySig_ref", &trk_dxySig_ref_br);

	totalEntries_ = treePtr_->GetEntries();
	currentEntry_ = 0;
	openedTree_ = true;
	edm::LogInfo("Tree2PlotsRun3") << "Opened tree with " << totalEntries_ << " entries";
}

void Tree2PlotsRun3::analyze(const edm::Event&, const edm::EventSetup&) {
	// Process exactly one TTree entry per framework analyze() call.
	if (processed_) return;
	if (!openedTree_ || !treePtr_) {
		edm::LogError("Tree2PlotsRun3") << "Tree not opened; nothing to process.";
		processed_ = true;
		return;
	}

	// If we've exhausted the TTree, mark done
	if (currentEntry_ >= totalEntries_) {
		edm::LogInfo("Tree2PlotsRun3") << "All TTree entries processed (" << totalEntries_ << ")";
		processed_ = true;
		return;
	}

	// Read a single TTree entry per framework analyze() call
	treePtr_->GetEntry(currentEntry_);

	// Process this entry
	processEntry(currentEntry_, refType_br_, beamspot_x_br_, beamspot_y_br_, avgPV_x_br_, avgPV_y_br_,
		vtx_x_br, vtx_y_br, vtx_chi2norm_br,
		vtx_ntracks_br, vtx_pt_br, vtx_eta_br, vtx_phi_br, vtx_mass_br,
		vtx_dBV_origin_br, vtx_dBV_ref_br, vtx_dBV_err_br,
		vtx_angleMin_br, vtx_angleMean_br, vtx_angleMax_br,
		vtx_pvRegion_br);

	++currentEntry_;

	// If we reached the end, mark processed_ so further analyze() calls are no-ops
	if (currentEntry_ >= totalEntries_) {
		edm::LogInfo("Tree2PlotsRun3") << "Finished processing all TTree entries (" << totalEntries_ << ")";
		processed_ = true;
	}
}

// processEntry: make sure ref_x/ref_y are computed locally
void Tree2PlotsRun3::processEntry(Long64_t /*iEntry*/, Int_t refType, Float_t beamspot_x, Float_t beamspot_y, Float_t avgPV_x, Float_t avgPV_y,
	std::vector<float>* vtx_x, std::vector<float>* vtx_y, std::vector<float>* vtx_chi2norm,
	std::vector<int>* vtx_ntracks, std::vector<float>* vtx_pt, std::vector<float>* vtx_eta,
	std::vector<float>* vtx_phi, std::vector<float>* vtx_mass, std::vector<float>* vtx_dBV_origin,
	std::vector<float>* vtx_dBV_ref, std::vector<float>* vtx_dBV_err,
	std::vector<float>* vtx_angleMin, std::vector<float>* vtx_angleMean, std::vector<float>* vtx_angleMax,
	std::vector<int>* vtx_pvRegion)
{
	float ref_x = (refType == 1) ? beamspot_x : avgPV_x;
	float ref_y = (refType == 1) ? beamspot_y : avgPV_y;

	int nSelVertices = 0;

	for (size_t iv = 0; iv < vtx_x->size(); ++iv) {
		int ntk = (*vtx_ntracks)[iv];
		float mass = (*vtx_mass)[iv];
		float chi2 = (*vtx_chi2norm)[iv];
		float dBV = (*vtx_dBV_ref)[iv];
		float minAngle = (*vtx_angleMin)[iv];

		bool vertexFilledAny = false; // track whether this vertex contributed to any branch

		for (size_t i_ntk = 0; i_ntk < cut_ntk_.size(); ++i_ntk) {
			const auto& ntk_set = cut_ntk_[i_ntk];
			bool ntk_pass = ntk_set.empty() || std::find(ntk_set.begin(), ntk_set.end(), ntk) != ntk_set.end();
			if (!ntk_pass) continue;

			// Build SAME ntk name as in beginJob
			std::string ntkName;
			if (ntk_set.empty()) {
				ntkName = "ntk_any";
			} else if (ntk_set.size() == 1) {
				ntkName = "ntk_" + std::to_string(ntk_set[0]);
			} else {
				ntkName = "ntk";
				for (size_t j = 0; j < ntk_set.size(); ++j) {
					if (j > 0) ntkName += "_or_";
					ntkName += std::to_string(ntk_set[j]);
				}
			}

			for (size_t i_angle = 0; i_angle < cut_opening_angle_min_.size(); ++i_angle) {
				double angle_cut = cut_opening_angle_min_[i_angle];
				if (angle_cut >= 0 && minAngle < angle_cut) continue;

				if (required_invmass_ > 0 && mass < required_invmass_) continue;
				if (required_chi2_ > 0 && chi2 > required_chi2_) continue;
				if (required_dBV_min_ > 0 && dBV < required_dBV_min_) continue;
				if (required_dBV_max_ > 0 && dBV > required_dBV_max_) continue;

				std::string angleName = angle_cut < 0 ? "angle_any" : "angle_gt_" + std::to_string(int(angle_cut*100));
				std::string key = ntkName + "/" + angleName;
				auto& h = histMap_[key];

				// Mark that this vertex passed at least one branch selection
				vertexFilledAny = true;

				h.chi2norm->Fill(chi2);
				h.pt->Fill((*vtx_pt)[iv]);
				h.eta->Fill((*vtx_eta)[iv]);
				h.phi->Fill((*vtx_phi)[iv]);
				h.mass->Fill(mass);
				h.nTracks->Fill(ntk);
				h.xy_global->Fill((*vtx_x)[iv], (*vtx_y)[iv]);
				h.xy_ref->Fill((*vtx_x)[iv] - ref_x, (*vtx_y)[iv] - ref_y);
				h.dBV_origin->Fill((*vtx_dBV_origin)[iv]);
				h.dBV_ref->Fill(dBV);
				h.dBV_error->Fill((*vtx_dBV_err)[iv]);
				h.angleMin->Fill(minAngle);
				h.angleMean->Fill((*vtx_angleMean)[iv]);
				h.angleMax->Fill((*vtx_angleMax)[iv]);

				if (std::fabs((*vtx_eta)[iv]) < 1.0) {
					h.barrel_mass->Fill(mass);
					h.barrel_dBV->Fill(dBV);
				} else {
					h.endcap_mass->Fill(mass);
					h.endcap_dBV->Fill(dBV);
				}

				int region = (*vtx_pvRegion)[iv];
				if (region == 0) { h.regionA_mass->Fill(mass); h.regionA_dBV->Fill(dBV); }
				else if (region == 1) { h.regionB_mass->Fill(mass); h.regionB_dBV->Fill(dBV); }
				else { h.regionC_mass->Fill(mass); h.regionC_dBV->Fill(dBV); }
			}
		}

		if (vertexFilledAny) ++nSelVertices;

		// --- Fill vertex-associated track histograms using trk_* branches if available ---
		if (trk_pt_br && trk_pt_br->size() > iv) {
			const auto& vtrks_pt = (*trk_pt_br)[iv];
			// safe-guard other track arrays sizes
			bool has_eta = trk_eta_br && trk_eta_br->size() > iv;
			bool has_phi = trk_phi_br && trk_phi_br->size() > iv;
			bool has_dxy_ref = trk_dxy_ref_br && trk_dxy_ref_br->size() > iv;
			bool has_dxyErr = trk_dxyErr_br && trk_dxyErr_br->size() > iv;
			bool has_dxySig = trk_dxySig_ref_br && trk_dxySig_ref_br->size() > iv;

			size_t ntrks = vtrks_pt.size();
			for (size_t it = 0; it < ntrks; ++it) {
				float pt = vtrks_pt[it];
				float eta = (has_eta ? (*trk_eta_br)[iv][it] : 0.0f);
				float phi = (has_phi ? (*trk_phi_br)[iv][it] : 0.0f);
				float dxyRef = (has_dxy_ref ? (*trk_dxy_ref_br)[iv][it] : -999.0f);
				float dxyErr = (has_dxyErr ? (*trk_dxyErr_br)[iv][it] : -999.0f);
				float dxySig = (has_dxySig ? (*trk_dxySig_ref_br)[iv][it] : -999.0f);

				trk_pt->Fill(pt);
				trk_eta->Fill(eta);
				trk_phi->Fill(phi);
				if (has_dxy_ref) trk_dxy_ref->Fill(dxyRef);
				if (has_dxySig) trk_dxySig_ref->Fill(dxySig);
				if (has_dxyErr) {
					trk_dxyErr->Fill(dxyErr);
					if (std::fabs(eta) < 1.0) trk_dxyErr_barrel->Fill(dxyErr);
					else trk_dxyErr_endcap->Fill(dxyErr);
				}
			}
		}
	} // end vertices loop

	// Fill event-level histograms
	event_nPrimaryVertices->Fill(static_cast<double>(nPV_br_));
	event_nSelectedVertices->Fill(static_cast<double>(nSelVertices));
	// avgPV - beamspot
	event_avgPV_vs_beamspot->Fill(avgPV_x_br_ - beamspot_x_br_, avgPV_y_br_ - beamspot_y_br_);
}

void Tree2PlotsRun3::endJob() {
	if (file_) {
		file_->Close();
		file_ = nullptr;
		treePtr_ = nullptr;
	}
}

void Tree2PlotsRun3::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
	edm::ParameterSetDescription desc;
	desc.add<std::string>("inputFile", "ScoutingTree_Output.root");
	desc.add<std::string>("inputTree", "scoutingTree/vertexTree");
	edm::ParameterSetDescription ntkPSet;
	ntkPSet.add<std::vector<int>>("values", {});
	desc.addVPSet("cut_ntk", ntkPSet, {});
	desc.add<std::vector<double>>("cut_opening_angle_min", {-1.0});
	desc.add<double>("required_invmass", -1.0);
	desc.add<double>("required_chi2", -1.0);
	desc.add<double>("required_dBV_min", -1.0);
	desc.add<double>("required_dBV_max", -1.0);
	desc.add<int>("PVBoundary1", -1);
	desc.add<int>("PVBoundary2", -1);
	descriptions.add("tree2PlotsRun3", desc);
}

DEFINE_FWK_MODULE(Tree2PlotsRun3);
