// ScoutingComparisonLinePlotter.cc – Line-graph landscape & multi-line metric plots
//
// Design notes (per request):
//  * All former TH3F "3D" plots are gone entirely — replaced by TGraph/TMultiGraph
//    line plots (spectra summed by mass, spectra summed by c-tau, and mean-profile
//    lines for the xy-resolution).
//  * All former TH2F "2D heatmaps" are gone entirely — replaced by TMultiGraph line
//    plots: one graph per mass value (x = c-tau) and one graph per c-tau value
//    (x = mass), i.e. projected onto each axis with multiple lines.
//  * The surviving 1D histograms (mean/std match distance vs mass, vs c-tau) are
//    now CATEGORICAL: one bin per actual (mass,ct) value seen in the input list,
//    bin width = 1, so the left edge of each bar sits exactly on that category and
//    there is no ambiguous "spanning" bin. Axis labels are the literal input values
//    only (e.g. "100 GeV"), nothing interpolated.

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "TFile.h"
#include "TH1F.h"
#include "TH2.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TMultiGraph.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TColor.h"

class ScoutingComparisonLinePlotter
    : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit ScoutingComparisonLinePlotter(const edm::ParameterSet&);
  ~ScoutingComparisonLinePlotter() override = default;
  void analyze(const edm::Event&, const edm::EventSetup&) override {}
  void beginJob() override;
  void endJob()   override;

private:
  struct Sample {
    std::string path;
    int    mass = 0;   // GeV
    double ct   = 0.0; // mm (may be fractional, e.g. "CT0p1" -> 0.1)
  };

  struct WStats {
    double sumW   = 0.0;
    double sumWX  = 0.0;
    double sumWX2 = 0.0;
    void add(double x, double w = 1.0) {
      sumW += w; sumWX += w*x; sumWX2 += w*x*x;
    }
    double mean() const { return sumW > 0.0 ? sumWX / sumW : 0.0; }
    double stddev() const {
      if (sumW < 2.0) return 0.0;
      double m = mean();
      double v = sumWX2 / sumW - m * m;
      return v > 0.0 ? std::sqrt(v) : 0.0;
    }
    void merge(const WStats& o) {
      sumW += o.sumW; sumWX += o.sumWX; sumWX2 += o.sumWX2;
    }
  };

  std::string inputListFile_;
  std::string xyDiffHistPath_;
  std::string matchDistHistPath_;
  std::string nSelVtxHistPath_;
  std::string massRecoHistPath_;
  std::string rDistGenHistPath_;
  std::string vtxNtracksHistPath_;

  std::vector<Sample> samples_;

  // Discrete, actually-seen axis values (NOT bin edges) — everything downstream
  // is indexed off these so plots only ever show real input values.
  std::vector<int>    massValues_;
  std::vector<double> ctValues_;

  // Source fine-grained detector resolution axis (for the match-distance spectrum)
  int    md_nx_   = 140;
  double md_xmin_ = 0.0, md_xmax_ = 0.05;

  // ---- Per-(mass,ct) accumulators, one entry per sample (files are 1:1 with
  //      (mass,ct) pairs) ----
  std::map<std::pair<int,double>, WStats>              matchDistStats_;   // mean/std of match distance
  std::map<std::pair<int,double>, std::vector<double>>  matchDistSpectrum_; // full spectrum (md_nx_ bins)
  std::map<std::pair<int,double>, double>               meanDx_, meanDy_;   // xy resolution profile
  std::map<std::pair<int,double>, std::vector<double>>  nSelSpectrum_;      // N_sel_vtx spectrum (0..20)
  std::map<std::pair<int,double>, double>               meanMassReco_;
  std::map<std::pair<int,double>, double>               meanRDistGen_;
  std::map<std::pair<int,double>, double>               meanVtxNtracks_;

  // ---- surviving categorical 1D histograms ----
  TH1F* h_mean_match_distance_vs_mass_ = nullptr;
  TH1F* h_mean_match_distance_vs_ct_   = nullptr;
  TH1F* h_std_match_distance_vs_mass_  = nullptr;
  TH1F* h_std_match_distance_vs_ct_    = nullptr;

  static std::string trim(std::string s) {
    auto f = [](unsigned char c){ return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), f));
    s.erase(std::find_if(s.rbegin(), s.rend(), f).base(), s.end());
    return s;
  }

  // Filenames look like ..._MS30_CT0p1.root or ..._MS30_CT10.root
  //  - mass token: "M" optionally followed by letters (e.g. "MS"), then digits
  //  - ct token:   "CT" then digits, optionally followed by "p" + more digits
  //                for a decimal point (e.g. "CT0p1" -> 0.1)
  static bool parseMassCt(const std::string& path, int& mass, double& ct) {
    static const std::regex re(R"(M[A-Za-z]*(\d+)_CT(\d+)(?:p(\d+))?)");
    std::smatch m;
    if (!std::regex_search(path, m, re) || m.size() < 3) return false;
    mass = std::stoi(m[1].str());
    ct   = std::stod(m[2].str());
    if (m[3].matched) {
      const std::string& frac = m[3].str();
      ct += std::stod(frac) / std::pow(10.0, (double)frac.size());
    }
    return true;
  }

  static std::string labelGeV(int v) { return std::to_string(v) + " GeV"; }
  static std::string labelMm(double v) {
    // Print without a trailing ".0" for whole values, but keep the decimal
    // for fractional c-tau values (e.g. 0.1 mm).
    std::ostringstream oss;
    if (std::fabs(v - std::round(v)) < 1e-9) oss << (long long)std::llround(v);
    else oss << v;
    oss << " mm";
    return oss.str();
  }

  // Cycle through a fixed, readable line-color palette.
  static int paletteColor(int i) {
    static const int cols[] = {
      kRed+1, kBlue+1, kGreen+2, kMagenta+1, kOrange+7,
      kCyan+2, kViolet+2, kSpring+3, kAzure+2, kPink+7
    };
    return cols[i % (int)(sizeof(cols)/sizeof(cols[0]))];
  }

  void readInputList();
  void deriveAxisValues();
  void inspectAxes();
  void bookCategoricalHistograms();
  void fillFromFile(const Sample& s);
  void fillMatchDistanceSummaryHistos();

  // Build a TMultiGraph + legend + canvas from a set of named (x,y) series and
  // write it to the TFileService output file.
  void writeLineGraphSet(const std::string& name,
                          const std::string& title,
                          const std::string& xTitle,
                          const std::string& yTitle,
                          const std::vector<std::pair<std::string, std::vector<std::pair<double,double>>>>& series);

  // For a metric keyed by (mass,ct), build BOTH projections:
  //   x = mass, one line per ct  AND  x = ct, one line per mass.
  void writeGroupedMetricLines(const std::string& baseName,
                                const std::string& title,
                                const std::string& metricAxisTitle,
                                const std::map<std::pair<int,double>, double>& data);

  // For a spectrum keyed by (mass,ct), sum over the "other" axis and build one
  // line per mass value and one line per ct value.
  void writeGroupedSpectrumLines(const std::string& baseName,
                                  const std::string& title,
                                  const std::string& xTitle,
                                  const std::map<std::pair<int,double>, std::vector<double>>& spectra,
                                  int nBins, double xmin, double xmax);
};

// ================================================================
ScoutingComparisonLinePlotter::ScoutingComparisonLinePlotter(const edm::ParameterSet& cfg)
  : inputListFile_(cfg.getParameter<std::string>("inputListFile"))
  , xyDiffHistPath_(cfg.getUntrackedParameter<std::string>("xyDiffHistPath", "scoutingComparer/xy_diff_1"))
  , matchDistHistPath_(cfg.getUntrackedParameter<std::string>("matchDistHistPath", "scoutingComparer/match_distance_1"))
  , nSelVtxHistPath_(cfg.getUntrackedParameter<std::string>("nSelVtxHistPath", "scoutingComparer/n_selected_vertices"))
  , massRecoHistPath_(cfg.getUntrackedParameter<std::string>("massRecoHistPath", "scoutingComparer/mass_reco_1"))
  , rDistGenHistPath_(cfg.getUntrackedParameter<std::string>("rDistGenHistPath", "scoutingComparer/rdist_xy_beamspot_gen_1"))
  , vtxNtracksHistPath_(cfg.getUntrackedParameter<std::string>("vtxNtracksHistPath", "scoutingComparer/vtx_ntracks_1"))
{ usesResource("TFileService"); }

// ================================================================
void ScoutingComparisonLinePlotter::readInputList() {
  std::ifstream in(inputListFile_);
  if (!in.is_open())
    throw cms::Exception("InputFileError") << "Cannot open input list file: " << inputListFile_;

  std::string line;
  while (std::getline(in, line)) {
    line = trim(line);
    if (line.empty() || line[0] == '#') continue;
    Sample s;
    if (!parseMassCt(line, s.mass, s.ct))
      throw cms::Exception("InputFileError") << "Failed parsing fields from path token: " << line;
    s.path = line;
    samples_.push_back(s);
  }
}

// ================================================================
void ScoutingComparisonLinePlotter::deriveAxisValues() {
  std::set<int> massSet, ctSet;
  for (const auto& s : samples_) { massSet.insert(s.mass); ctSet.insert(s.ct); }
  massValues_.assign(massSet.begin(), massSet.end());
  ctValues_.assign(ctSet.begin(), ctSet.end());
  std::sort(massValues_.begin(), massValues_.end());
  std::sort(ctValues_.begin(), ctValues_.end());
}

// ================================================================
void ScoutingComparisonLinePlotter::inspectAxes() {
  for (const auto& s : samples_) {
    std::unique_ptr<TFile> f(TFile::Open(s.path.c_str(), "READ"));
    if (!f || f->IsZombie()) continue;
    TH1* hmd = dynamic_cast<TH1*>(f->Get(matchDistHistPath_.c_str()));
    if (!hmd) continue;
    md_nx_   = hmd->GetNbinsX();
    md_xmin_ = hmd->GetXaxis()->GetXmin();
    md_xmax_ = hmd->GetXaxis()->GetXmax();
    return;
  }
}

// ================================================================
// Categorical bins: N bins of width 1, bin i covers exactly value #i.
// This makes "the left edge of the bar" coincide exactly with the value's slot,
// and the only axis labels present are the literal input values.
void ScoutingComparisonLinePlotter::bookCategoricalHistograms() {
  edm::Service<TFileService> fs;

  int nMass = (int)massValues_.size();
  int nCt   = (int)ctValues_.size();

  h_mean_match_distance_vs_mass_ = fs->make<TH1F>("mean_match_distance_vs_mass", "Mean Match Distance vs Mass;;Mean [cm]", nMass, 0.5, nMass + 0.5);
  h_std_match_distance_vs_mass_  = fs->make<TH1F>("std_match_distance_vs_mass",  "StdDev Match Distance vs Mass;;#sigma [cm]", nMass, 0.5, nMass + 0.5);
  h_mean_match_distance_vs_ct_   = fs->make<TH1F>("mean_match_distance_vs_ct",   "Mean Match Distance vs c#tau;;Mean [cm]", nCt, 0.5, nCt + 0.5);
  h_std_match_distance_vs_ct_    = fs->make<TH1F>("std_match_distance_vs_ct",    "StdDev Match Distance vs c#tau;;#sigma [cm]", nCt, 0.5, nCt + 0.5);

  for (int i = 0; i < nMass; ++i) {
    h_mean_match_distance_vs_mass_->GetXaxis()->SetBinLabel(i + 1, labelGeV(massValues_[i]).c_str());
    h_std_match_distance_vs_mass_->GetXaxis()->SetBinLabel(i + 1, labelGeV(massValues_[i]).c_str());
  }
  for (int i = 0; i < nCt; ++i) {
    h_mean_match_distance_vs_ct_->GetXaxis()->SetBinLabel(i + 1, labelMm(ctValues_[i]).c_str());
    h_std_match_distance_vs_ct_->GetXaxis()->SetBinLabel(i + 1, labelMm(ctValues_[i]).c_str());
  }

  for (auto* h : {h_mean_match_distance_vs_mass_, h_std_match_distance_vs_mass_,
                   h_mean_match_distance_vs_ct_,  h_std_match_distance_vs_ct_}) {
    h->SetBarWidth(1.0);
    h->SetBarOffset(0.0);
    h->SetFillColor(kAzure+1);
    h->GetXaxis()->LabelsOption("h");
    h->SetDrawOption("bar");
  }
}

// ================================================================
void ScoutingComparisonLinePlotter::fillFromFile(const Sample& s) {
  std::unique_ptr<TFile> f(TFile::Open(s.path.c_str(), "READ"));
  if (!f || f->IsZombie()) return;

  const auto key = std::make_pair(s.mass, s.ct);

  TH2* hxy    = dynamic_cast<TH2*>(f->Get(xyDiffHistPath_.c_str()));
  TH1* hmd    = dynamic_cast<TH1*>(f->Get(matchDistHistPath_.c_str()));
  TH1* hnSel  = dynamic_cast<TH1*>(f->Get(nSelVtxHistPath_.c_str()));
  TH1* hMass  = dynamic_cast<TH1*>(f->Get(massRecoHistPath_.c_str()));
  TH1* hrDist = dynamic_cast<TH1*>(f->Get(rDistGenHistPath_.c_str()));
  TH1* hTrk   = dynamic_cast<TH1*>(f->Get(vtxNtracksHistPath_.c_str()));

  // xy resolution -> simple mean-profile points (replaces the old 3D xy_diff cubes)
  if (hxy) {
    meanDx_[key] = hxy->GetMean(1);
    meanDy_[key] = hxy->GetMean(2);
  }

  // match-distance spectrum + weighted mean/std (replaces the old 3D landscape)
  if (hmd) {
    auto& spectrum = matchDistSpectrum_[key];
    spectrum.assign(md_nx_, 0.0);
    for (int iz = 1; iz <= hmd->GetNbinsX(); ++iz) {
      double content = hmd->GetBinContent(iz);
      if (content == 0.0) continue;
      spectrum[iz - 1] += content;
      double center = hmd->GetXaxis()->GetBinCenter(iz);
      matchDistStats_[key].add(center, content);
    }
  }

  // N_sel_vtx spectrum (replaces the old 2D event-class heatmap)
  if (hnSel) {
    auto& spectrum = nSelSpectrum_[key];
    spectrum.assign(21, 0.0); // bins 0..20
    for (int iy = 1; iy <= hnSel->GetNbinsX(); ++iy) {
      double content = hnSel->GetBinContent(iy);
      if (content == 0.0) continue;
      int n = (int)std::lround(hnSel->GetXaxis()->GetBinCenter(iy));
      if (n >= 0 && n < 21) spectrum[n] += content;
    }
  }

  // Scalar means per (mass,ct) (replace the old 2D metric heatmaps)
  if (hMass)  meanMassReco_[key]   = hMass->GetMean();
  if (hrDist) meanRDistGen_[key]   = hrDist->GetMean();
  if (hTrk)   meanVtxNtracks_[key] = hTrk->GetMean();
}

// ================================================================
void ScoutingComparisonLinePlotter::fillMatchDistanceSummaryHistos() {
  std::map<int, WStats> byMass, byCt;
  for (const auto& kv : matchDistStats_) {
    if (kv.second.sumW < 1.0) continue;
    byMass[kv.first.first].merge(kv.second);
    byCt[kv.first.second].merge(kv.second);
  }

  for (int i = 0; i < (int)massValues_.size(); ++i) {
    auto it = byMass.find(massValues_[i]);
    if (it == byMass.end()) continue;
    h_mean_match_distance_vs_mass_->SetBinContent(i + 1, it->second.mean());
    h_std_match_distance_vs_mass_->SetBinContent(i + 1, it->second.stddev());
  }
  for (int i = 0; i < (int)ctValues_.size(); ++i) {
    auto it = byCt.find(ctValues_[i]);
    if (it == byCt.end()) continue;
    h_mean_match_distance_vs_ct_->SetBinContent(i + 1, it->second.mean());
    h_std_match_distance_vs_ct_->SetBinContent(i + 1, it->second.stddev());
  }
}

// ================================================================
void ScoutingComparisonLinePlotter::writeLineGraphSet(
    const std::string& name, const std::string& title,
    const std::string& xTitle, const std::string& yTitle,
    const std::vector<std::pair<std::string, std::vector<std::pair<double,double>>>>& series) {

  edm::Service<TFileService> fs;
  TMultiGraph* mg = fs->make<TMultiGraph>();
  mg->SetName(name.c_str());
  mg->SetTitle((title + ";" + xTitle + ";" + yTitle).c_str());

  TCanvas* c = fs->make<TCanvas>(("c_" + name).c_str(), title.c_str(), 900, 650);
  TLegend* leg = new TLegend(0.72, 0.68, 0.90, 0.90);
  leg->SetBorderSize(0);
  leg->SetFillStyle(0);

  int colorIdx = 0;
  for (const auto& s : series) {
    const auto& pts = s.second;
    if (pts.empty()) continue;
    TGraph* g = fs->make<TGraph>((int)pts.size());
    g->SetName((name + "_" + s.first).c_str());
    for (int i = 0; i < (int)pts.size(); ++i) g->SetPoint(i, pts[i].first, pts[i].second);
    int col = paletteColor(colorIdx++);
    g->SetLineColor(col);
    g->SetLineWidth(2);
    g->SetMarkerColor(col);
    g->SetMarkerStyle(20);
    mg->Add(g, "LP");
    leg->AddEntry(g, s.first.c_str(), "lp");
  }

  mg->Draw("ALP");
  leg->Draw();
  c->Write();
}

// ================================================================
void ScoutingComparisonLinePlotter::writeGroupedMetricLines(
    const std::string& baseName, const std::string& title,
    const std::string& metricAxisTitle,
    const std::map<std::pair<int,double>, double>& data) {

  // x = mass, one line per ct
  {
    std::vector<std::pair<std::string, std::vector<std::pair<double,double>>>> series;
    for (double ct : ctValues_) {
      std::vector<std::pair<double,double>> pts;
      for (int mass : massValues_) {
        auto it = data.find({mass, ct});
        if (it != data.end()) pts.emplace_back(mass, it->second);
      }
      if (!pts.empty()) series.emplace_back(labelMm(ct), pts);
    }
    writeLineGraphSet(baseName + "_vs_mass_byCt", title + " vs Mass (one line per c#tau)",
                       "mass [GeV]", metricAxisTitle, series);
  }

  // x = ct, one line per mass
  {
    std::vector<std::pair<std::string, std::vector<std::pair<double,double>>>> series;
    for (int mass : massValues_) {
      std::vector<std::pair<double,double>> pts;
      for (double ct : ctValues_) {
        auto it = data.find({mass, ct});
        if (it != data.end()) pts.emplace_back(ct, it->second);
      }
      if (!pts.empty()) series.emplace_back(labelGeV(mass), pts);
    }
    writeLineGraphSet(baseName + "_vs_ct_byMass", title + " vs c#tau (one line per mass)",
                       "c#tau [mm]", metricAxisTitle, series);
  }
}

// ================================================================
void ScoutingComparisonLinePlotter::writeGroupedSpectrumLines(
    const std::string& baseName, const std::string& title, const std::string& xTitle,
    const std::map<std::pair<int,double>, std::vector<double>>& spectra,
    int nBins, double xmin, double xmax) {

  double width = (xmax - xmin) / nBins;
  auto binCenter = [&](int i) { return xmin + (i + 0.5) * width; };

  // sum over ct -> one line per mass
  {
    std::vector<std::pair<std::string, std::vector<std::pair<double,double>>>> series;
    for (int mass : massValues_) {
      std::vector<double> sum(nBins, 0.0);
      bool any = false;
      for (double ct : ctValues_) {
        auto it = spectra.find({mass, ct});
        if (it == spectra.end()) continue;
        any = true;
        for (int i = 0; i < nBins && i < (int)it->second.size(); ++i) sum[i] += it->second[i];
      }
      if (!any) continue;
      std::vector<std::pair<double,double>> pts;
      for (int i = 0; i < nBins; ++i)
        if (sum[i] != 0.0) pts.emplace_back(binCenter(i), sum[i]);
      if (!pts.empty()) series.emplace_back(labelGeV(mass), pts);
    }
    writeLineGraphSet(baseName + "_byMass", title + " (summed over c#tau, one line per mass)",
                       xTitle, "Entries", series);
  }

  // sum over mass -> one line per ct
  {
    std::vector<std::pair<std::string, std::vector<std::pair<double,double>>>> series;
    for (double ct : ctValues_) {
      std::vector<double> sum(nBins, 0.0);
      bool any = false;
      for (int mass : massValues_) {
        auto it = spectra.find({mass, ct});
        if (it == spectra.end()) continue;
        any = true;
        for (int i = 0; i < nBins && i < (int)it->second.size(); ++i) sum[i] += it->second[i];
      }
      if (!any) continue;
      std::vector<std::pair<double,double>> pts;
      for (int i = 0; i < nBins; ++i)
        if (sum[i] != 0.0) pts.emplace_back(binCenter(i), sum[i]);
      if (!pts.empty()) series.emplace_back(labelMm(ct), pts);
    }
    writeLineGraphSet(baseName + "_byCt", title + " (summed over mass, one line per c#tau)",
                       xTitle, "Entries", series);
  }
}

// ================================================================
void ScoutingComparisonLinePlotter::beginJob() {
  readInputList();
  deriveAxisValues();
  inspectAxes();
  bookCategoricalHistograms();
}

void ScoutingComparisonLinePlotter::endJob() {
  for (const auto& s : samples_) fillFromFile(s);

  fillMatchDistanceSummaryHistos();

  // --- match-distance spectrum ("3D landscape" replacement) ---
  writeGroupedSpectrumLines("match_distance_spectrum", "Match Distance Spectrum",
                             "match distance [cm]", matchDistSpectrum_, md_nx_, md_xmin_, md_xmax_);

  // --- xy resolution profile ("3D xy_diff" replacement) ---
  writeGroupedMetricLines("mean_dx", "Mean #Deltax", "Mean #Deltax [cm]", meanDx_);
  writeGroupedMetricLines("mean_dy", "Mean #Deltay", "Mean #Deltay [cm]", meanDy_);

  // --- N_sel_vtx spectrum ("2D event-class heatmap" replacement) ---
  writeGroupedSpectrumLines("nSelVtx_spectrum", "N_{sel. vtx} Spectrum",
                             "N_{sel. vtx}", nSelSpectrum_, 21, -0.5, 20.5);

  // --- scalar metric maps ("2D heatmap" replacement) ---
  writeGroupedMetricLines("mean_massReco",   "Mean Reco Mass",       "Mean Reco Mass [GeV]", meanMassReco_);
  writeGroupedMetricLines("mean_rDistGen",   "Mean Gen d_{xy}^{BS}", "Mean d_{xy} [cm]",      meanRDistGen_);
  writeGroupedMetricLines("mean_vtxNtracks", "Mean Vtx N_{tracks}",  "Mean N_{tracks}",       meanVtxNtracks_);
}

DEFINE_FWK_MODULE(ScoutingComparisonLinePlotter);