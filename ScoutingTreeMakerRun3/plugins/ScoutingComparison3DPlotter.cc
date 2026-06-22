// ScoutingComparison3DPlotter.cc – Tiled Landscape & Uniform Box Color Edition
#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <regex>
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
#include "TH2F.h"
#include "TH3F.h"

class ScoutingComparison3DPlotter
    : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit ScoutingComparison3DPlotter(const edm::ParameterSet&);
  ~ScoutingComparison3DPlotter() override = default;
  void analyze(const edm::Event&, const edm::EventSetup&) override {}
  void beginJob() override;
  void endJob()   override;

private:
  struct Sample {
    std::string path;
    int mass = 0; // GeV
    int ct   = 0; // mm
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

  std::vector<Sample> samples_;

  // Source fine-grained detector resolution axes
  int    xy_nx_ = 220, xy_ny_ = 220;
  double xy_xmin_ = -0.1, xy_xmax_ = 0.1;
  double xy_ymin_ = -0.1, xy_ymax_ = 0.1;
  int    md_nx_   = 140;
  double md_xmin_ = 0.0,  md_xmax_ = 0.05;

  // Custom Contiguous Bin Boundaries (No spaces/gaps, tiles the landscape perfectly)
  // Mass centers will fall perfectly into bins: 200, 400, 600, 800
  std::vector<double> massEdges_ = {100.0, 300.0, 500.0, 700.0, 900.0};
  
  // ctau centers will fall perfectly into bins: 1, 3, 10
  std::vector<double> ctEdges_   = {0.0, 2.0, 6.0, 14.0};

  // 3D Grid Histograms (Color-mapped content via COLZ)
  TH3F* h_xyDiff_vs_mass_            = nullptr; 
  TH3F* h_xyDiff_vs_ct_              = nullptr; 
  TH3F* h_matchDist_vs_mass_ct_      = nullptr; 

  // Calculated Parameter Metrics
  TH3F* h_mean_match_distance_vs_mass_ct_ = nullptr; 
  TH3F* h_std_match_distance_vs_mass_ct_  = nullptr; 

  // 1D Tiled Profile Histograms (Connected bins, easy order tracking)
  TH1F* h_mean_match_distance_vs_mass_    = nullptr;
  TH1F* h_mean_match_distance_vs_ct_      = nullptr;
  TH1F* h_std_match_distance_vs_mass_     = nullptr;
  TH1F* h_std_match_distance_vs_ct_      = nullptr;

  std::map<std::pair<int,int>, WStats> stats_;

  static std::string trim(std::string s) {
    auto f = [](unsigned char c){ return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), f));
    s.erase(std::find_if(s.rbegin(), s.rend(), f).base(), s.end());
    return s;
  }

  static bool parseMassCt(const std::string& path, int& mass, int& ct) {
    static const std::regex re(R"(M(\d+)_CT(\d+))");
    std::smatch m;
    if (!std::regex_search(path, m, re) || m.size() < 3) return false;
    mass = std::stoi(m[1].str());
    ct   = std::stoi(m[2].str());
    return true;
  }

  void readInputList();
  void inspectAxes();
  void bookHistograms();
  void fillFromFile(const Sample& s);
  void fillSummaries();

  static void accumulateTH2intoZslice(TH2* src, TH3F* dst, double zValue) {
    if (!src || !dst) return;
    int iZ = dst->GetZaxis()->FindBin(zValue);
    for (int ix = 1; ix <= src->GetNbinsX(); ++ix) {
      for (int iy = 1; iy <= src->GetNbinsY(); ++iy) {
        double content = src->GetBinContent(ix, iy);
        if (content == 0.0) continue;
        double current = dst->GetBinContent(ix, iy, iZ);
        dst->SetBinContent(ix, iy, iZ, current + content);
      }
    }
  }
};

// ================================================================
ScoutingComparison3DPlotter::ScoutingComparison3DPlotter(const edm::ParameterSet& cfg)
  : inputListFile_(cfg.getParameter<std::string>("inputListFile"))
  , xyDiffHistPath_(cfg.getUntrackedParameter<std::string>("xyDiffHistPath", "scoutingComparer/xy_diff_1"))
  , matchDistHistPath_(cfg.getUntrackedParameter<std::string>("matchDistHistPath", "scoutingComparer/match_distance_1"))
{ usesResource("TFileService"); }

// ================================================================
void ScoutingComparison3DPlotter::readInputList() {
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
void ScoutingComparison3DPlotter::inspectAxes() {
  for (const auto& s : samples_) {
    std::unique_ptr<TFile> f(TFile::Open(s.path.c_str(), "READ"));
    if (!f || f->IsZombie()) continue;
    TH2* hxy = dynamic_cast<TH2*>(f->Get(xyDiffHistPath_.c_str()));
    TH1* hmd = dynamic_cast<TH1*>(f->Get(matchDistHistPath_.c_str()));
    if (!hxy || !hmd) continue;
    xy_nx_ = hxy->GetNbinsX(); xy_ny_ = hxy->GetNbinsY();
    xy_xmin_ = hxy->GetXaxis()->GetXmin(); xy_xmax_ = hxy->GetXaxis()->GetXmax();
    xy_ymin_ = hxy->GetYaxis()->GetXmin(); xy_ymax_ = hxy->GetYaxis()->GetXmax();
    md_nx_ = hmd->GetNbinsX();
    md_xmin_ = hmd->GetXaxis()->GetXmin(); md_xmax_ = hmd->GetXaxis()->GetXmax();
    return;
  }
}

// ================================================================
void ScoutingComparison3DPlotter::bookHistograms() {
  edm::Service<TFileService> fs;

  int nMassBins = massEdges_.size() - 1;
  int nCtBins   = ctEdges_.size() - 1;

  // Generate continuous coordinate bin boundaries for uniform axes to avoid mixing constructor signatures
  std::vector<double> xy_xEdges(xy_nx_ + 1);
  double dx = (xy_xmax_ - xy_xmin_) / xy_nx_;
  for (int i = 0; i <= xy_nx_; ++i) xy_xEdges[i] = xy_xmin_ + i * dx;

  std::vector<double> xy_yEdges(xy_ny_ + 1);
  double dy = (xy_ymax_ - xy_ymin_) / xy_ny_;
  for (int i = 0; i <= xy_ny_; ++i) xy_yEdges[i] = xy_ymin_ + i * dy;

  std::vector<double> md_Edges(md_nx_ + 1);
  double dmd = (md_xmax_ - md_xmin_) / md_nx_;
  for (int i = 0; i <= md_nx_; ++i) md_Edges[i] = md_xmin_ + i * dmd;

  // 1. Booking Physics Grids using the uniform variable array constructor (8 arguments)
  h_xyDiff_vs_mass_ = fs->make<TH3F>(
    "xy_diff_vs_mass", "xy distribution vs Mass Profile;#Deltax [cm];#Deltay [cm];mass [GeV]",
    xy_nx_, xy_xEdges.data(), xy_ny_, xy_yEdges.data(), nMassBins, massEdges_.data()
  );

  h_xyDiff_vs_ct_ = fs->make<TH3F>(
    "xy_diff_vs_ct", "xy distribution vs c#tau Profile;#Deltax [cm];#Deltay [cm];c#tau [mm]",
    xy_nx_, xy_xEdges.data(), xy_ny_, xy_yEdges.data(), nCtBins, ctEdges_.data()
  );

  h_matchDist_vs_mass_ct_ = fs->make<TH3F>(
    "match_distance_vs_mass_ct", "Continuous Match Distance Landscape;mass [GeV];c#tau [mm];match distance [cm]",
    nMassBins, massEdges_.data(), nCtBins, ctEdges_.data(), md_nx_, md_Edges.data()
  );

  // 2. Booking Metric Value Maps
  h_mean_match_distance_vs_mass_ct_ = fs->make<TH3F>(
    "mean_match_distance_vs_mass_ct", "Mean Match Distance Value Map;mass [GeV];c#tau [mm];Mean [cm]",
    nMassBins, massEdges_.data(), nCtBins, ctEdges_.data(), md_nx_, md_Edges.data()
  );

  h_std_match_distance_vs_mass_ct_ = fs->make<TH3F>(
    "std_match_distance_vs_mass_ct", "StdDev Match Distance Value Map;mass [GeV];c#tau [mm];#sigma [cm]",
    nMassBins, massEdges_.data(), nCtBins, ctEdges_.data(), md_nx_, md_Edges.data()
  );

  // Apply strict color-only drawing tags so JSROOT uses uniform tiling sizes automatically
  h_xyDiff_vs_mass_->SetDrawOption("COLZ");
  h_xyDiff_vs_ct_->SetDrawOption("COLZ");
  h_matchDist_vs_mass_ct_->SetDrawOption("COLZ");
  h_mean_match_distance_vs_mass_ct_->SetDrawOption("COLZ");
  h_std_match_distance_vs_mass_ct_->SetDrawOption("COLZ");

  // 3. Booking Contiguous 1D Outline Profiles (tiled side-by-side for perfect contrast)
  h_mean_match_distance_vs_mass_ = fs->make<TH1F>("mean_match_distance_vs_mass", "Mean Match Distance vs Mass;mass [GeV];Mean [cm]", nMassBins, massEdges_.data());
  h_mean_match_distance_vs_ct_   = fs->make<TH1F>("mean_match_distance_vs_ct",   "Mean Match Distance vs c#tau;c#tau [mm];Mean [cm]", nCtBins, ctEdges_.data());
  h_std_match_distance_vs_mass_  = fs->make<TH1F>("std_match_distance_vs_mass",  "StdDev Match Distance vs Mass;mass [GeV];#sigma [cm]", nMassBins, massEdges_.data());
  h_std_match_distance_vs_ct_    = fs->make<TH1F>("std_match_distance_vs_ct",    "StdDev Match Distance vs c#tau;c#tau [mm];#sigma [cm]", nCtBins, ctEdges_.data());
}

// ================================================================
void ScoutingComparison3DPlotter::fillFromFile(const Sample& s) {
  std::unique_ptr<TFile> f(TFile::Open(s.path.c_str(), "READ"));
  if (!f || f->IsZombie()) return;

  TH2* hxy = dynamic_cast<TH2*>(f->Get(xyDiffHistPath_.c_str()));
  TH1* hmd = dynamic_cast<TH1*>(f->Get(matchDistHistPath_.c_str()));

  if (hxy) {
    accumulateTH2intoZslice(hxy, h_xyDiff_vs_mass_, s.mass);
    accumulateTH2intoZslice(hxy, h_xyDiff_vs_ct_,   s.ct);
  }

  if (hmd) {
    int iM = h_matchDist_vs_mass_ct_->GetXaxis()->FindBin(s.mass);
    int iC = h_matchDist_vs_mass_ct_->GetYaxis()->FindBin(s.ct);

    for (int iz = 1; iz <= hmd->GetNbinsX(); ++iz) {
      double content = hmd->GetBinContent(iz);
      if (content == 0.0) continue;
      double current = h_matchDist_vs_mass_ct_->GetBinContent(iM, iC, iz);
      h_matchDist_vs_mass_ct_->SetBinContent(iM, iC, iz, current + content);

      double center = hmd->GetXaxis()->GetBinCenter(iz);
      stats_[{s.mass, s.ct}].add(center, content);
    }
  }
}

// ================================================================
void ScoutingComparison3DPlotter::fillSummaries() {
  std::map<int, WStats> byMass, byCt;

  for (const auto& kv : stats_) {
    int mass = kv.first.first;
    int ct   = kv.first.second;
    const WStats& st = kv.second;
    if (st.sumW < 1.0) continue;

    double meanVal = st.mean();
    double stdVal  = st.stddev();

    int iM = h_mean_match_distance_vs_mass_ct_->GetXaxis()->FindBin(mass);
    int iC = h_mean_match_distance_vs_mass_ct_->GetYaxis()->FindBin(ct);

    // Map calculated continuous stats to color-intensity value heights on the exact cell coordinate
    int iZ_mean = h_mean_match_distance_vs_mass_ct_->GetZaxis()->FindBin(meanVal);
    h_mean_match_distance_vs_mass_ct_->SetBinContent(iM, iC, iZ_mean, meanVal);

    int iZ_std = h_std_match_distance_vs_mass_ct_->GetZaxis()->FindBin(stdVal);
    h_std_match_distance_vs_mass_ct_->SetBinContent(iM, iC, iZ_std, stdVal);

    byMass[mass].merge(st);
    byCt[ct].merge(st);
  }

  // Populate 1D profiles (now contiguous with zero spacer gaps, tracking relative sizes perfectly)
  for (const auto& kv : byMass) {
    int bin = h_mean_match_distance_vs_mass_->FindBin(kv.first);
    h_mean_match_distance_vs_mass_->SetBinContent(bin, kv.second.mean());
    h_std_match_distance_vs_mass_->SetBinContent(bin, kv.second.stddev());
  }

  for (const auto& kv : byCt) {
    int bin = h_mean_match_distance_vs_ct_->FindBin(kv.first);
    h_mean_match_distance_vs_ct_->SetBinContent(bin, kv.second.mean());
    h_std_match_distance_vs_ct_->SetBinContent(bin, kv.second.stddev());
  }
}

// ================================================================
void ScoutingComparison3DPlotter::beginJob() {
  readInputList();
  inspectAxes();
  bookHistograms();
}

void ScoutingComparison3DPlotter::endJob() {
  for (const auto& s : samples_) {
    fillFromFile(s);
  }
  fillSummaries();
}

DEFINE_FWK_MODULE(ScoutingComparison3DPlotter);