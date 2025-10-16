#include "TPaveText.h"
#include "TLatex.h"
#include "TString.h"
#include "TH2.h"
#include "TMath.h"
#include "TPaletteAxis.h" // added for palette handling

void QuickCMSFormat(TH2F* h, TString filename) {
    // Axis titles and fonts
    h->SetStats(0); // Disable default stats box to remove duplicate
    h->SetTitle(""); // Remove any histogram title to avoid duplicate
    h->GetXaxis()->SetTitle("x (cm)");
    h->GetYaxis()->SetTitle("y (cm)");
    h->GetZaxis()->SetTitle("Entries/(0.1#times0.1 mm^{2})"); // Include bin area
    h->GetXaxis()->SetTitleFont(42);
    h->GetYaxis()->SetTitleFont(42);
    h->GetZaxis()->SetTitleFont(42);
    h->GetXaxis()->SetTitleSize(0.05);
    h->GetYaxis()->SetTitleSize(0.05);
    h->GetZaxis()->SetTitleSize(0.04); // Shrink Z title
    h->GetXaxis()->SetTitleOffset(0.9); // Closer to axis
    h->GetYaxis()->SetTitleOffset(1.0); // Closer to axis
    // h->GetZaxis()->SetTitleOffset(0.5); // Center/adjust Z title position
    
    // Canvas: wider and taller to fit everything and fix aspect
    TCanvas* c = new TCanvas("c", "", 1200, 1145); // Increased width and height
    c->SetLogz();
    c->SetRightMargin(0.25);  // Room for color bar
    c->SetTopMargin(0.20);    // Room for title and labels
    c->SetLeftMargin(0.15);   // Room for y-axis labels
    c->SetBottomMargin(0.15); // Room for x-axis labels

    // compute numbers from the histogram itself
    Double_t entries = h->GetEntries();
    Double_t meanX = h->GetMean(1);
    Double_t meanY = h->GetMean(2);
    Double_t stdX  = h->GetStdDev(1);
    Double_t stdY  = h->GetStdDev(2);

    // bin sizes (assume uniform bins)
    Double_t binX = h->GetXaxis()->GetBinWidth(1);
    Double_t binY = h->GetYaxis()->GetBinWidth(1);

    // try to extract units from axis titles like "x (cm)"
    TString xTitle = h->GetXaxis()->GetTitle();
    TString yTitle = h->GetYaxis()->GetTitle();
    TString xUnit = "";
    TString yUnit = "";
    Ssiz_t p1 = xTitle.Index("(");
    Ssiz_t p2 = xTitle.Index(")");
    if (p1 != kNPOS && p2 > p1) xUnit = xTitle(p1+1, p2-p1-1);
    p1 = yTitle.Index("(");
    p2 = yTitle.Index(")");
    if (p1 != kNPOS && p2 > p1) yUnit = yTitle(p1+1, p2-p1-1);

    // prefer using xUnit if both equal, otherwise show both units separated
    TString unitStr;
    if (xUnit.Length() && xUnit == yUnit) {
        unitStr = xUnit;
    } else if (xUnit.Length() && yUnit.Length()) {
        unitStr = Form("%s#times%s", xUnit.Data(), yUnit.Data());
    } else if (xUnit.Length()) {
        unitStr = xUnit;
    } else if (yUnit.Length()) {
        unitStr = yUnit;
    } else {
        unitStr = ""; // unknown unit
    }

    // Z axis title: show bin area using the per-axis bin widths
    if (unitStr.Length())
        h->GetZaxis()->SetTitle(Form("Entries/(%.3g#times%.3g %s^{2})", binX, binY, unitStr.Data()));
    else
        h->GetZaxis()->SetTitle(Form("Entries/(%.3g#times%.3g)", binX, binY));

    c->cd();
    h->Draw("COLZ");
    c->Update(); // Ensure histogram is drawn before overlays
    gPad->SetFixedAspectRatio(1); // Ensure plot is square

    // Increase number of labels on the color palette (if present)
    TPaletteAxis* palette = (TPaletteAxis*)gPad->GetPrimitive("palette");
    if (!palette && h->GetListOfFunctions()) palette = (TPaletteAxis*)h->GetListOfFunctions()->FindObject("palette");
    if (palette) {
        palette->SetNdivisions(510); // more divisions/labels
        palette->SetLabelSize(0.035);
        c->Update();

        // capture the Z title, clear defaults to avoid overlap, and draw custom lower label
        TString ztitle = h->GetZaxis()->GetTitle();
        h->GetZaxis()->SetTitle("");
        palette->SetTitle("");
        // draw custom vertical Z title lower along the palette (NDC coords)
        TLatex* zt = new TLatex(0.9, 0.5, ztitle); // lowered y from ~0.66 to ~0.5
        zt->SetNDC();
        zt->SetTextAngle(90);      // vertical
        zt->SetTextAlign(22);      // centered
        zt->SetTextFont(42);
        zt->SetTextSize(0.045);
        zt->Draw();
        c->Update();
        // keep zt in primitives so it remains visible
    }

    // Custom stats box: use computed numbers instead of hardcoded values
    TPaveText* stats = new TPaveText(0.75, 0.80, 0.98, 0.94, "NDC");
    stats->SetBorderSize(1);
    stats->SetFillColor(0);
    stats->SetTextFont(42);
    stats->SetTextSize(0.03); // Smaller text to fit
    stats->SetTextAlign(12);
    stats->AddText(Form("Entries %.0f", entries));
    stats->AddText(Form("Mean x %.4g", meanX));
    stats->AddText(Form("Mean y %.4g", meanY));
    stats->AddText(Form("Std Dev x %.4g", stdX));
    stats->AddText(Form("Std Dev y %.4g", stdY));
    stats->Draw();

    // Title: top center, slightly down
    TLatex* title = new TLatex(0.5, 0.96, "Vertex XY Position Global, Region C (40 <= nPV)");
    title->SetNDC();
    title->SetTextFont(42);
    title->SetTextSize(0.04);
    title->SetTextAlign(21); // Center
    title->Draw();

    // CMS: top left, above plot
    TLatex* cms = new TLatex(0.15, 0.93, "CMS");
    cms->SetNDC();
    cms->SetTextFont(61);
    cms->SetTextSize(0.055);
    cms->SetTextAlign(13); // Left, top
    cms->Draw();

    // Preliminary: right of CMS
    TLatex* prelim = new TLatex(0.32, 0.93, "Preliminary");
    prelim->SetNDC();
    prelim->SetTextFont(52);
    prelim->SetTextSize(0.04);
    prelim->SetTextAlign(13);
    prelim->Draw();

    // Data/energy: below CMS, left aligned
    TLatex* data = new TLatex(0.15, 0.88, "Data 2024, 13.6 TeV");
    data->SetNDC();
    data->SetTextFont(42);
    data->SetTextSize(0.04);
    data->SetTextAlign(13); // Left, top
    data->Draw();

    // Binning info: left side, below data
    TLatex* bin = new TLatex(0.15, 0.83, "100 #mu m #times 100 #mu m");
    bin->SetNDC();
    bin->SetTextSize(0.035);
    bin->SetTextFont(42);
    bin->SetTextAlign(13);
    bin->Draw();

    // Dataset: bottom left
    TLatex* ds = new TLatex(0.15, 0.04, "/ScoutingPFRun3/Run2024H-v1/HLTSCOUT");
    ds->SetNDC();
    ds->SetTextAlign(13); // Left, bottom
    ds->SetTextSize(0.035);
    ds->SetTextFont(42);
    ds->Draw();

    c->SaveAs(filename);
    delete c;
}