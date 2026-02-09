# README — Offline-miniAOD adaptation for Fresh-BeamPipe-Mapping

**Purpose**: This README explains the goal, current approach, known problems, and a concrete, step-by-step plan to adapt your existing Run-3 *scouting* workflow to run on *offline* MiniAOD/MINIAOD (or similar) files. It describes which files to change, exact hooks (InputTags / produced labels), and how to test on a small sample.

---

## 1) Overall goal

Detect (and map) the beampipe material and related detector structures using the same analysis chain you already have for scouting data, but apply it to *offline-quality* data (MiniAOD / MINIAOD / RECO) so that higher-quality tracks and hit/geometry information can reveal features that scouting reconstruction may obscure.

**Why**: Joey suggested that the better track quality in offline datasets may reveal the beampipe while scouting tracks (reduced information and compressed formats) might smear or bias the shapes.

## 2) Current approach — file-by-file, how data flows

### Key code files in your repo (as seen in your message):

* `HLTScoutingUnpackProducer` (C++):

  * Path: `HLTScoutingUnpackProducer/plugins/...` (your project)
  * What it does: unpacks Run3 scouting objects (`Run3ScoutingTrack`, `Run3ScoutingVertex`, `Run3ScoutingParticle`) into **reco** equivalents ( `reco::Track`, `reco::Vertex`, `reco::PFCandidate` ).
  * Outputs (when `isScouting=true`):

    * `std::vector<reco::Vertex>` labeled `PrimaryVertex` (use label `PrimaryVertex`).
    * `std::vector<reco::Track>` labeled `Track` (use label `Track`).
    * `RefMap` value-maps that point back to original scouting collections (label suffix `-RefToOriginal`).
  * Where it's hooked: your python config creates `process.hltScoutingUnpackProducer` and then Vertexer is fed with `seed_tracks_src = cms.InputTag('hltScoutingUnpackProducer', 'Track')` and `primaryVertices = cms.InputTag('hltScoutingUnpackProducer', 'PrimaryVertex')`.

* `Vertexer` (C++):

  * Path: `Vertexer/plugins/Vertexer.cc` (your repo)
  * What it does: takes a `std::vector<reco::Track>` and optional PV/BeamSpot and runs the vertex-seeding/fitting/merging logic, producing a `reco::VertexCollection` (module output label equals the `produces()` entry; in your config it is `process.Vertexer` with default label `Vertexer` in the path). Vertexer expects `seed_tracks_src` to be a `reco::Track` collection.
  * Important inputs (in your `Vertexer.py` config): `seed_tracks_src`, `primaryVertices`, `beamspot_src`. The `seed_tracks_src` is the *exact hook* for tracks to seed vertexing.

* `ScoutingPlotMakerRun3` (C++ analyzer):

  * Reads Vertexer output (displaced vertices) + track collections and beamspot/primary vertices to produce histograms.
  * Configured to read the tracks produced by the unpacker (same label `hltScoutingUnpackProducer:Track`) and displaced vertices created by Vertexer.

* `Vertexer.py` (python configuration):

  * The driver .py defines `process.hltScoutingUnpackProducer`, `process.Vertexer`, the `process.p` path and the `process.scoutingPlots` analyzer.
  * The connection points are the cms.InputTag strings in each module's parameters.

**Where outputs/inputs are hooked** (explicit):

* Unpacker -> produces `Track` label: `hltScoutingUnpackProducer:Track`.
* Vertexer takes `seed_tracks_src = cms.InputTag('hltScoutingUnpackProducer','Track')`.
* Vertexer produces displaced vertices under its produced label (used by `scoutingPlots` as `displacedVertices = cms.InputTag('Vertexer')`).
* ScoutingPlotMakerRun3 reads `tracks = cms.InputTag('hltScoutingUnpackProducer', 'Track')` and `displacedVertices = cms.InputTag('Vertexer')`.

## 3) Possible problems with the current approach (why beampipe may be invisible)

1. **Scouting track fidelity**: scouting tracks are compressed / have limited hit detail (and hit patterns are approximated). Their hit resolution or hit counts may underestimate layer thickness or smear radial structure, hiding narrow ~1 mm signals like the beampipe.

2. **Hit-pattern / layer assumptions**: the unpacker and PackedCandidate-based fake hit pattern builders approximate or assume layer counts which can bias vertex IP/weighting.

3. **Reference vertex choice**: using BeamSpot vs avgPV affects dBV calculations; mismatches across modules can blur features.

4. **Seed selection too tight**: high IP-sigma cuts (4σ) remove tracks from prompt material interactions that still point to origin. Lowering to 2σ may increase signal but also background; you already tried 2σ.

5. **Binning / histogram ranges**: if the beampipe is only ~1 mm, binning and coordinate precision must be adequate (but you already tried ultrafine bins and still didn’t see it).

6. **Conversion bugs**: minor mistakes in converting scouting fields into reco::Track / reco::Vertex (e.g. wrong covariances, wrong reference point) can distort geometry.

7. **Physics vs acceptance**: material interactions that produce visible secondaries may be below pt thresholds or have opening angles that fail selection.

## 4) Plausible thing to try and compare against current approach

**Do the entire chain on MINIAOD / MiniAOD (offline-quality) files** — extract good charged tracks from `pat::PackedCandidateCollection` (the `packedPFCandidates` collection in MiniAOD), convert them to a `reco::TrackCollection` (via `cand.pseudoTrack()`), and feed that track collection to your `Vertexer` and `ScoutingPlotMakerRun3` *exactly the same way* you did for scouting data.

Then compare:

* XY histograms, dBV distributions, opening-angle histograms, and the presence (or absence) of beampipe feature.
* See if offline reveals beampipe while scouting does not. If offline shows beampipe clearly, you will have evidence scouting is losing information.

## 5) How is the new offline approach different?

* **Source collections**: instead of `Run3ScoutingTrack`/`Run3ScoutingVertex` and `hltScoutingUnpackProducer`, you will use `pat::PackedCandidateCollection` (label usually `packedPFCandidates` in MiniAOD) and convert *charged* candidates with track details into `reco::Track` objects using `cand.pseudoTrack()`.

* **Quality**: offline MiniAOD tracks (via `pat::PackedCandidate` pseudoTracks) often have richer hit information accessible and are derived from full reconstruction, so IP estimates and hit patterns are more faithful.

* **No unpacker required**: `HLTScoutingUnpackProducer` is a *scouting-specific* unpacker. For offline data you either remove it or add an intermediate producer that converts `pat::PackedCandidateCollection` to a `reco::TrackCollection` that Vertexer can consume.

## 6) How do inputs differ (concrete)

### Scouting workflow inputs

* `Run3ScoutingTrackCollection` (custom scouting format)
* `Run3ScoutingVertexCollection` (scouting PVs)
* `Run3ScoutingParticle` / `Run3ScoutingPFJet` (optional)
* **Python config hooks**: `hltScoutingUnpackProducer` consumes scouting tags and `produces("Track")` for Vertexer.

### Offline/MiniAOD workflow inputs

* `pat::PackedCandidateCollection` (label typical: `packedPFCandidates`) — for charged candidates
* `std::vector<reco::Vertex>` from the file (if present) or `offlineBeamSpot` available in event setup
* `reco::Track` collections may or may not exist directly in MiniAOD; often you convert `pat::PackedCandidate::pseudoTrack()` to `reco::Track`

**Concrete differences**:

* In scouting you call `hltScoutingUnpackProducer` to create `Track`. For offline you will either:

  * write a small producer to convert `packedPFCandidates` -> `reco::TrackCollection` (recommended), or
  * use an existing standard producer if you find one in CMSSW that does that (less likely & depends on release).

## 7) Exactly what to modify, where (step-by-step) — "the patch plan"

> The principle: keep *Vertexer* and *ScoutingPlotMakerRun3* unchanged where possible; only modify how `reco::TrackCollection` is produced and how the path wires modules together.

### A — Add a small C++ EDProducer: `PackedCandidateToTrackProducer`

**Location**: create a new plugin in your repo, e.g.

```
PackedCandidateToTrackProducer/plugins/PackedCandidateToTrackProducer.cc
PackedCandidateToTrackProducer/interface/PackedCandidateToTrackProducer.h
```

**What it does**: reads `pat::PackedCandidateCollection` (InputTag configurable), loops over candidates, and for each charged candidate with `hasTrackDetails()` pushes back `cand.pseudoTrack()` into a `reco::TrackCollection`. Optionally apply minimal `pt` or hit/layer filters here.

**Key implementation notes**:

* `edm::Handle<pat::PackedCandidateCollection>` and `pat::PackedCandidate` API: `cand.charge()`, `cand.hasTrackDetails()`, `cand.pseudoTrack()` (this returns a `reco::Track` object). The producers in your config will call this module.
* Module label example: `process.packedCandidateToTrack = cms.EDProducer('PackedCandidateToTrackProducer', src = cms.InputTag('packedPFCandidates'), minPt = cms.double(0.5))`.
* The module should `produces<std::vector<reco::Track>>('Track')` so it matches what Vertexer expects. Alternatively use label `'UnpackedTrack'` but then set Vertexer to read that label.

**Minimal skeleton (conceptual)** — what to implement in `produce()`:

```cpp
edm::Handle<pat::PackedCandidateCollection> packedHandle;
evt.getByToken(packedToken_, packedHandle);
auto out = std::make_unique<reco::TrackCollection>();
for (const auto & cand : *packedHandle) {
  if (cand.charge() == 0) continue;
  if (!cand.hasTrackDetails()) continue;
  const reco::Track & tk = cand.pseudoTrack();
  if (tk.pt() < minPt_) continue;
  out->push_back(tk);
}
event.put(std::move(out), "Track");
```

*(I can generate full C++ skeleton code if you want; say so and indicate the CMSSW release you use.)*

### B — Change python config to use the new producer and offline input files

1. Replace / remove `process.hltScoutingUnpackProducer` from the path for the offline run.
2. Add the new producer module in the python cfg, e.g.

```python
process.packedCandidateToTrack = cms.EDProducer('PackedCandidateToTrackProducer',
    src = cms.InputTag('packedPFCandidates'),
    minPt = cms.double(0.5)
)
```

3. Change `process.Vertexer` config so `seed_tracks_src` points to the new producer's output:

```python
process.Vertexer = cms.EDProducer('Vertexer',
    seed_tracks_src = cms.InputTag('packedCandidateToTrack','Track'),
    primaryVertices = cms.InputTag('offlinePrimaryVertices'),  # or from the file
    beamspot_src = cms.InputTag('offlineBeamSpot'),
    ...
)
```

4. The `scoutingPlots` module should read the new tracks: change `tracks = cms.InputTag('packedCandidateToTrack', 'Track')` or to whichever label you choose.

**Concrete lines to edit** (based on your existing `Vertexer.py`):

* Remove or comment the `process.hltScoutingUnpackProducer` block and its use in the `process.p` path.
* Add `process.packedCandidateToTrack` as shown above.
* In `process.Vertexer` change `seed_tracks_src = cms.InputTag('hltScoutingUnpackProducer', 'Track')` to `cms.InputTag('packedCandidateToTrack', 'Track')`.
* In `process.scoutingPlots` change the `tracks = cms.InputTag('hltScoutingUnpackProducer', 'Track')` to the same `packedCandidateToTrack` tag.

**Alternative**: if you prefer *not* to write a C++ plugin, you can quickly produce a temporary Track collection using a `cms.EDProducer` implemented in python via `cms.EDProducer('PATElectronProducer')`? — this is typically not available. **Writing the small C++ plugin is the cleanest and most reproducible approach.**

### C — Optional filters & hit-layer checks

You (and Joey) suggested adding stricter checks (pixel layers, strip layers, missing inner hits). You can implement those either in the `PackedCandidateToTrackProducer` or let `Vertexer` do the filtering if it supports similar parameters. The advantage of doing them in the producer is you reduce the number of tracks early and limit CPU.

Example (from Joey) of `pass_tk()` logic to port into the C++ producer (or call similar checks in Vertexer):

```cpp
bool pass_tk(const reco::Track& tk) const {
  return tk.pt() >= 1 &&
         tk.hitPattern().pixelLayersWithMeasurement() >= 2 &&
         tk.hitPattern().stripLayersWithMeasurement() >= 6 &&
         ( tk.hitPattern().hasValidHitInPixelLayer(PixelSubdetector::PixelBarrel,1) || (tk.hitPattern().hasValidHitInPixelLayer(PixelSubdetector::PixelBarrel,2) && tk.hitPattern().numberOfLostHits(TrackingRecHit::MISSING_INNER_HITS)==0) ) &&
         (fabs(tk.dxy() / tk.dxyError()) > 4);
}
```

*(Adjust numbers for seed_minIPSig etc; this is a suggested starting rule.)*

### D — Path ordering

Your `process.p` should become, e.g.:

```python
process.p = cms.Path(
    process.scoutingTrackCount +             # optional
    process.packedCandidateToTrack +
    process.offlineBeamSpot +
    process.Vertexer +
    process.scoutingPlots
)
```

## 8) What exact input files to use and how to inspect them (commands & recommended tests)

### Where to find candidate sample files (examples)

* Use a small MiniAOD (or MINIAODSIM) dataset that is similar to your physics process — e.g. the DY->2mu+jets MiniAOD you previously had in commented lines is a good starting point.
* If you don't already have local copies, request a small subset from DAS (or copy one remote file to dCache/xrootd). Joey suggested starting from an MC MiniAOD.

### Quick checks of file contents (EDM introspection)

Run these on a terminal with your CMSSW environment set up (cmsenv):

1. **List event content (what branch names exist)**

```bash
edmDumpEventContent root://.../someMiniAODFile.root
```

Look for `patPackedCandidates` / `packedPFCandidates` or a `pat::PackedCandidateCollection` entry.

2. **Dump provenance / event**

```bash
edmProvDump root://.../someMiniAODFile.root
```

3. **Quick list from ROOT (non-EDM aware)**

```bash
root -l root://.../someMiniAODFile.root
TFile *f = TFile::Open("root://.../someMiniAODFile.root"); f->ls();
```

(This shows top-level keys but not all EDM products; prefer `edmDumpEventContent`.)

4. **Check for PackedCandidates specifically (grep)**

```bash
edmDumpEventContent someMiniAODFile.root | grep PackedCandidate -n
```

### How to run the workflow on a *small* sample for testing

1. In your python cfg file (modified as described in step 7), set `process.maxEvents.input = 100` (or 1000) and put a single miniAOD file in `process.source.fileNames`.
2. Run `cmsRun yourModifiedVertexer.py` (or whatever config name you used).
3. Inspect the TFile output from `TFileService` (your `test-outputs/...root`) and examine histograms with ROOT.

### Validation checks to compare scouting vs offline

* Run the **same** analysis selections for scouting and offline and compare:

  * `Vertices/Selected/ntk_*/angle_*/Spatial/xy_global` 2D histograms — visually check for beampipe structure.
  * `Vertices/*/Distance/dBV_ref` histograms — look for characteristic peak at beampipe radius.
  * `Tracks/*/ImpactParameter` distributions — compare IP sigma distributions.
  * Event counts after each selection (to be sure you’re comparing similar populations).

---

## Quick checklist for your immediate actions (copy-paste friendly)

1. **Write the small EDProducer** `PackedCandidateToTrackProducer` that converts `packedPFCandidates` → `reco::TrackCollection` (label the output branch `Track`).
2. Add it to your `Vertexer.py` (or create a new offline cfg) and change `seed_tracks_src` and `scoutingPlots.tracks` to read from `packedCandidateToTrack:Track`.
3. Replace input files in the python cfg to a single MiniAOD sample and set `maxEvents = 100`.
4. Build (scram b) and run `cmsRun` on that cfg. Inspect the resulting `.root`.
5. Compare histograms to your scouting run.

---

## If you want, I can provide next items (pick one):

* [ ] Full C++ skeleton for `PackedCandidateToTrackProducer` (header + cc) ready to drop into your CMSSW plugin dir.
* [ ] A complete patched `Vertexer.py` (the offline-ready version) with exact lines changed.
* [ ] A small test driver config `Vertexer_miniAOD_test.py` with one example input, maxEvents=100, and the new producer inserted.

Tell me which and I will generate the code/config and place it in the repo (or paste the patch) based on your CMSSW release.

---

### Notes & caveats

* This README assumes you can add a small plugin and recompile the release used for the project.
* If you prefer not to recompile, we can attempt an alternate python-only solution, but that tends to be messier and less reproducible.
* The beampipe signal is narrow — ensure coordinate units and axis ranges are consistent and that vertex positions are not being shifted by an incorrect reference.

---

*End of README.*
