from CRABClient.UserUtilities import config
config = config()

theTag = "RealData_noLFNDirectory_All_EventAwareLumiBased_100_v1"  # Change this to your desired tag

config.General.requestName = theTag
config.General.transferOutputs = True
config.General.transferLogs = True

#increased max run time and increased memory
config.JobType.maxMemoryMB = 2500  # value in MB (max: 4000)
config.JobType.maxJobRuntimeMin = 2000  # value in minutes (max: 2750)



config.JobType.pluginName = 'Analysis'
config.JobType.psetName = 'UnifiedScoutingVertexingTreeMaker.py'  # <-- This file you created
# config.Data.inputDataset = '/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/RunIII2024Summer24MiniAOD-140X_mcRun3_2024_realistic_v26-v2/MINIAODSIM'
config.Data.inputDataset = '/ScoutingPFRun3/Run2024H-v1/HLTSCOUT'
config.Data.inputDBS = 'global'
config.Data.splitting = 'EventAwareLumiBased'
config.Data.unitsPerJob = 100
# config.Data.totalUnits = 2

# Info about /ScoutingPFRun3/Run2024H-v1/HLTSCOUT Dataset
#nlumis:16134
# num_lumi:16134
# max_ldate:1727773376
# num_file:13795
# median_ldate:1726761172
# size:91507190452904
# name:[]interface {}{"/ScoutingPFRun3/Run2024H-v1/HLTSCOUT"}
# nblocks:321
# nevents:7833545761
# nfiles:13795
# median_cdate:1726761172

# config.Data.splitting = 'LumiBased'
config.Data.unitsPerJob = 2  # Target 20,000 events per job
# config.Data.totalUnits = 1000000  # Total number of events to process
# config.Data.splitting = 'Automatic'
# FileBased
# config.Data.splitting = 'FileBased'
# config.Data.unitsPerJob = 1  # Number of files per job
# config.Data.totalUnits = 10000  # Total number of files to process


config.Data.outputDatasetTag = theTag
config.Data.publication = False  # Set to True if publishing to DAS

# Mandatory to allow non-valid dataset
config.Data.allowNonValidInputDataset = True

# Site settings
config.Site.storageSite = 'T3_CH_CERNBOX'
# config.Data.outLFNDirBase = '/store/user/amalhotr/DY2Mu4Jets_ScoutingTree/'
