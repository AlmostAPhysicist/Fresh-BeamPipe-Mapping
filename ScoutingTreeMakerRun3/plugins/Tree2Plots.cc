// ============================================================================
// Tree2Plots: Standalone ROOT analyzer to convert TTree output to histograms
// ============================================================================
//
// PURPOSE:
//   Read ScoutingTree_Output.root (from ScoutingTreeMakerRun3) and produce
//   the same histograms as ScoutingPlotMakerRun3, applying configurable cuts
//
// USAGE:
//   root -l -b -q 'Tree2Plots.cc+("Tree2PlotsConfig.txt","input.root","output.root")'
//
// INPUTS:
//   - Config file: Tree2PlotsConfig.txt (defines ntk, angle, and other cuts)
//   - Input TTree: ScoutingTree_Output.root
//   - Output file: Tree2Plots_Output.root (histograms organized in TDirectories)
//
// ============================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cmath>

#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TDirectory.h"
#include "TLorentzVector.h"
#include "TVector3.h"

// Configuration structure
struct PlotConfig {
    std::vector<std::vector<int>> cut_ntk;
    std::vector<double> cut_opening_angle_min;
    double required_invmass;
    double required_chi2;
    double required_dBV_min;
    double required_dBV_max;
    int PVBoundary1;
    int PVBoundary2;
};

// Histogram structure (mirrors ScoutingPlotMakerRun3)
struct HistogramSet {
    TH1F* chi2norm;
    TH1F* pt;
    TH1F* eta;
    TH1F* phi;
    TH1F* mass;
    TH1F* nTracks;
    TH2F* xy_global;
    TH2F* xy_ref;
    TH1F* dBV_origin;
    TH1F* dBV_ref;
    TH1F* dBV_beamspot;
    TH1F* dBV_avgPV;
    TH1F* dBV_error;
    TH1F* angleMin;
    TH1F* angleMean;
    TH1F* angleMax;
    // Topology
    TH1F* barrel_mass;
    TH1F* barrel_dBV;
    TH1F* endcap_mass;
    TH1F* endcap_dBV;
    // PV regions
    TH1F* regionA_mass;
    TH1F* regionA_dBV;
    TH1F* regionB_mass;
    TH1F* regionB_dBV;
    TH1F* regionC_mass;
    TH1F* regionC_dBV;
};

// Read configuration file
PlotConfig readConfig(const std::string& configFile) {
    PlotConfig config;
    std::ifstream infile(configFile);
    std::string line;
    
    while (std::getline(infile, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string key;
        iss >> key;
        
        if (key == "cut_ntk") {
            std::string values;
            std::getline(iss, values);
            std::istringstream viss(values);
            std::vector<int> ntk_set;
            int val;
            while (viss >> val) ntk_set.push_back(val);
            config.cut_ntk.push_back(ntk_set);
        } else if (key == "cut_opening_angle_min") {
            double val;
            while (iss >> val) config.cut_opening_angle_min.push_back(val);
        } else if (key == "required_invmass") {
            iss >> config.required_invmass;
        } else if (key == "required_chi2") {
            iss >> config.required_chi2;
        } else if (key == "required_dBV_min") {
            iss >> config.required_dBV_min;
        } else if (key == "required_dBV_max") {
            iss >> config.required_dBV_max;
        } else if (key == "PVBoundary1") {
            iss >> config.PVBoundary1;
        } else if (key == "PVBoundary2") {
            iss >> config.PVBoundary2;
        }
    }
    
    return config;
}

// Create histogram set for a given branch
HistogramSet createHistograms(TDirectory* dir, const std::string& name) {
    HistogramSet hists;
    
    TDirectory* kinDir = dir->mkdir("Kinematics");
    kinDir->cd();
    hists.chi2norm = new TH1F("chi2norm", "Vertex #chi^{2}/ndof; #chi^{2}/ndof; Vertices", 200, 0, 20);
    hists.pt = new TH1F("pt", "Vertex p_{T}; p_{T} [GeV]; Vertices", 100, 0, 100);
    hists.eta = new TH1F("eta", "Vertex #eta; #eta; Vertices", 100, -5, 5);
    hists.phi = new TH1F("phi", "Vertex #phi; #phi; Vertices", 100, -3.14, 3.14);
    hists.mass = new TH1F("mass", "Vertex Mass; Mass [GeV]; Vertices", 100, 0, 10);
    hists.nTracks = new TH1F("nTracks", "Number of Tracks; N_{tracks}; Vertices", 50, 0, 50);
    
    TDirectory* spatialDir = dir->mkdir("Spatial");
    spatialDir->cd();
    hists.xy_global = new TH2F("xy_global", "Vertex XY (Global); X [cm]; Y [cm]", 800, -10, 10, 800, -10, 10);
    hists.xy_ref = new TH2F("xy_ref", "Vertex XY (ref); X-X_{ref} [cm]; Y-Y_{ref} [cm]", 800, -10, 10, 800, -10, 10);
    
    TDirectory* distDir = dir->mkdir("Distance");
    distDir->cd();
    hists.dBV_origin = new TH1F("dBV_origin", "d_{BV} wrt (0,0); d_{BV} [cm]; Vertices", 200, 0, 10);
    hists.dBV_ref = new TH1F("dBV_ref", "d_{BV} wrt ref; d_{BV} [cm]; Vertices", 200, 0, 10);
    hists.dBV_beamspot = new TH1F("dBV_beamspot", "d_{BV} wrt BS; d_{BV} [cm]; Vertices", 200, 0, 10);
    hists.dBV_avgPV = new TH1F("dBV_avgPV", "d_{BV} wrt avgPV; d_{BV} [cm]; Vertices", 200, 0, 10);
    hists.dBV_error = new TH1F("dBV_error", "d_{BV} Uncertainty; #sigma_{dBV} [cm]; Vertices", 1000, 0, 0.1);
    
    TDirectory* angleDir = dir->mkdir("OpeningAngles");
    angleDir->cd();
    hists.angleMin = new TH1F("min", "Min Opening Angle; Min Angle [rad]; Vertices", 180, 0, 3.14159);
    hists.angleMean = new TH1F("mean", "Mean Opening Angle; <Angle> [rad]; Vertices", 180, 0, 3.14159);
    hists.angleMax = new TH1F("max", "Max Opening Angle; Max Angle [rad]; Vertices", 180, 0, 3.14159);
    
    TDirectory* topoDir = dir->mkdir("Topology");
    TDirectory* barrelDir = topoDir->mkdir("Barrel");
    barrelDir->cd();
    hists.barrel_mass = new TH1F("mass", "Mass (Barrel); Mass [GeV]; Vertices", 100, 0, 10);
    hists.barrel_dBV = new TH1F("dBV", "d_{BV} (Barrel); d_{BV} [cm]; Vertices", 100, 0, 10);
    
    TDirectory* endcapDir = topoDir->mkdir("Endcap");
    endcapDir->cd();
    hists.endcap_mass = new TH1F("mass", "Mass (Endcap); Mass [GeV]; Vertices", 100, 0, 10);
    hists.endcap_dBV = new TH1F("dBV", "d_{BV} (Endcap); d_{BV} [cm]; Vertices", 100, 0, 10);
    
    TDirectory* regionDir = dir->mkdir("PVRegions");
    TDirectory* regADir = regionDir->mkdir("RegionA");
    regADir->cd();
    hists.regionA_mass = new TH1F("mass", "Mass (Region A); Mass [GeV]; Vertices", 100, 0, 10);
    hists.regionA_dBV = new TH1F("dBV", "d_{BV} (Region A); d_{BV} [cm]; Vertices", 100, 0, 10);
    
    TDirectory* regBDir = regionDir->mkdir("RegionB");
    regBDir->cd();
    hists.regionB_mass = new TH1F("mass", "Mass (Region B); Mass [GeV]; Vertices", 100, 0, 10);
    hists.regionB_dBV = new TH1F("dBV", "d_{BV} (Region B); d_{BV} [cm]; Vertices", 100, 0, 10);
    
    TDirectory* regCDir = regionDir->mkdir("RegionC");
    regCDir->cd();
    hists.regionC_mass = new TH1F("mass", "Mass (Region C); Mass [GeV]; Vertices", 100, 0, 10);
    hists.regionC_dBV = new TH1F("dBV", "d_{BV} (Region C); d_{BV} [cm]; Vertices", 100, 0, 10);
    
    return hists;
}

void Tree2Plots(const std::string& configFile = "Tree2PlotsConfig.txt",
                const std::string& inputFile = "ScoutingTree_Output.root",
                const std::string& outputFile = "Tree2Plots_Output.root") {
    
    std::cout << "=== Tree2Plots ===" << std::endl;
    std::cout << "Config: " << configFile << std::endl;
    std::cout << "Input:  " << inputFile << std::endl;
    std::cout << "Output: " << outputFile << std::endl;
    
    // Read configuration
    PlotConfig config = readConfig(configFile);
    std::cout << "Loaded " << config.cut_ntk.size() << " ntk cuts" << std::endl;
    std::cout << "Loaded " << config.cut_opening_angle_min.size() << " angle cuts" << std::endl;
    
    // Open input file
    TFile* inFile = TFile::Open(inputFile.c_str(), "READ");
    if (!inFile || inFile->IsZombie()) {
        std::cerr << "Error: Cannot open input file" << std::endl;
        return;
    }
    
    TTree* tree = (TTree*)inFile->Get("vertexTree");
    if (!tree) {
        std::cerr << "Error: Cannot find vertexTree" << std::endl;
        return;
    }
    
    // Set up branch addresses
    UInt_t run, lumi, event;
    Int_t nPV, refType;
    Float_t beamspot_x, beamspot_y, avgPV_x, avgPV_y;
    std::vector<float>* vtx_x = nullptr;
    std::vector<float>* vtx_y = nullptr;
    std::vector<float>* vtx_chi2norm = nullptr;
    std::vector<int>* vtx_ntracks = nullptr;
    std::vector<float>* vtx_pt = nullptr;
    std::vector<float>* vtx_eta = nullptr;
    std::vector<float>* vtx_phi = nullptr;
    std::vector<float>* vtx_mass = nullptr;
    std::vector<float>* vtx_dBV_origin = nullptr;
    std::vector<float>* vtx_dBV_ref = nullptr;
    std::vector<float>* vtx_dBV_bs = nullptr;
    std::vector<float>* vtx_dBV_avgPV = nullptr;
    std::vector<float>* vtx_dBV_err = nullptr;
    std::vector<float>* vtx_angleMin = nullptr;
    std::vector<float>* vtx_angleMean = nullptr;
    std::vector<float>* vtx_angleMax = nullptr;
    std::vector<int>* vtx_pvRegion = nullptr;
    
    tree->SetBranchAddress("run", &run);
    tree->SetBranchAddress("lumi", &lumi);
    tree->SetBranchAddress("event", &event);
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
    tree->SetBranchAddress("vtx_dBV_bs", &vtx_dBV_bs);
    tree->SetBranchAddress("vtx_dBV_avgPV", &vtx_dBV_avgPV);
    tree->SetBranchAddress("vtx_dBV_err", &vtx_dBV_err);
    tree->SetBranchAddress("vtx_angleMin", &vtx_angleMin);
    tree->SetBranchAddress("vtx_angleMean", &vtx_angleMean);
    tree->SetBranchAddress("vtx_angleMax", &vtx_angleMax);
    tree->SetBranchAddress("vtx_pvRegion", &vtx_pvRegion);
    
    // Create output file
    TFile* outFile = TFile::Open(outputFile.c_str(), "RECREATE");
    TDirectory* verticesDir = outFile->mkdir("Vertices");
    TDirectory* vtxSelDir = verticesDir->mkdir("Selected");
    
    // Create histogram sets for each ntk × angle combination
    std::map<std::string, HistogramSet> histMap;
    
    for (size_t i_ntk = 0; i_ntk < config.cut_ntk.size(); ++i_ntk) {
        const auto& ntk_set = config.cut_ntk[i_ntk];
        
        std::string ntkBranchName;
        if (ntk_set.empty()) {
            ntkBranchName = "ntk_any";
        } else if (ntk_set.size() == 1) {
            ntkBranchName = "ntk_" + std::to_string(ntk_set[0]);
        } else {
            ntkBranchName = "ntk";
            for (size_t j = 0; j < ntk_set.size(); ++j) {
                if (j > 0) ntkBranchName += "_or_";
                ntkBranchName += std::to_string(ntk_set[j]);
            }
        }
        
        TDirectory* ntkDir = vtxSelDir->mkdir(ntkBranchName.c_str());
        
        for (size_t i_angle = 0; i_angle < config.cut_opening_angle_min.size(); ++i_angle) {
            double angle_cut = config.cut_opening_angle_min[i_angle];
            
            std::string angleBranchName;
            if (angle_cut < 0) {
                angleBranchName = "angle_any";
            } else {
                std::ostringstream oss;
                oss << "angle_gt_" << std::fixed << std::setprecision(2) << angle_cut;
                angleBranchName = oss.str();
                std::replace(angleBranchName.begin(), angleBranchName.end(), '.', 'p');
            }
            
            std::string branchKey = ntkBranchName + "/" + angleBranchName;
            TDirectory* branchDir = ntkDir->mkdir(angleBranchName.c_str());
            
            histMap[branchKey] = createHistograms(branchDir, branchKey);
        }
    }
    
    // Process events
    Long64_t nEntries = tree->GetEntries();
    std::cout << "Processing " << nEntries << " events..." << std::endl;
    
    for (Long64_t iEntry = 0; iEntry < nEntries; ++iEntry) {
        if (iEntry % 10000 == 0) {
            std::cout << "  Event " << iEntry << " / " << nEntries << "\r" << std::flush;
        }
        
        tree->GetEntry(iEntry);
        
        // Get reference position
        float ref_x = (refType == 1) ? beamspot_x : avgPV_x;
        float ref_y = (refType == 1) ? beamspot_y : avgPV_y;
        
        // Loop over vertices
        for (size_t ivtx = 0; ivtx < vtx_x->size(); ++ivtx) {
            int ntk = (*vtx_ntracks)[ivtx];
            float mass = (*vtx_mass)[ivtx];
            float chi2norm = (*vtx_chi2norm)[ivtx];
            float dBV_ref = (*vtx_dBV_ref)[ivtx];
            float dBV_err = (*vtx_dBV_err)[ivtx];
            float minAngle = (*vtx_angleMin)[ivtx];
            
            // Loop over ntk × angle combinations
            for (size_t i_ntk = 0; i_ntk < config.cut_ntk.size(); ++i_ntk) {
                const auto& ntk_set = config.cut_ntk[i_ntk];
                
                bool ntk_pass = ntk_set.empty();
                if (!ntk_pass) {
                    for (int allowed_ntk : ntk_set) {
                        if (ntk == allowed_ntk) {
                            ntk_pass = true;
                            break;
                        }
                    }
                }
                if (!ntk_pass) continue;
                
                std::string ntkBranchName;
                if (ntk_set.empty()) ntkBranchName = "ntk_any";
                else if (ntk_set.size() == 1) ntkBranchName = "ntk_" + std::to_string(ntk_set[0]);
                else {
                    ntkBranchName = "ntk";
                    for (size_t j = 0; j < ntk_set.size(); ++j) {
                        if (j > 0) ntkBranchName += "_or_";
                        ntkBranchName += std::to_string(ntk_set[j]);
                    }
                }
                
                for (size_t i_angle = 0; i_angle < config.cut_opening_angle_min.size(); ++i_angle) {
                    double angle_cut = config.cut_opening_angle_min[i_angle];
                    if (angle_cut >= 0 && minAngle < angle_cut) continue;
                    
                    std::string angleBranchName;
                    if (angle_cut < 0) angleBranchName = "angle_any";
                    else {
                        std::ostringstream oss;
                        oss << "angle_gt_" << std::fixed << std::setprecision(2) << angle_cut;
                        angleBranchName = oss.str();
                        std::replace(angleBranchName.begin(), angleBranchName.end(), '.', 'p');
                    }
                    
                    // Apply other cuts
                    if (config.required_invmass != -1 && mass < config.required_invmass) continue;
                    if (config.required_chi2 != -1 && chi2norm > config.required_chi2) continue;
                    if (config.required_dBV_min != -1 && dBV_ref < config.required_dBV_min) continue;
                    if (config.required_dBV_max != -1 && dBV_ref > config.required_dBV_max) continue;
                    
                    std::string branchKey = ntkBranchName + "/" + angleBranchName;
                    auto& hists = histMap[branchKey];
                    
                    // Fill histograms
                    hists.chi2norm->Fill(chi2norm);
                    hists.pt->Fill((*vtx_pt)[ivtx]);
                    hists.eta->Fill((*vtx_eta)[ivtx]);
                    hists.phi->Fill((*vtx_phi)[ivtx]);
                    hists.mass->Fill(mass);
                    hists.nTracks->Fill(ntk);
                    hists.xy_global->Fill((*vtx_x)[ivtx], (*vtx_y)[ivtx]);
                    hists.xy_ref->Fill((*vtx_x)[ivtx] - ref_x, (*vtx_y)[ivtx] - ref_y);
                    hists.dBV_origin->Fill((*vtx_dBV_origin)[ivtx]);
                    hists.dBV_ref->Fill(dBV_ref);
                    if ((*vtx_dBV_bs)[ivtx] > -998) hists.dBV_beamspot->Fill((*vtx_dBV_bs)[ivtx]);
                    if ((*vtx_dBV_avgPV)[ivtx] > -998) hists.dBV_avgPV->Fill((*vtx_dBV_avgPV)[ivtx]);
                    hists.dBV_error->Fill(dBV_err);
                    hists.angleMin->Fill(minAngle);
                    hists.angleMean->Fill((*vtx_angleMean)[ivtx]);
                    hists.angleMax->Fill((*vtx_angleMax)[ivtx]);
                    
                    // Topology
                    if (std::fabs((*vtx_eta)[ivtx]) < 1.0) {
                        hists.barrel_mass->Fill(mass);
                        hists.barrel_dBV->Fill(dBV_ref);
                    } else {
                        hists.endcap_mass->Fill(mass);
                        hists.endcap_dBV->Fill(dBV_ref);
                    }
                    
                    // PV regions
                    int pvRegion = (*vtx_pvRegion)[ivtx];
                    if (pvRegion == 0) {
                        hists.regionA_mass->Fill(mass);
                        hists.regionA_dBV->Fill(dBV_ref);
                    } else if (pvRegion == 1) {
                        hists.regionB_mass->Fill(mass);
                        hists.regionB_dBV->Fill(dBV_ref);
                    } else {
                        hists.regionC_mass->Fill(mass);
                        hists.regionC_dBV->Fill(dBV_ref);
                    }
                }
            }
        }
    }
    
    std::cout << std::endl << "Writing output..." << std::endl;
    outFile->Write();
    outFile->Close();
    inFile->Close();
    
    std::cout << "Done! Output written to " << outputFile << std::endl;
}
