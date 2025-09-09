from CRABClient.UserUtilities import config

config = config()

TAG = 'DY2Mu4Jets_Unpacker_10_events_test_Invalid_v6'

# General settings
config.General.requestName = TAG
config.General.workArea = 'crab_projects'
config.General.transferOutputs = True
config.General.transferLogs = True

# Job type
config.JobType.pluginName = 'Analysis'
config.JobType.psetName = 'TrackProducer.py'  # Required, but overridden by scriptExe
config.JobType.scriptExe = 'run_chain.sh'
config.JobType.inputFiles = ['TrackProducer.py', 'VertexProducer.py', 'TREE.py', 'run_chain.sh', 'FrameworkJobReport.xml']
config.JobType.outputFiles = ['DY2M_ScoutingTree_Output.root', 'FrameworkJobReport.xml']
config.JobType.disableAutomaticOutputCollection = True
config.JobType.maxMemoryMB = 2500
config.JobType.numCores = 1

# Data settings
config.Data.inputDataset = '/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/RunIII2024Summer24MiniAOD-140X_mcRun3_2024_realistic_v26-v2/MINIAODSIM'
config.Data.inputDBS = 'global'
config.Data.splitting = 'FileBased'
config.Data.unitsPerJob = 1  # One file per job for testing
config.Data.totalUnits = 1   # Process one file for testing (~10 events)
config.Data.publication = False  # Set to True if publishing to DAS
config.Data.outputDatasetTag = TAG

# For nonvalid DataSet
config.Data.allowNonValidInputDataset = True

# Site settings
config.Site.storageSite = 'T3_CH_CERNBOX'
config.Data.outLFNDirBase = '/store/user/amalhotr/DY2Mu4Jets_ScoutingTree/'
