#include <TFile.h>
#include <TDirectory.h>
#include <TH1.h>
#include <TH2.h>
#include <TList.h>
#include <TKey.h>
#include <TString.h>
#include <iostream>
#include <fstream>
#include <map>
#include <string>

void combineHistogramsFromFileList(const char* inputListFile) {
    TH1::AddDirectory(kFALSE);

    // Read input file list
    std::ifstream fileList(inputListFile);
    if (!fileList.is_open()) {
        std::cerr << "Error: Cannot open input file list " << inputListFile << std::endl;
        return;
    }

    std::map<std::string, TH1*> histMap;
    int skippedFiles = 0;
    int totalFiles = 0;
    std::string fileName;

    // Process each file in the list
    while (std::getline(fileList, fileName)) {
        totalFiles++;
        TString fileNameTStr = fileName.c_str();
        TFile *file = TFile::Open(fileNameTStr);

        if (!file || file->IsZombie()) {
            std::cerr << "Error: Cannot open file " << fileName << std::endl;
            skippedFiles++;
            continue;
        }

        TDirectory *dir = (TDirectory*)file->Get("scoutingTree");
        if (!dir) {
            std::cerr << "Error: Cannot access scoutingTree in " << fileName << std::endl;
            file->Close();
            delete file;
            skippedFiles++;
            continue;
        }

        TIter next(dir->GetListOfKeys());
        TKey *key;
        while ((key = (TKey*)next())) {
            TString histName = key->GetName();
            TString className = key->GetClassName();

            if (!className.BeginsWith("TH1") && !className.BeginsWith("TH2")) continue;

            TH1 *hist = (TH1*)dir->Get(histName);
            if (!hist) continue;

            std::string histNameStr = histName.Data();
            if (histMap.find(histNameStr) == histMap.end()) {
                histMap[histNameStr] = (TH1*)hist->Clone(TString::Format("%s_combined", histName.Data()));
                histMap[histNameStr]->SetDirectory(0);
            } else {
                histMap[histNameStr]->Add(hist);
            }
        }

        file->Close();
        delete file;
    }

    fileList.close();

    if (histMap.empty()) {
        std::cerr << "No histograms were combined. Exiting." << std::endl;
        return;
    }

    // Create output file
    TString inputBaseName = TString(inputListFile).ReplaceAll(".txt", "");
    TString outputFileName = TString::Format("combined_histograms_%s_%dFiles.root", inputBaseName.Data(), totalFiles);
    TFile *outputFile = new TFile(outputFileName, "RECREATE");
    TDirectory *outputDir = outputFile->mkdir("scoutingTree");
    outputDir->cd();

    for (auto const &entry : histMap) {
        entry.second->Write();
    }

    outputFile->Close();
    delete outputFile;

    // Clean up histograms
    for (auto const &entry : histMap) {
        delete entry.second;
    }

    std::cout << "Histogram combination complete. Results saved in " << outputFileName << std::endl;
    std::cout << "Processed files: " << totalFiles << std::endl;
    std::cout << "Skipped files: " << skippedFiles << std::endl;
}