void QuickCMSFormat(TH2F* h, TString filename) {
    // Axis titles and fonts
    h->SetStats(0); // Disable default stats box to remove duplicate
    h->SetTitle(""); // Remove any histogram title to avoid duplicate
    h->GetXaxis()->SetTitle("x (cm)");
    h->GetYaxis()->SetTitle("y (cm)");
    h->GetZaxis()->SetTitle("Entries/(0.1#times0.1 mm^{2})             "); // Include bin area
    h->GetXaxis()->SetTitleFont(42);
    h->GetYaxis()->SetTitleFont(42);
    h->GetZaxis()->SetTitleFont(42);
    h->GetXaxis()->SetTitleSize(0.05);
    h->GetYaxis()->SetTitleSize(0.05);
    h->GetZaxis()->SetTitleSize(0.04); // Shrink Z title
    h->GetXaxis()->SetTitleOffset(0.9); // Closer to axis
    h->GetYaxis()->SetTitleOffset(1.0); // Closer to axis
    h->GetZaxis()->SetTitleOffset(1.2); // Center/adjust Z title position
    
    // Canvas: wider and taller to fit everything and fix aspect
    TCanvas* c = new TCanvas("c", "", 1200, 1145); // Increased width and height
    c->SetLogz();
    c->SetRightMargin(0.25);  // Room for color bar
    c->SetTopMargin(0.20);    // Room for title and labels
    c->SetLeftMargin(0.15);   // Room for y-axis labels
    c->SetBottomMargin(0.15); // Room for x-axis labels

    c->cd();
    h->Draw("COLZ");
    c->Update(); // Ensure histogram is drawn before overlays
    gPad->SetFixedAspectRatio(1); // Ensure plot is square

    // Custom stats box, moved to the right
    TPaveText* stats = new TPaveText(0.80, 0.78, 0.98, 0.92, "NDC");
    stats->SetBorderSize(1);
    stats->SetFillColor(0);
    stats->SetTextFont(42);
    stats->SetTextSize(0.03); // Smaller text to fit
    stats->SetTextAlign(12);
    stats->AddText("Entries 2808488");
    stats->AddText("Mean x 0.1541");
    stats->AddText("Mean y -0.3181");
    stats->AddText("Std Dev x 2.196");
    stats->AddText("Std Dev y 2.172");
    stats->Draw();

    // Title: top center, slightly down
    TLatex* title = new TLatex(0.5, 0.96, "Vertex XY Position (Global)");
    title->SetNDC();
    title->SetTextFont(42);
    title->SetTextSize(0.055);
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