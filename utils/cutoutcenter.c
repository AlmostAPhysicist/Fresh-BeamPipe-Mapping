#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TDirectory.h>
#include <TKey.h>
#include <TString.h>
#include <TSystem.h>
#include <iostream>
#include <cmath>

// Recursive helper to process all objects in a directory
// Copies everything, but applies annulus filter to TH2 histograms (KEEP r1 <= r <= r2)
static void ProcessDirectory(TDirectory* sourceDir, TDirectory* targetDir, 
                             double h, double k, double r1, double r2) {
    if (!sourceDir || !targetDir) return;
    
    targetDir->cd();
    
    TIter next(sourceDir->GetListOfKeys());
    TKey* key;
    while ((key = (TKey*)next())) {
        TString keyName = key->GetName();
        TString className = key->GetClassName();
        
        TObject* obj = key->ReadObj();
        if (!obj) continue;
        
        // Handle subdirectories recursively
        if (TDirectory* subdir = dynamic_cast<TDirectory*>(obj)) {
            TDirectory* newSubdir = targetDir->mkdir(keyName);
            ProcessDirectory(subdir, newSubdir, h, k, r1, r2);
            delete obj;
            continue;
        }
        
        // Handle TH2 histograms with annulus filter (KEEP r1 <= r <= r2)
        if (TH2* h2 = dynamic_cast<TH2*>(obj)) {
            TH2* newHist = (TH2*)h2->Clone(keyName);
            newHist->SetDirectory(0);
            
            // Apply annulus filter: KEEP bins with r1 <= r <= r2, ZERO everything else
            for (int ix = 1; ix <= newHist->GetNbinsX(); ++ix) {
                double x = newHist->GetXaxis()->GetBinCenter(ix);
                for (int iy = 1; iy <= newHist->GetNbinsY(); ++iy) {
                    double y = newHist->GetYaxis()->GetBinCenter(iy);
                    double r = std::sqrt((x-h)*(x-h) + (y-k)*(y-k));
                    
                    // Zero out bins OUTSIDE the annulus (r < r1 OR r > r2)
                    if (r < r1 || r > r2) {
                        newHist->SetBinContent(ix, iy, 0);
                        newHist->SetBinError(ix, iy, 0);
                    }
                }
            }
            
            targetDir->cd();
            newHist->Write();
            delete newHist;
            delete obj;
            continue;
        }
        
        // Handle TH1 (and other objects): copy as-is
        if (TH1* h1 = dynamic_cast<TH1*>(obj)) {
            TH1* newHist = (TH1*)h1->Clone(keyName);
            newHist->SetDirectory(0);
            targetDir->cd();
            newHist->Write();
            delete newHist;
            delete obj;
            continue;
        }
        
        // Copy other objects directly
        targetDir->cd();
        obj->Write();
        delete obj;
    }
}

// Main function: apply annulus filter to all TH2 histograms in input file
// KEEPS bins with r1 <= r <= r2, zeros everything else
// h, k: center of annulus (default: 0, 0)
// r1, r2: inner and outer radius (default: 1.0 - 10.0 cm)
// outpath: output file path (if nullptr, auto-generated from input filename)
void cutoutcenter(const char* infile, double h = 0.0, double k = 0.0, 
                  double r1 = 1.0, double r2 = 10.0, const char* outpath = nullptr) {
    
    // Open input file
    TFile* fin = TFile::Open(infile, "READ");
    if (!fin || fin->IsZombie()) {
        std::cerr << "Error: Cannot open input file " << infile << std::endl;
        return;
    }
    
    // Build output filename if not provided
    TString outpath_t;
    if (outpath && std::strlen(outpath) > 0) {
        outpath_t = outpath;
    } else {
        // Format: inputname_filtered_h_k_r1_r2.root
        auto format_val = [](double val) {
            TString s = TString::Format("%.3g", val);
            s.ReplaceAll(".", "_");
            s.ReplaceAll("-", "m");
            return s;
        };
        
        TString inpath = infile;
        TString indir = gSystem->DirName(inpath);
        TString base = gSystem->BaseName(inpath);
        base.ReplaceAll(".root", "");
        
        outpath_t = TString::Format("%s/%s_filtered_%s_%s_%s_%s.root",
            indir.Data(), base.Data(),
            format_val(h).Data(),
            format_val(k).Data(),
            format_val(r1).Data(),
            format_val(r2).Data()
        );
    }
    
    // Create output file
    TFile* fout = TFile::Open(outpath_t, "RECREATE");
    if (!fout || fout->IsZombie()) {
        std::cerr << "Error: Cannot create output file " << outpath_t.Data() << std::endl;
        fin->Close();
        delete fin;
        return;
    }
    
    std::cout << "Processing file: " << infile << std::endl;
    std::cout << "Annulus filter: h=" << h << ", k=" << k 
              << ", r1=" << r1 << ", r2=" << r2 << std::endl;
    std::cout << "Keeping bins with " << r1 << " <= r <= " << r2 << " cm" << std::endl;
    
    // Process all directories and objects recursively
    ProcessDirectory(fin, fout, h, k, r1, r2);
    
    // Close files
    fout->Close();
    fin->Close();
    delete fout;
    delete fin;
    
    std::cout << "Output saved to: " << outpath_t.Data() << std::endl;
}

/*
Usage examples:

1) Default annulus filter (keep 1-10 cm from origin):
   .L cutoutcenter.c
   cutoutcenter("input.root")

2) Custom annulus (keep 1.3-5 cm):
   cutoutcenter("input.root", 0.0, 0.0, 1.3, 5.0)

3) Specify output filename:
   cutoutcenter("input.root", 0.0, 0.0, 1.0, 10.0, "filtered_output.root")

4) Remove beam pipe + keep inner tracker (1-10 cm):
   cutoutcenter("combined_histograms.root", 0.0, 0.0, 1.0, 10.0)

5) Keep specific annulus for vertex studies (0.5-3 cm):
   cutoutcenter("combined.root", 0.0, 0.0, 0.5, 3.0, "vertices_0p5_3cm.root")

Notes:
- All TH2 histograms KEEP bins with r1 <= r <= r2 (annulus)
- Bins with r < r1 OR r > r2 are ZEROED
- Other objects (TH1, TTree, etc.) are copied unchanged
- Directory structure is preserved in the output
- Useful for removing detector holes (r < r1) and far outliers (r > r2)
*/
