// // // // #include <TFile.h>
// // // // #include <TDirectory.h>
// // // // #include <TH2.h>
// // // // #include <TCanvas.h>
// // // // #include <TStyle.h>
// // // // #include <TColor.h>
// // // // #include <TString.h>
// // // // #include <iostream>
// // // // #include <vector>

// // // // void createColoredHistograms() {
// // // //     // Open the input file with combined histograms
// // // //     TFile *inputFile = TFile::Open("combined_histograms.root");
// // // //     if (!inputFile || inputFile->IsZombie()) {
// // // //         std::cerr << "Error: Cannot open combined_histograms.root" << std::endl;
// // // //         return;
// // // //     }

// // // //     // Access the scoutingTree directory
// // // //     TDirectory *inputDir = (TDirectory*)inputFile->Get("scoutingTree");
// // // //     if (!inputDir) {
// // // //         std::cerr << "Error: Cannot access scoutingTree directory" << std::endl;
// // // //         inputFile->Close();
// // // //         delete inputFile;
// // // //         return;
// // // //     }

// // // //     // Create a new output file
// // // //     TFile *outputFile = new TFile("coloredCombinedHistograms.root", "RECREATE");
// // // //     TDirectory *outputDir = outputFile->mkdir("scoutingTree");
// // // //     outputDir->cd();

// // // //     // Define a custom color gradient: red (low) to blue (high)
// // // //     const Int_t nColors = 100;
// // // //     Double_t stops[3] = {0.0, 0.5, 1.0}; // Positions along the gradient
// // // //     Double_t red[3]   = {1.0, 1.0, 0.0}; // Red: full red -> yellow -> no red
// // // //     Double_t green[3] = {0.0, 1.0, 0.0}; // Green: no green -> yellow -> no green
// // // //     Double_t blue[3]  = {0.0, 0.0, 1.0}; // Blue: no blue -> yellow -> full blue
// // // //     TColor::CreateGradientColorTable(3, stops, red, green, blue, nColors);
// // // //     Int_t palette[nColors];
// // // //     for (Int_t i = 0; i < nColors; i++) {
// // // //         palette[i] = TColor::GetColorPalette(i);
// // // //     }
// // // //     gStyle->SetPalette(nColors, palette);

// // // //     // List of XY position histogram names (as C-style array)
// // // //     const char *xyHistNames[] = {
// // // //         "vertex_xy_global_combined",
// // // //         "vertex_xy_beamspot_combined",
// // // //         "vertex_xy_global_ntracks2_combined",
// // // //         "vertex_xy_beamspot_ntracks2_combined",
// // // //         "vertex_xy_beamspot_not_ntracks2_combined",
// // // //         "beamspot_global_combined"
// // // //     };
// // // //     int nHistNames = sizeof(xyHistNames) / sizeof(xyHistNames[0]);

// // // //     // Drawing styles (as C-style array)
// // // //     const char *drawStyles[] = {"COLZ", "CONTZ", "SURF7"};
// // // //     int nDrawStyles = sizeof(drawStyles) / sizeof(drawStyles[0]);

// // // //     // Canvas for rendering
// // // //     TCanvas *canvas = new TCanvas("canvas", "Histogram Canvas", 600, 400);
// // // //     gStyle->SetOptStat(0); // Disable stats box

// // // //     // Loop over histograms
// // // //     for (int h = 0; h < nHistNames; h++) {
// // // //         TH2F *hist = (TH2F*)inputDir->Get(xyHistNames[h]);
// // // //         if (!hist) {
// // // //             std::cerr << "Warning: Histogram " << xyHistNames[h] << " not found, skipping..." << std::endl;
// // // //             continue;
// // // //         }

// // // //         // Check for valid content
// // // //         if (hist->GetMaximum() <= 0) {
// // // //             std::cerr << "Warning: Histogram " << xyHistNames[h] << " has no positive content, skipping..." << std::endl;
// // // //             continue;
// // // //         }

// // // //         // Apply each drawing style and save
// // // //         for (int s = 0; s < nDrawStyles; s++) {
// // // //             canvas->cd();
// // // //             hist->Draw(drawStyles[s]); // Draw with the specified style
// // // //             canvas->SetLogz(); // Logarithmic Z-axis for color mapping

// // // //             // Clone and save the histogram with the style suffix
// // // //             TString newHistName = TString::Format("%s_%s", xyHistNames[h], drawStyles[s]);
// // // //             TH2F *histClone = (TH2F*)hist->Clone(newHistName);
// // // //             histClone->SetDirectory(outputDir);
// // // //             histClone->Write();
// // // //         }
// // // //     }

// // // //     // Clean up
// // // //     outputFile->Close();
// // // //     delete outputFile;
// // // //     inputFile->Close();
// // // //     delete inputFile;
// // // //     delete canvas;

// // // //     std::cout << "Histograms with COLZ, CONTZ, and SURF7 styles saved in coloredCombinedHistograms.root" << std::endl;
// // // // }


// // // #include <TFile.h>
// // // #include <TDirectory.h>
// // // #include <TH2.h>
// // // #include <TCanvas.h>
// // // #include <TStyle.h>
// // // #include <TColor.h>
// // // #include <TRandom.h>
// // // #include <iostream>

// // // void createColoredHistograms() {
// // //     // Create a new output file
// // //     TFile *outputFile = new TFile("coloredCombinedHistograms.root", "RECREATE");
// // //     TDirectory *outputDir = outputFile->mkdir("scoutingTree");
// // //     outputDir->cd();

// // //     // Create canvas and histogram
// // //     TCanvas *c1 = new TCanvas("c1", "c1", 600, 400);
// // //     TH2F *hcol1 = new TH2F("hcol1", "Option COLor example ", 40, -4, 4, 40, -20, 20);

// // //     // Fill histogram with random data
// // //     float px, py;
// // //     for (Int_t i = 0; i < 25000; i++) {
// // //         gRandom->Rannor(px, py);
// // //         hcol1->Fill(px, 5 * py);
// // //     }

// // //     // Draw with COLZ
// // //     c1->cd();
// // //     hcol1->Draw("COLZ");

// // //     // Save to the output directory
// // //     outputDir->cd();
// // //     hcol1->Write();

// // //     // Clean up
// // //     outputFile->Close();
// // //     delete outputFile;
// // //     delete c1; // Deletes the canvas and histogram (since histogram is owned by canvas)
    
// // //     std::cout << "Simple COLZ histogram saved in coloredCombinedHistograms.root" << std::endl;
// // // }

// // #include <TFile.h>
// // #include <TDirectory.h>
// // #include <TH2.h>
// // #include <TCanvas.h>
// // #include <TStyle.h>
// // #include <TColor.h>
// // #include <TString.h>
// // #include <iostream>
// // #include <cmath>

// // void createColoredHistograms() {
// //     // Open the input file with combined histograms
// //     TFile *inputFile = TFile::Open("combined_histograms.root");
// //     if (!inputFile || inputFile->IsZombie()) {
// //         std::cerr << "Error: Cannot open combined_histograms.root" << std::endl;
// //         return;
// //     }

// //     // Access the scoutingTree directory
// //     TDirectory *inputDir = (TDirectory*)inputFile->Get("scoutingTree");
// //     if (!inputDir) {
// //         std::cerr << "Error: Cannot access scoutingTree directory" << std::endl;
// //         inputFile->Close();
// //         delete inputFile;
// //         return;
// //     }

// //     // Create a new output file
// //     TFile *outputFile = new TFile("coloredCombinedHistograms.root", "RECREATE");
// //     TDirectory *outputDir = outputFile->mkdir("scoutingTree");
// //     outputDir->cd();

// //     // Define a custom color gradient: red (low) to blue (high)
// //     const Int_t nColors = 100;
// //     Double_t stops[3] = {0.0, 0.5, 1.0};
// //     Double_t red[3]   = {1.0, 1.0, 0.0}; // Red -> Yellow -> Blue
// //     Double_t green[3] = {0.0, 1.0, 0.0};
// //     Double_t blue[3]  = {0.0, 0.0, 1.0};
// //     TColor::CreateGradientColorTable(3, stops, red, green, blue, nColors);
// //     Int_t palette[nColors];
// //     for (Int_t i = 0; i < nColors; i++) {
// //         palette[i] = TColor::GetColorPalette(i);
// //     }
// //     gStyle->SetPalette(nColors, palette);

// //     // List of XY position histogram names
// //     const char *xyHistNames[] = {
// //         "vertex_xy_global_combined",
// //         "vertex_xy_beamspot_combined",
// //         "vertex_xy_global_ntracks2_combined",
// //         "vertex_xy_beamspot_ntracks2_combined",
// //         "vertex_xy_beamspot_not_ntracks2_combined",
// //         "beamspot_global_combined"
// //     };
// //     int nHistNames = sizeof(xyHistNames) / sizeof(xyHistNames[0]);

// //     // Canvas for rendering
// //     TCanvas *canvas = new TCanvas("canvas", "Histogram Canvas", 600, 400);
// //     gStyle->SetOptStat(0);

// //     // Loop over histograms
// //     for (int h = 0; h < nHistNames; h++) {
// //         TH2F *hist = (TH2F*)inputDir->Get(xyHistNames[h]);
// //         if (!hist) {
// //             std::cerr << "Warning: Histogram " << xyHistNames[h] << " not found, skipping..." << std::endl;
// //             continue;
// //         }

// //         // Check for valid content
// //         Double_t maxContent = hist->GetMaximum();
// //         if (maxContent <= 0) {
// //             std::cerr << "Warning: Histogram " << xyHistNames[h] << " has no positive content, skipping..." << std::endl;
// //             continue;
// //         }

// //         // Create a new histogram for logarithmic mapping
// //         TString newHistName = TString::Format("%s_colored", xyHistNames[h]);
// //         TH2F *histColored = (TH2F*)hist->Clone(newHistName);
// //         histColored->SetDirectory(0);

// //         // Loop over bins and apply logarithmic mapping
// //         Int_t nBinsX = hist->GetNbinsX();
// //         Int_t nBinsY = hist->GetNbinsY();
// //         Double_t minContent = hist->GetMinimum(0.1); // Small positive value to avoid log(0)
// //         Double_t logMin = std::log10(minContent);
// //         Double_t logMax = std::log10(maxContent);

// //         for (Int_t i = 1; i <= nBinsX; i++) {
// //             for (Int_t j = 1; j <= nBinsY; j++) {
// //                 Double_t content = hist->GetBinContent(i, j);
// //                 if (content <= 0) {
// //                     histColored->SetBinContent(i, j, 0);
// //                     continue;
// //                 }
// //                 // Logarithmic mapping to [0, 1] range
// //                 Double_t logContent = std::log10(content);
// //                 Double_t fraction = (logContent - logMin) / (logMax - logMin);
// //                 if (fraction < 0) fraction = 0;
// //                 if (fraction > 1) fraction = 1;
// //                 // Scale to palette range (0 to nColors-1)
// //                 Double_t newContent = fraction * (nColors - 1);
// //                 histColored->SetBinContent(i, j, newContent);
// //             }
// //         }

// //         // Draw and save
// //         canvas->cd();
// //         histColored->Draw("COLZ");
// //         outputDir->cd();
// //         histColored->Write();
// //     }

// //     // Clean up
// //     outputFile->Close();
// //     delete outputFile;
// //     inputFile->Close();
// //     delete inputFile;
// //     delete canvas;

// //     std::cout << "Histograms with logarithmic red-to-blue color mapping saved in coloredCombinedHistograms.root" << std::endl;
// // }


// #include <TFile.h>
// #include <TDirectory.h>
// #include <TH2.h>
// #include <TCanvas.h>
// #include <TStyle.h>
// #include <TColor.h>
// #include <TString.h>
// #include <iostream>
// #include <cmath>

// void createColoredHistograms() {
//     // Open the input file with combined histograms
//     TFile *inputFile = TFile::Open("combined_histograms.root");
//     if (!inputFile || inputFile->IsZombie()) {
//         std::cerr << "Error: Cannot open combined_histograms.root" << std::endl;
//         return;
//     }

//     // Access the scoutingTree directory
//     TDirectory *inputDir = (TDirectory*)inputFile->Get("scoutingTree");
//     if (!inputDir) {
//         std::cerr << "Error: Cannot access scoutingTree directory" << std::endl;
//         inputFile->Close();
//         delete inputFile;
//         return;
//     }

//     // Create a new output file
//     TFile *outputFile = new TFile("coloredCombinedHistograms.root", "RECREATE");
//     TDirectory *outputDir = outputFile->mkdir("scoutingTree");
//     outputDir->cd();

//     // Define a custom color gradient: red (low) to blue (high)
//     const Int_t nColors = 100;
//     Double_t stops[3] = {0.0, 0.5, 1.0};
//     Double_t red[3]   = {1.0, 1.0, 0.0}; // Red -> Yellow -> Blue
//     Double_t green[3] = {0.0, 1.0, 0.0};
//     Double_t blue[3]  = {0.0, 0.0, 1.0};
//     TColor::CreateGradientColorTable(3, stops, red, green, blue, nColors);
//     Int_t palette[nColors];
//     for (Int_t i = 0; i < nColors; i++) {
//         palette[i] = TColor::GetColorPalette(i);
//     }
//     gStyle->SetPalette(nColors, palette);

//     // List of XY position histogram names
//     const char *xyHistNames[] = {
//         "vertex_xy_global_combined",
//         "vertex_xy_beamspot_combined",
//         "vertex_xy_global_ntracks2_combined",
//         "vertex_xy_beamspot_ntracks2_combined",
//         "vertex_xy_beamspot_not_ntracks2_combined",
//         "beamspot_global_combined"
//     };
//     int nHistNames = sizeof(xyHistNames) / sizeof(xyHistNames[0]);

//     // Canvas for rendering
//     TCanvas *canvas = new TCanvas("canvas", "Histogram Canvas", 600, 400);
//     gStyle->SetOptStat(0);

//     // Loop over histograms
//     for (int h = 0; h < nHistNames; h++) {
//         TH2F *hist = (TH2F*)inputDir->Get(xyHistNames[h]);
//         if (!hist) {
//             std::cerr << "Warning: Histogram " << xyHistNames[h] << " not found, skipping..." << std::endl;
//             continue;
//         }

//         // Check for valid content
//         Double_t maxContent = hist->GetMaximum();
//         if (maxContent <= 0) {
//             std::cerr << "Warning: Histogram " << xyHistNames[h] << " has no positive content, skipping..." << std::endl;
//             continue;
//         }

//         // Create a new histogram for logarithmic mapping
//         TString newHistName = TString::Format("%s_colored", xyHistNames[h]);
//         TH2F *histColored = (TH2F*)hist->Clone(newHistName);
//         histColored->SetDirectory(0);

//         // Loop over bins and apply logarithmic mapping
//         Int_t nBinsX = hist->GetNbinsX();
//         Int_t nBinsY = hist->GetNbinsY();
//         Double_t minContent = hist->GetMinimum(0.1); // Small positive value to avoid log(0)
//         Double_t logMin = std::log10(minContent);
//         Double_t logMax = std::log10(maxContent);

//         for (Int_t i = 1; i <= nBinsX; i++) {
//             for (Int_t j = 1; j <= nBinsY; j++) {
//                 Double_t content = hist->GetBinContent(i, j);
//                 if (content <= 0) {
//                     histColored->SetBinContent(i, j, 0);
//                     continue;
//                 }
//                 // Logarithmic mapping to [0, 1] range
//                 Double_t logContent = std::log10(content);
//                 Double_t fraction = (logContent - logMin) / (logMax - logMin);
//                 if (fraction < 0) fraction = 0;
//                 if (fraction > 1) fraction = 1;
//                 // Scale to palette range (0 to nColors-1)
//                 Double_t newContent = fraction * (nColors - 1);
//                 histColored->SetBinContent(i, j, newContent);
//             }
//         }

//         // Draw and save
//         canvas->cd();
//         histColored->Draw("COLZ");
//         outputDir->cd();
//         histColored->Write();
//     }

//     // Clean up
//     outputFile->Close();
//     delete outputFile;
//     inputFile->Close();
//     delete inputFile;
//     delete canvas;

//     std::cout << "Histograms with logarithmic red-to-blue color mapping saved in coloredCombinedHistograms.root" << std::endl;
// }



#include <TFile.h>
#include <TDirectory.h>
#include <TH2.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TColor.h>
#include <TString.h>
#include <iostream>
#include <cmath>

void createColoredHistograms() {
    // Open the input file with combined histograms
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

    // Create a new output file
    TFile *outputFile = new TFile("coloredCombinedHistograms3.root", "RECREATE");
    TDirectory *outputDir = outputFile->mkdir("scoutingTree");
    outputDir->cd();

    // Define a custom color gradient: red (low) to blue (high)
    const Int_t nColors = 100;
    Double_t stops[2] = {0.0, 1.0}; // Direct red-to-blue
    Double_t red[2]   = {1.0, 0.0};
    Double_t green[2] = {0.0, 0.0};
    Double_t blue[2]  = {0.0, 1.0};
    TColor::CreateGradientColorTable(2, stops, red, green, blue, nColors);
    Int_t palette[nColors];
    for (Int_t i = 0; i < nColors; i++) {
        palette[i] = TColor::GetColorPalette(i);
    }
    gStyle->SetPalette(nColors, palette);

    // List of XY position histogram names
    const char *xyHistNames[] = {
        "vertex_xy_global_combined",
        "vertex_xy_beamspot_combined",
        "vertex_xy_global_ntracks2_combined",
        "vertex_xy_beamspot_ntracks2_combined",
        "vertex_xy_beamspot_not_ntracks2_combined",
        "beamspot_global_combined"
    };
    int nHistNames = sizeof(xyHistNames) / sizeof(xyHistNames[0]);

    // Drawing styles
    const char *drawStyles[] = {"COLZ", "CONTZ", "SURF7"};
    int nDrawStyles = sizeof(drawStyles) / sizeof(drawStyles[0]);

    // Canvas for rendering
    TCanvas *canvas = new TCanvas("canvas", "Histogram Canvas", 600, 400);
    gStyle->SetOptStat(0);

    // Loop over histograms
    for (int h = 0; h < nHistNames; h++) {
        TH2F *hist = (TH2F*)inputDir->Get(xyHistNames[h]);
        if (!hist) {
            std::cerr << "Warning: Histogram " << xyHistNames[h] << " not found, skipping..." << std::endl;
            continue;
        }

        // Check for valid content
        Double_t maxContent = hist->GetMaximum();
        if (maxContent <= 0) {
            std::cerr << "Warning: Histogram " << xyHistNames[h] << " has no positive content, skipping..." << std::endl;
            continue;
        }

        // Apply each drawing style
        for (int s = 0; s < nDrawStyles; s++) {
            // Create a new histogram for this style
            TString newHistName = TString::Format("%s_%s", xyHistNames[h], drawStyles[s]);
            TH2F *histColored = (TH2F*)hist->Clone(newHistName);
            histColored->SetDirectory(0);

            // Loop over bins and apply logarithmic mapping
            Int_t nBinsX = hist->GetNbinsX();
            Int_t nBinsY = hist->GetNbinsY();
            Double_t minContent = hist->GetMinimum(0.1); // Small positive value to avoid log(0)
            Double_t logMin = std::log10(minContent);
            Double_t logMax = std::log10(maxContent);

            for (Int_t i = 1; i <= nBinsX; i++) {
                for (Int_t j = 1; j <= nBinsY; j++) {
                    Double_t content = hist->GetBinContent(i, j);
                    if (content <= 0) {
                        histColored->SetBinContent(i, j, 0);
                        continue;
                    }
                    // Logarithmic mapping to [0, 1] range
                    Double_t logContent = std::log10(content);
                    Double_t fraction = (logContent - logMin) / (logMax - logMin);
                    if (fraction < 0) fraction = 0;
                    if (fraction > 1) fraction = 1;
                    // Scale to palette range (0 to nColors-1)
                    Double_t newContent = fraction * (nColors - 1);
                    histColored->SetBinContent(i, j, newContent);
                }
            }

            // Draw and save
            canvas->cd();
            histColored->Draw(drawStyles[s]);
            outputDir->cd();
            histColored->Write();
        }
    }

    // Clean up
    outputFile->Close();
    delete outputFile;
    inputFile->Close();
    delete inputFile;
    delete canvas;

    std::cout << "Histograms with COLZ, CONTZ, and SURF7 styles (logarithmic red-to-blue mapping) saved in coloredCombinedHistograms.root" << std::endl;
}