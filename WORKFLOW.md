# Run3 Scouting Analysis Workflow Documentation

**Package**: `Run3ScoutingAnalysisTools`  
**Author**: Adapted by Aryan Malhotra (based on original by David Sperka, Bruno Lopes, et al.)  
**Last Updated**: 2025-01-30  
**CMSSW Version**: `CMSSW_14_0_18_patch1`

---

## Table of Contents

1. [Overview](#overview)
2. [Quick Start](#quick-start)
3. [Module Descriptions](#module-descriptions)
4. [Configuration Guide](#configuration-guide)
5. [Output Structure](#output-structure)
6. [CRAB Grid Submission](#crab-grid-submission)
7. [Troubleshooting](#troubleshooting)
8. [Physics Notes](#physics-notes)

---

## Overview

This package provides a complete pipeline for **displaced vertex reconstruction** and **analysis** from CMS Run3 Scouting data:

```mermaid
graph LR
    A[Scouting Data] --> B[HLTScoutingUnpackProducer]
    B --> C[Vertexer]
    C --> D[ScoutingTreeMakerRun3]
    D --> E[ROOT Histograms]
```

### Pipeline Modules

1. **HLTScoutingUnpackProducer**: Unpacks scouting formats → `reco::Track`, `reco::Vertex`, `reco::PFCandidate`
2. **Vertexer**: Reconstructs displaced vertices from high-IP tracks using KalmanVertexFitter
3. **ScoutingTreeMakerRun3**: Creates analysis histograms with flexible ntk/angle cuts

---

## Quick Start

### Setup Environment

```bash
cd /afs/cern.ch/user/a/amalhotr/CMSSW_14_0_18_patch1/src
cmsenv
scram b -j8
```

### Local Interactive Test

```bash
cd Run3ScoutingAnalysisTools/
cmsRun UnifiedScoutingVertexingTreeMaker.py

# Output: ScoutingTree_Output.root (default)
```

### Quick Configuration Check

```bash
# Verify modules are loaded
edmPluginDump | grep -E "Vertexer|ScoutingTreeMakerRun3|HLTScoutingUnpackProducer"

# Check parameter descriptions
edmConfigDump -e UnifiedScoutingVertexingTreeMaker.py
```

---

## Module Descriptions

### 1. HLTScoutingUnpackProducer

**Purpose**: Convert packed Scouting data formats to standard CMSSW `reco::` objects.

**Key Features**:
- Unpacks `Run3ScoutingTrack` → `reco::Track`
- Unpacks `Run3ScoutingVertex` → `reco::Vertex`
- Builds HitPattern from ScoutingPFCandidate + ScoutingTrack
- Cross-links PFCandidates with matching Tracks

**Output Collections**:
```python
"hltScoutingUnpackProducer:Track"          # reco::TrackCollection
"hltScoutingUnpackProducer:PrimaryVertex"  # reco::VertexCollection
"hltScoutingUnpackProducer:PFCandidate"    # reco::PFCandidateCollection (optional)
```

**Important**: Set `isScouting = cms.bool(True)` to enable scouting mode!

---

### 2. Vertexer

**Purpose**: Reconstruct displaced vertices from high-impact-parameter tracks.

#### Algorithm Overview

1. **Seed Track Selection**:
   - `|IP|/σ(IP) > minSeedIPSig` (default: 4.0) wrt reference
   - `pT > minSeedPt` (default: 0.9 GeV)
   - No hit/layer requirements (avoids physics bias)

2. **Seed Vertex Formation**:
   - Fit all N-track combinations (N = `n_tracks_per_seed_vertex`, default: 2)
   - Keep vertices with `χ²/ndof < max_seed_vertex_chi2` (default: 5)

3. **Track Sharing Resolution**:
   - If vertices share tracks and are "close" → merge
   - If vertices share tracks but are "far" → arbitrate (keep track in closer vertex)

4. **Split-Vertex Merging** (optional):
   - **Loose**: Merge if `dBV < merge_anyway_dist` OR `dBV/σ < merge_anyway_sig`
   - **Tight**: Merge if `|Δφ| < 0.5`, `dBV_2D < 300 μm`, both `dBV > 100 μm`

5. **N-1 Track Removal** (optional):
   - Refit without each track; remove if vertex moves > `max_nm1_refit_dist3`

#### Key Parameters

```python
# Seed selection (PHYSICS-CRITICAL)
minSeedIPSig = cms.untracked.double(4.0)  # |IP|/σ wrt reference
minSeedPt    = cms.untracked.double(0.9)  # GeV

# Reference vertex choice
refPreference = cms.untracked.string("BS")  # "BS" or "PV"

# Distance calculations
use_2d_vertex_dist = cms.bool(False)  # False = 3D (default)
use_2d_track_dist  = cms.bool(True)   # True = 2D (default)

# Merging thresholds
merge_shared_sig = cms.double(4)      # Merge if dBV/σ < 4
max_track_vertex_sig = cms.double(5)  # Arbitrate if track_dist/σ > 5

# Split-vertex resolution
resolve_split_vertices_tight = cms.bool(True)   # Tight merging (recommended)
resolve_split_vertices_loose = cms.bool(False)  # Loose merging (off by default)
```

#### Reference Vertex Logic

```python
refPreference = "BS"  # or "PV"
```

- **`"BS"` (BeamSpot, default)**:
  - Primary: Use beam spot position
  - Fallback: Average good primary vertices
  - Best for: Early data, high-lumi runs

- **`"PV"` (Primary Vertex)**:
  - Primary: Average good PVs (ndof > 4, not fake)
  - Fallback: Beam spot
  - Best for: MC, stable beam conditions

**Covariance Matrix**: Diagonal-only (off-diagonals = 0)
- AvgPV: `σ_x = σ_y = 15 μm`, `σ_z = 50 μm` (fixed)
- BeamSpot: From `BeamSpot::covariance()` diagonal terms

---

### 3. ScoutingTreeMakerRun3

**Purpose**: Create analysis-ready ROOT histograms with flexible cuts.

#### Branch Structure

The analyzer creates **dynamic histogram branches** for each `ntk × opening_angle` combination:

````markdown

## Combining Multiple ROOT Files

### Option 1: Using ROOT's Built-in `hadd` (Recommended for Simple Cases)

```bash
# Merge files directly
hadd -f combined_output.root file1.root file2.root file3.root

# From file list
hadd -f combined_output.root @filelist.txt

# With progress bar
hadd -f -v 1 combined_output.root file*.root
```

**Advantages**: Fast, native ROOT tool, standard workflow  
**Limitations**: No custom processing, requires all files to have identical structure

### Option 2: Using `combineHistogramsFromFileList.c` (Advanced)

For complex merges or custom processing:

```cpp
// filepath: utils/combineHistogramsFromFileList.c
root -l
.L utils/combineHistogramsFromFileList.c
combineHistogramsFromFileList("filelist.txt", "merged_output.root")
```

**Advantages**:
- Automatic `TH1I`/`TH2I` → `TH1D`/`TH2D` conversion
- Progress tracking
- Works with any directory structure
- Can be customized in macro

**Use `hadd` for routine merges, use the C utility for special cases!** 🎯
````

