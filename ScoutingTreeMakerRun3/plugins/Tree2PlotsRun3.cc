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
    
    void processTTreeFile();  // NEW: process TTree in beginJob
    bool processed_ = false;
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
    
    // NOW process the TTree immediately after creating histograms
    processTTreeFile();
}

void Tree2PlotsRun3::analyze(const edm::Event&, const edm::EventSetup&) {
    // Do nothing - all processing happens in beginJob
}

void Tree2PlotsRun3::processTTreeFile() {
    if (processed_) return;
    
    edm::LogInfo("Tree2PlotsRun3") << "Opening TTree file: " << inputFile_;
    
    TFile* file = TFile::Open(inputFile_.c_str(), "READ");
    if (!file || file->IsZombie()) {
        edm::LogError("Tree2PlotsRun3") << "Cannot open file: " << inputFile_;
        processed_ = true;
        return;
    }
    
    // The TTree is inside a TDirectory created by TFileService
    // Need to navigate: file -> "scoutingTree" directory -> "vertexTree" TTree
    TDirectory* dir = file->GetDirectory("scoutingTree");
    if (!dir) {
        edm::LogError("Tree2PlotsRun3") << "Cannot find TDirectory 'scoutingTree' in file: " << inputFile_;
        edm::LogError("Tree2PlotsRun3") << "Available objects in file:";
        file->ls();
        file->Close();
        processed_ = true;
        return;
    }
    
    TTree* tree = (TTree*)dir->Get(inputTree_.c_str());
    if (!tree) {
        edm::LogError("Tree2PlotsRun3") << "Cannot find tree: " << inputTree_ << " in TDirectory 'scoutingTree'";
        edm::LogError("Tree2PlotsRun3") << "Available objects in directory:";
        dir->ls();
        file->Close();
        processed_ = true;
        return;
    }
    
    edm::LogInfo("Tree2PlotsRun3") << "Found TTree with " << tree->GetEntries() << " entries";
    
    // Set branch addresses
    Int_t nPV, refType;
    Float_t beamspot_x, beamspot_y, avgPV_x, avgPV_y;
    std::vector<float> *vtx_x=0, *vtx_y=0, *vtx_chi2norm=0, *vtx_pt=0, *vtx_eta=0, *vtx_phi=0, *vtx_mass=0;
    std::vector<float> *vtx_dBV_origin=0, *vtx_dBV_ref=0, *vtx_dBV_err=0, *vtx_angleMin=0, *vtx_angleMean=0, *vtx_angleMax=0;
    std::vector<int> *vtx_ntracks=0, *vtx_pvRegion=0;
    
    tree->SetBranchAddress("nPV", &nPV);
    tree->SetBranchAddress("refType", &refType);
    tree->SetBranchAddress("beamspot_x", &beamspot_x);
    tree->SetBranchAddress("beamspot_y", &beamspot_y);
    tree->SetBranchAddress("avgPV_x", &avgPV_x);
    tree->SetBranchAddress("avgPV_y", &avgPV_y);
    tree->SetBranchAddress("vtx_x", &vtx_x);
    tree->SetBranchAddress("vtx_y", &vtx_y);
    tree->SetBranchAddress("vtx_chi2norm", &vtx_chi2norm);
    tree->SetBranchAddress("vtx_ntracks", &vtx_ntracks);
    tree->SetBranchAddress("vtx_pt", &vtx_pt);
    tree->SetBranchAddress("vtx_eta", &vtx_eta);
    tree->SetBranchAddress("vtx_phi", &vtx_phi);
    tree->SetBranchAddress("vtx_mass", &vtx_mass);
    tree->SetBranchAddress("vtx_dBV_origin", &vtx_dBV_origin);
    tree->SetBranchAddress("vtx_dBV_ref", &vtx_dBV_ref);
    tree->SetBranchAddress("vtx_dBV_err", &vtx_dBV_err);
    tree->SetBranchAddress("vtx_angleMin", &vtx_angleMin);
    tree->SetBranchAddress("vtx_angleMean", &vtx_angleMean);
    tree->SetBranchAddress("vtx_angleMax", &vtx_angleMax);
    tree->SetBranchAddress("vtx_pvRegion", &vtx_pvRegion);
    
    // Process all entries
    Long64_t nEntries = tree->GetEntries();
    edm::LogInfo("Tree2PlotsRun3") << "Processing " << nEntries << " TTree entries";
    
    for (Long64_t i = 0; i < nEntries; ++i) {
        if (i % 1000 == 0) {
            edm::LogInfo("Tree2PlotsRun3") << "  Processed " << i << " / " << nEntries << " entries";
        }
        
        tree->GetEntry(i);
        float ref_x = (refType == 1) ? beamspot_x : avgPV_x;
        float ref_y = (refType == 1) ? beamspot_y : avgPV_y;
        
        for (size_t iv = 0; iv < vtx_x->size(); ++iv) {
            int ntk = (*vtx_ntracks)[iv];
            float mass = (*vtx_mass)[iv];
            float chi2 = (*vtx_chi2norm)[iv];
            float dBV = (*vtx_dBV_ref)[iv];
            float minAngle = (*vtx_angleMin)[iv];
            
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
        }
    }
    
    edm::LogInfo("Tree2PlotsRun3") << "Finished processing TTree";
    file->Close();
    processed_ = true;
}

void Tree2PlotsRun3::endJob() {}

void Tree2PlotsRun3::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    desc.add<std::string>("inputFile", "ScoutingTree_Output.root");
    desc.add<std::string>("inputTree", "vertexTree");
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
