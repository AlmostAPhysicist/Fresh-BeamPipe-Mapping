#include <TFile.h>
#include <TDirectory.h>
#include <TH1.h>
#include <TH2.h>
#include <TList.h>
#include <TKey.h>
#include <TString.h>
#include <iostream>
#include <map>
#include <string>

void addHistogramsFromFiles() {
    TH1::AddDirectory(kFALSE);

    TString basePath = "/eos/user/a/amalhotr/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/DYto2Mu4Jets_ScoutingTree_100_NoCoreFresh_WithEta_HighRes/250423_040257/0000/DY2M_ScoutingTree_Output_";
    int nFiles = 43;

    std::map<std::string, TH1*> histMap; // Use std::string instead of TString

    int skippedFiles = 0;

    for (int i = 1; i <= nFiles; i++) {
        TString fileName = TString::Format("%s%d.root", basePath.Data(), i);
        TFile *file = TFile::Open(fileName);

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

            std::string histNameStr = histName.Data(); // Convert TString to std::string
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

    if (histMap.empty()) {
        std::cerr << "No histograms were combined. Exiting." << std::endl;
        return;
    }

    TString outputFileName = TString::Format("combined_histograms_%dFiles.root", nFiles);
    TFile *outputFile = new TFile(outputFileName, "RECREATE");
    TDirectory *outputDir = outputFile->mkdir("scoutingTree");
    outputDir->cd();

    for (auto const &entry : histMap) {
        entry.second->Write();
    }

    outputFile->Close();
    delete outputFile;

    for (auto const &entry : histMap) {
        delete entry.second;
    }

    std::cout << "Histogram combination complete. Results saved in " << outputFileName << std::endl;
    std::cout << "Skipped files: " << skippedFiles << std::endl;
}