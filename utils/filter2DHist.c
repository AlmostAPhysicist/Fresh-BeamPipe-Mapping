#include <TFile.h>
#include <TH2.h>
#include <TString.h>
#include <TSystem.h>
#include <iostream>
#include <cmath>

void filter2DHist(const char* infile, double h, double k, double r1, double r2) {
    // Open input file
    TFile* fin = TFile::Open(infile, "READ");
    if (!fin || fin->IsZombie()) {
        std::cerr << "Error: Cannot open input file " << infile << std::endl;
        return;
    }

    // Get the histogram
    TH2* hist = nullptr;
    TDirectory* dir = (TDirectory*)fin->Get("scoutingTree");
    if (dir) {
        hist = (TH2*)dir->Get("vertex_xy_global_combined");
    }
    if (!hist) {
        std::cerr << "Error: Histogram vertex_xy_global_combined not found in scoutingTree." << std::endl;
        fin->Close();
        return;
    }

    // Define new histogram bounds
    double x_min = h - r2 - 0.5;
    double x_max = h + r2 + 0.5;
    double y_min = k - r2 - 0.5;
    double y_max = k + r2 + 0.5;

    // Use same bin numbers as original, or optionally recalculate
    int nBinsX = hist->GetNbinsX();
    int nBinsY = hist->GetNbinsY();

    // Create new histogram with new bounds
    TString filterDesc = TString::Format("; filtered: h=%.3g, k=%.3g, r1=%.3g, r2=%.3g", h, k, r1, r2);
    TString newTitle = TString::Format("%s%s", hist->GetTitle(), filterDesc.Data());
    TH2* filteredHist = new TH2D("filtered_vertex_xy_global_combined",
                                 newTitle,
                                 nBinsX, x_min, x_max,
                                 nBinsY, y_min, y_max);
    // Copy axis labels
    filteredHist->GetXaxis()->SetTitle(hist->GetXaxis()->GetTitle());
    filteredHist->GetYaxis()->SetTitle(hist->GetYaxis()->GetTitle());

    // Loop over bins of the new histogram and fill from original if within annulus
    for (int ix = 1; ix <= nBinsX; ++ix) {
        double x = filteredHist->GetXaxis()->GetBinCenter(ix);
        for (int iy = 1; iy <= nBinsY; ++iy) {
            double y = filteredHist->GetYaxis()->GetBinCenter(iy);
            double r = std::sqrt((x-h)*(x-h) + (y-k)*(y-k));
            if (r >= r1 && r <= r2) {
                // Find corresponding bin in original histogram
                int orig_ix = hist->GetXaxis()->FindBin(x);
                int orig_iy = hist->GetYaxis()->FindBin(y);
                filteredHist->SetBinContent(ix, iy, hist->GetBinContent(orig_ix, orig_iy));
                filteredHist->SetBinError(ix, iy, hist->GetBinError(orig_ix, orig_iy));
            }
        }
    }

    // Helper lambda to convert double to string with underscores instead of dots
    auto format_val = [](double val) {
        TString s = TString::Format("%.3g", val);
        s.ReplaceAll(".", "_");
        s.ReplaceAll("-", "m");
        return s;
    };

    // Compose output file path in same folder as input
    TString inpath = infile;
    TString indir = gSystem->DirName(inpath);
    TString base = gSystem->BaseName(inpath);
    base.ReplaceAll(".root", "");
    TString outname = TString::Format("%s__%s__%s__%s__%s.root",
        base.Data(),
        format_val(h).Data(),
        format_val(k).Data(),
        format_val(r1).Data(),
        format_val(r2).Data()
    );
    TString outpath = indir + "/" + outname;

    TFile* fout = TFile::Open(outpath, "RECREATE");
    filteredHist->Write();
    fout->Close();

    std::cout << "Filtered histogram saved to " << outpath << std::endl;

    fin->Close();
}

// helper to format values (underscores instead of dots, m for minus)
static TString format_val_str(double val) {
    TString s = TString::Format("%.3g", val);
    s.ReplaceAll(".", "_");
    s.ReplaceAll("-", "m");
    return s;
}

// New overload: operate directly on an in-memory histogram.
// If outpath==nullptr a filename is constructed from the histogram name and parameters.
void filter2DHist(TH2* hist, double h, double k, double r1, double r2, const char* outpath = nullptr) {
    if (!hist) {
        std::cerr << "Error: null histogram passed to filter2DHist(TH2*, ...)" << std::endl;
        return;
    }

    // Define new histogram bounds (similar to file-based version)
    double x_min = h - r2 - 0.5;
    double x_max = h + r2 + 0.5;
    double y_min = k - r2 - 0.5;
    double y_max = k + r2 + 0.5;

    // Use same bin numbers as original
    int nBinsX = hist->GetNbinsX();
    int nBinsY = hist->GetNbinsY();

    // Create new histogram with new bounds
    TString filterDesc = TString::Format("; filtered: h=%.3g, k=%.3g, r1=%.3g, r2=%.3g", h, k, r1, r2);
    TString newTitle = TString::Format("%s%s", hist->GetTitle(), filterDesc.Data());
    TH2* filteredHist = new TH2D("filtered_vertex_xy_global_combined",
                                 newTitle,
                                 nBinsX, x_min, x_max,
                                 nBinsY, y_min, y_max);
    // Copy axis labels
    filteredHist->GetXaxis()->SetTitle(hist->GetXaxis()->GetTitle());
    filteredHist->GetYaxis()->SetTitle(hist->GetYaxis()->GetTitle());

    // Loop over bins of the new histogram and fill from original if within annulus
    for (int ix = 1; ix <= nBinsX; ++ix) {
        double x = filteredHist->GetXaxis()->GetBinCenter(ix);
        for (int iy = 1; iy <= nBinsY; ++iy) {
            double y = filteredHist->GetYaxis()->GetBinCenter(iy);
            double rr = std::sqrt((x-h)*(x-h) + (y-k)*(y-k));
            if (rr >= r1 && rr <= r2) {
                // Find corresponding bin in original histogram
                int orig_ix = hist->GetXaxis()->FindBin(x);
                int orig_iy = hist->GetYaxis()->FindBin(y);
                filteredHist->SetBinContent(ix, iy, hist->GetBinContent(orig_ix, orig_iy));
                filteredHist->SetBinError(ix, iy, hist->GetBinError(orig_ix, orig_iy));
            }
        }
    }

    // Build output path if not provided
    TString outpath_t;
    if (outpath && std::strlen(outpath) > 0) {
        outpath_t = outpath;
    } else {
        TString base = hist->GetName();
        base.ReplaceAll(".root", "");
        outpath_t = TString::Format("%s__%s__%s__%s__%s.root",
            base.Data(),
            format_val_str(h).Data(),
            format_val_str(k).Data(),
            format_val_str(r1).Data(),
            format_val_str(r2).Data()
        );
    }

    // Write to file
    TFile* fout = TFile::Open(outpath_t, "RECREATE");
    if (!fout || fout->IsZombie()) {
        std::cerr << "Error: cannot create output file " << outpath_t.Data() << std::endl;
        delete filteredHist;
        return;
    }
    filteredHist->Write();
    fout->Close();
    delete fout;
    delete filteredHist;

    std::cout << "Filtered histogram saved to " << outpath_t.Data() << std::endl;
}