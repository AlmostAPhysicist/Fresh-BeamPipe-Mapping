#include <TFile.h>
#include <TTree.h>
#include <TH2F.h>
#include <iostream>
#include <fstream>
#include <string>

int combineVertexXY(const std::string &fileList) {
    // Open the file list
    std::ifstream inputFile(fileList);
    if (!inputFile.is_open()) {
        std::cerr << "Error: Could not open file list!" << std::endl;
        return 1;
    }

    // Define cumulative histogram
    TH2F *h_vertex_xy_global_combined = nullptr;

    std::string filePath;
    while (std::getline(inputFile, filePath)) {
        // Open each ROOT file
        TFile *file = TFile::Open(filePath.c_str());
        if (!file || file->IsZombie()) {
            std::cerr << "Error: Could not open file " << filePath << std::endl;
            continue;
        }

        // Access the tree
        TDirectoryFile *dir = (TDirectoryFile*)file->Get("scoutingTree");
        if (!dir) {
            std::cerr << "Error: Could not find 'scoutingTree' in file " << filePath << std::endl;
            file->Close();
            continue;
        }
        TTree *tree = (TTree*)dir->Get("histTree");
        if (!tree) {
            std::cerr << "Error: Could not find 'histTree' in file " << filePath << std::endl;
            file->Close();
            continue;
        }

        // Define pointer to the histogram in the tree
        TH2F *h_vertex_xy_global = nullptr;

        // Set branch address
        if (tree->SetBranchAddress("h_vertex_xy_global", &h_vertex_xy_global) != 0) {
            std::cerr << "Error: Branch 'h_vertex_xy_global' not found in file " << filePath << std::endl;
            file->Close();
            continue;
        }

        // Loop over entries in the tree
        Long64_t nEntries = tree->GetEntries();
        for (Long64_t i = 0; i < nEntries; i++) {
            tree->GetEntry(i);

            // Combine h_vertex_xy_global
            if (h_vertex_xy_global) {
                if (!h_vertex_xy_global_combined) {
                    h_vertex_xy_global_combined = (TH2F*)h_vertex_xy_global->Clone("h_vertex_xy_global_combined");
                    h_vertex_xy_global_combined->SetDirectory(0); // Detach from the file
                } else {
                    h_vertex_xy_global_combined->Add(h_vertex_xy_global);
                }
            }
        }

        // Close the file
        file->Close();
    }

    // Save combined histogram to a new ROOT file
    TFile *outFile = TFile::Open("combined_vertex_xy_global.root", "RECREATE");
    if (h_vertex_xy_global_combined) h_vertex_xy_global_combined->Write();
    outFile->Close();

    std::cout << "Combined histogram saved to 'combined_vertex_xy_global.root'" << std::endl;

    return 0;
}

// Main function
int main() {
    // Provide the file list containing paths to all ROOT files
    std::string fileList = "/eos/user/a/amalhotr/BeamGeo/tree100individualpaths.txt";
    return combineVertexXY(fileList);
}