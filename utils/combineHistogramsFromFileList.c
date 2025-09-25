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
#include <sys/stat.h>
#include <sys/types.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <cstdio> // for std::remove

// Helper function to convert TH1/TH2 to TH1D/TH2D
static TH1* ConvertToDoubleHist(TH1* hist, const TString& newName) {
    TString className = hist->ClassName();
    if (className.BeginsWith("TH2")) {
        TH2* h2 = dynamic_cast<TH2*>(hist);
        TH2D* newHist = new TH2D(
            newName, h2->GetTitle(),
            h2->GetNbinsX(), h2->GetXaxis()->GetXmin(), h2->GetXaxis()->GetXmax(),
            h2->GetNbinsY(), h2->GetYaxis()->GetXmin(), h2->GetYaxis()->GetXmax()
        );
        for (int ix = 1; ix <= h2->GetNbinsX(); ++ix)
            for (int iy = 1; iy <= h2->GetNbinsY(); ++iy) {
                newHist->SetBinContent(h2->GetBin(ix, iy), h2->GetBinContent(ix, iy));
                newHist->SetBinError(h2->GetBin(ix, iy), h2->GetBinError(ix, iy));
            }
        newHist->SetDirectory(0);
        return (TH1*)newHist;
    } else {
        TH1* h1 = hist;
        TH1D* newHist = new TH1D(
            newName, h1->GetTitle(),
            h1->GetNbinsX(), h1->GetXaxis()->GetXmin(), h1->GetXaxis()->GetXmax()
        );
        for (int ix = 1; ix <= h1->GetNbinsX(); ++ix) {
            newHist->SetBinContent(ix, h1->GetBinContent(ix));
            newHist->SetBinError(ix, h1->GetBinError(ix));
        }
        newHist->SetDirectory(0);
        return (TH1*)newHist;
    }
}

struct HistEntry {
    std::string name;
    TH1* hist;
};

// Helper to merge ROOT files (batch outputs) into a final output
static void MergeBatchFiles(const std::vector<std::string>& batchFiles, const std::string& finalFile) {
    TH1::AddDirectory(kFALSE);
    std::unordered_map<std::string, TH1*> histMap;
    std::vector<std::string> histOrder;

    size_t totalBatches = batchFiles.size();
    for (size_t i = 0; i < totalBatches; ++i) {
        std::cout << "[Merge Progress] " << (int)((100.0 * (i+1)) / totalBatches + 0.5)
                  << "% (" << (i+1) << "/" << totalBatches << " batch files processed)" << std::endl;
        TFile* file = TFile::Open(batchFiles[i].c_str());
        if (!file || file->IsZombie()) {
            std::cerr << "Error: Cannot open batch file " << batchFiles[i] << std::endl;
            continue;
        }
        TDirectory* dir = (TDirectory*)file->Get("scoutingTree");
        if (!dir) {
            std::cerr << "Error: Cannot access scoutingTree in " << batchFiles[i] << std::endl;
            file->Close();
            delete file;
            continue;
        }
        TIter next(dir->GetListOfKeys());
        TKey* key;
        while ((key = (TKey*)next())) {
            TString histName = key->GetName();
            TString className = key->GetClassName();
            TH1* hist = (TH1*)dir->Get(histName);
            if (!hist) continue;
            std::string histNameStr = histName.Data();
            if (histMap.find(histNameStr) == histMap.end()) {
                TH1* newHist = (TH1*)hist->Clone(histName);
                newHist->SetDirectory(0);
                histMap[histNameStr] = newHist;
                histOrder.push_back(histNameStr);
            } else {
                histMap[histNameStr]->Add(hist);
            }
        }
        file->Close();
        delete file;
    }

    TFile* outputFile = new TFile(finalFile.c_str(), "RECREATE");
    TDirectory* outputDir = outputFile->mkdir("scoutingTree");
    outputDir->cd();
    for (size_t i = 0; i < histOrder.size(); ++i) {
        histMap[histOrder[i]]->Write();
        delete histMap[histOrder[i]];
    }
    outputFile->Close();
    delete outputFile;
    std::cout << "Final merge complete. Output: " << finalFile << std::endl;
    std::cout << "Total unique histograms in final file: " << histOrder.size() << std::endl;
}

void combineHistogramsFromFileList(const char* inputListFile, const char* combinedFileName = nullptr, int batch_start = 1) {
    TH1::AddDirectory(kFALSE);

    // Read input file list into a vector
    std::ifstream fileList(inputListFile);
    if (!fileList.is_open()) {
        std::cerr << "Error: Cannot open input file list " << inputListFile << std::endl;
        return;
    }
    std::vector<std::string> allFiles;
    std::string tempFileName;
    while (std::getline(fileList, tempFileName)) {
        if (!tempFileName.empty()) allFiles.push_back(tempFileName);
    }
    fileList.close();

    int nFilesToProcess = allFiles.size();
    int batchSize = 100;
    int nBatches = (nFilesToProcess + batchSize - 1) / batchSize;
    TString outputDirName = "outputs";
    struct stat st = {0};
    if (stat(outputDirName.Data(), &st) == -1) {
        if (mkdir(outputDirName.Data(), 0755) != 0) {
            std::cerr << "Error: Could not create output directory 'outputs'." << std::endl;
            return;
        }
    }

    std::vector<std::string> batchOutputFiles;
    for (int batchIdx = 0; batchIdx < nBatches; ++batchIdx) {
        int batchNum = batchIdx + 1;
        TString tag = combinedFileName ? combinedFileName : "auto";
        TString batchFileName = TString::Format("%s/combined_histograms_%s_batch%d.root", outputDirName.Data(), tag.Data(), batchNum);
        batchOutputFiles.push_back(batchFileName.Data());
    }

    // If batch_start > nBatches or batch_start == -1, skip batch processing and just merge
    if (batch_start > nBatches || batch_start == -1) {
        std::cout << "Batch start (" << batch_start << ") is greater than number of batches (" << nBatches << ") or is -1. Skipping batch processing and merging batch files only." << std::endl;
        TString tag = combinedFileName ? combinedFileName : "auto";
        TString finalFileName = TString::Format("outputs/combined_histograms_%s_final.root", tag.Data());
        MergeBatchFiles(batchOutputFiles, finalFileName.Data());
        return;
    }

    for (int batchIdx = 0; batchIdx < nBatches; ++batchIdx) {
        int batchNum = batchIdx + 1;
        if (batchNum < batch_start) {
            continue;
        }

        int startIdx = batchIdx * batchSize;
        int endIdx = std::min(startIdx + batchSize, nFilesToProcess);

        // Prepare batch output file name
        TString tag = combinedFileName ? combinedFileName : "auto";
        TString batchFileName = TString::Format("%s/combined_histograms_%s_batch%d.root", outputDirName.Data(), tag.Data(), batchNum);
        batchOutputFiles.push_back(batchFileName.Data());

        // Delete batch file if it exists
        std::remove(batchFileName.Data());

        // Progress print
        std::cout << "Processing batch " << batchNum << "/" << nBatches
                  << " (files " << (startIdx+1) << " to " << endIdx << ")" << std::endl;

        // Histogram containers for this batch
        std::unordered_map<std::string, TH1*> histMap;
        std::vector<std::string> histOrder;
        int skippedFiles = 0;
        int totalFiles = 0;

        for (int i = startIdx; i < endIdx; ++i) {
            const std::string& fileName = allFiles[i];
            if (fileName.empty()) continue;
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

                TH1 *hist = nullptr;
                if (className.BeginsWith("TH1") || className.BeginsWith("TH2")) {
                    hist = (TH1*)dir->Get(histName);
                } else if (className.BeginsWith("TProfile")) {
                    hist = (TH1*)dir->Get(histName); // TProfile inherits TH1
                } else {
                    continue;
                }
                if (!hist) continue;

                std::string histNameStr = histName.Data();
                if (histMap.find(histNameStr) == histMap.end()) {
                    TH1* newHist = nullptr;
                    if (className.BeginsWith("TH1I") || className.BeginsWith("TH2I")) {
                        newHist = ConvertToDoubleHist(hist, TString::Format("%s_combined", histName.Data()));
                    } else {
                        newHist = (TH1*)hist->Clone(TString::Format("%s_combined", histName.Data()));
                        newHist->SetDirectory(0);
                    }
                    histMap[histNameStr] = newHist;
                    histOrder.push_back(histNameStr);
                } else {
                    histMap[histNameStr]->Add(hist);
                }
            }

            file->Close();
            delete file;

            if ((i - startIdx + 1) % 10 == 0 || (i == endIdx - 1)) {
                int percent = (int)(100.0 * (i - startIdx + 1) / (endIdx - startIdx) + 0.5);
                int mergerPercent = (int)(100.0 * (i + 1) / nFilesToProcess + 0.5);
                std::cout << "\r  [Batch Progress] " << percent << "% | [Merger Progress] " << mergerPercent << "%" << std::flush;
            }
        }

        // Write batch output file
        TFile *outputFile = new TFile(batchFileName, "RECREATE");
        TDirectory *outputDir = outputFile->mkdir("scoutingTree");
        outputDir->cd();
        for (size_t i = 0; i < histOrder.size(); ++i) {
            histMap[histOrder[i]]->Write();
            delete histMap[histOrder[i]];
        }
        outputFile->Close();
        delete outputFile;

        std::cout << "\nBatch " << (batchIdx+1) << " complete. Output: " << batchFileName << std::endl;
        std::cout << "  Files processed: " << (endIdx - startIdx) << ", Skipped: " << skippedFiles << std::endl;
        std::cout << "  Unique histograms merged: " << histOrder.size() << std::endl;
    }

    // Automatically merge batch files into final output
    TString tag = combinedFileName ? combinedFileName : "auto";
    TString finalFileName = TString::Format("outputs/combined_histograms_%s_final.root", tag.Data());
    MergeBatchFiles(batchOutputFiles, finalFileName.Data());
}