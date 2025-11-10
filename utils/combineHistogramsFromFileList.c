#include <TFile.h>
#include <TDirectory.h>
#include <TH1.h>
#include <TH2.h>
#include <TList.h>
#include <TKey.h>
#include <TString.h>
#include <TObject.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>
#include <unordered_map>
#include <cstdio>

// Helper to convert TH1/TH2 to TH1D/TH2D for proper merging
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

// Recursive function to collect all histograms from a directory (and subdirectories)
// pathPrefix is the full path from root (e.g., "dir1/dir2/") to maintain structure
static void CollectHistogramsRecursive(TDirectory* dir, const TString& pathPrefix,
                                      std::unordered_map<std::string, TH1*>& histMap,
                                      std::vector<std::string>& histOrder) {
    if (!dir) return;
    
    TIter next(dir->GetListOfKeys());
    TKey* key;
    while ((key = (TKey*)next())) {
        TString keyName = key->GetName();
        TString className = key->GetClassName();
        
        TObject* obj = key->ReadObj();
        if (!obj) continue;
        
        // Check if it's a directory
        if (TDirectory* subdir = dynamic_cast<TDirectory*>(obj)) {
            TString newPrefix = pathPrefix + keyName + "/";
            CollectHistogramsRecursive(subdir, newPrefix, histMap, histOrder);
            delete obj;
            continue;
        }
        
        // Check if it's a histogram
        TH1* hist = dynamic_cast<TH1*>(obj);
        if (!hist) {
            delete obj;
            continue;
        }
        
        // Build full path key
        std::string fullPath = (pathPrefix + keyName).Data();
        
        if (histMap.find(fullPath) == histMap.end()) {
            // First occurrence: create and store
            TH1* newHist = nullptr;
            if (className.BeginsWith("TH1I") || className.BeginsWith("TH2I")) {
                newHist = ConvertToDoubleHist(hist, keyName);
            } else {
                newHist = (TH1*)hist->Clone(keyName);
                newHist->SetDirectory(0);
            }
            histMap[fullPath] = newHist;
            histOrder.push_back(fullPath);
        } else {
            // Add to existing
            histMap[fullPath]->Add(hist);
        }
        delete obj;
    }
}

// Helper to recreate directory structure in output file
static TDirectory* EnsureDirectoryPath(TDirectory* base, const TString& path) {
    if (path.IsNull() || path == "") return base;
    
    TObjArray* tokens = path.Tokenize("/");
    TDirectory* current = base;
    for (Int_t i = 0; i < tokens->GetEntries(); ++i) {
        TString dirName = ((TObjString*)tokens->At(i))->GetString();
        if (dirName.IsNull()) continue;
        
        TDirectory* subdir = (TDirectory*)current->Get(dirName);
        if (!subdir) {
            subdir = current->mkdir(dirName);
        }
        current = subdir;
    }
    delete tokens;
    return current;
}

// Main function: combine histograms from list of files
// Creates output file with IDENTICAL structure to inputs, but with combined entries
void combineHistogramsFromFileList(const char* inputListFile, const char* outputFileName = "combined_histograms.root") {
    TH1::AddDirectory(kFALSE);

    // Read input file list
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

    if (allFiles.empty()) {
        std::cerr << "Error: No files in input list" << std::endl;
        return;
    }

    std::cout << "Combining " << allFiles.size() << " files..." << std::endl;

    // Histogram containers
    std::unordered_map<std::string, TH1*> histMap;
    std::vector<std::string> histOrder;

    // Process all files
    for (size_t i = 0; i < allFiles.size(); ++i) {
        const std::string& fileName = allFiles[i];
        if (fileName.empty()) continue;
        
        TFile* file = TFile::Open(fileName.c_str());
        if (!file || file->IsZombie()) {
            std::cerr << "Error: Cannot open file " << fileName << std::endl;
            continue;
        }

        // Recursively collect all histograms from entire file structure
        CollectHistogramsRecursive(file, "", histMap, histOrder);

        file->Close();
        delete file;

        if ((i + 1) % 10 == 0 || (i == allFiles.size() - 1)) {
            int percent = (int)(100.0 * (i + 1) / allFiles.size() + 0.5);
            std::cout << "\r[Progress] " << percent << "% (" << (i+1) << "/" << allFiles.size() << " files)" << std::flush;
        }
    }
    std::cout << std::endl;

    // Write output file with IDENTICAL structure to inputs
    TFile* outputFile = new TFile(outputFileName, "RECREATE");
    for (size_t i = 0; i < histOrder.size(); ++i) {
        const std::string& fullPath = histOrder[i];
        TH1* hist = histMap[fullPath];
        
        // Split path into directory and histogram name
        TString pathStr = fullPath.c_str();
        Ssiz_t lastSlash = pathStr.Last('/');
        TString dirPath = (lastSlash >= 0) ? pathStr(0, lastSlash) : "";
        TString histName = (lastSlash >= 0) ? pathStr(lastSlash+1, pathStr.Length()) : pathStr;
        
        // Navigate to correct directory (creates if needed)
        TDirectory* targetDir = EnsureDirectoryPath(outputFile, dirPath);
        targetDir->cd();
        
        hist->Write(histName);
        delete hist;
    }
    outputFile->Close();
    delete outputFile;
    
    std::cout << "Combined output saved to: " << outputFileName << std::endl;
    std::cout << "Total histograms merged: " << histOrder.size() << std::endl;
}