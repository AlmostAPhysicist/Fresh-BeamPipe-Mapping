#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TChain.h>
#include <TDirectory.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>

void combine_all_histograms() {
  // List of histogram names in the glocal directory
  const std::vector<std::string> histNames = {
    "vertex_xy_global", "vertex_xy_beamspot", "ntracks_global", "ntracks_beamspot",
    "track_momenta_global", "track_momenta_beamspot", "radial_distance_global",
    "radial_distance_beamspot", "eta_distribution_global", "eta_distribution_beamspot",
    "phi_distribution_global", "phi_distribution_beamspot", "beamspot_global",
    "vertex_xy_global_ntracks", "vertex_xy_beamspot_ntracks", "vertex_xy_beamspot_not_ntracks",
    "h_vertex_pt", "h_vertex_eta", "h_vertex_phi", "h_vertex_mass"
  };

  // Map to store combined histograms
  std::map<std::string, TH1*> combinedHists;

  // Open file list
  std::ifstream fileList("/eos/user/a/amalhotr/BeamGeo/tree100individualpaths.txt");
  std::string line;
  int fileCount = 0;

  // Loop over files to combine histograms
  while (std::getline(fileList, line)) {
    TFile* f = TFile::Open(line.c_str());
    if (!f || f->IsZombie()) {
      std::cerr << "Error opening file: " << line << std::endl;
      continue;
    }
    fileCount++;

    // Access the glocal directory
    TDirectory* dir = (TDirectory*)f->Get("glocal");
    if (!dir) {
      std::cerr << "Warning: glocal directory not found in " << line << std::endl;
      f->Close();
      delete f;
      continue;
    }

    // Combine histograms from glocal directory
    for (const auto& name : histNames) {
      TH1* h = (TH1*)dir->Get(name.c_str());
      if (!h) {
        std::cerr << "Warning: Histogram " << name << " not found in glocal directory of " << line << std::endl;
        continue;
      }
      if (combinedHists.find(name) == combinedHists.end()) {
        combinedHists[name] = (TH1*)h->Clone(name.c_str());
        combinedHists[name]->SetDirectory(0);
      } else {
        combinedHists[name]->Add(h);
      }
    }

    f->Close();
    delete f;
  }
  fileList.close();

  // Chain the scoutingTree
  TChain* scoutingChain = new TChain("scoutingTree");
  fileList.open("file_list.txt");
  while (std::getline(fileList, line)) {
    scoutingChain->Add(line.c_str());
  }
  fileList.close();

  // Create output file
  TFile* outputFile = new TFile("combined_all_histograms.root", "RECREATE");

  // Write combined histograms to glocal directory
  outputFile->mkdir("glocal");
  outputFile->cd("glocal");
  for (const auto& pair : combinedHists) {
    pair.second->Write();
  }

  // Write chained scoutingTree
  outputFile->cd();
  scoutingChain->Write();

  // Cleanup
  outputFile->Close();
  delete outputFile;
  delete scoutingChain;
  for (auto& pair : combinedHists) delete pair.second;

  std::cout << "Processed " << fileCount << " files. Output saved to combined_all_histograms.root" << std::endl;
}