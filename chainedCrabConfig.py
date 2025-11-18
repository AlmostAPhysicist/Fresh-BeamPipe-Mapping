from CRABClient.UserUtilities import config
config = config()

theTag = "ScoutingData_2024G"  # updated tag
# theTag = "ScoutingData_Updated_10mod15_v1"  # updated tag

config.General.requestName = theTag
config.General.transferOutputs = True
config.General.transferLogs = True

# increased max run time and increased memory
config.JobType.maxMemoryMB = 3000  # value in MB (max: 3000)
# set to CRAB maximum since 2 files/job may be large for this dataset
# config.JobType.maxJobRuntimeMin = 2750  # value in minutes (max: 2750)

config.JobType.pluginName = 'Analysis'
config.JobType.psetName = 'UnifiedScoutingVertexingTreeMaker.py'
# config.JobType.numCores = 4


# dasgoclient --query="dataset=/DYto2Mu-4Jets*/RunIII*/MINIAODSIM"
# /DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/RunIII2024Summer24MiniAOD-140X_mcRun3_2024_realistic_v26-v3/MINIAODSIM
# /DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/RunIII2024Summer24MiniAOD-140X_mcRun3_2024_realistic_v26_ext1-v2/MINIAODSIM
# /DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/RunIII2024Summer24MiniAODv6-150X_mcRun3_2024_realistic_v2-v3/MINIAODSIM
# config.Data.inputDataset = '/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/RunIII2024Summer24MiniAOD-140X_mcRun3_2024_realistic_v26-v2/MINIAODSIM'



# config.Data.inputDataset = '/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/RunIII2024Summer24MiniAOD-140X_mcRun3_2024_realistic_v26-v3/MINIAODSIM'
# config.Data.inputDataset = '/ScoutingPFRun3/Run2024H-v1/HLTSCOUT'# 91.5TB
config.Data.inputDataset = '/ScoutingPFRun3/Run2024G-v1/HLTSCOUT' #673.1TB
config.Data.inputDBS = 'global'

# File-based splitting: 2 files per job (deterministic). 
# WARNING: check per-job runtime after a small test submission — may require unitsPerJob=1.

# config.Data.splitting = 'FileBased'
# config.Data.unitsPerJob = 30  # Number of files per job
# config.Data.totalUnits = 1000

config.Data.splitting = 'Automatic'

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
# config.Data.unitsPerJob = 2  # Target 20,000 events per job
# config.Data.totalUnits = 1000000  # Total number of events to process
# config.Data.splitting = 'Automatic'
# FileBased
# config.Data.splitting = 'FileBased'
# config.Data.unitsPerJob = 1  # Number of files per job
# config.Data.totalUnits = 10000  # Total number of files to process


config.JobType.outputFiles = ["ScoutingTree_Output.root"]


config.Data.outputDatasetTag = theTag
config.Data.publication = False  # Set to True if publishing to DAS

# Mandatory to allow non-valid dataset
config.Data.allowNonValidInputDataset = True

# Site settings
config.Site.storageSite = 'T3_CH_CERNBOX'
# config.Data.outLFNDirBase = '/store/user/amalhotr/DY2Mu4Jets_ScoutingTree/'










#--------------------------------------
# [amalhotr@lxplus960 Run3ScoutingAnalysisTools]$ crab status -d ./crab_Data_ScoutingPFRun3_FullPV_1mod20_v1
# Rucio client intialized for account amalhotr
# CRAB project directory:         /afs/cern.ch/user/a/amalhotr/CMSSW_14_0_18_patch1/src/Run3ScoutingAnalysisTools/crab_Data_ScoutingPFRun3_FullPV_1mod20_v1
# Task name:                      251009_014918:amalhotr_crab_Data_ScoutingPFRun3_FullPV_1mod20_v1
# Grid scheduler - Task Worker:   crab3@vocms0144.cern.ch - crab-prod-tw01
# Status on the CRAB server:      SUBMITTED
# Task URL to use for HELP:       https://cmsweb.cern.ch/crabserver/ui/task/251009_014918%3Aamalhotr_crab_Data_ScoutingPFRun3_FullPV_1mod20_v1
# Dashboard monitoring URL:       https://monit-grafana.cern.ch/d/cmsTMDetail/cms-task-monitoring-task-view?orgId=11&var-user=amalhotr&var-task=251009_014918%3Aamalhotr_crab_Data_ScoutingPFRun3_FullPV_1mod20_v1&from=1759970958000&to=now
# Warning:                        Task requests 3000 MB of memory, but only 2500 are guaranteed to be available. Jobs may not find a site where to run and stay idle forever.
# Status on the scheduler:        SUBMITTED

# Jobs status:                    idle                     14.6% (1010/6898)
#                                 running                   0.5% (  32/6898)
#                                 unsubmitted              84.9% (5856/6898)

# No publication information (publication has been disabled in the CRAB configuration file)
# Log file is /afs/cern.ch/user/a/amalhotr/CMSSW_14_0_18_patch1/src/Run3ScoutingAnalysisTools/crab_Data_ScoutingPFRun3_FullPV_1mod20_v1/crab.log
# [amalhotr@lxplus960 Run3ScoutingAnalysisTools]$ crab status -d ./crab_MC_DYto2Mu_FullPV_1mod20_v1
# Rucio client intialized for account amalhotr
# CRAB project directory:         /afs/cern.ch/user/a/amalhotr/CMSSW_14_0_18_patch1/src/Run3ScoutingAnalysisTools/crab_MC_DYto2Mu_FullPV_1mod20_v1
# Task name:                      251009_020032:amalhotr_crab_MC_DYto2Mu_FullPV_1mod20_v1
# Grid scheduler - Task Worker:   crab3@vocms0194.cern.ch - crab-prod-tw01
# Status on the CRAB server:      SUBMITTED
# Task URL to use for HELP:       https://cmsweb.cern.ch/crabserver/ui/task/251009_020032%3Aamalhotr_crab_MC_DYto2Mu_FullPV_1mod20_v1
# Dashboard monitoring URL:       https://monit-grafana.cern.ch/d/cmsTMDetail/cms-task-monitoring-task-view?orgId=11&var-user=amalhotr&var-task=251009_020032%3Aamalhotr_crab_MC_DYto2Mu_FullPV_1mod20_v1&from=1759971632000&to=now
# Warning:                        Task requests 3000 MB of memory, but only 2500 are guaranteed to be available. Jobs may not find a site where to run and stay idle forever.
# Status on the scheduler:        SUBMITTED

# Jobs status:                    running                  10.0% ( 10/100)
#                                 unsubmitted              90.0% ( 90/100)

# No publication information (publication has been disabled in the CRAB configuration file)
# Log file is /afs/cern.ch/user/a/amalhotr/CMSSW_14_0_18_patch1/src/Run3ScoutingAnalysisTools/crab_MC_DYto2Mu_FullPV_1mod20_v1/crab.log
# [amalhotr@lxplus960 Run3ScoutingAnalysisTools]$ 