#include <TFile.h>
#include <TH2.h>
#include <TDirectory.h>
#include <TKey.h>
#include <TString.h>
#include <TSystem.h>
#include <iostream>
#include <cmath>

// Helper to format values (underscores instead of dots, m for minus)
static TString format_val_str(double val) {
    TString s = TString::Format("%.3g", val);
    s.ReplaceAll(".", "_");
    s.ReplaceAll("-", "m");
    return s;
}

// Main function: filter a TH2 histogram (in-memory) and save to file
// h, k: center of annulus
// r1, r2: inner and outer radius
// outpath: output file path (if nullptr, auto-generated from histogram name)
void filter2DHist(TH2* hist, double h, double k, double r1, double r2, const char* outpath = nullptr) {
    if (!hist) {
        std::cerr << "Error: null histogram passed to filter2DHist" << std::endl;
        return;
    }

    // Define new histogram bounds
    double x_min = h - r2 - 0.5;
    double x_max = h + r2 + 0.5;
    double y_min = k - r2 - 0.5;
    double y_max = k + r2 + 0.5;

    int nBinsX = hist->GetNbinsX();
    int nBinsY = hist->GetNbinsY();

    // Create filtered histogram
    TString filterDesc = TString::Format("; filtered: h=%.3g, k=%.3g, r1=%.3g, r2=%.3g", h, k, r1, r2);
    TString newTitle = TString::Format("%s%s", hist->GetTitle(), filterDesc.Data());
    TH2* filteredHist = new TH2D("filtered_histogram",
                                 newTitle,
                                 nBinsX, x_min, x_max,
                                 nBinsY, y_min, y_max);
    filteredHist->GetXaxis()->SetTitle(hist->GetXaxis()->GetTitle());
    filteredHist->GetYaxis()->SetTitle(hist->GetYaxis()->GetTitle());

    // Fill filtered histogram (annulus filter)
    for (int ix = 1; ix <= nBinsX; ++ix) {
        double x = filteredHist->GetXaxis()->GetBinCenter(ix);
        for (int iy = 1; iy <= nBinsY; ++iy) {
            double y = filteredHist->GetYaxis()->GetBinCenter(iy);
            double r = std::sqrt((x-h)*(x-h) + (y-k)*(y-k));
            if (r >= r1 && r <= r2) {
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