import argparse

def read_file_lines(file_path, start_line, end_line):
    """
    Read lines from start_line to end_line (1-based indexing) from the file.
    Returns a list of file paths with 'root://cmsxrootd.fnal.gov/' prepended.
    """
    with open(file_path, 'r') as f:
        lines = f.readlines()
    
    # Convert to 0-based indexing and validate
    start_idx = start_line - 1
    end_idx = end_line
    if start_idx < 0 or end_idx > len(lines) or start_idx >= end_idx:
        raise ValueError(f"Invalid line range: {start_line} to {end_line}. File has {len(lines)} lines.")
    
    # Strip whitespace and prepend XRootD prefix
    selected_lines = [f"root://cmsxrootd.fnal.gov/{line.strip()}" for line in lines[start_idx:end_idx]]
    return selected_lines

def generate_crab_config(file_list, output_file, tag):
    """
    Generate a CRAB configuration file with the given file list and tag.
    """
    # Start building the configuration as a string
    theTag = f"'DYto2Mu4Jets_ScoutingTree_2025_{tag}'"
    config_content = [
        "from CRABClient.UserUtilities import config",
        "config = config()",
        "",
        "theTag = " + theTag,
        "",
        "# General section: Job metadata",
        f"config.General.requestName = theTag",
        "config.General.workArea = 'crab_projects'",
        "config.General.transferOutputs = True",
        "config.General.transferLogs = True",
        "",
        "# JobType section: Define the job executable",
        "config.JobType.pluginName = 'Analysis'",
        "config.JobType.psetName = 'UnifiedScoutingVertexingTreeMaker.py'  # Your analyzer configuration file",
        "config.JobType.maxMemoryMB = 2500  # Memory limit",
        "# config.JobType.numCores = 1  # Number of CPU cores (default is 1)",
        "",
        "# Data section: Input file paths and splitting",
        "config.Data.userInputFiles = ["
    ]

    # Add each file path with proper indentation and quoting
    for file_path in file_list:
        config_content.append(f"    '{file_path}',")
    config_content.append("]")

    # Continue with the rest of the configuration
    config_content.extend([
        "config.Data.outputPrimaryDataset = 'DYto2Mu4Jets_ScoutingTree'  # Primary dataset name for outputs",
        "config.Data.inputDBS = 'global'  # Required but not used for userInputFiles",
        "config.Data.splitting = 'FileBased'  # File-based splitting",
        "config.Data.unitsPerJob = 1  # One file per job (adjust as needed)",
        "config.Data.outLFNDirBase = '/store/user/amalhotr/DY2Mu4Jets_ScoutingTree/'  # Output directory",
        f"config.Data.outputDatasetTag = theTag  # Tag for the output dataset",
        "# config.Data.totalUnits = -1  # Process all files (default)",
        "",
        "# config.Data.publication = False  # No publication for user-specified files",
        "",
        "# Site section: Storage location",
        "config.Site.storageSite = 'T3_CH_CERNBOX'  # Store output at CERNBOX Tier-3",
        "",
        "# Optional: Debugging or site preferences",
        "# config.Site.whitelist = ['T2_*']  # Uncomment to restrict to Tier-2 sites"
    ])

    # Write the configuration to a file
    with open(output_file, 'w') as f:
        f.write("\n".join(config_content))

    print(f"CRAB configuration file generated: {output_file}")

def main():
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description="Generate CRAB config for a range of input files.")
    parser.add_argument('--input-file', default='DatasetFiles_DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8.txt',
                        help='Text file containing file paths (one per line)')
    parser.add_argument('--start-line', type=int, required=True,
                        help='Starting line number (1-based indexing)')
    parser.add_argument('--end-line', type=int, required=True,
                        help='Ending line number (1-based indexing)')
    parser.add_argument('--output-file', default='crabConfig_range.py',
                        help='Output CRAB configuration file name')
    args = parser.parse_args()

    # Read the specified range of lines
    file_list = read_file_lines(args.input_file, args.start_line, args.end_line)

    # Generate a unique tag based on the line range
    tag = f'lines_{args.start_line}_to_{args.end_line}'

    # Generate the CRAB configuration file
    generate_crab_config(file_list, args.output_file, tag)

if __name__ == "__main__":
    main()