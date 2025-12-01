/*
===============================================================================
Tree2PlotsRun3
===============================================================================
Description:
  CMSSW EDAnalyzer that reads the TTree produced by ScoutingTreeMakerRun3
  (vertexTree) and produces the same set of histograms as ScoutingPlotMakerRun3.
  The module performs on-the-fly computations of derived quantities (vertex
  pT/eta/phi/mass, dBV wrt chosen reference, opening angles, track IPs) from
  the primitive information stored in the TTree.

Purpose:
  - Allow rapid re-analysis of the stored TTree with different cuts and branches.
  - Reproduce PlotMaker histograms using only data stored in the TTree.
  - Keep analysis logic in one place (Tree2Plots) so PlotMaker and TreeMaker
    responsibilities are separated.

What is read (input):
  - ROOT file containing TDirectory "scoutingTree" and TTree "vertexTree".
  - Event-level scalars: run, lumi, event, nPV, beamspot_x/y, avgPV_x/y, refType.
  - Vertex-level primitive vectors: vtx_x/y/z, vtx_xErr/yErr/zErr, vtx_chi2norm,
    vtx_ntracks, vtx_pvRegion.
  - Track-level nested vectors per vertex: trk_pt, trk_eta, trk_phi,
    trk_dxy_origin (dxy wrt global 0), trk_dxyErr, hit counts, etc.

What is produced (output):
  - Histograms organized under Vertices/Selected/<ntk branch>/<angle branch>/...
    matching ScoutingPlotMakerRun3 layout (Kinematics, Spatial, Distance,
    OpeningAngles, Topology, PVRegions).
  - Event-level and track-level histograms (Event, Tracks directories).

Selection & computation policy:
  - Tree2Plots computes derived quantities from primitives:
    * Vertex 4-vector and invariant mass from stored per-vertex track kinematics
      (pion mass assumption).
    * dBV wrt reference computed from stored vertex (x,y) and chosen ref (beamspot / avgPV).
    * dBV uncertainty computed from stored vtx_xErr, vtx_yErr (propagated in 2D).
    * Opening angles computed from per-track 3-vectors built from stored (pt,eta,phi).
  - Cuts applied are fully configurable via the analyzer ParameterSet:
    cut_ntk, cut_opening_angle_min, required_invmass, required_chi2,
    required_dBV_min, required_dBV_max, PVBoundary1, PVBoundary2.
  - The analyzer processes one TTree entry per framework analyze() call; use
    process.maxEvents in the python config to control how many TTree entries are read.

Usage:
  - Configure Tree2PlotsRun3 via a CMSSW python config (Tree2PlotsConfig.py).
  - Example: set process.source = EmptySource and process.maxEvents to the
    number of TTree entries to process (one analyze() per entry).
  - Ensure inputFile and inputTree parameters point to the correct file/path.

Notes / Caveats:
  - Some exact signed IP computations require full helix parameters; the TTree
    stores a pragmatic set of track primitives. If exact IP reproduction is
    required, consider adding track helix parameters to the TreeMaker (optional).
  - All distance computations used for comparison with Vertexer are 2D (XY)
    unless explicitly toggled; this matches Vertexer choices by default.
===============================================================================
*/

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
#include "TLorentzVector.h"
#include "TVector3.h"
#include <cmath>
#include <vector>
#include <map>
#include <string>
#include <algorithm> // for std::find
#include <limits>    // NEW: for quiet_NaN()
#include <cstdlib>   // ADDED: for std::exit()

// File-local sentinel for missing floats written by the TreeMaker (NaN)
namespace {
    constexpr float kMissingFloat = std::numeric_limits<float>::quiet_NaN();
}

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

	// --- persistent vertex branch pointers (only primitives that TreeMaker stores) ---
	std::vector<float>* vtx_x_br = nullptr;
	std::vector<float>* vtx_y_br = nullptr;
	std::vector<float>* vtx_xErr_br = nullptr;
	std::vector<float>* vtx_yErr_br = nullptr;
	std::vector<float>* vtx_chi2norm_br = nullptr;
	std::vector<int>*   vtx_ntracks_br = nullptr;
	std::vector<int>*   vtx_pvRegion_br = nullptr;

	// Track nested pointers (one declaration only — removed duplicate declarations)
	std::vector<std::vector<float>>* trk_pt_br = nullptr;
	std::vector<std::vector<float>>* trk_eta_br = nullptr;
	std::vector<std::vector<float>>* trk_phi_br = nullptr;
	std::vector<std::vector<float>>* trk_dxy_origin_br = nullptr;
	std::vector<std::vector<float>>* trk_dxyErr_br = nullptr;
	std::vector<std::vector<int>>*   trk_nPixelHits_br = nullptr;
	std::vector<std::vector<int>>*   trk_nStripHits_br = nullptr;
	std::vector<std::vector<int>>*   trk_nTrackerLayers_br = nullptr;

	// Event histograms (DECLARATIONS ADDED — required by beginJob())
	TH1F* event_nPrimaryVertices = nullptr;
	TH1F* event_nSelectedVertices = nullptr;
	TH2F* event_avgPV_vs_beamspot = nullptr;

	// Histogram members (unchanged)
	TH1F* trk_pt = nullptr;
	TH1F* trk_eta = nullptr;
	TH1F* trk_phi = nullptr;
	TH1F* trk_dxy_ref = nullptr;
	TH1F* trk_dxySig_ref = nullptr;
	TH1F* trk_dxyErr = nullptr;
	TH1F* trk_dxyErr_barrel = nullptr;
	TH1F* trk_dxyErr_endcap = nullptr;

	void processEntry(Long64_t iEntry, Int_t refType, Float_t beamspot_x, Float_t beamspot_y, Float_t avgPV_x, Float_t avgPV_y,
		std::vector<float>* vtx_x, std::vector<float>* vtx_y, std::vector<float>* vtx_xErr, std::vector<float>* vtx_yErr,
		std::vector<float>* vtx_chi2norm, std::vector<int>* vtx_ntracks, std::vector<int>* vtx_pvRegion,
		std::vector<std::vector<float>>* trk_pt_br_in, std::vector<std::vector<float>>* trk_eta_br_in, std::vector<std::vector<float>>* trk_phi_br_in,
		std::vector<std::vector<float>>* trk_dxy_origin_br_in, std::vector<std::vector<float>>* trk_dxyErr_br_in,
		std::vector<std::vector<int>>* trk_nPixelHits_br_in, std::vector<std::vector<int>>* trk_nStripHits_br_in, std::vector<std::vector<int>>* trk_nTrackerLayers_br_in);
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

	// --- Set branch addresses only for branches that ScoutingTreeMakerRun3 actually writes ---
	treePtr_->SetBranchAddress("nPV", &nPV_br_);
	treePtr_->SetBranchAddress("refType", &refType_br_);
	treePtr_->SetBranchAddress("beamspot_x", &beamspot_x_br_);
	treePtr_->SetBranchAddress("beamspot_y", &beamspot_y_br_);
	treePtr_->SetBranchAddress("avgPV_x", &avgPV_x_br_);
	treePtr_->SetBranchAddress("avgPV_y", &avgPV_y_br_);

	// Vertex primitives (match TreeMaker)
	treePtr_->SetBranchAddress("vtx_x", &vtx_x_br);
	treePtr_->SetBranchAddress("vtx_y", &vtx_y_br);
	treePtr_->SetBranchAddress("vtx_xErr", &vtx_xErr_br);
	treePtr_->SetBranchAddress("vtx_yErr", &vtx_yErr_br);
	treePtr_->SetBranchAddress("vtx_chi2norm", &vtx_chi2norm_br);
	treePtr_->SetBranchAddress("vtx_ntracks", &vtx_ntracks_br);
	treePtr_->SetBranchAddress("vtx_pvRegion", &vtx_pvRegion_br);

	// Track nested vectors (match TreeMaker)
	treePtr_->SetBranchAddress("trk_pt", &trk_pt_br);
	treePtr_->SetBranchAddress("trk_eta", &trk_eta_br);
	treePtr_->SetBranchAddress("trk_phi", &trk_phi_br);
	treePtr_->SetBranchAddress("trk_dxy_origin", &trk_dxy_origin_br);
	treePtr_->SetBranchAddress("trk_dxyErr", &trk_dxyErr_br);
	treePtr_->SetBranchAddress("trk_nPixelHits", &trk_nPixelHits_br);
	treePtr_->SetBranchAddress("trk_nStripHits", &trk_nStripHits_br);
	treePtr_->SetBranchAddress("trk_nTrackerLayers", &trk_nTrackerLayers_br);

	totalEntries_ = treePtr_->GetEntries();
	currentEntry_ = 0;
	openedTree_ = true;
	edm::LogInfo("Tree2PlotsRun3") << "Opened tree with " << totalEntries_ << " entries";
}

void Tree2PlotsRun3::analyze(const edm::Event&, const edm::EventSetup&) {
	// Process exactly one TTree entry per framework analyze() call.
	if (processed_) return;
	if (!openedTree_ || !treePtr_) { processed_ = true; return; }

	if (currentEntry_ >= totalEntries_) { processed_ = true; return; }

	// Read a single TTree entry per framework analyze() call
	treePtr_->GetEntry(currentEntry_);

	// SAFETY: pass only the branches we know exist (avoid dereferencing missing pointers)
	processEntry(currentEntry_,
	             refType_br_, beamspot_x_br_, beamspot_y_br_, avgPV_x_br_, avgPV_y_br_,
	             vtx_x_br, vtx_y_br, vtx_xErr_br, vtx_yErr_br, vtx_chi2norm_br,
	             vtx_ntracks_br, vtx_pvRegion_br,
	             trk_pt_br, trk_eta_br, trk_phi_br, trk_dxy_origin_br, trk_dxyErr_br,
	             trk_nPixelHits_br, trk_nStripHits_br, trk_nTrackerLayers_br);

	++currentEntry_;
	if (currentEntry_ >= totalEntries_) { 
		processed_ = true;

		// --- FINALIZE: flush TFileService/ROOT output and exit when tree exhausted ---
		// Minimal, explicit shutdown so framework run with maxEvents = -1 will stop
		// as soon as all TTree entries have been consumed.

		// --- REPLACED: avoid calling gFile->Write() because gFile may point to a
		// file opened in READ mode (input TTree). Instead, flush the TFileService
		// output file if available and safe to write.
		{
			edm::Service<TFileService> fs;
			TFile* outFile = nullptr;
			if (fs.isAvailable()) {
				// TFileService::file() returns a TFile reference; take its address to get a TFile*
				outFile = &(fs->file());
			}

			// If both files exist and have the same name, do NOT attempt to write the file
			// (this indicates the user is trying to read and write the same ROOT file in one job).
			bool sameName = false;
			if (outFile && file_) {
				const char* outName = outFile->GetName();
				const char* inName  = file_->GetName();
				if (outName && inName && std::string(outName) == std::string(inName)) sameName = true;
			}

			if (outFile) {
				if (sameName) {
					edm::LogError("Tree2PlotsRun3") << "Input file equals TFileService output file ('"
						<< (outFile ? outFile->GetName() : std::string("unknown")) 
						<< "'). Refusing to Write/Close the file to avoid ROOT write-on-read errors.";
				} else {
					edm::LogInfo("Tree2PlotsRun3") << "Flushing TFileService output file before exiting (processed all " << totalEntries_ << " entries).";
					outFile->Write();
					outFile->Close();
				}
			} else {
				edm::LogInfo("Tree2PlotsRun3") << "No TFileService output file available to flush.";
			}

			// Close the input file if open
			if (file_) {
				file_->Close();
				file_ = nullptr;
				treePtr_ = nullptr;
			}
		}

		std::exit(0); // immediate, controlled process termination
	}
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

// define static const outside the class
// const float Tree2PlotsRun3::kMissingFloat = std::numeric_limits<float>::quiet_NaN();

DEFINE_FWK_MODULE(Tree2PlotsRun3);

void Tree2PlotsRun3::processEntry(Long64_t /*iEntry*/, Int_t refType, Float_t beamspot_x, Float_t beamspot_y, Float_t avgPV_x, Float_t avgPV_y,
                                  std::vector<float>* vtx_x, std::vector<float>* vtx_y,
                                  std::vector<float>* vtx_xErr, std::vector<float>* vtx_yErr,
                                  std::vector<float>* vtx_chi2norm,
                                  std::vector<int>* vtx_ntracks, std::vector<int>* vtx_pvRegion,
                                  std::vector<std::vector<float>>* trk_pt_br_in, std::vector<std::vector<float>>* trk_eta_br_in, std::vector<std::vector<float>>* trk_phi_br_in,
                                  std::vector<std::vector<float>>* trk_dxy_origin_br_in, std::vector<std::vector<float>>* trk_dxyErr_br_in,
                                  std::vector<std::vector<int>>* trk_nPixelHits_br_in, std::vector<std::vector<int>>* trk_nStripHits_br_in, std::vector<std::vector<int>>* trk_nTrackerLayers_br_in)
 {
	// Basic sanity checks: bail out early if fundamental branches are missing
	if (!vtx_x || !vtx_y || !vtx_chi2norm || !vtx_ntracks) return;

	const float ref_x = (refType == 1) ? beamspot_x : avgPV_x;
	const float ref_y = (refType == 1) ? beamspot_y : avgPV_y;

	// iterate over vertices safely
	const size_t nV = vtx_x->size();
	for (size_t iv = 0; iv < nV; ++iv) {
		// guard against inconsistent vector sizes
		if (iv >= vtx_y->size() || iv >= vtx_chi2norm->size() || iv >= vtx_ntracks->size()) continue;

		int ntk = (*vtx_ntracks)[iv];
		float chi2 = (*vtx_chi2norm)[iv];
		float vx = (*vtx_x)[iv];
		float vy = (*vtx_y)[iv];

		// compute dBV wrt reference using vertex primitive coords (2D)
		float dBV_ref = std::hypot(vx - ref_x, vy - ref_y);
		// dBV_err from vertex x/y errors if present
		float vxErr = (vtx_xErr && iv < vtx_xErr->size()) ? (*vtx_xErr)[iv] : 0.0f;
		float vyErr = (vtx_yErr && iv < vtx_yErr->size()) ? (*vtx_yErr)[iv] : 0.0f;
		float dBV_err = std::hypot(vxErr, vyErr);

		// Build vertex 4-vector from per-vertex tracks (pion mass assumption)
		TLorentzVector sumVec(0,0,0,0);
		std::vector<TVector3> trackVecs;
		if (trk_pt_br_in && trk_pt_br_in->size() > iv) {
			const auto& pts = (*trk_pt_br_in)[iv];
			const auto& etas = (trk_eta_br_in && trk_eta_br_in->size() > iv) ? (*trk_eta_br_in)[iv] : std::vector<float>();
			const auto& phis = (trk_phi_br_in && trk_phi_br_in->size() > iv) ? (*trk_phi_br_in)[iv] : std::vector<float>();
			for (size_t it = 0; it < pts.size(); ++it) {
				double pt = pts[it];
				double eta = (etas.size() > it) ? etas[it] : 0.0;
				double phi = (phis.size() > it) ? phis[it] : 0.0;
				TLorentzVector tv; tv.SetPtEtaPhiM(pt, eta, phi, 0.13957);
				sumVec += tv;
				trackVecs.emplace_back(tv.Px(), tv.Py(), tv.Pz());
			}
		}

		float mass = static_cast<float>(sumVec.M());
		float vpt = static_cast<float>(sumVec.Pt());
		float veta = static_cast<float>(sumVec.Eta());
		float vphi = static_cast<float>(sumVec.Phi());

		// Opening angles
		double minAngle = 0.0, meanAngle = 0.0, maxAngle = 0.0;
		if (trackVecs.size() > 1) {
			double sumAngles = 0.0; int npairs = 0;
			minAngle = 1e9; maxAngle = 0.0;
			for (size_t i=0;i<trackVecs.size();++i) for (size_t j=i+1;j<trackVecs.size();++j) {
				double ang = trackVecs[i].Angle(trackVecs[j]);
				sumAngles += ang; ++npairs;
				minAngle = std::min(minAngle, ang);
				maxAngle = std::max(maxAngle, ang);
			}
			meanAngle = (npairs>0) ? sumAngles/npairs : 0.0;
		}

		// Apply configured selection cuts (same as before)
		bool filledAny = false;
		for (size_t i_ntk = 0; i_ntk < cut_ntk_.size(); ++i_ntk) {
			const auto& ntk_set = cut_ntk_[i_ntk];
			bool ntk_pass = ntk_set.empty() || std::find(ntk_set.begin(), ntk_set.end(), ntk) != ntk_set.end();
			if (!ntk_pass) continue;
			for (size_t i_angle = 0; i_angle < cut_opening_angle_min_.size(); ++i_angle) {
				double angle_cut = cut_opening_angle_min_[i_angle];
				if (angle_cut >= 0 && meanAngle < angle_cut) continue;
				if (required_invmass_ > 0 && mass < required_invmass_) continue;
				if (required_chi2_ > 0 && chi2 > required_chi2_) continue;
				if (required_dBV_min_ > 0 && dBV_ref < required_dBV_min_) continue;
				if (required_dBV_max_ > 0 && dBV_ref > required_dBV_max_) continue;

				std::string angleName = angle_cut < 0 ? "angle_any" : "angle_gt_" + std::to_string(int(angle_cut*100));
				std::string ntkName;
				if (ntk_set.empty()) ntkName = "ntk_any";
				else if (ntk_set.size() == 1) ntkName = "ntk_" + std::to_string(ntk_set[0]);
				else {
					ntkName = "ntk";
					for (size_t j = 0; j < ntk_set.size(); ++j) { if (j>0) ntkName += "_or_"; ntkName += std::to_string(ntk_set[j]); }
				}
				std::string key = ntkName + "/" + angleName;
				auto& h = histMap_[key];

				// Fill histos (same as before)
				h.chi2norm->Fill(chi2);
				h.pt->Fill(vpt);
				h.eta->Fill(veta);
				h.phi->Fill(vphi);
				h.mass->Fill(mass);
				h.nTracks->Fill(ntk);
				h.xy_global->Fill(vx, vy);
				h.xy_ref->Fill(vx - ref_x, vy - ref_y);
				h.dBV_origin->Fill(std::hypot(vx, vy));
				h.dBV_ref->Fill(dBV_ref);
				h.dBV_error->Fill(dBV_err);
				h.angleMin->Fill(minAngle);
				h.angleMean->Fill(meanAngle);
				h.angleMax->Fill(maxAngle);

				if (std::fabs(veta) < 1.0) { h.barrel_mass->Fill(mass); h.barrel_dBV->Fill(dBV_ref); }
				else { h.endcap_mass->Fill(mass); h.endcap_dBV->Fill(dBV_ref); }

				int region = (vtx_pvRegion && iv < vtx_pvRegion->size()) ? (*vtx_pvRegion)[iv] : 0;
				if (region == 0) { h.regionA_mass->Fill(mass); h.regionA_dBV->Fill(dBV_ref); }
				else if (region == 1) { h.regionB_mass->Fill(mass); h.regionB_dBV->Fill(dBV_ref); }
				else { h.regionC_mass->Fill(mass); h.regionC_dBV->Fill(dBV_ref); }

				filledAny = true;
			}
		}

		// mark 'filledAny' as intentionally unused for now (silence -Werror=unused-but-set-variable)
		(void)filledAny;

		// Fill per-vertex associated track histograms from nested track arrays (guarded)
		if (trk_pt_br_in && trk_pt_br_in->size() > iv) {
			const auto& vtrks_pt = (*trk_pt_br_in)[iv];
			bool has_eta = trk_eta_br_in && trk_eta_br_in->size() > iv;
			bool has_phi = trk_phi_br_in && trk_phi_br_in->size() > iv;
			bool has_dxy_origin = trk_dxy_origin_br_in && trk_dxy_origin_br_in->size() > iv;
			bool has_dxyErr = trk_dxyErr_br_in && trk_dxyErr_br_in->size() > iv;

			for (size_t it = 0; it < vtrks_pt.size(); ++it) {
				float pt = vtrks_pt[it];
				float eta = (has_eta ? (*trk_eta_br_in)[iv][it] : 0.0f);
				float phi = (has_phi ? (*trk_phi_br_in)[iv][it] : 0.0f);
				float dxyOrigin = (has_dxy_origin ? (*trk_dxy_origin_br_in)[iv][it] : std::numeric_limits<float>::quiet_NaN());
				float dxyErr = (has_dxyErr ? (*trk_dxyErr_br_in)[iv][it] : std::numeric_limits<float>::quiet_NaN());

				// Fill the histogram members (trk_pt, trk_eta, trk_phi, ...), not the branch vectors
				trk_pt->Fill(pt);
				trk_eta->Fill(eta);
				trk_phi->Fill(phi);
				if (std::isfinite(dxyOrigin)) trk_dxy_ref->Fill(dxyOrigin);
				if (std::isfinite(dxyErr) && dxyErr > 0.0f) {
					trk_dxyErr->Fill(dxyErr);
					if (std::fabs(eta) < 1.0) trk_dxyErr_barrel->Fill(dxyErr);
					else trk_dxyErr_endcap->Fill(dxyErr);
				}
			}
		}
	} // end vertices
}
