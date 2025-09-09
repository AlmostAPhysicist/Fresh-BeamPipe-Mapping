#include <TFile.h>
#include <TDirectory.h>
#include <TH2.h>
#include <TKey.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <iostream>

void colorCodeHistograms() {
    // Open the input file
    TFile *inputFile = TFile::Open("combined_histograms.root");
    if (!inputFile || inputFile->IsZombie()) {
        std::cerr << "Error: Cannot open input file combined_histograms.root" << std::endl;
        return;
    }

    // Access the scoutingTree directory
    TDirectory *inputDir = (TDirectory*)inputFile->Get("scoutingTree");
    if (!inputDir) {
        std::cerr << "Error: scoutingTree directory not found in input file" << std::endl;
        inputFile->Close();
        return;
    }

    // Create the output file
    TFile *outputFile = new TFile("coloredCombinedHistograms.root", "RECREATE");
    TDirectory *outputDir = outputFile->mkdir("scoutingTree");
    outputDir->cd();

    // Set up the logarithmic color gradient
    const Int_t nColors = 50;
    const Int_t nLevels = 999;
    gStyle->SetNumberContours(nLevels);

    Double_t stops[] = {0.00, 0.50, 1.00};
    Double_t red[]   = {0.00, 0.00, 1.00};
    Double_t green[] = {0.00, 1.00, 0.00};
    Double_t blue[]  = {1.00, 0.00, 0.00};
    TColor::CreateGradientColorTable(3, stops, red, green, blue, nColors);
    gStyle->SetPalette(nColors);

    // Loop over all histograms in the input directory
    TIter next(inputDir->GetListOfKeys());
    TKey *key;
    while ((key = (TKey*)next())) {
        TString histName = key->GetName();
        TString className = key->GetClassName();

        // Only process 2D histograms (TH2F)
        if (className != "TH2F") continue;

        // Get the histogram
        TH2F *hist = (TH2F*)inputDir->Get(histName);
        if (!hist) continue;

        // Clone the histogram for output
        TH2F *coloredHist = (TH2F*)hist->Clone(TString::Format("%s_colored", histName.Data()));
        coloredHist->SetDirectory(0); // Detach from file

        // Apply logarithmic scale to the Z-axis
        TCanvas *c = new TCanvas("c", "Canvas", 800, 800);
        c->SetLogz();
        coloredHist->Draw("COLZ");

        // Save the histogram to the output file
        outputDir->cd();
        coloredHist->Write();

        // Clean up
        delete c;
        delete coloredHist;
    }

    // Close the files
    inputFile->Close();
    outputFile->Close();

    std::cout << "Color-coded histograms saved in coloredCombinedHistograms.root" << std::endl;
}