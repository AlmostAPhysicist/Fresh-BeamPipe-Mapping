#include <TFile.h>
#include <TDirectory.h>
#include <TH2.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TColor.h>
#include <iostream>

void createColoredHistogramsSURF7() {
    // Open the input file
    TFile *inputFile = TFile::Open("combined_histograms.root");
    if (!inputFile || inputFile->IsZombie()) {
        std::cerr << "Error: Cannot open combined_histograms.root" << std::endl;
        return;
    }

    // Access the scoutingTree directory
    TDirectory *inputDir = (TDirectory*)inputFile->Get("scoutingTree");
    if (!inputDir) {
        std::cerr << "Error: Cannot access scoutingTree directory" << std::endl;
        inputFile->Close();
        delete inputFile;
        return;
    }

    // Create output file
    TFile *outputFile = new TFile("coloredCombinedHistogramsSURF7.root", "RECREATE");
    TDirectory *outputDir = outputFile->mkdir("scoutingTree");
    outputDir->cd();

    // Red-to-blue palette (linear)
    const Int_t nColors = 100;
    Double_t stops[2] = {0.0, 1.0};
    Double_t red[2]   = {1.0, 0.0};
    Double_t green[2] = {0.0, 0.0};
    Double_t blue[2]  = {0.0, 1.0};
    TColor::CreateGradientColorTable(2, stops, red, green, blue, nColors);
    Int_t palette[nColors];
    for (Int_t i = 0; i < nColors; i++) {
        palette[i] = TColor::GetColorPalette(i);
    }
    gStyle->SetPalette(nColors, palette);

    // Canvas
    TCanvas *c2 = new TCanvas("c2", "c2", 600, 400);
    gStyle->SetOptStat(0);

    // Process each histogram
    TH2F *hsurf7_1 = (TH2F*)inputDir->Get("vertex_xy_global_combined");
    if (hsurf7_1) {
        TH2F *hClone1 = (TH2F*)hsurf7_1->Clone("vertex_xy_global_combined_SURF7");
        hClone1->SetDirectory(0);
        c2->cd();
        hClone1->Draw("SURF7");
        outputDir->cd();
        hClone1->Write();
    }

    TH2F *hsurf7_2 = (TH2F*)inputDir->Get("vertex_xy_beamspot_combined");
    if (hsurf7_2) {
        TH2F *hClone2 = (TH2F*)hsurf7_2->Clone("vertex_xy_beamspot_combined_SURF7");
        hClone2->SetDirectory(0);
        c2->cd();
        hClone2->Draw("SURF7");
        outputDir->cd();
        hClone2->Write();
    }

    TH2F *hsurf7_3 = (TH2F*)inputDir->Get("vertex_xy_global_ntracks2_combined");
    if (hsurf7_3) {
        TH2F *hClone3 = (TH2F*)hsurf7_3->Clone("vertex_xy_global_ntracks2_combined_SURF7");
        hClone3->SetDirectory(0);
        c2->cd();
        hClone3->Draw("SURF7");
        outputDir->cd();
        hClone3->Write();
    }

    TH2F *hsurf7_4 = (TH2F*)inputDir->Get("vertex_xy_beamspot_ntracks2_combined");
    if (hsurf7_4) {
        TH2F *hClone4 = (TH2F*)hsurf7_4->Clone("vertex_xy_beamspot_ntracks2_combined_SURF7");
        hClone4->SetDirectory(0);
        c2->cd();
        hClone4->Draw("SURF7");
        outputDir->cd();
        hClone4->Write();
    }

    TH2F *hsurf7_5 = (TH2F*)inputDir->Get("vertex_xy_beamspot_not_ntracks2_combined");
    if (hsurf7_5) {
        TH2F *hClone5 = (TH2F*)hsurf7_5->Clone("vertex_xy_beamspot_not_ntracks2_combined_SURF7");
        hClone5->SetDirectory(0);
        c2->cd();
        hClone5->Draw("SURF7");
        outputDir->cd();
        hClone5->Write();
    }

    TH2F *hsurf7_6 = (TH2F*)inputDir->Get("beamspot_global_combined");
    if (hsurf7_6) {
        TH2F *hClone6 = (TH2F*)hsurf7_6->Clone("beamspot_global_combined_SURF7");
        hClone6->SetDirectory(0);
        c2->cd();
        hClone6->Draw("SURF7");
        outputDir->cd();
        hClone6->Write();
    }

    // Clean up
    outputFile->Close();
    delete outputFile;
    inputFile->Close();
    delete inputFile;
    delete c2;

    std::cout << "SURF7 histograms saved in coloredCombinedHistogramsSURF7.root" << std::endl;
}