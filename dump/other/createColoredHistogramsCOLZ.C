#include <TFile.h>
#include <TDirectory.h>
#include <TH2.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TColor.h>
#include <iostream>

void createColoredHistogramsCOLZ() {
    TFile *inputFile = TFile::Open("combined_histograms.root");
    if (!inputFile || inputFile->IsZombie()) {
        std::cerr << "Error: Cannot open combined_histograms.root" << std::endl;
        return;
    }

    TDirectory *inputDir = (TDirectory*)inputFile->Get("scoutingTree");
    if (!inputDir) {
        std::cerr << "Error: Cannot access scoutingTree directory" << std::endl;
        inputFile->Close();
        delete inputFile;
        return;
    }

    TFile *outputFile = new TFile("coloredCombinedHistogramsCOLZ.root", "RECREATE");
    TDirectory *outputDir = outputFile->mkdir("scoutingTree");
    outputDir->cd();

    // Red to blue palette
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

    TCanvas *c1 = new TCanvas("c1", "c1", 600, 400);
    gStyle->SetOptStat(0);

    TH2F *h1 = (TH2F*)inputDir->Get("vertex_xy_global_combined");
    if (h1) {
        TH2F *h1Clone = (TH2F*)h1->Clone("vertex_xy_global_combined_COLZ");
        h1Clone->SetDirectory(0);
        c1->cd();
        h1Clone->Draw("COLZ");
        c1->SetLogz();
        outputDir->cd();
        h1Clone->Write();
    }

    TH2F *h2 = (TH2F*)inputDir->Get("vertex_xy_beamspot_combined");
    if (h2) {
        TH2F *h2Clone = (TH2F*)h2->Clone("vertex_xy_beamspot_combined_COLZ");
        h2Clone->SetDirectory(0);
        c1->cd();
        h2Clone->Draw("COLZ");
        c1->SetLogz();
        outputDir->cd();
        h2Clone->Write();
    }

    TH2F *h3 = (TH2F*)inputDir->Get("vertex_xy_global_ntracks2_combined");
    if (h3) {
        TH2F *h3Clone = (TH2F*)h3->Clone("vertex_xy_global_ntracks2_combined_COLZ");
        h3Clone->SetDirectory(0);
        c1->cd();
        h3Clone->Draw("COLZ");
        c1->SetLogz();
        outputDir->cd();
        h3Clone->Write();
    }

    TH2F *h4 = (TH2F*)inputDir->Get("vertex_xy_beamspot_ntracks2_combined");
    if (h4) {
        TH2F *h4Clone = (TH2F*)h4->Clone("vertex_xy_beamspot_ntracks2_combined_COLZ");
        h4Clone->SetDirectory(0);
        c1->cd();
        h4Clone->Draw("COLZ");
        c1->SetLogz();
        outputDir->cd();
        h4Clone->Write();
    }

    TH2F *h5 = (TH2F*)inputDir->Get("vertex_xy_beamspot_not_ntracks2_combined");
    if (h5) {
        TH2F *h5Clone = (TH2F*)h5->Clone("vertex_xy_beamspot_not_ntracks2_combined_COLZ");
        h5Clone->SetDirectory(0);
        c1->cd();
        h5Clone->Draw("COLZ");
        c1->SetLogz();
        outputDir->cd();
        h5Clone->Write();
    }

    TH2F *h6 = (TH2F*)inputDir->Get("beamspot_global_combined");
    if (h6) {
        TH2F *h6Clone = (TH2F*)h6->Clone("beamspot_global_combined_COLZ");
        h6Clone->SetDirectory(0);
        c1->cd();
        h6Clone->Draw("COLZ");
        c1->SetLogz();
        outputDir->cd();
        h6Clone->Write();
    }

    outputFile->Close();
    delete outputFile;
    inputFile->Close();
    delete inputFile;
    delete c1;

    std::cout << "COLZ histograms saved in coloredCombinedHistogramsCOLZ.root" << std::endl;
}