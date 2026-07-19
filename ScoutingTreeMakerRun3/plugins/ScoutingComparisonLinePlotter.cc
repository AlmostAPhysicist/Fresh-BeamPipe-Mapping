// ScoutingComparisonLinePlotter.cc – Line-graph landscape & multi-line metric plots
//
// Design notes:
//  * All "3D" plots are TGraph/TMultiGraph line plots (spectra summed by mass,
//    spectra summed by c-tau, and mean-profile lines for the xy-resolution).
//  * All "2D heatmaps" are TMultiGraph line plots: one graph per mass value
//    (x = c-tau) and one graph per c-tau value (x = mass).
//  * The categorical 1D bar histograms (mean/std match distance vs mass, vs
//    c-tau) have one bin per actual (mass,ct) value seen in the input list,
//    bin width = 1, so the left edge of each bar sits exactly on that
//    category. Axis labels are the literal input values only.
//
//  * OUTPUT ORGANIZATION (folders/TDirectories inside the ROOT file):
//        MatchDistanceSpectrum/
//            ByMass/
//                Entries/{Summed_AllCtau, Ctau_<label>, ...}
//                CumulativeFraction/{Summed_AllCtau, Ctau_<label>, ...}
//            ByCtau/
//                Entries/{Summed_AllMasses, Mass_<label>, ...}
//                CumulativeFraction/{Summed_AllMasses, Mass_<label>, ...}
//        NSelVtxSpectrum/
//            ByMass/
//                Entries/{Summed_AllCtau, Ctau_<label>, ...}
//                CumulativeFraction/{Summed_AllCtau, Ctau_<label>, ...}
//            ByCtau/
//                Entries/{Summed_AllMasses, Mass_<label>, ...}
//                CumulativeFraction/{Summed_AllMasses, Mass_<label>, ...}
//        VtxNTracks/
//            ByMass/
//                Entries/{Summed_AllCtau, Ctau_<label>, ...}
//                CumulativeFraction/{Summed_AllCtau, Ctau_<label>, ...}
//            ByCtau/
//                Entries/{Summed_AllMasses, Mass_<label>, ...}
//                CumulativeFraction/{Summed_AllMasses, Mass_<label>, ...}
//        ScalarMetrics/
//            mean_massReco/{...}, mean_rDistGen/{...}, mean_vtxNtracks/{...}
//        MatchDistanceSummary/
//            mean_match_distance_vs_mass, std_match_distance_vs_mass,
//            mean_match_distance_vs_ct,  std_match_distance_vs_ct
//
//    (XYResolution was removed -- MatchDistanceSpectrum/MatchDistanceSummary
//    already cover that information.)
//
//    In addition to the "summed over the other axis" spectrum line plots that
//    previously existed, the ByMass/ByCtau spectrum bundles now ALSO contain
//    one plot per individual c-tau (resp. mass) value, using only that
//    slice's data (not summed). Every spectrum (match distance, N_sel_vtx,
//    vtx N_tracks) gets a "CumulativeFraction" companion for every one of
//    those plots: same lines/categories, but y = running sum / total, i.e.
//    "what fraction of entries fall below x".
//
//  * 99%-LINE (match distance only): each match-distance CumulativeFraction
//    canvas additionally draws a dashed horizontal reference line at
//    y = 0.99, and for each line (one per mass, or per c-tau) the legend
//    entry is annotated with the match-distance value at which that curve
//    first crosses 99% (linearly interpolated between the two straddling
//    points) -- i.e. "below this match distance, 99% of entries for that
//    mass/c-tau fall".
//
//  * DUPLICATE-PLOT FIX: every canvas used to be created via
//    fs->make<TCanvas>(...) (which already schedules the object to be
//    written once, automatically, at end of job) and was then ALSO written
//    explicitly via c->Write(), producing two on-disk cycles of the same
//    canvas. The TGraph/TMultiGraph objects feeding each canvas were also
//    created via fs->make<T>(...), which persisted them a second time as
//    bare, unstyled objects alongside the finished canvas. Both of these are
//    now fixed: canvases are still made via TFileDirectory::make<TCanvas>
//    (so they're written exactly once, automatically) with NO extra
//    ->Write() call, and the constituent TGraph/TMultiGraph objects are built
//    with plain `new` (not through the file service), so they are never
//    independently persisted -- only the finished, styled canvas ends up in
//    the file.
//
//  * INVALIDDIRECTORY FIX: TFileDirectory subdirectories are created LAZILY
//    by CMSSW/ROOT -- the on-disk TDirectory for e.g.
//    "MatchDistanceSpectrum/ByMass/Entries" doesn't actually exist until the
//    FIRST TFileDirectory::make<T>() call happens inside it. The previous
//    version of writeLineGraphSet() called dir.getBareDirectory() to derive
//    a globally-unique TCanvas name from the on-disk path -- but it did this
//    *before* ever calling dir.make<>() in that directory, so for the very
//    first folder written (MatchDistanceSpectrum/ByMass/Entries) the
//    directory did not exist yet and getBareDirectory() threw
//    'InvalidDirectory ... doesn't exist'. Fixed by never introspecting the
//    on-disk directory at all: every call site now passes down an explicit
//    canvasNamePrefix string (built purely from the folder hierarchy we
//    already know in C++), and the ROOT directory itself is touched only via
//    TFileDirectory::make<TCanvas>(), which lazily creates it correctly.

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
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
// #include "CommonTools/UtilAlgos/interface/TFileDirectory.h"

#include "TFile.h"
#include "TH1F.h"
#include "TH2.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TMultiGraph.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TLine.h"
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
  std::string matchDistHistPath_;
  std::string nSelVtxHistPath_;
  std::string massRecoHistPath_;
  std::string rDistGenHistPath_;
  std::string vtxNtracksHistPath_;
  bool        makeCumulativeMatchDistance_ = true;

  std::vector<Sample> samples_;

  // Discrete, actually-seen axis values (NOT bin edges) — everything downstream
  // is indexed off these so plots only ever show real input values.
  std::vector<int>    massValues_;   // GeV
  std::vector<double> ctValues_;     // mm

  // Source fine-grained detector resolution axis (for the match-distance spectrum)
  int    md_nx_   = 140;
  double md_xmin_ = 0.0, md_xmax_ = 0.05; // cm

  // Source binning for the vtx N_tracks spectrum, auto-discovered the same way
  int    trk_nx_   = 21;
  double trk_xmin_ = -0.5, trk_xmax_ = 20.5;

  // ---- Per-(mass,ct) accumulators, one entry per sample (files are 1:1 with
  //      (mass,ct) pairs) ----
  std::map<std::pair<int,double>, WStats>              matchDistStats_;   // mean/std of match distance [cm]
  std::map<std::pair<int,double>, std::vector<double>>  matchDistSpectrum_; // full spectrum (md_nx_ bins)
  std::map<std::pair<int,double>, std::vector<double>>  nSelSpectrum_;      // N_sel_vtx spectrum (0..20)
  std::map<std::pair<int,double>, std::vector<double>>  vtxNtracksSpectrum_; // vtx N_tracks spectrum (trk_nx_ bins)
  std::map<std::pair<int,double>, double>               meanMassReco_;      // [GeV]
  std::map<std::pair<int,double>, double>               meanRDistGen_;      // [cm]
  std::map<std::pair<int,double>, double>               meanVtxNtracks_;

  // ---- top-level output folders, created once in beginJob ----
  std::unique_ptr<TFileDirectory> dirMatchDistSpectrum_;
  std::unique_ptr<TFileDirectory> dirNSelVtxSpectrum_;
  std::unique_ptr<TFileDirectory> dirVtxNTracks_;
  std::unique_ptr<TFileDirectory> dirScalarMetrics_;
  std::unique_ptr<TFileDirectory> dirSummary_;

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

  // Turn a human label ("0.1 mm", "200 GeV") into a valid, readable ROOT
  // object/folder name ("0p1mm", "200GeV") — no spaces, no dots.
  static std::string sanitize(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
      if (c == ' ') continue;
      else if (c == '.') out += 'p';
      else if (c == '-') out += 'm';
      else out += c;
    }
    return out;
  }

  // Filenames look like ..._M200_CT1.root (Stop samples) or
  // ..._H_MS30_CT0p1.root (Higgs->Scalar samples):
  //  - mass token: "M" optionally followed by letters (e.g. "S" for the
  //    scalar-mass prefix "MS"), then digits                       -> GeV
  //  - ct token:   "CT" then digits, optionally followed by "p" + more
  //    digits for a decimal point (e.g. "CT0p1" -> 0.1)            -> mm
  // The (?:^|_) / (?=$|_|\.) boundaries keep this from ever matching a
  // stray capital "M" embedded elsewhere in the path (both sample sets'
  // paths are checked against this pattern).
  static bool parseMassCt(const std::string& path, int& mass, double& ct) {
    static const std::regex re(R"((?:^|_)M[A-Za-z]*(\d+)_CT(\d+)(?:p(\d+))?(?=$|_|\.))");
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

  // Build a TMultiGraph + legend + canvas from a set of named (x,y) series
  // and write ONLY the finished canvas into `dir`. See the duplicate-plot
  // fix note at the top of this file for why the constituent graphs are
  // built with plain `new` instead of TFileDirectory::make<>().
  //
  // `canvasNamePrefix` must be unique per *call site* (i.e. per folder in the
  // output hierarchy) so that the final canvas name
  // ("c_" + canvasNamePrefix + "_" + name) is unique job-wide -- ROOT tracks
  // TCanvas names globally (gROOT's list of canvases), not scoped to the
  // TFileDirectory they end up written into, and none of these canvases are
  // deleted before end of job. See the InvalidDirectory fix note at the top
  // of this file for why this is passed in explicitly rather than derived
  // from the TFileDirectory itself.
  // If `markP99Crossings` is set, each series additionally: (1) gets its
  // legend label annotated with the x-value at which it first crosses
  // y = 0.99 (linearly interpolated), and (2) contributes to a single
  // dashed horizontal reference line at y = 0.99 spanning the plotted
  // x-range, added as its own legend entry. Intended for cumulative-fraction
  // series only. `p99Unit` is the unit string appended to the annotated
  // x-value (e.g. "cm").
  void writeLineGraphSet(TFileDirectory& dir,
                          const std::string& canvasNamePrefix,
                          const std::string& name,
                          const std::string& title,
                          const std::string& xTitle,
                          const std::string& yTitle,
                          const std::vector<std::pair<std::string, std::vector<std::pair<double,double>>>>& series,
                          bool markP99Crossings = false,
                          const std::string& p99Unit = "");

  // For a scalar metric keyed by (mass,ct), build BOTH projections:
  //   x = mass, one line per ct  AND  x = ct, one line per mass.
  // Everything lands in parentDir/baseName/{vs_Mass_byCtau, vs_Ctau_byMass}.
  // `baseName` is assumed unique job-wide (true for all current callers:
  // mean_dx, mean_dy, mean_massReco, mean_rDistGen, mean_vtxNtracks) and is
  // reused directly as the canvas-name prefix.
  void writeGroupedMetricLines(TFileDirectory& parentDir,
                                const std::string& baseName,
                                const std::string& title,
                                const std::string& metricAxisTitle,
                                const std::map<std::pair<int,double>, double>& data);

  // For a spectrum keyed by (mass,ct):
  //   ByMass/Entries:  one line-per-mass plot summed over all c-tau, PLUS one
  //                    line-per-mass plot for each individual c-tau value.
  //   ByCtau/Entries:  one line-per-ct plot summed over all mass, PLUS one
  //                    line-per-ct plot for each individual mass value.
  //   If makeCumulative, every one of the plots above gets a companion
  //   "CumulativeFraction" plot (same lines, y = running sum / total).
  // `baseName` is assumed unique job-wide (e.g. "match_distance", "nSelVtx").
  // If `makeCumulative` and `markCumulativeP99` are both set, every
  // CumulativeFraction canvas produced (Entries canvases are unaffected)
  // gets the 99%-crossing annotation and reference line described above
  // `writeLineGraphSet`; `p99Unit` is forwarded as the unit label.
  void writeGroupedSpectrumLines(TFileDirectory& baseDir,
                                  const std::string& baseName,
                                  const std::string& title,
                                  const std::string& xTitle,
                                  const std::map<std::pair<int,double>, std::vector<double>>& spectra,
                                  int nBins, double xmin, double xmax,
                                  bool makeCumulative,
                                  bool markCumulativeP99 = false,
                                  const std::string& p99Unit = "");
};

// ================================================================
ScoutingComparisonLinePlotter::ScoutingComparisonLinePlotter(const edm::ParameterSet& cfg)
  : inputListFile_(cfg.getParameter<std::string>("inputListFile"))
  , matchDistHistPath_(cfg.getUntrackedParameter<std::string>("matchDistHistPath", "scoutingComparer/match_distance_1"))
  , nSelVtxHistPath_(cfg.getUntrackedParameter<std::string>("nSelVtxHistPath", "scoutingComparer/n_selected_vertices"))
  , massRecoHistPath_(cfg.getUntrackedParameter<std::string>("massRecoHistPath", "scoutingComparer/mass_reco_1"))
  , rDistGenHistPath_(cfg.getUntrackedParameter<std::string>("rDistGenHistPath", "scoutingComparer/rdist_xy_beamspot_gen_1"))
  , vtxNtracksHistPath_(cfg.getUntrackedParameter<std::string>("vtxNtracksHistPath", "scoutingComparer/vtx_ntracks_1"))
  , makeCumulativeMatchDistance_(cfg.getUntrackedParameter<bool>("makeCumulativeMatchDistance", true))
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
  std::set<int> massSet;
  std::set<double> ctSet;
  for (const auto& s : samples_) { massSet.insert(s.mass); ctSet.insert(s.ct); }
  massValues_.assign(massSet.begin(), massSet.end());
  ctValues_.assign(ctSet.begin(), ctSet.end());
  std::sort(massValues_.begin(), massValues_.end());
  std::sort(ctValues_.begin(), ctValues_.end());
}

// ================================================================
void ScoutingComparisonLinePlotter::inspectAxes() {
  bool haveMd = false, haveTrk = false;
  for (const auto& s : samples_) {
    if (haveMd && haveTrk) break;
    std::unique_ptr<TFile> f(TFile::Open(s.path.c_str(), "READ"));
    if (!f || f->IsZombie()) continue;

    if (!haveMd) {
      if (TH1* hmd = dynamic_cast<TH1*>(f->Get(matchDistHistPath_.c_str()))) {
        md_nx_   = hmd->GetNbinsX();
        md_xmin_ = hmd->GetXaxis()->GetXmin();
        md_xmax_ = hmd->GetXaxis()->GetXmax();
        haveMd = true;
      }
    }
    if (!haveTrk) {
      if (TH1* htrk = dynamic_cast<TH1*>(f->Get(vtxNtracksHistPath_.c_str()))) {
        trk_nx_   = htrk->GetNbinsX();
        trk_xmin_ = htrk->GetXaxis()->GetXmin();
        trk_xmax_ = htrk->GetXaxis()->GetXmax();
        haveTrk = true;
      }
    }
  }
}

// ================================================================
// Categorical bins: N bins of width 1, bin i covers exactly value #i.
void ScoutingComparisonLinePlotter::bookCategoricalHistograms() {
  int nMass = (int)massValues_.size();
  int nCt   = (int)ctValues_.size();

  h_mean_match_distance_vs_mass_ = dirSummary_->make<TH1F>("mean_match_distance_vs_mass", "Mean Match Distance vs Mass;;Mean [cm]", nMass, 0.5, nMass + 0.5);
  h_std_match_distance_vs_mass_  = dirSummary_->make<TH1F>("std_match_distance_vs_mass",  "StdDev Match Distance vs Mass;;#sigma [cm]", nMass, 0.5, nMass + 0.5);
  h_mean_match_distance_vs_ct_   = dirSummary_->make<TH1F>("mean_match_distance_vs_ct",   "Mean Match Distance vs c#tau;;Mean [cm]", nCt, 0.5, nCt + 0.5);
  h_std_match_distance_vs_ct_    = dirSummary_->make<TH1F>("std_match_distance_vs_ct",    "StdDev Match Distance vs c#tau;;#sigma [cm]", nCt, 0.5, nCt + 0.5);

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

  TH1* hmd    = dynamic_cast<TH1*>(f->Get(matchDistHistPath_.c_str()));
  TH1* hnSel  = dynamic_cast<TH1*>(f->Get(nSelVtxHistPath_.c_str()));
  TH1* hMass  = dynamic_cast<TH1*>(f->Get(massRecoHistPath_.c_str()));
  TH1* hrDist = dynamic_cast<TH1*>(f->Get(rDistGenHistPath_.c_str()));
  TH1* hTrk   = dynamic_cast<TH1*>(f->Get(vtxNtracksHistPath_.c_str()));

  // match-distance spectrum + weighted mean/std [cm]
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

  // N_sel_vtx spectrum
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

  // vtx N_tracks spectrum
  if (hTrk) {
    auto& spectrum = vtxNtracksSpectrum_[key];
    spectrum.assign(trk_nx_, 0.0);
    for (int iz = 1; iz <= hTrk->GetNbinsX() && iz <= trk_nx_; ++iz) {
      double content = hTrk->GetBinContent(iz);
      if (content != 0.0) spectrum[iz - 1] += content;
    }
  }

  // Scalar means per (mass,ct)
  if (hMass)  meanMassReco_[key]   = hMass->GetMean();  // GeV
  if (hrDist) meanRDistGen_[key]   = hrDist->GetMean(); // cm
  if (hTrk)   meanVtxNtracks_[key] = hTrk->GetMean();
}

// ================================================================
void ScoutingComparisonLinePlotter::fillMatchDistanceSummaryHistos() {
  std::map<int, WStats> byMass;
  std::map<double, WStats> byCt;
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
    TFileDirectory& dir, const std::string& canvasNamePrefix, const std::string& name,
    const std::string& title, const std::string& xTitle, const std::string& yTitle,
    const std::vector<std::pair<std::string, std::vector<std::pair<double,double>>>>& series,
    bool markP99Crossings, const std::string& p99Unit) {

  const bool anyPoints = std::any_of(series.begin(), series.end(),
      [](const auto& s) { return !s.second.empty(); });
  if (!anyPoints) return; // nothing to draw -> skip rather than write an empty canvas

  // For a (assumed monotonically non-decreasing, e.g. cumulative-fraction)
  // series, find the x-value where it first crosses y = 0.99, linearly
  // interpolating between the two straddling points.
  const auto p99CrossingX = [](const std::vector<std::pair<double,double>>& pts) -> std::pair<bool,double> {
    for (size_t i = 0; i < pts.size(); ++i) {
      if (pts[i].second >= 0.99) {
        if (i == 0) return {true, pts[i].first};
        double x0 = pts[i-1].first, y0 = pts[i-1].second;
        double x1 = pts[i].first,   y1 = pts[i].second;
        double frac = (y1 > y0) ? (0.99 - y0) / (y1 - y0) : 0.0;
        return {true, x0 + frac * (x1 - x0)};
      }
    }
    return {false, 0.0};
  };

  TMultiGraph* mg = new TMultiGraph();
  mg->SetName(name.c_str());
  mg->SetTitle((title + ";" + xTitle + ";" + yTitle).c_str());

  // Globally-unique canvas name, built purely from strings we already know
  // (no ROOT directory introspection -- see the InvalidDirectory fix note at
  // the top of this file for why that used to blow up).
  const std::string canvasName = "c_" + canvasNamePrefix + "_" + name;

  TCanvas* c = dir.make<TCanvas>(canvasName.c_str(), title.c_str(), 900, 650);
  TLegend* leg = new TLegend(0.62, 0.60, 0.90, 0.90);
  leg->SetBorderSize(0);
  leg->SetFillStyle(0);
  leg->SetTextSize(0.025);

  double globalXmin = 0.0, globalXmax = 0.0;
  bool haveGlobalX = false;

  int colorIdx = 0;
  for (const auto& s : series) {
    const auto& pts = s.second;
    if (pts.empty()) continue;
    TGraph* g = new TGraph((int)pts.size());
    g->SetName((name + "_" + s.first).c_str());
    for (int i = 0; i < (int)pts.size(); ++i) {
      g->SetPoint(i, pts[i].first, pts[i].second);
      if (!haveGlobalX) { globalXmin = globalXmax = pts[i].first; haveGlobalX = true; }
      else { globalXmin = std::min(globalXmin, pts[i].first); globalXmax = std::max(globalXmax, pts[i].first); }
    }
    int col = paletteColor(colorIdx++);
    g->SetLineColor(col);
    g->SetLineWidth(2);
    g->SetMarkerColor(col);
    g->SetMarkerStyle(20);
    mg->Add(g, "LP");

    std::string label = s.first;
    if (markP99Crossings) {
      auto cross = p99CrossingX(pts);
      if (cross.first) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(4) << cross.second;
        label += " (99%: " + oss.str();
        if (!p99Unit.empty()) label += " " + p99Unit;
        label += ")";
      }
    }
    leg->AddEntry(g, label.c_str(), "lp");
  }

  mg->Draw("ALP");

  if (markP99Crossings && haveGlobalX && globalXmax > globalXmin) {
    TLine* line99 = new TLine(globalXmin, 0.99, globalXmax, 0.99);
    line99->SetLineColor(kBlack);
    line99->SetLineStyle(2); // dashed
    line99->SetLineWidth(2);
    line99->Draw();
    leg->AddEntry(line99, "99% cumulative", "l");
  }

  leg->Draw();
  // NOTE: deliberately no c->Write() here -- TFileDirectory::make<TCanvas>()
  // already schedules exactly one write of this canvas at end of job.
}

// ================================================================
void ScoutingComparisonLinePlotter::writeGroupedMetricLines(
    TFileDirectory& parentDir, const std::string& baseName, const std::string& title,
    const std::string& metricAxisTitle,
    const std::map<std::pair<int,double>, double>& data) {

  TFileDirectory sub = parentDir.mkdir(baseName);

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
    writeLineGraphSet(sub, baseName, "vs_Mass_byCtau", title + " vs Mass (one line per c#tau)",
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
    writeLineGraphSet(sub, baseName, "vs_Ctau_byMass", title + " vs c#tau (one line per mass)",
                       "c#tau [mm]", metricAxisTitle, series);
  }
}

// ================================================================
void ScoutingComparisonLinePlotter::writeGroupedSpectrumLines(
    TFileDirectory& baseDir, const std::string& baseName, const std::string& title,
    const std::string& xTitle,
    const std::map<std::pair<int,double>, std::vector<double>>& spectra,
    int nBins, double xmin, double xmax, bool makeCumulative,
    bool markCumulativeP99, const std::string& p99Unit) {

  const double width = (xmax - xmin) / nBins;
  const auto binCenter = [&](int i) { return xmin + (i + 0.5) * width; };

  const auto sumOverCt = [&](int mass) {
    std::vector<double> sum(nBins, 0.0);
    bool any = false;
    for (double ct : ctValues_) {
      auto it = spectra.find({mass, ct});
      if (it == spectra.end()) continue;
      any = true;
      for (int i = 0; i < nBins && i < (int)it->second.size(); ++i) sum[i] += it->second[i];
    }
    return std::make_pair(any, sum);
  };
  const auto sumOverMass = [&](double ct) {
    std::vector<double> sum(nBins, 0.0);
    bool any = false;
    for (int mass : massValues_) {
      auto it = spectra.find({mass, ct});
      if (it == spectra.end()) continue;
      any = true;
      for (int i = 0; i < nBins && i < (int)it->second.size(); ++i) sum[i] += it->second[i];
    }
    return std::make_pair(any, sum);
  };

  // sparse "raw entries" points, skipping empty bins
  const auto toEntryPoints =
      [&](const std::vector<std::pair<std::string, std::vector<double>>>& raw) {
    std::vector<std::pair<std::string, std::vector<std::pair<double,double>>>> out;
    for (const auto& r : raw) {
      std::vector<std::pair<double,double>> pts;
      for (int i = 0; i < nBins && i < (int)r.second.size(); ++i)
        if (r.second[i] != 0.0) pts.emplace_back(binCenter(i), r.second[i]);
      if (!pts.empty()) out.emplace_back(r.first, pts);
    }
    return out;
  };

  // running-sum / total: "fraction of entries with match distance < x"
  const auto toCumulativeFraction =
      [&](const std::vector<std::pair<std::string, std::vector<double>>>& raw) {
    std::vector<std::pair<std::string, std::vector<std::pair<double,double>>>> out;
    for (const auto& r : raw) {
      double total = 0.0;
      for (int i = 0; i < nBins && i < (int)r.second.size(); ++i) total += r.second[i];
      if (total <= 0.0) continue;
      std::vector<std::pair<double,double>> pts;
      double running = 0.0;
      for (int i = 0; i < nBins && i < (int)r.second.size(); ++i) {
        running += r.second[i];
        pts.emplace_back(binCenter(i), running / total);
      }
      out.emplace_back(r.first, pts);
    }
    return out;
  };

  // ==================== ByMass: lines = mass ====================
  {
    TFileDirectory byMass  = baseDir.mkdir("ByMass");
    TFileDirectory entries = byMass.mkdir("Entries");
    const std::string entriesPrefix = baseName + "_ByMass_Entries";

    std::vector<std::pair<std::string, std::vector<double>>> summedRaw;
    for (int mass : massValues_) {
      auto sm = sumOverCt(mass);
      if (sm.first) summedRaw.emplace_back(labelGeV(mass), sm.second);
    }
    writeLineGraphSet(entries, entriesPrefix, "Summed_AllCtau",
                       title + " Spectrum vs " + xTitle + " (summed over c#tau, one line per mass)",
                       xTitle, "Entries", toEntryPoints(summedRaw));

    for (double ct : ctValues_) {
      std::vector<std::pair<std::string, std::vector<double>>> raw;
      for (int mass : massValues_) {
        auto it = spectra.find({mass, ct});
        if (it != spectra.end()) raw.emplace_back(labelGeV(mass), it->second);
      }
      if (raw.empty()) continue;
      const std::string tag = "Ctau_" + sanitize(labelMm(ct));
      writeLineGraphSet(entries, entriesPrefix, tag,
                         title + " Spectrum vs " + xTitle + " (c#tau = " + labelMm(ct) + ", one line per mass)",
                         xTitle, "Entries", toEntryPoints(raw));
    }

    if (makeCumulative) {
      TFileDirectory cumul = byMass.mkdir("CumulativeFraction");
      const std::string cumulPrefix = baseName + "_ByMass_CumulativeFraction";
      writeLineGraphSet(cumul, cumulPrefix, "Summed_AllCtau",
                         title + " Cumulative Fraction vs " + xTitle + " (summed over c#tau, one line per mass)",
                         xTitle, "Cumulative fraction < x", toCumulativeFraction(summedRaw),
                         markCumulativeP99, p99Unit);
      for (double ct : ctValues_) {
        std::vector<std::pair<std::string, std::vector<double>>> raw;
        for (int mass : massValues_) {
          auto it = spectra.find({mass, ct});
          if (it != spectra.end()) raw.emplace_back(labelGeV(mass), it->second);
        }
        if (raw.empty()) continue;
        const std::string tag = "Ctau_" + sanitize(labelMm(ct));
        writeLineGraphSet(cumul, cumulPrefix, tag,
                           title + " Cumulative Fraction vs " + xTitle + " (c#tau = " + labelMm(ct) + ", one line per mass)",
                           xTitle, "Cumulative fraction < x", toCumulativeFraction(raw),
                           markCumulativeP99, p99Unit);
      }
    }
  }

  // ==================== ByCtau: lines = ct ====================
  {
    TFileDirectory byCtau  = baseDir.mkdir("ByCtau");
    TFileDirectory entries = byCtau.mkdir("Entries");
    const std::string entriesPrefix = baseName + "_ByCtau_Entries";

    std::vector<std::pair<std::string, std::vector<double>>> summedRaw;
    for (double ct : ctValues_) {
      auto sm = sumOverMass(ct);
      if (sm.first) summedRaw.emplace_back(labelMm(ct), sm.second);
    }
    writeLineGraphSet(entries, entriesPrefix, "Summed_AllMasses",
                       title + " Spectrum vs " + xTitle + " (summed over mass, one line per c#tau)",
                       xTitle, "Entries", toEntryPoints(summedRaw));

    for (int mass : massValues_) {
      std::vector<std::pair<std::string, std::vector<double>>> raw;
      for (double ct : ctValues_) {
        auto it = spectra.find({mass, ct});
        if (it != spectra.end()) raw.emplace_back(labelMm(ct), it->second);
      }
      if (raw.empty()) continue;
      const std::string tag = "Mass_" + sanitize(labelGeV(mass));
      writeLineGraphSet(entries, entriesPrefix, tag,
                         title + " Spectrum vs " + xTitle + " (mass = " + labelGeV(mass) + ", one line per c#tau)",
                         xTitle, "Entries", toEntryPoints(raw));
    }

    if (makeCumulative) {
      TFileDirectory cumul = byCtau.mkdir("CumulativeFraction");
      const std::string cumulPrefix = baseName + "_ByCtau_CumulativeFraction";
      writeLineGraphSet(cumul, cumulPrefix, "Summed_AllMasses",
                         title + " Cumulative Fraction vs " + xTitle + " (summed over mass, one line per c#tau)",
                         xTitle, "Cumulative fraction < x", toCumulativeFraction(summedRaw),
                         markCumulativeP99, p99Unit);
      for (int mass : massValues_) {
        std::vector<std::pair<std::string, std::vector<double>>> raw;
        for (double ct : ctValues_) {
          auto it = spectra.find({mass, ct});
          if (it != spectra.end()) raw.emplace_back(labelMm(ct), it->second);
        }
        if (raw.empty()) continue;
        const std::string tag = "Mass_" + sanitize(labelGeV(mass));
        writeLineGraphSet(cumul, cumulPrefix, tag,
                           title + " Cumulative Fraction vs " + xTitle + " (mass = " + labelGeV(mass) + ", one line per c#tau)",
                           xTitle, "Cumulative fraction < x", toCumulativeFraction(raw),
                           markCumulativeP99, p99Unit);
      }
    }
  }
}

// ================================================================
void ScoutingComparisonLinePlotter::beginJob() {
  readInputList();
  deriveAxisValues();
  inspectAxes();

  edm::Service<TFileService> fs;
  dirMatchDistSpectrum_ = std::make_unique<TFileDirectory>(fs->mkdir("MatchDistanceSpectrum"));
  dirNSelVtxSpectrum_   = std::make_unique<TFileDirectory>(fs->mkdir("NSelVtxSpectrum"));
  dirVtxNTracks_        = std::make_unique<TFileDirectory>(fs->mkdir("VtxNTracks"));
  dirScalarMetrics_     = std::make_unique<TFileDirectory>(fs->mkdir("ScalarMetrics"));
  dirSummary_           = std::make_unique<TFileDirectory>(fs->mkdir("MatchDistanceSummary"));

  bookCategoricalHistograms();
}

void ScoutingComparisonLinePlotter::endJob() {
  for (const auto& s : samples_) fillFromFile(s);

  fillMatchDistanceSummaryHistos();

  // --- match-distance spectrum: Entries + CumulativeFraction, with the 99%
  //     crossing annotated/marked on every CumulativeFraction canvas ---
  writeGroupedSpectrumLines(*dirMatchDistSpectrum_, "match_distance", "Match Distance",
                             "match distance [cm]", matchDistSpectrum_, md_nx_, md_xmin_, md_xmax_,
                             makeCumulativeMatchDistance_,
                             /*markCumulativeP99=*/true, /*p99Unit=*/"cm");

  // --- N_sel_vtx spectrum: Entries + CumulativeFraction (no 99% marking) ---
  writeGroupedSpectrumLines(*dirNSelVtxSpectrum_, "nSelVtx", "N_{sel. vtx}",
                             "N_{sel. vtx}", nSelSpectrum_, 21, -0.5, 20.5, /*makeCumulative=*/true);

  // --- vtx N_tracks spectrum: Entries + CumulativeFraction (no 99% marking) ---
  writeGroupedSpectrumLines(*dirVtxNTracks_, "vtxNtracks", "Vtx N_{tracks}",
                             "N_{tracks}", vtxNtracksSpectrum_, trk_nx_, trk_xmin_, trk_xmax_,
                             /*makeCumulative=*/true);

  // --- scalar metric maps ---
  writeGroupedMetricLines(*dirScalarMetrics_, "mean_massReco",   "Mean Reco Mass",       "Mean Reco Mass [GeV]", meanMassReco_);
  writeGroupedMetricLines(*dirScalarMetrics_, "mean_rDistGen",   "Mean Gen d_{xy}^{BS}", "Mean d_{xy} [cm]",      meanRDistGen_);
  writeGroupedMetricLines(*dirScalarMetrics_, "mean_vtxNtracks", "Mean Vtx N_{tracks}",  "Mean N_{tracks}",       meanVtxNtracks_);
}

DEFINE_FWK_MODULE(ScoutingComparisonLinePlotter);