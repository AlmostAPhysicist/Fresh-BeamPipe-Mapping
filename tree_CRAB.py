from CRABClient.UserUtilities import config
config = config()

# General section: Job metadata

theTag = 'DYto2Mu4Jets_ScoutingTree_100_NoCoreFresh_WithEta_HighRes'
config.General.requestName = theTag
config.General.workArea = 'crab_projects'
config.General.transferOutputs = True
config.General.transferLogs = True

# JobType section: Define the job executable
config.JobType.pluginName = 'Analysis'
config.JobType.psetName = 'tree.py'           # Your analyzer configuration file
config.JobType.maxMemoryMB = 2500             # Memory limit (adjust if needed)
# config.JobType.numCores = 1                   # Number of CPU cores

# Data section: Input dataset and splitting
config.Data.inputDataset = '/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/amalhotr-DYto2Mu4Jets_Vertexer_100_EventAwareLumiBased-8eb043b42a967140b9dd5cab43cfc767/USER'
config.Data.inputDBS = 'phys03'               # User-produced dataset in phys03 DBS
config.Data.splitting = 'FileBased' # Matches your dataset naming convention
config.Data.unitsPerJob = 5                    # Number of events per job
# config.Data.totalUnits = -1                   # Process all events
config.Data.outputDatasetTag = theTag

# config.Data.publication = True  # Publish output to DAS

# Site section: Storage location
config.Site.storageSite = 'T3_CH_CERNBOX'     # Store output at CERNBOX Tier-3

# Optional: Debugging or site preferences
# config.Site.whitelist = ['T2_*']            # Uncomment to restrict to Tier-2 sites