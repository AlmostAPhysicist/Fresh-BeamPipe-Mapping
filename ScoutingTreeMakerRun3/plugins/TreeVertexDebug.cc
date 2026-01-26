#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "TFile.h"
#include "TTree.h"

#include <vector>
#include <string>
#include <iostream>
#include <csignal>

class TreeVertexDebug : public edm::one::EDAnalyzer<> {
public:
  explicit TreeVertexDebug(const edm::ParameterSet&);
  ~TreeVertexDebug() override = default;

  void analyze(const edm::Event&, const edm::EventSetup&) override;

private:
  std::string inputFile_;
  std::string inputTree_;

  bool ran_ = false;

  static bool stopRequested_;
  static void handleSigint(int) { stopRequested_ = true; }
};

bool TreeVertexDebug::stopRequested_ = false;

TreeVertexDebug::TreeVertexDebug(const edm::ParameterSet& cfg)
  : inputFile_(cfg.getParameter<std::string>("inputFile")),
    inputTree_(cfg.getParameter<std::string>("inputTree"))
{}

void TreeVertexDebug::analyze(const edm::Event&, const edm::EventSetup&) {

  if (ran_) return;
  ran_ = true;

  std::signal(SIGINT, TreeVertexDebug::handleSigint);

  std::cout << "\nOpening file: " << inputFile_ << std::endl;

  TFile* file = TFile::Open(inputFile_.c_str(), "READ");
  if (!file || file->IsZombie()) {
    std::cerr << "ERROR: cannot open file\n";
    return;
  }

  TTree* tree = dynamic_cast<TTree*>(file->Get(inputTree_.c_str()));
  if (!tree) {
    std::cerr << "ERROR: cannot find tree: " << inputTree_ << "\n";
    return;
  }

  Long64_t nEntries = tree->GetEntries();
  std::cout << "Tree opened successfully.\n";
  std::cout << "Total entries = " << nEntries << "\n";

  std::vector<int>* vtx_ntracks = nullptr;
  tree->SetBranchAddress("vtx_ntracks", &vtx_ntracks);

  // --- milestone setup ---
  int milestonePercent = 10;
  Long64_t nextMilestone = (milestonePercent * nEntries) / 100;

  Long64_t globalVertex = 0;

  for (Long64_t i = 0; i < nEntries; i++) {

    if (stopRequested_) {
      std::cout << "\nCtrl+C received. Stopping at entry "
                << i << " (globalVertex=" << globalVertex << ")\n";
      break;
    }

    tree->GetEntry(i);

    // ✅ Trigger milestone print ONCE
    if (i >= nextMilestone) {

      std::cout << "\n=====================================\n";
      std::cout << "=== Progress: " << milestonePercent
                << "% reached at entry " << i << " ===\n";

      // Search forward for an entry with vertices
      bool printed = false;
      for (Long64_t j = i; j < nEntries && j < i + 10000; j++) {
        tree->GetEntry(j);
        if (vtx_ntracks && vtx_ntracks->size() > 0) {
          std::cout << "Found vertices at entry " << j << "\n";
          size_t nPrint = std::min((size_t)5, vtx_ntracks->size());
          for (size_t iv = 0; iv < nPrint; iv++) {
            std::cout << "  Vertex " << (globalVertex + iv)
                      << "  nTracks = " << (*vtx_ntracks)[iv] << "\n";
          }
          printed = true;
          break;
        }
      }
      if (!printed) {
        std::cout << "(No vertices found in next 10k entries)\n";
      }

      std::cout << "=====================================\n";

      // Move to next milestone
      milestonePercent += 10;
      nextMilestone = (milestonePercent * nEntries) / 100;

      // Stop at 100%
      if (milestonePercent > 100) break;
    }

    // Always count vertices, but DO NOT print
    if (vtx_ntracks) {
      globalVertex += vtx_ntracks->size();
    }
  }

  std::cout << "\nFinished scan.\n";
  file->Close();
}

DEFINE_FWK_MODULE(TreeVertexDebug);
