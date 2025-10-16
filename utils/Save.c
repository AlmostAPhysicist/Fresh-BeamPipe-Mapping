#include "TCanvas.h"
#include "TH1.h"
#include "TH2.h"
#include "TString.h"
#include "TPad.h"
#include "TPaletteAxis.h" // added for palette handling

void Save(TH1* h, const char* filename) {
    if (!h) return;
    TCanvas* c = new TCanvas("save_canvas","",800,600);
    c->cd();

    bool is2D = (dynamic_cast<TH2*>(h) != nullptr);

    // create a pad that fills most of the canvas and enforce a 1:1 aspect ratio
    TPad* pad = new TPad("pad", "pad", 0.05, 0.05, 0.95, 0.95);
    // leave more room on the right if we need the color palette
    pad->SetRightMargin(is2D ? 0.16 : 0.05);
    pad->SetFixedAspectRatio(1); // enforce square plot area
    pad->Draw();
    pad->cd();

    // If it's a TH2, draw with "COLZ" to show the palette
    if (is2D) {
        h->Draw("COLZ");
    } else {
        h->Draw();
    }
    pad->Update();

    // Increase number of labels on the color palette (if present)
    if (is2D) {
        TPaletteAxis* palette = (TPaletteAxis*)gPad->GetPrimitive("palette");
        if (!palette && h->GetListOfFunctions()) palette = (TPaletteAxis*)h->GetListOfFunctions()->FindObject("palette");
        if (palette) {
            palette->SetNdivisions(510); // more divisions/labels
            palette->SetLabelSize(0.03);
            pad->Update(); // apply changes
        }
    }

    c->cd();
    c->SaveAs(filename);
    delete pad;
    delete c;
}