from CRABClient.UserUtilities import config
config = config()

# mass = "M-800"
# lifetime = "CTau-1mm"

# theTag = "StopStopbarTo2Dbar2D_"+mass+"_"+lifetime+ "_Summer24_InputToVertexer_v1"
theTag = "DYto2Mu4Jets_Unpacker_test2"
config.General.requestName = theTag

config.General.transferOutputs = True  # Transfer output files to storage
config.General.transferLogs = True    # Transfer log files

config.JobType.pluginName = 'Analysis'
# Name of the CMSSW configuration file
config.JobType.psetName = 'HLTScoutingUnpackProducer.py'
#config.JobType.maxMemoryMB = 5000

# config.Data.inputDBS = 'phys03'
# config.Data.inputDataset = '/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_v1/brlopesd-StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_v1-e5438bb52f42c4163dc01d5bb2e896e9/USER'
# config.Data.splitting = 'Automatic'
# #config.Data.unitsPerJob = 1
# config.Data.publication = True
# # This string is used to construct the output dataset name
# config.Data.outputDatasetTag = theTag
# Data settings
config.Data.inputDataset = '/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/RunIII2024Summer24MiniAOD-140X_mcRun3_2024_realistic_v26-v2/MINIAODSIM'  # Single file specified in Vertexer.py, not a dataset
# config.Data.userInputFiles = [
#     'root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24MiniAOD/DYto2Mu-4Jets_Bin-MLL-50_TuneCP5_13p6TeV_madgraphMLM-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2560000/ad749229-3094-410f-94e3-2e408518e395.root'
# ]
config.Data.splitting = 'FileBased'  # One job per file
config.Data.unitsPerJob = 1  # One file per job
config.Data.totalUnits = 1   # Only one file
# config.Data.outputPrimaryDataset = theTag  # Required when using private files
config.Data.outputDatasetTag = theTag  # Output tag matching requestName
config.Data.publication = True  # Publish output to DAS


# These values only make sense for processing data
#    Select input data based on a lumi mask
#config.Data.lumiMask = 'Cert_190456-208686_8TeV_PromptReco_Collisions12_JSON.txt'
#    Select input data based on run-ranges
#config.Data.runRange = '190456-194076'

# Where the output files will be transmitted to
# config.Site.storageSite = 'T3_CH_CERNBOX'
config.Site.storageSite = 'T2_BR_SPRACE'
