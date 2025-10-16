#include "TH1.h"
#include "TH2.h"
#include "TString.h"
#include "TPad.h"
#include "TMath.h"

// Rebin a histogram.
// rebinX/rebinY are scale factors (e.g. 2 doubles the bin size in X/Y).
// For 1D: rebinY is ignored.
// For 2D: both used.
// Returns a new histogram (caller must delete). The input histogram is never modified.
TH1* HistEdit_Rebin(TH1* h, Int_t rebinX = 2, Int_t rebinY = 2) {
	if (!h) return nullptr;

	// if no rebin requested, just return a clone
	if (rebinX <= 1 && rebinY <= 1) {
		return (TH1*)h->Clone(TString(h->GetName())); // removed suffix
	}

	if (TH2* h2 = dynamic_cast<TH2*>(h)) {
		// ensure positive integers
		Int_t nx = rebinX > 1 ? rebinX : 1;
		Int_t ny = rebinY > 1 ? rebinY : 1;
		// Rebin2D returns a new TH2* and does not modify the original
		TH2* out = h2->Rebin2D(nx, ny, TString(h2->GetName())); // removed suffix
		return out;
	} else {
		// 1D histogram: use TH1::Rebin which returns a new TH1* if newname is provided
		Int_t nx = rebinX > 1 ? rebinX : 1;
		if (nx == 1) {
			return (TH1*)h->Clone(TString(h->GetName())); // removed suffix
		}
		TH1* out = h->Rebin(nx, TString(h->GetName())); // removed suffix
		return out;
	}
}

// Clone a histogram and set X range on the clone (user-visible range).
// Returns a new histogram (caller must delete). Original untouched.
TH1* HistEdit_SetXRange(TH1* h, Double_t xmin, Double_t xmax) {
	if (!h) return nullptr;
	TH1* out = (TH1*)h->Clone(TString(h->GetName())); // removed suffix
	out->GetXaxis()->SetRangeUser(xmin, xmax);
	return out;
}

// Clone a histogram and set Y range (only meaningful for TH2).
// Returns a new histogram (caller must delete). Original untouched.
TH1* HistEdit_SetYRange(TH1* h, Double_t ymin, Double_t ymax) {
	if (!h) return nullptr;
	if (TH2* h2 = dynamic_cast<TH2*>(h)) {
		TH2* out2 = (TH2*)h2->Clone(TString(h2->GetName())); // removed suffix
		out2->GetYaxis()->SetRangeUser(ymin, ymax);
		return out2;
	} else {
		// For 1D histograms, no Y axis — return a clone unchanged
		TH1* out = (TH1*)h->Clone(TString(h->GetName())); // removed suffix
		return out;
	}
}

// Clone a histogram and set Z (color) range using SetMinimum/SetMaximum.
// For TH2 this controls the palette; for 1D it controls drawing scale.
// Returns a new histogram (caller must delete). Original untouched.
TH1* HistEdit_SetZRange(TH1* h, Double_t zmin, Double_t zmax) {
	if (!h) return nullptr;
	TH1* out = (TH1*)h->Clone(TString(h->GetName())); // removed suffix
	out->SetMinimum(zmin);
	out->SetMaximum(zmax);
	return out;
}

// Set log axes on the provided pad (or current pad if nullptr).
// Returns the same histogram pointer for convenience. This does not modify the histogram.
TH1* HistEdit_SetLog(TH1* h, Bool_t logx, Bool_t logy, Bool_t logz, TPad* pad = nullptr) {
	if (!h) return nullptr;
	TPad* p = pad ? pad : (TPad*)gPad;
	if (!p) return h;
	p->cd();
	p->SetLogx(logx ? 1 : 0);
	p->SetLogy(logy ? 1 : 0);
	p->SetLogz(logz ? 1 : 0);
	p->Update();
	return h;
}

/*
Sample usage examples (ROOT macro / C++ style):

// 1) Rebin 1D histogram (scale factor 4 => bin width *4)
TH1* h1 = (TH1*)gFile->Get("h1");
TH1* h1_rebinned = HistEdit_Rebin(h1, 4); // new TH1* (caller must delete)
TCanvas* c1 = new TCanvas();
h1_rebinned->Draw();
c1->SaveAs("h1_rebinned.png");
delete h1_rebinned;
delete c1;

// 2) Rebin 2D histogram (scale factors 2x in X and 3x in Y)
TH2* h2 = (TH2*)gFile->Get("h2");
TH1* h2_rebinned = HistEdit_Rebin(h2, 2, 3); // returns new TH2* as TH1*
TH2* h2r = dynamic_cast<TH2*>(h2_rebinned);
TCanvas* c2 = new TCanvas();
h2r->Draw("COLZ");
c2->SaveAs("h2_rebinned.png");
delete h2r;
delete c2;

// 3) Set X/Y/Z ranges (clones returned; originals untouched)
TH1* h_xrange = HistEdit_SetXRange(h1, 0.0, 10.0);
TH1* h_zrange  = HistEdit_SetZRange(h2, 1.0, 100.0); // affects color scale for TH2
TCanvas* c3 = new TCanvas();
h_xrange->Draw();
c3->SaveAs("h_xrange.png");
delete h_xrange;
delete h_zrange;
delete c3;

// 4) Set log axes on the current pad (no new histogram returned)
TPad* pad = (TPad*)gPad; // or any TPad*
HistEdit_SetLog(h2, kFALSE, kFALSE, kTRUE, pad); // enable logz on pad

// 5) Chaining safely (manage ownership explicitly)
TH1* tmp = HistEdit_Rebin(h2, 2, 2);             // new hist
TH1* final = HistEdit_SetZRange(tmp, 1, 1e4);    // new clone from tmp
// draw final...
delete tmp;   // free intermediate
delete final; // free final

Notes:
- All functions that return TH1* produce new histograms (clones or results); caller is responsible for deletion.
- HistEdit_SetLog modifies the provided pad (or current pad) and does not modify the histogram itself.
*/