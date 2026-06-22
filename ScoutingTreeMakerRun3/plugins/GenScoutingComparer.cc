#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"

#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "CondFormats/BeamSpotObjects/interface/BeamSpotOnlineObjects.h"
#include "CondFormats/DataRecord/interface/BeamSpotOnlineHLTObjectsRcd.h"
#include "TrackingTools/IPTools/interface/IPTools.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "TrackingTools/TransientTrack/interface/TransientTrack.h"
#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"

#include "DataFormats/BeamSpot/interface/BeamSpot.h"
#include "DataFormats/Candidate/interface/Candidate.h"
#include "DataFormats/Common/interface/Ref.h"
#include "DataFormats/Common/interface/ValueMap.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/Math/interface/Vector3D.h"
#include "DataFormats/Scouting/interface/Run3ScoutingPFJet.h"
#include "DataFormats/Scouting/interface/Run3ScoutingTrack.h"
#include "DataFormats/TrackReco/interface/HitPattern.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/VertexReco/interface/Vertex.h"

#include "TH1F.h"
#include "TH2F.h"
#include "TH3F.h"

// ============================================================
// GenScoutingComparer
//
// Pure gen-truth ↔ selected-reco-vertex comparer.
// Track selection (IPSig, hits, ΔR jet cut, arbitration, N-1)
// is performed entirely inside the Vertexer; every track in
// each input vertex is already accepted.
// ============================================================

class GenScoutingComparer : public edm::one::EDAnalyzer<edm::one::SharedResources> {
private:
    using LorentzVector = reco::Candidate::LorentzVector;

    static constexpr double kPionMass = 0.13957039;

    enum class MatchDistanceMode { k2D, k3D };

    // ------------------------------------------------------------------
    // Per-vertex information built in buildSelectedVertices()
    // ------------------------------------------------------------------
    struct SelectedVertexInfo {
        const reco::Vertex* vertex  = nullptr;
        LorentzVector       p4{0., 0., 0., 0.};
        math::XYZVector     p3{0., 0., 0.};
        unsigned int        nTracks = 0;
        std::vector<double> trackPts;        // pT of every track in the vertex
        double              totalPt  = 0.0; // scalar sum of track pT
        double              chi2     = 0.0;
        double              dBV      = 0.0;
        double              dBVErr   = 0.0;
        double              significance = -1.0;
        double              rDistxyGlobal00  = 0.0;
        double              rDistxyzGlobal00 = 0.0;
        double              rDistxyBeamSpot  = 0.0;
        double              rDistxyzBeamSpot = 0.0;
        double              cosT = -999.0;
    };

    // ------------------------------------------------------------------
    // Matching result
    // ------------------------------------------------------------------
    struct MatchSolution {
        std::array<int, 2>    recoForGen{{-1, -1}};
        std::array<double, 2> distance{{-1.0, -1.0}};
        double totalScore = std::numeric_limits<double>::max();
    };

    // ------------------------------------------------------------------
    // Per-bucket (per gen vertex) histograms
    // ------------------------------------------------------------------
    struct BucketPlots {
        // --- position residuals ---
        TH2F* xyDiff   = nullptr;
        TH3F* xyzDiff  = nullptr;

        // --- absolute positions ---
        TH2F* xyGlobal00Gen    = nullptr;
        TH2F* xyGlobal00Reco   = nullptr;
        TH3F* xyzGlobal00Gen   = nullptr;
        TH3F* xyzGlobal00Reco  = nullptr;
        TH2F* xyBeamSpotGen    = nullptr;
        TH2F* xyBeamSpotReco   = nullptr;
        TH3F* xyzBeamSpotGen   = nullptr;
        TH3F* xyzBeamSpotReco  = nullptr;

        // --- radial distances (particle / vertex wrt origin or beamspot) ---
        TH1F* rDistxyGlobal00Gen    = nullptr;
        TH1F* rDistxyGlobal00Reco   = nullptr;
        TH1F* rDistxyGlobal00Diff   = nullptr;
        TH1F* rDistxyzGlobal00Gen   = nullptr;
        TH1F* rDistxyzGlobal00Reco  = nullptr;
        TH1F* rDistxyzGlobal00Diff  = nullptr;
        TH1F* rDistxyBeamSpotGen    = nullptr;
        TH1F* rDistxyBeamSpotReco   = nullptr;
        TH1F* rDistxyBeamSpotDiff   = nullptr;
        TH1F* rDistxyzBeamSpotGen   = nullptr;
        TH1F* rDistxyzBeamSpotReco  = nullptr;
        TH1F* rDistxyzBeamSpotDiff  = nullptr;

        // --- kinematics ---
        TH1F* ptGen   = nullptr;
        TH1F* ptReco  = nullptr;
        TH1F* ptDiff  = nullptr;
        TH1F* etaGen  = nullptr;
        TH1F* etaReco = nullptr;
        TH1F* etaDiff = nullptr;
        TH1F* phiGen  = nullptr;
        TH1F* phiReco = nullptr;
        TH1F* phiDiff = nullptr;
        TH1F* massGen  = nullptr;
        TH1F* massReco = nullptr;
        TH1F* massDiff = nullptr;

        // --- vertex quality ---
        TH1F* chi2Reco        = nullptr;
        TH1F* significanceReco = nullptr;   // dBV / sigma(dBV)
        TH1F* dxyErrorReco    = nullptr;    // sigma(dBV) [cm]

        // --- matching ---
        TH1F* deltaR       = nullptr;
        TH1F* matchDistance = nullptr;

        // --- ΔR vs match distance vs gen particle d_xy wrt beamspot (3D) ---
        TH3F* deltaR_vs_matchDist_vs_genDxyBS = nullptr;

        // --- vertex tracks ---
        TH1F* vtxNtracks    = nullptr;  // number of tracks per matched vertex
        TH1F* vtxTrackPt    = nullptr;  // pT of each track in matched vertex
        TH1F* vtxTotalTrackPt = nullptr; // scalar sum of track pT per matched vertex



        void book(edm::Service<TFileService>& fs,
                  const std::string& suffix,
                  const std::string& modeLabel,
                  double massHint,
                  double decayHint) {
            const auto n = [&](const std::string& base) { return base + "_" + suffix; };
            
            const double scaling_factor = (0.1 + decayHint/10);
            const double xyPosMax   = 5.0  * scaling_factor;
            const double xyzPosMax  = 100.0 * scaling_factor;
            const double xyDiffMax  = 1.0  * scaling_factor;
            const double xyzDiffMax = 1.0  * scaling_factor;
            const double rxyMax     = 10.0 * scaling_factor;
            const double rxyzMax    = 100.0 * scaling_factor;
            const double massMax    = 2.0  * massHint;
            const double massDiffMax = 1.1*massHint;
            const double ptDiffMax  = 1.5  * massHint;

            // position residuals
            xyDiff  = fs->make<TH2F>(n("xy_diff").c_str(),
                ";#Deltax (reco#minusgen) [cm];#Deltay (reco#minusgen) [cm];Events",
                220, -xyDiffMax, xyDiffMax, 220, -xyDiffMax, xyDiffMax);
            xyzDiff = fs->make<TH3F>(n("xyz_diff").c_str(),
                ";#Deltax [cm];#Deltay [cm];#Deltaz [cm];Events",
                160, -xyDiffMax, xyDiffMax,
                160, -xyDiffMax, xyDiffMax,
                160, -xyzDiffMax, xyzDiffMax);

            // absolute positions – global origin
            xyGlobal00Gen  = fs->make<TH2F>(n("xy_global00_gen").c_str(),
                ";x_{gen} wrt (0,0) [cm];y_{gen} wrt (0,0) [cm];Events",
                220, -xyPosMax, xyPosMax, 220, -xyPosMax, xyPosMax);
            xyGlobal00Reco = fs->make<TH2F>(n("xy_global00_reco").c_str(),
                ";x_{reco} wrt (0,0) [cm];y_{reco} wrt (0,0) [cm];Events",
                220, -xyPosMax, xyPosMax, 220, -xyPosMax, xyPosMax);
            xyzGlobal00Gen  = fs->make<TH3F>(n("xyz_global00_gen").c_str(),
                ";x_{gen} [cm];y_{gen} [cm];z_{gen} [cm];Events",
                160, -xyPosMax, xyPosMax, 160, -xyPosMax, xyPosMax, 160, -xyzPosMax, xyzPosMax);
            xyzGlobal00Reco = fs->make<TH3F>(n("xyz_global00_reco").c_str(),
                ";x_{reco} [cm];y_{reco} [cm];z_{reco} [cm];Events",
                160, -xyPosMax, xyPosMax, 160, -xyPosMax, xyPosMax, 160, -xyzPosMax, xyzPosMax);

            // absolute positions – relative to beamspot
            xyBeamSpotGen  = fs->make<TH2F>(n("xy_beamspot_gen").c_str(),
                ";x_{gen} wrt beamspot [cm];y_{gen} wrt beamspot [cm];Events",
                220, -xyPosMax, xyPosMax, 220, -xyPosMax, xyPosMax);
            xyBeamSpotReco = fs->make<TH2F>(n("xy_beamspot_reco").c_str(),
                ";x_{reco} wrt beamspot [cm];y_{reco} wrt beamspot [cm];Events",
                220, -xyPosMax, xyPosMax, 220, -xyPosMax, xyPosMax);
            xyzBeamSpotGen  = fs->make<TH3F>(n("xyz_beamspot_gen").c_str(),
                ";x_{gen} wrt BS [cm];y_{gen} wrt BS [cm];z_{gen} wrt BS [cm];Events",
                160, -xyzPosMax, xyzPosMax, 160, -xyzPosMax, xyzPosMax, 160, -xyzPosMax, xyzPosMax);
            xyzBeamSpotReco = fs->make<TH3F>(n("xyz_beamspot_reco").c_str(),
                ";x_{reco} wrt BS [cm];y_{reco} wrt BS [cm];z_{reco} wrt BS [cm];Events",
                160, -xyzPosMax, xyzPosMax, 160, -xyzPosMax, xyzPosMax, 160, -xyzPosMax, xyzPosMax);

            // radial distances – global origin
            rDistxyGlobal00Gen  = fs->make<TH1F>(n("rdist_xy_global00_gen").c_str(),
                ";|r_{xy}| (particle wrt origin) [cm];Events", 160, 0.0, rxyMax);
            rDistxyGlobal00Reco = fs->make<TH1F>(n("rdist_xy_global00_reco").c_str(),
                ";|r_{xy}| (vertex wrt origin) [cm];Events",   160, 0.0, rxyMax);
            rDistxyGlobal00Diff = fs->make<TH1F>(n("rdist_xy_global00_diff").c_str(),
                ";|r_{xy}| vertex #minus particle wrt origin [cm];Events", 160, -rxyMax, rxyMax);

            rDistxyzGlobal00Gen  = fs->make<TH1F>(n("rdist_xyz_global00_gen").c_str(),
                ";|r| (particle wrt origin) [cm];Events", 160, 0.0, rxyzMax);
            rDistxyzGlobal00Reco = fs->make<TH1F>(n("rdist_xyz_global00_reco").c_str(),
                ";|r| (vertex wrt origin) [cm];Events",   160, 0.0, rxyzMax);
            rDistxyzGlobal00Diff = fs->make<TH1F>(n("rdist_xyz_global00_diff").c_str(),
                ";|r| vertex #minus particle wrt origin [cm];Events", 160, -rxyzMax, rxyzMax);

            // radial distances – beamspot
            rDistxyBeamSpotGen  = fs->make<TH1F>(n("rdist_xy_beamspot_gen").c_str(),
                ";d_{xy} (particle wrt beamspot) [cm];Events", 160, 0.0, rxyMax);
            rDistxyBeamSpotReco = fs->make<TH1F>(n("rdist_xy_beamspot_reco").c_str(),
                ";d_{xy} (vertex wrt beamspot) [cm];Events",   160, 0.0, rxyMax);
            rDistxyBeamSpotDiff = fs->make<TH1F>(n("rdist_xy_beamspot_diff").c_str(),
                ";d_{xy} vertex #minus particle wrt beamspot [cm];Events", 160, -rxyMax, rxyMax);

            rDistxyzBeamSpotGen  = fs->make<TH1F>(n("rdist_xyz_beamspot_gen").c_str(),
                ";d_{3D} (particle wrt beamspot) [cm];Events", 160, 0.0, rxyzMax);
            rDistxyzBeamSpotReco = fs->make<TH1F>(n("rdist_xyz_beamspot_reco").c_str(),
                ";d_{3D} (vertex wrt beamspot) [cm];Events",   160, 0.0, rxyzMax);
            rDistxyzBeamSpotDiff = fs->make<TH1F>(n("rdist_xyz_beamspot_diff").c_str(),
                ";d_{3D} vertex #minus particle wrt beamspot [cm];Events", 160, -rxyzMax, rxyzMax);

            // kinematics
            ptGen  = fs->make<TH1F>(n("pt_gen").c_str(),  ";p_{T,gen} [GeV];Events",  180, 0.0, massMax);
            ptReco = fs->make<TH1F>(n("pt_reco").c_str(), ";p_{T,reco} [GeV];Events", 180, 0.0, massMax);
            ptDiff = fs->make<TH1F>(n("pt_diff").c_str(),
                ";p_{T,reco} #minus p_{T,gen} [GeV];Events", 180, -ptDiffMax, ptDiffMax);

            etaGen  = fs->make<TH1F>(n("eta_gen").c_str(),  ";#eta_{gen};Events",  120, -6.0, 6.0);
            etaReco = fs->make<TH1F>(n("eta_reco").c_str(), ";#eta_{reco};Events", 120, -6.0, 6.0);
            etaDiff = fs->make<TH1F>(n("eta_diff").c_str(), ";#eta_{reco} #minus #eta_{gen};Events", 120, -6.0, 6.0);

            phiGen  = fs->make<TH1F>(n("phi_gen").c_str(),  ";#phi_{gen};Events",  128, -3.2, 3.2);
            phiReco = fs->make<TH1F>(n("phi_reco").c_str(), ";#phi_{reco};Events", 128, -3.2, 3.2);
            phiDiff = fs->make<TH1F>(n("phi_diff").c_str(), ";#phi_{reco} #minus #phi_{gen};Events", 128, -3.2, 3.2);

            massGen  = fs->make<TH1F>(n("mass_gen").c_str(),  ";m_{gen} [GeV];Events",  180, 0.0, massMax);
            massReco = fs->make<TH1F>(n("mass_reco").c_str(), ";m_{reco} [GeV];Events", 180, 0.0, massMax);
            massDiff = fs->make<TH1F>(n("mass_diff").c_str(),
                ";m_{reco} #minus m_{gen} [GeV];Events", 180, -massDiffMax, massDiffMax);

            // vertex quality
            chi2Reco         = fs->make<TH1F>(n("chi2_reco").c_str(),
                ";reduced #chi^{2} (vertex);Vertices", 120, 0.0, 10.0);
            significanceReco = fs->make<TH1F>(n("significance_reco").c_str(),
                ";d_{BV}/#sigma(d_{BV}) (vertex);Vertices", 200, 0.0, 200.0);
            dxyErrorReco     = fs->make<TH1F>(n("dxy_error_reco").c_str(),
                ";#sigma(d_{BV}) (vertex) [cm];Vertices", 200, 0.0, 0.02);

            // matching
            deltaR       = fs->make<TH1F>(n("deltaR").c_str(),
                ";#DeltaR(reco vertex, gen particle);Events", 140, 0.0, 5.0);
            matchDistance = fs->make<TH1F>(n("match_distance").c_str(),
                (";match distance (" + modeLabel + ") [cm];Events").c_str(),
                140, 0.0, 0.05);

            // ΔR vs match distance vs gen particle d_xy wrt beamspot (3D diagnostic)
            deltaR_vs_matchDist_vs_genDxyBS = fs->make<TH3F>(
                n("deltaR_vs_matchDist_vs_genDxyBS").c_str(),
                ";#DeltaR;Match Distance [cm];Gen d_{xy}^{BS} [cm]",
                100, 0.0, 5.0,
                100, 0.0, 0.1,
                100, 0.0, rxyMax);

            // track-level plots for matched vertices
            vtxNtracks      = fs->make<TH1F>(n("vtx_ntracks").c_str(),
                ";N_{tracks} per matched vertex;Vertices", 50, -0.5, 49.5);
            vtxTrackPt      = fs->make<TH1F>(n("vtx_track_pt").c_str(),
                ";track p_{T} in matched vertex [GeV];Tracks", 100, 0.0, 100.0);
            vtxTotalTrackPt = fs->make<TH1F>(n("vtx_total_track_pt").c_str(),
                ";#Sigma p_{T} of tracks in matched vertex [GeV];Vertices", 100, 0.0, massMax);
        }
    };

    struct CounterSet {
        uint64_t events           = 0;
        uint64_t truthEvents      = 0;
        uint64_t eventsWithMatches = 0;
        uint64_t selectedVertices  = 0;
    };

    // ------------------------------------------------------------------
    // Tokens / ES tokens
    // ------------------------------------------------------------------
    edm::EDGetTokenT<std::vector<reco::GenParticle>>   genParticlesToken_;
    edm::EDGetTokenT<std::vector<reco::Vertex>>        scoutingVerticesToken_;
    edm::EDGetTokenT<reco::BeamSpot>                   offlineBeamspotToken_;
    edm::EDGetTokenT<std::vector<Run3ScoutingPFJet>>   scoutingJetsToken_;
    edm::EDGetTokenT<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>> trackToScoutingMapToken_;
    edm::ESGetToken<TransientTrackBuilder, TransientTrackRecord> ttBuilderToken_;
    edm::ESGetToken<BeamSpotOnlineObjects, BeamSpotOnlineHLTObjectsRcd> bsOnlineToken_;

    bool useOnlineBeamSpot_ = false;

    // ------------------------------------------------------------------
    // Signal-region vertex cuts (analysis definition)
    // ------------------------------------------------------------------
    double       cut_vtx_chi2_max_   = 2.5;
    double       cut_vtx_dbv_min_    = 0.01;
    double       cut_vtx_dbv_max_    = 2.0;
    unsigned int cut_vtx_tracks_min_ = 8;
    unsigned int cut_vtx_tracks_max_ = 100;
    double       cut_vtx_ddbv_max_   = 0.005;
    double       cut_vtx_cosT_min_   = 0.0;

    // ------------------------------------------------------------------
    // Jet gate
    // ------------------------------------------------------------------
    double       jet_pt_min_         = 30.0;
    double       jet_eta_max_        = 2.5;
    unsigned int min_selected_jets_  = 3;
    bool         require_jet_selection_ = true;

    // ------------------------------------------------------------------
    // Misc config
    // ------------------------------------------------------------------
    bool        verbose_           = false;
    bool        verbose_unselected_ = false;

    MatchDistanceMode matchMode_      = MatchDistanceMode::k2D;
    std::string       matchModeLabel_ = "2D";
    double            massHint_       = 200.0;
    double            decayHint_      = 0.1;

    std::vector<int> parentPDG_;
    std::vector<int> daughterPDG_;

    // ------------------------------------------------------------------
    // Histograms (global / bookkeeping)
    // ------------------------------------------------------------------
    std::array<BucketPlots, 2> plots_;
    CounterSet counters_;

    // per-event / all-selected-vertices (before matching)
    TH1F* h_nSelectedVertices  = nullptr;  // N selected vertices per event
    TH3F* h_bs_pos             = nullptr;  // beamspot (x,y,z) per event
    TH1F* h_sel_vtx_ntracks    = nullptr;  // tracks per selected vertex (all events)
    TH1F* h_sel_vtx_trackPt    = nullptr;  // track pT in selected vertices
    TH1F* h_sel_vtx_totalTrackPt = nullptr; // scalar sum pT per selected vertex

    TH1F* h_trackPreselIpsig         = nullptr;
    TH1F* h_trackPreselNValidPixelHits = nullptr;
    TH1F* h_trackPreselNValidStripHits = nullptr;
    TH1F* h_trackPreselNTrackerLayers  = nullptr;

    // ------------------------------------------------------------------
    // Static helpers
    // ------------------------------------------------------------------
    static double wrapDeltaPhi(double a, double b) {
        constexpr double kPi = 3.14159265358979323846;
        double d = a - b;
        while (d >  kPi) d -= 2.0 * kPi;
        while (d <= -kPi) d += 2.0 * kPi;
        return d;
    }

    static LorentzVector trackP4(const reco::Track& trk) {
        const double p = trk.p();
        const double e = std::sqrt(p * p + kPionMass * kPionMass);
        return LorentzVector(trk.px(), trk.py(), trk.pz(), e);
    }

    static double deltaR(const LorentzVector& a, const LorentzVector& b) {
        const double dEta = a.eta() - b.eta();
        const double dPhi = wrapDeltaPhi(a.phi(), b.phi());
        return std::hypot(dEta, dPhi);
    }

    static double dist2D(double x1, double y1, double x2, double y2) {
        return std::hypot(x1 - x2, y1 - y2);
    }

    static double dist3D(double x1, double y1, double z1,
                         double x2, double y2, double z2) {
        return std::sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
    }

    static double vertexDistxy(double x, double y)            { return std::hypot(x, y); }
    static double vertexDistxyz(double x, double y, double z) { return std::sqrt(x*x + y*y + z*z); }

    double matchDistanceValue(const reco::Vertex& v, const reco::GenParticle& g) const {
        if (matchMode_ == MatchDistanceMode::k3D)
            return dist3D(v.x(), v.y(), v.z(), g.vx(), g.vy(), g.vz());
        return dist2D(v.x(), v.y(), g.vx(), g.vy());
    }

    // ------------------------------------------------------------------
    // Gen-truth search
    // ------------------------------------------------------------------
    std::array<const reco::GenParticle*, 2>
    findStops(const std::vector<reco::GenParticle>& genParticles,
              std::ostringstream* log) const {
        std::array<const reco::GenParticle*, 2> stops{{nullptr, nullptr}};
        for (const auto& p : genParticles) {
            if (std::find(parentPDG_.begin(), parentPDG_.end(), p.pdgId()) == parentPDG_.end())
                continue;
            int dauMatch = 0;
            for (size_t d = 0; d < p.numberOfDaughters(); ++d) {
                const reco::Candidate* dau = p.daughter(d);
                if (!dau) continue;
                if (std::find(daughterPDG_.begin(), daughterPDG_.end(), dau->pdgId()) != daughterPDG_.end())
                    ++dauMatch;
            }
            if (dauMatch != 2) continue;
            if (!stops[0])      stops[0] = &p;
            else if (!stops[1]) { stops[1] = &p; break; }
        }
        if (log && stops[0] && stops[1]) {
            *log << "========== 2 STOPs found ==========\n";
            for (int i = 0; i < 2; ++i)
                *log << "Stop" << (i+1)
                     << " x=" << stops[i]->vx() << " y=" << stops[i]->vy() << " z=" << stops[i]->vz()
                     << " pt=" << stops[i]->pt() << " mass=" << stops[i]->mass()
                     << " eta=" << stops[i]->eta() << " phi=" << stops[i]->phi()
                     << " r_xy=" << std::hypot(stops[i]->vx(), stops[i]->vy()) << "\n";
        }
        return stops;
    }

    // ------------------------------------------------------------------
    // Build selected vertex collection
    //
    // Every track in the input vertex is already accepted by the
    // Vertexer (IPSig, hit cuts, ΔR jet gate, arbitration, N-1).
    // We simply sum all tracks and apply signal-region cuts.
    // ------------------------------------------------------------------
    std::vector<SelectedVertexInfo>
    buildSelectedVertices(const edm::Handle<std::vector<reco::Vertex>>& scoutingVertices,
                      const reco::BeamSpot* beamspot,
                      bool eventPassesJetGate,
                      const edm::Handle<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>>& scoutingMapH,
                      bool haveScoutingMap,
                      const TransientTrackBuilder& tt_builder,
                      std::ostringstream* log) {
        std::vector<SelectedVertexInfo> selected;
        if (!scoutingVertices.isValid()) return selected;
        selected.reserve(scoutingVertices->size());

        const double bsX = beamspot ? beamspot->x0() : 0.0;
        const double bsY = beamspot ? beamspot->y0() : 0.0;
        const double bsZ = beamspot ? beamspot->z0() : 0.0;

        for (size_t i = 0; i < scoutingVertices->size(); ++i) {
            const auto& v = scoutingVertices->at(i);
            SelectedVertexInfo info;
            info.vertex  = &v;
            info.chi2    = v.normalizedChi2();
            info.dBV     = std::hypot(v.x() - bsX, v.y() - bsY);
            info.dBVErr  = std::hypot(v.xError(), v.yError());
            info.significance = (info.dBVErr > 0.0) ? info.dBV / info.dBVErr : -1.0;
            info.rDistxyGlobal00   = vertexDistxy(v.x(), v.y());
            info.rDistxyzGlobal00  = vertexDistxyz(v.x(), v.y(), v.z());
            info.rDistxyBeamSpot   = std::hypot(v.x() - bsX, v.y() - bsY);
            info.rDistxyzBeamSpot  = dist3D(v.x(), v.y(), v.z(), bsX, bsY, bsZ);

            // Sum over all tracks – selection already done in Vertexer
            math::XYZVector pSum(0., 0., 0.);
            double energySum = 0.0;
            info.nTracks = 0;

            std::ostringstream trackLog;
            unsigned int trackIndex = 0;
            for (auto it = v.tracks_begin(); it != v.tracks_end(); ++it, ++trackIndex) {
                reco::TrackRef trk = it->castTo<reco::TrackRef>();
                if (!trk.isNonnull()) continue;

                pSum += trk->momentum();
                energySum += trackP4(*trk).E();
                ++info.nTracks;
                info.trackPts.push_back(trk->pt());
                info.totalPt += trk->pt();

                int nPixelHits = trk->hitPattern().numberOfValidPixelHits();
                int nStripHits = trk->hitPattern().numberOfValidStripHits();
                int nLayers    = trk->hitPattern().trackerLayersWithMeasurement();

                if (haveScoutingMap) {
                    const auto scRef = (*scoutingMapH)[trk];
                    if (scRef.isNonnull()) {
                        nPixelHits = scRef->tk_nValidPixelHits();
                        nStripHits = scRef->tk_nValidStripHits();
                        nLayers    = scRef->tk_nTrackerLayersWithMeasurement();
                    }
                }

                if (h_trackPreselNValidPixelHits) h_trackPreselNValidPixelHits->Fill(nPixelHits);
                if (h_trackPreselNValidStripHits) h_trackPreselNValidStripHits->Fill(nStripHits);
                if (h_trackPreselNTrackerLayers)  h_trackPreselNTrackerLayers->Fill(nLayers);
                if (h_trackPreselIpsig) {
                    reco::Vertex::Error bsErr = beamspot ? beamspot->covariance3D() : reco::Vertex::Error{};
                    reco::Vertex fakeBsVtx(reco::Vertex::Point(bsX, bsY, bsZ), bsErr);

                    auto tt = tt_builder.build(trk);
                    if (tt.isValid()) {
                        auto ip = IPTools::absoluteTransverseImpactParameter(tt, fakeBsVtx);
                        if (ip.first) h_trackPreselIpsig->Fill(ip.second.significance());
                    }
                }
            }


            // cos(angle between displacement and momentum)
            const math::XYZVector disp(v.x() - bsX, v.y() - bsY, v.z() - bsZ);
            if (disp.R() > 0.0 && pSum.R() > 0.0)
                info.cosT = disp.Dot(pSum) / (disp.R() * pSum.R());

            // Signal-region cuts
            const bool passChi2   = info.chi2 < cut_vtx_chi2_max_;
            const bool passDbvMin = info.dBV  >= cut_vtx_dbv_min_;
            const bool passDbvMax = info.dBV  <  cut_vtx_dbv_max_;
            const bool passDdbv   = info.dBVErr < cut_vtx_ddbv_max_;
            const bool passCosT   = info.cosT   > cut_vtx_cosT_min_;
            const bool passNtk    = info.nTracks > cut_vtx_tracks_min_ && info.nTracks < cut_vtx_tracks_max_;
            const bool passAll    = passChi2 && passDbvMin && passDbvMax
                                 && passDdbv && passCosT  && passNtk
                                 && eventPassesJetGate;

            if (verbose_unselected_ && log) {
                *log << "Vertex " << i << ":\n"
                     << "  nTracks  : " << info.nTracks
                     << (passNtk ? " (> " : " (!> ") << cut_vtx_tracks_min_ << " (or the relevant tracks_max))\n"
                     << "  chi2/dof : " << info.chi2
                     << (passChi2 ? " (< " : " (!< ") << cut_vtx_chi2_max_ << ")\n"
                     << "  d_BV     : " << info.dBV
                     << (passDbvMin ? " (>= " : " (!>= ") << cut_vtx_dbv_min_
                     << ") and " << (passDbvMax ? "(< " : "(!< ") << cut_vtx_dbv_max_ << ")\n"
                     << "  sigma_dBV: " << info.dBVErr
                     << (passDdbv ? " (< " : " (!< ") << cut_vtx_ddbv_max_ << ")\n"
                     << "  cosT     : " << info.cosT
                     << (passCosT ? " (> " : " (!> ") << cut_vtx_cosT_min_ << ")\n"
                     << "  -> " << (passAll ? "PASSED" : "FAILED") << "\n"
                     << trackLog.str();
            }

            if (passAll) {
                info.p3 = pSum;
                info.p4 = LorentzVector(pSum.x(), pSum.y(), pSum.z(), energySum);
                selected.push_back(info);
            }
        }
        return selected;
    }

    // ------------------------------------------------------------------
    // Optimal matching (min-score bipartite assignment)
    // ------------------------------------------------------------------
    MatchSolution
    matchVertices(const std::vector<SelectedVertexInfo>& recoVertices,
                  const std::array<const reco::GenParticle*, 2>& genStops) const {
        MatchSolution out;
        if (!genStops[0] || !genStops[1] || recoVertices.empty()) return out;

        const auto dist = [&](const SelectedVertexInfo& v, const reco::GenParticle& g) {
            return matchDistanceValue(*v.vertex, g);
        };

        if (recoVertices.size() == 1) {
            const double d0 = dist(recoVertices[0], *genStops[0]);
            const double d1 = dist(recoVertices[0], *genStops[1]);
            if (d0 <= d1) { out.recoForGen[0] = 0; out.distance[0] = d0; out.totalScore = d0; }
            else          { out.recoForGen[1] = 0; out.distance[1] = d1; out.totalScore = d1; }
            return out;
        }

        for (size_t i = 0; i < recoVertices.size(); ++i) {
            for (size_t j = i + 1; j < recoVertices.size(); ++j) {
                const double d00 = dist(recoVertices[i], *genStops[0]);
                const double d01 = dist(recoVertices[i], *genStops[1]);
                const double d10 = dist(recoVertices[j], *genStops[0]);
                const double d11 = dist(recoVertices[j], *genStops[1]);

                const double scoreA = d00 + d11;
                if (scoreA < out.totalScore) {
                    out.totalScore = scoreA;
                    out.recoForGen = std::array<int, 2>{{static_cast<int>(i), static_cast<int>(j)}};
                    out.distance   = std::array<double, 2>{{d00, d11}};
                }
                const double scoreB = d10 + d01;
                if (scoreB < out.totalScore) {
                    out.totalScore = scoreB;
                    out.recoForGen = std::array<int, 2>{{static_cast<int>(j), static_cast<int>(i)}};
                    out.distance   = std::array<double, 2>{{d10, d01}};
                }
            }
        }
        return out;
    }

    // ------------------------------------------------------------------
    // Fill per-bucket histograms for one matched pair
    // ------------------------------------------------------------------
    void fillBucket(size_t bucketIndex,
                    const SelectedVertexInfo& reco,
                    const reco::GenParticle& gen,
                    const reco::BeamSpot* beamspot,
                    double matchMetric) const {
        if (bucketIndex >= plots_.size()) return;
        const auto& h = plots_[bucketIndex];
        const auto& v = *reco.vertex;

        const double bsX = beamspot ? beamspot->x0() : 0.0;
        const double bsY = beamspot ? beamspot->y0() : 0.0;
        const double bsZ = beamspot ? beamspot->z0() : 0.0;

        // position residuals
        const double dx = v.x() - gen.vx();
        const double dy = v.y() - gen.vy();
        const double dz = v.z() - gen.vz();

        if (h.xyDiff)  h.xyDiff->Fill(dx, dy);
        if (h.xyzDiff) h.xyzDiff->Fill(dx, dy, dz);

        // absolute positions – global origin
        if (h.xyGlobal00Gen)   h.xyGlobal00Gen->Fill(gen.vx(), gen.vy());
        if (h.xyGlobal00Reco)  h.xyGlobal00Reco->Fill(v.x(), v.y());
        if (h.xyzGlobal00Gen)  h.xyzGlobal00Gen->Fill(gen.vx(), gen.vy(), gen.vz());
        if (h.xyzGlobal00Reco) h.xyzGlobal00Reco->Fill(v.x(), v.y(), v.z());

        // absolute positions – beamspot
        if (h.xyBeamSpotGen)   h.xyBeamSpotGen->Fill(gen.vx()-bsX, gen.vy()-bsY);
        if (h.xyBeamSpotReco)  h.xyBeamSpotReco->Fill(v.x()-bsX,   v.y()-bsY);
        if (h.xyzBeamSpotGen)  h.xyzBeamSpotGen->Fill(gen.vx()-bsX, gen.vy()-bsY, gen.vz()-bsZ);
        if (h.xyzBeamSpotReco) h.xyzBeamSpotReco->Fill(v.x()-bsX,   v.y()-bsY,   v.z()-bsZ);

        // radial distances – global origin
        const double genRxy00   = vertexDistxy(gen.vx(), gen.vy());
        const double genRxyz00  = vertexDistxyz(gen.vx(), gen.vy(), gen.vz());
        const double recoRxy00  = reco.rDistxyGlobal00;
        const double recoRxyz00 = reco.rDistxyzGlobal00;

        if (h.rDistxyGlobal00Gen)   h.rDistxyGlobal00Gen->Fill(genRxy00);
        if (h.rDistxyGlobal00Reco)  h.rDistxyGlobal00Reco->Fill(recoRxy00);
        if (h.rDistxyGlobal00Diff)  h.rDistxyGlobal00Diff->Fill(recoRxy00 - genRxy00);
        if (h.rDistxyzGlobal00Gen)  h.rDistxyzGlobal00Gen->Fill(genRxyz00);
        if (h.rDistxyzGlobal00Reco) h.rDistxyzGlobal00Reco->Fill(recoRxyz00);
        if (h.rDistxyzGlobal00Diff) h.rDistxyzGlobal00Diff->Fill(recoRxyz00 - genRxyz00);

        // radial distances – beamspot
        const double genDxyBS   = std::hypot(gen.vx()-bsX, gen.vy()-bsY);
        const double genDxyzBS  = dist3D(gen.vx(), gen.vy(), gen.vz(), bsX, bsY, bsZ);
        const double recoDxyBS  = reco.rDistxyBeamSpot;
        const double recoDxyzBS = reco.rDistxyzBeamSpot;

        if (h.rDistxyBeamSpotGen)   h.rDistxyBeamSpotGen->Fill(genDxyBS);
        if (h.rDistxyBeamSpotReco)  h.rDistxyBeamSpotReco->Fill(recoDxyBS);
        if (h.rDistxyBeamSpotDiff)  h.rDistxyBeamSpotDiff->Fill(recoDxyBS - genDxyBS);
        if (h.rDistxyzBeamSpotGen)  h.rDistxyzBeamSpotGen->Fill(genDxyzBS);
        if (h.rDistxyzBeamSpotReco) h.rDistxyzBeamSpotReco->Fill(recoDxyzBS);
        if (h.rDistxyzBeamSpotDiff) h.rDistxyzBeamSpotDiff->Fill(recoDxyzBS - genDxyzBS);

        // kinematics
        const LorentzVector genP4(gen.px(), gen.py(), gen.pz(), gen.energy());
        const LorentzVector& recoP4 = reco.p4;

        if (h.ptGen)   h.ptGen->Fill(genP4.pt());
        if (h.ptReco)  h.ptReco->Fill(recoP4.pt());
        if (h.ptDiff)  h.ptDiff->Fill(recoP4.pt() - genP4.pt());
        if (h.etaGen)  h.etaGen->Fill(genP4.eta());
        if (h.etaReco) h.etaReco->Fill(recoP4.eta());
        if (h.etaDiff) h.etaDiff->Fill(recoP4.eta() - genP4.eta());
        if (h.phiGen)  h.phiGen->Fill(genP4.phi());
        if (h.phiReco) h.phiReco->Fill(recoP4.phi());
        if (h.phiDiff) h.phiDiff->Fill(wrapDeltaPhi(recoP4.phi(), genP4.phi()));
        if (h.massGen)  h.massGen->Fill(genP4.mass());
        if (h.massReco) h.massReco->Fill(recoP4.mass());
        if (h.massDiff) h.massDiff->Fill(recoP4.mass() - genP4.mass());

        // vertex quality
        if (h.chi2Reco)         h.chi2Reco->Fill(reco.chi2);
        if (h.significanceReco) h.significanceReco->Fill(reco.significance);
        if (h.dxyErrorReco)     h.dxyErrorReco->Fill(reco.dBVErr);

        // matching
        const double dR = deltaR(recoP4, genP4);
        if (h.deltaR)        h.deltaR->Fill(dR);
        if (h.matchDistance) h.matchDistance->Fill(matchMetric);

        // ΔR vs match distance vs gen particle d_xy wrt beamspot
        if (h.deltaR_vs_matchDist_vs_genDxyBS)
            h.deltaR_vs_matchDist_vs_genDxyBS->Fill(dR, matchMetric, genDxyBS);

        // per-vertex track plots
        if (h.vtxNtracks)      h.vtxNtracks->Fill(static_cast<double>(reco.nTracks));
        if (h.vtxTotalTrackPt) h.vtxTotalTrackPt->Fill(reco.totalPt);
        if (h.vtxTrackPt) {
            for (double pt : reco.trackPts) h.vtxTrackPt->Fill(pt);
        }
    }

public:
    explicit GenScoutingComparer(const edm::ParameterSet& config) {
        usesResource("TFileService");

        genParticlesToken_    = consumes<std::vector<reco::GenParticle>>(config.getParameter<edm::InputTag>("genParticles"));
        scoutingVerticesToken_ = consumes<std::vector<reco::Vertex>>(config.getParameter<edm::InputTag>("scoutingVertices"));
        offlineBeamspotToken_  = consumes<reco::BeamSpot>(config.getParameter<edm::InputTag>("beamspot"));
        scoutingJetsToken_     = consumes<std::vector<Run3ScoutingPFJet>>(config.getParameter<edm::InputTag>("scoutingJets"));
        trackToScoutingMapToken_ =
            consumes<
                edm::ValueMap<
                    edm::Ref<std::vector<Run3ScoutingTrack>>
                >
            >(config.getParameter<edm::InputTag>("trackToScoutingMap"));
        bsOnlineToken_         = esConsumes<BeamSpotOnlineObjects, BeamSpotOnlineHLTObjectsRcd>();
        ttBuilderToken_ = esConsumes<TransientTrackBuilder,TransientTrackRecord>(edm::ESInputTag("", "TransientTrackBuilder"));
        

        cut_vtx_chi2_max_   = config.getParameter<double>("vtx_chi2_max");
        cut_vtx_dbv_min_    = config.getParameter<double>("vtx_dbv_min");
        cut_vtx_dbv_max_    = config.getParameter<double>("vtx_dbv_max");
        cut_vtx_tracks_min_ = config.getParameter<unsigned int>("vtx_tracks_min");
        cut_vtx_tracks_max_ = config.getParameter<unsigned int>("vtx_tracks_max");
        cut_vtx_ddbv_max_   = config.getParameter<double>("vtx_ddbv_max");
        cut_vtx_cosT_min_   = config.getParameter<double>("vtx_cosT_min");

        jet_pt_min_          = config.getParameter<double>("jet_pt_min");
        jet_eta_max_         = config.getParameter<double>("jet_eta_max");
        min_selected_jets_   = config.getParameter<unsigned int>("min_selected_jets");
        require_jet_selection_ = config.getUntrackedParameter<bool>("require_jet_selection", true);

        useOnlineBeamSpot_ = config.getUntrackedParameter<bool>("useOnlineBeamSpot", true);
        massHint_          = config.getUntrackedParameter<double>("masshint", 200.0);
        decayHint_         = config.getUntrackedParameter<double>("decayhint", 0.1);
        verbose_           = config.getUntrackedParameter<bool>("verbose", false);
        verbose_unselected_ = config.getUntrackedParameter<bool>("verbose_unselected", false);

        matchModeLabel_ = config.getUntrackedParameter<std::string>("match_distance_mode", "2D");
        if (matchModeLabel_ == "3D" || matchModeLabel_ == "3d") {
            matchMode_ = MatchDistanceMode::k3D; matchModeLabel_ = "3D";
        } else {
            matchMode_ = MatchDistanceMode::k2D; matchModeLabel_ = "2D";
        }

        for (int v : config.getUntrackedParameter<std::vector<int>>("parent_pdgids",  {1000006, -1000006})) parentPDG_.push_back(v);
        for (int v : config.getUntrackedParameter<std::vector<int>>("daughter_pdgids", {1, -1}))            daughterPDG_.push_back(v);
    }

    void beginJob() override {
        edm::Service<TFileService> fs;
        if (!fs) { edm::LogWarning("GenScoutingComparer") << "TFileService missing"; return; }

        plots_[0].book(fs, "1", matchModeLabel_, massHint_, decayHint_);
        plots_[1].book(fs, "2", matchModeLabel_, massHint_, decayHint_);

        // global bookkeeping histograms
        h_nSelectedVertices  = fs->make<TH1F>("n_selected_vertices",
            "N selected vertices per event;N_{sel. vtx};Events", 21, -0.5, 20.5);
        h_bs_pos             = fs->make<TH3F>("beamspot_xyz",
            "Beamspot position per event;x_{BS} [cm];y_{BS} [cm];z_{BS} [cm]",
            200, -0.1, 0.1, 200, -0.1, 0.1, 400, -50.0, 50.0);
        h_sel_vtx_ntracks    = fs->make<TH1F>("sel_vtx_ntracks",
            "Tracks per selected vertex (all events);N_{tracks};Vertices",
            50, -0.5, 49.5);
        h_sel_vtx_trackPt    = fs->make<TH1F>("sel_vtx_track_pt",
            "Track p_{T} in selected vertices (all events);p_{T,track} [GeV];Tracks",
            100, 0.0, 100.0);
        h_sel_vtx_totalTrackPt = fs->make<TH1F>("sel_vtx_total_track_pt",
            "#Sigma p_{T} of tracks per selected vertex (all events);#Sigma p_{T} [GeV];Vertices",
            100, 0.0, 3.0 * massHint_);

        h_trackPreselIpsig = fs->make<TH1F>(
            "track_presel_ipsig",
            "Track preselection IP significance;IP significance;Tracks",
            200, 0.0, 200.0);

        h_trackPreselNValidPixelHits = fs->make<TH1F>(
            "track_presel_nValidPixelHits",
            "Track preselection valid pixel hits;N_{pixel};Tracks",
            20, -0.5, 19.5);

        h_trackPreselNValidStripHits = fs->make<TH1F>(
            "track_presel_nValidStripHits",
            "Track preselection valid strip hits;N_{strip};Tracks",
            30, -0.5, 29.5);

        h_trackPreselNTrackerLayers = fs->make<TH1F>(
            "track_presel_nTrackerLayers",
            "Track preselection tracker layers;N_{layers};Tracks",
            30, -0.5, 29.5);

        edm::LogInfo("GenScoutingComparer") << "setup done; match mode: " << matchModeLabel_;
    }

    void analyze(const edm::Event& event, const edm::EventSetup& eventSetup) override {
        ++counters_.events;

        std::ostringstream log;
        if (verbose_ || verbose_unselected_)
            log << "========== Event: " << event.id().event() << " ==========\n";

        // ---- gen particles ----
        edm::Handle<std::vector<reco::GenParticle>> genParticles;
        if (!event.getByToken(genParticlesToken_, genParticles) || !genParticles.isValid()) return;

        // ---- reco vertices ----
        edm::Handle<std::vector<reco::Vertex>> scoutingVertices;
        if (!event.getByToken(scoutingVerticesToken_, scoutingVertices) || !scoutingVertices.isValid()) return;

        // ---- beamspot (offline fallback, online preferred) ----
        edm::Handle<reco::BeamSpot> beamspot;
        const bool haveOffline = event.getByToken(offlineBeamspotToken_, beamspot) && beamspot.isValid();
        const reco::BeamSpot* bs = haveOffline ? &(*beamspot) : nullptr;

        if (useOnlineBeamSpot_) {
            auto bsOnlineHandle = eventSetup.getHandle(bsOnlineToken_);
            if (bsOnlineHandle.isValid()) {
                static reco::BeamSpot onlineBs;
                reco::BeamSpot::CovarianceMatrix onlineCov{};
                for (int i = 0; i < 7; ++i) {
                    for (int j = i; j < 7; ++j) {
                        onlineCov(i,j) = bsOnlineHandle->covariance(i,j);
                        onlineCov(j,i) = bsOnlineHandle->covariance(i,j);
                    }
                }
                const reco::BeamSpot::Point onlinePos(bsOnlineHandle->x(), bsOnlineHandle->y(), bsOnlineHandle->z());
                onlineBs = reco::BeamSpot(onlinePos, bsOnlineHandle->sigmaZ(),
                                          bsOnlineHandle->dxdz(), bsOnlineHandle->dydz(),
                                          bsOnlineHandle->beamWidthX(), onlineCov);
                bs = &onlineBs;
            } else {
                edm::LogWarning("GenScoutingComparer")
                    << "Online beamspot unavailable; using " << (bs ? "offline" : "null") << " beamspot.";
            }
        }

        edm::Handle<edm::ValueMap<edm::Ref<std::vector<Run3ScoutingTrack>>>> scoutingMapH;
        const bool haveScoutingMap =
            !trackToScoutingMapToken_.isUninitialized() &&
            event.getByToken(trackToScoutingMapToken_, scoutingMapH) &&
            scoutingMapH.isValid();

        auto const& tt_builder = eventSetup.getData(ttBuilderToken_);

        // ---- jet gate ----
        edm::Handle<std::vector<Run3ScoutingPFJet>> scoutingJets;
        event.getByToken(scoutingJetsToken_, scoutingJets);

        unsigned int nSelectedJets = 0;
        if (scoutingJets.isValid()) {
            for (const auto& jet : *scoutingJets)
                if (jet.pt() > jet_pt_min_ && std::abs(jet.eta()) < jet_eta_max_)
                    ++nSelectedJets;
        }
        const bool eventPassesJetGate = !require_jet_selection_ || (nSelectedJets >= min_selected_jets_);

        // ---- gen truth ----
        auto stops = findStops(*genParticles, (verbose_ || verbose_unselected_) ? &log : nullptr);
        if (stops[0] && stops[1]) ++counters_.truthEvents;

        // ---- selected reco vertices ----
        std::vector<SelectedVertexInfo> selectedVertices =
            buildSelectedVertices(scoutingVertices, bs, eventPassesJetGate,
                          scoutingMapH, haveScoutingMap, tt_builder,
                          (verbose_ || verbose_unselected_) ? &log : nullptr);

        counters_.selectedVertices += selectedVertices.size();

        // global bookkeeping fills (every event)
        if (h_nSelectedVertices) h_nSelectedVertices->Fill(static_cast<int>(selectedVertices.size()));
        if (bs && h_bs_pos)      h_bs_pos->Fill(bs->x0(), bs->y0(), bs->z0());
        for (const auto& vinfo : selectedVertices) {
            if (h_sel_vtx_ntracks)     h_sel_vtx_ntracks->Fill(static_cast<double>(vinfo.nTracks));
            if (h_sel_vtx_totalTrackPt) h_sel_vtx_totalTrackPt->Fill(vinfo.totalPt);
            if (h_sel_vtx_trackPt)
                for (double pt : vinfo.trackPts) h_sel_vtx_trackPt->Fill(pt);
        }

        // ---- matching and resolution plots ----
        if (stops[0] && stops[1] && !selectedVertices.empty()) {
            const MatchSolution match = matchVertices(selectedVertices, stops);

            for (int gi = 0; gi < 2; ++gi) {
                const int ri = match.recoForGen[gi];
                if (ri < 0) continue;
                fillBucket(static_cast<size_t>(gi),
                           selectedVertices.at(static_cast<size_t>(ri)),
                           *stops[gi], bs, match.distance[gi]);
            }

            if (match.recoForGen[0] >= 0 || match.recoForGen[1] >= 0)
                ++counters_.eventsWithMatches;

            if (verbose_) {
                log << "========== SCOUTING JETS selected=" << nSelectedJets
                    << " / total=" << (scoutingJets.isValid() ? scoutingJets->size() : 0)
                    << " (need>=" << min_selected_jets_ << ") => "
                    << (eventPassesJetGate ? "PASS" : "FAIL") << " ==========\n";
                log << "========== SELECTED RECO VERTICES (" << selectedVertices.size() << ") ==========\n";
                for (size_t i = 0; i < selectedVertices.size(); ++i) {
                    const auto& info = selectedVertices[i];
                    log << "  Vtx " << i
                        << "  x=" << info.vertex->x() << " y=" << info.vertex->y() << " z=" << info.vertex->z()
                        << "  pt=" << info.p4.pt() << "  mass=" << info.p4.mass()
                        << "  nTracks=" << info.nTracks
                        << "  sumPt=" << info.totalPt
                        << "  chi2=" << info.chi2
                        << "  dBV=" << info.dBV << "+/-" << info.dBVErr
                        << "  sig=" << info.significance << "\n";
                }
            }
        }

        if (verbose_ || verbose_unselected_)
            edm::LogVerbatim("GenScoutingComparer") << log.str();
    }

    void endJob() override {
        edm::LogInfo("GenScoutingComparer")
            << "events="          << counters_.events
            << " truthEvents="   << counters_.truthEvents
            << " matchedEvents=" << counters_.eventsWithMatches
            << " selectedVtx="   << counters_.selectedVertices;
    }
};

DEFINE_FWK_MODULE(GenScoutingComparer);