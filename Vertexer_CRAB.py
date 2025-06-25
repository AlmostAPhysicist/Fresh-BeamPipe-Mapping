# # # from CRABClient.UserUtilities import config
# # # config = config()

# # # mass = "M-200"
# # # lifetime = "CTau-10mm"

# # # theTag = "StopStopbarTo2Dbar2D_"+mass+"_"+lifetime+ "_Summer24_WithVertex_v1"
# # # config.General.requestName = theTag

# # # config.JobType.pluginName = 'Analysis'
# # # # Name of the CMSSW configuration file
# # # config.JobType.psetName = 'Vertexer.py'
# # # config.JobType.maxMemoryMB = 5000

# # # config.Data.inputDBS = 'phys03'
# # # config.Data.inputDataset = '/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_v1/brlopesd-StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_InputToVertexer_v1-5aba6c30b99a0b6779fca57b7c186350/USER'
# # # config.Data.splitting = 'Automatic'
# # # #config.Data.unitsPerJob = 1
# # # config.Data.publication = True
# # # # This string is used to construct the output dataset name
# # # config.Data.outputDatasetTag = theTag

# # # # These values only make sense for processing data
# # # #    Select input data based on a lumi mask
# # # #config.Data.lumiMask = 'Cert_190456-208686_8TeV_PromptReco_Collisions12_JSON.txt'
# # # #    Select input data based on run-ranges
# # # #config.Data.runRange = '190456-194076'

# # # # Where the output files will be transmitted to
# # # config.Site.storageSite = 'T2_BR_SPRACE'


# # # New Crab Config
# # from CRABClient.UserUtilities import config
# # config = config()

# # # Unique job name
# # config.General.requestName = 'DYto2Mu4Jets_Vertexer_Mar2025_4'
# # config.General.workArea = 'crab_projects'
# # config.General.transferOutputs = True
# # config.General.transferLogs = True

# # # Job type: Run Vertexer.py
# # config.JobType.pluginName = 'Analysis'
# # config.JobType.psetName = 'Vertexer.py'  # The CMSSW script to execute
# # config.JobType.maxMemoryMB = 2500

# # # Single file input
# # config.Data.inputDBS = 'global'
# # config.Data.userInputFiles = [
# #     '/store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2560000/ad749229-3094-410f-94e3-2e408518e395.root'
# # ]
# # config.Data.splitting = 'FileBased'
# # config.Data.unitsPerJob = 1

# # # Output settings
# # config.Data.publication = False
# # config.Data.outputDatasetTag = 'DYto2Mu4Jets_Vertexer_Mar20252'

# # # Storage site (updated to your working site)
# # config.Site.storageSite = 'T2_BR_SPRACE'


# #--------------------------------------------------------------------------------

# from CRABClient.UserUtilities import config
# config = config()

# # mass = "M-92"
# # lifetime = "CTau-0mm"

# # theTag = "StopStopbarTo2Dbar2D_"+mass+"_"+lifetime+ "_Summer24_WithVertex_v1"
# theTag = "DYto2Mu4Jets_Vertexer_Mar2025_OfflineReconstructed"
# config.General.requestName = theTag

# config.JobType.pluginName = 'Analysis'
# # Name of the CMSSW configuration file
# config.JobType.psetName = 'Vertexer.py'
# config.JobType.maxMemoryMB = 5000

# config.Data.inputDBS = 'phys03'
# config.Data.inputDataset = '/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_v1/brlopesd-StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_InputToVertexer_v1-5aba6c30b99a0b6779fca57b7c186350/USER'
# config.Data.splitting = 'Automatic'
# #config.Data.unitsPerJob = 1
# config.Data.publication = True
# # This string is used to construct the output dataset name
# config.Data.outputDatasetTag = theTag

# # These values only make sense for processing data
# #    Select input data based on a lumi mask
# #config.Data.lumiMask = 'Cert_190456-208686_8TeV_PromptReco_Collisions12_JSON.txt'
# #    Select input data based on run-ranges
# #config.Data.runRange = '190456-194076'

# # Where the output files will be transmitted to
# config.Site.storageSite = 'T3_CH_CERNBOX'


from CRABClient.UserUtilities import config
config = config()

# General settings
# mass = "M-91"  # Z boson mass for DY process
# lifetime = "CTau-0mm"  # Prompt decay for standard DY
# theTag = f"DYto2Mu4Jets_{mass}_{lifetime}_Summer24_Vertexer_v1"
theTag = "DYto2Mu4Jets_Vertexer_100_EventAwareLumiBased_testMiniDao"
config.General.requestName = theTag  # Unique name for your job
config.General.workArea = 'crab_projects'
config.General.transferOutputs = True  # Transfer output files to storage
config.General.transferLogs = True    # Transfer log files

# Job type settings
config.JobType.pluginName = 'Analysis'  # For processing MINIAODSIM
config.JobType.psetName = 'Vertexer.py'  # Your CMSSW config file
config.JobType.maxMemoryMB = 2500      # Increased memory for vertexing

# # Data settings
# config.Data.inputDataset = '/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/RunIII2024Summer24MiniAOD-140X_mcRun3_2024_realistic_v26-v2/MINIAODSIM'  # Single file specified in Vertexer.py, not a dataset
# config.Data.splitting = 'FileBased'  # One job per file
# config.Data.unitsPerJob = 1  # One file per job
# # config.Data.totalUnits = 1   # Only one file

# Use the output dataset as input
# config.Data.inputDataset = '/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/amalhotr-DYto2Mu4Jets_Unpacker_100-49feb6938084587a06052f9676570a55/USER'
# config.Data.inputDBS = 'phys03'  # Required for USER datasets

#TESTING OUR ORIGINAL DATASET
config.Data.inputDBS = 'global'  # Required for global datasets
config.Data.inputDataset = '/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/RunIII2024Summer24MiniAOD-140X_mcRun3_2024_realistic_v26-v2/MINIAODSIM'  # Single file specified in Vertexer.py, not a dataset


# config.Data.splitting = 'EventBased' #Instead of FileBased
# config.Data.unitsPerJob = 10000  # 10000 events per job instead of 1 file per job
# config.Data.splitting = 'Automatic'  # Let CRAB decide the number of events per job
config.Data.splitting = 'EventAwareLumiBased'
# config.Data.unitsPerJob = 20000  # Target 20,000 events per job
config.Data.unitsPerJob = 5  # Target 10,000 events per job
config.Data.totalUnits = 10  # Total number of events to process

config.Data.outputDatasetTag = theTag  # Output tag matching requestName
config.Data.publication = True  # Publish output to DAS

# Site settings
config.Site.storageSite = 'T2_BR_SPRACE'  # Your storage site (adjust if needed)

# Optional: Whitelist sites if the file is only available at specific locations
# config.Site.whitelist = ['T2_US_FNAL']  # Uncomment if jobs fail due to file access

#https://twiki.cern.ch/twiki/bin/view/CMSPublic/CRAB3AdvancedTutorial