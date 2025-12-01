# Run3ScoutingAnalysisTools

Displaced vertex reconstruction and analysis for Run-3 Scouting data.

## File Structure

### C++ Analyzers (in `ScoutingTreeMakerRun3/plugins/`)
1. **ScoutingPlotMakerRun3.cc** - Direct histogram production (CMSSW)
2. **ScoutingTreeMakerRun3.cc** - TTree production for offline analysis (CMSSW)

### Python Configs
1. **UnifiedScoutingVertexingPlotsMaker.py** - Config for ScoutingPlotMakerRun3
2. **UnifiedScoutingVertexingTreeMaker.py** - Config for ScoutingTreeMakerRun3
3. **Tree2PlotsConfig.py** - Config for Tree2Plots (generates text file)

### Standalone Analyzers (in `ScoutingTreeMakerRun3/`)
1. **Tree2Plots.cc** - Reads TTrees and makes histograms (ROOT macro)

### CRAB Config
1. **chainedCrabConfig.py** - Grid job submission (switchable between plots/trees)

## Workflow Options

### Option A: Direct Histograms (Fast, Limited Flexibility)
```bash
# Run locally
cmsRun UnifiedScoutingVertexingPlotsMaker.py

# Or submit to grid
crab submit -c chainedCrabConfig.py
# (make sure it uses UnifiedScoutingVertexingPlotsMaker.py)
```
**Output:** `ScoutingPlots_Output.root` with histograms

### Option B: TTrees → Histograms (Slower, Maximum Flexibility)
```bash
# Step 1: Make TTrees (can submit to grid)
cmsRun UnifiedScoutingVertexingTreeMaker.py

# Step 2: Make histograms from TTrees (fast, local)
python Tree2PlotsConfig.py           # Generate config
python Tree2PlotsConfig.py run       # Auto-run Tree2Plots
```
**Outputs:** 
- `ScoutingTree_Output.root` (TTrees)
- `Tree2Plots_Output.root` (histograms)

## Key Differences

| Feature | PlotMaker | TreeMaker + Tree2Plots |
|---------|-----------|------------------------|
| Speed | ✓ Faster | Slower (2-step) |
| Flexibility | Limited | ✓ Full offline flexibility |
| Cut changes | Rerun CMSSW | Just rerun Tree2Plots |
| Merging | Hadd histograms | ✓ Hadd TTrees, then plot |
| File size | ~10-50 MB/job | ~100-500 MB/job |

## Quick Commands

```bash
# Compile
scram b -j 8

# Test locally (plots)
cmsRun UnifiedScoutingVertexingPlotsMaker.py

# Test locally (trees)
cmsRun UnifiedScoutingVertexingTreeMaker.py

# Convert trees to plots
python Tree2PlotsConfig.py run

# Submit to grid
crab submit -c chainedCrabConfig.py
```

## Output File Names

- **Plots:** `ScoutingPlots_Output.root`
- **Trees:** `ScoutingTree_Output.root`
- **Plots from Trees:** `Tree2Plots_Output.root`

All outputs have same histogram structure for easy comparison!
