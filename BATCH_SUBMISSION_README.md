# Batch Tree2Plots HTCondor Submission System

This system processes ROOT files containing `scoutingTree/vertexTree` in batches using HTCondor, with automatic disk space management.

## Files Created

1. **`Tree2PlotsConfig_batch.py`** - CMSSW config with command-line argument support
2. **`scripts/run_tree2plots.sh`** - Wrapper script that sets up CMSSW environment and runs cmsRun
3. **`condor/tree2plots_wave.submit`** - HTCondor submit template
4. **`condor/submit_batches.py`** - Python orchestration script that manages waves and cleanup

## How It Works

- Processes files in "waves" of N jobs (default: 5)
- Keeps at most M output files on disk (default: 10)
- Automatically deletes outputs from wave (n-2) before submitting wave n
- Waits for each wave to complete before submitting the next
- Handles EOS files via XRootD (works on batch nodes without /eos mount)

## Usage

### Basic Example (10 files, test run)

```bash
cd /afs/cern.ch/user/a/amalhotr/CMSSW_14_0_18_patch1/src/Run3ScoutingAnalysisTools

# Dry-run first to see what would happen
python3 condor/submit_batches.py \
  --filelist path-text-files/TreeMakerv1_test10.txt \
  --outdir /eos/user/a/amalhotr/Tree2PlotsOut \
  --batch-size 5 \
  --max-keep 10 \
  --jobflavour longlunch \
  --dry-run

# Real submission (remove --dry-run)
python3 condor/submit_batches.py \
  --filelist path-text-files/TreeMakerv1_test10.txt \
  --outdir /eos/user/a/amalhotr/Tree2PlotsOut \
  --batch-size 5 \
  --max-keep 10 \
  --jobflavour longlunch
```

### Full Production Run (all 922 files)

```bash
python3 condor/submit_batches.py \
  --filelist path-text-files/TreeMakerv1.txt \
  --outdir /eos/user/a/amalhotr/Tree2PlotsOut \
  --batch-size 5 \
  --max-keep 10 \
  --jobflavour longlunch
```

This will:
- Submit wave 1: 5 jobs → wait for completion
- Submit wave 2: 5 jobs → wait for completion  
- Submit wave 3: 5 jobs (delete wave 1 outputs first) → wait
- ...continue until all 922 files processed

## Arguments

- `--filelist` - Text file with one ROOT file path per line
- `--outdir` - Output directory (can be EOS path like `/eos/user/...`)
- `--batch-size` - Number of jobs per wave (default: 5)
- `--max-keep` - Maximum output files to keep on disk (default: 10)
- `--input-tree` - TTree path inside ROOT file (default: `scoutingTree/vertexTree`)
- `--jobflavour` - HTCondor job flavor: `espresso` (20min), `microcentury` (1h), `longlunch` (2h), `workday` (8h), `tomorrow` (1d), `testmatch` (3d), `nextweek` (1w)
- `--dry-run` - Show what would happen without submitting

## File List Format

Text file with one absolute path per line:
```
/eos/user/a/amalhotr/ScoutingPFRun3/.../ScoutingTree_Output_1.root
/eos/user/a/amalhotr/ScoutingPFRun3/.../ScoutingTree_Output_2.root
...
```

## Monitoring Jobs

```bash
# Check condor queue
condor_q

# Check specific wave logs
tail -f logs/wave_001.log
tail -f logs/wave_001.*.out
tail -f logs/wave_001.*.err

# Check all running jobs
watch -n 5 condor_q
```

## Output Structure

```
/eos/user/a/amalhotr/Tree2PlotsOut/
├── ScoutingTree_Output_1_plots.root
├── ScoutingTree_Output_2_plots.root
...
```

## Cleanup

The script automatically deletes old waves, but you can manually clean up:

```bash
# Remove all generated condor files
rm -rf condor/wave_*.submit condor/items/*.items condor/waves/*.list logs/*.log logs/*.out logs/*.err

# Remove test output
rm -rf path-text-files/TreeMakerv1_test10.txt
```

## Troubleshooting

### Jobs fail with "command not found"
- Check that `scripts/run_tree2plots.sh` is executable: `chmod +x scripts/run_tree2plots.sh`

### Jobs fail with "Tree not found"
- Verify tree path with: `rootls -l /eos/user/.../file.root`
- Check if tree is in subdirectory: should be `scoutingTree/vertexTree`
- Omit cycle numbers (`;1`, `;12`, etc.) - ROOT auto-selects latest

### Jobs held in queue
```bash
condor_q -hold
condor_release <job_id>
```

### Memory issues
Edit `condor/tree2plots_wave.submit`:
```
request_memory  = 4000  # Increase from 2000 MB
```

## Advanced: Custom Cuts

Edit `Tree2PlotsConfig_batch.py` to change analysis cuts before submitting:

```python
process.tree2plots = cms.EDAnalyzer('Tree2PlotsRun3',
    cut_ntk = cms.VPSet(
        cms.PSet(values = cms.vint32(3)),
        cms.PSet(values = cms.vint32(3, 4)),
    ),
    cut_opening_angle_min = cms.vdouble(-1, 0.05, 0.1, 0.25, 0.5, 1.0),
    required_invmass = cms.double(2.0),
    # ... modify as needed
)
```

## Notes

- The system handles ROOT cycle numbers automatically (vertexTree;1, vertexTree;12, etc.)
- XRootD protocol ensures batch nodes can read EOS files
- Wave-based submission prevents overwhelming the batch system
- Automatic cleanup keeps disk usage under control
