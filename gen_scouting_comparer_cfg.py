import os
import FWCore.ParameterSet.Config as cms

process = cms.Process("SCOUTINGCOMPARE")

# -------------------- Choose sample key --------------------
# SAMPLE_KEY = os.environ.get("SAMPLE_KEY", "M800_CT10")

# SAMPLES = {
#     "M200_CT1": {
#         "files": [
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_miniAOD_v2/250214_150834/0000/stop_dbar_miniAOD_5.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_miniAOD_v2/250214_150834/0000/stop_dbar_miniAOD_3.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_miniAOD_v2/250214_150834/0000/stop_dbar_miniAOD_2.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_miniAOD_v2/250214_150834/0000/stop_dbar_miniAOD_6.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-1mm_Summer24_100k_miniAOD_v2/250214_150834/0000/stop_dbar_miniAOD_8.root",
#         ],
#         "masshint": 200.0,
#         "decayhint": 0.001,
#     },
#     "M200_CT3": {
#         "files": [
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_miniAOD_v2/250214_150918/0000/stop_dbar_miniAOD_10.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_miniAOD_v2/250214_150918/0000/stop_dbar_miniAOD_8.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_miniAOD_v2/250214_150918/0000/stop_dbar_miniAOD_7.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_miniAOD_v2/250214_150918/0000/stop_dbar_miniAOD_3.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-3mm_Summer24_100k_miniAOD_v2/250214_150918/0000/stop_dbar_miniAOD_4.root",
#         ],
#         "masshint": 200.0,
#         "decayhint": 0.003,
#     },
#     "M200_CT10": {
#         "files": [
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_miniAOD_v2/250214_144856/0000/stop_dbar_miniAOD_2.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_miniAOD_v2/250214_144856/0000/stop_dbar_miniAOD_8.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_miniAOD_v2/250214_144856/0000/stop_dbar_miniAOD_9.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_miniAOD_v2/250214_144856/0000/stop_dbar_miniAOD_1.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-200_CTau-10mm_Summer24_100k_miniAOD_v2/250214_144856/0000/stop_dbar_miniAOD_4.root",
#         ],
#         "masshint": 200.0,
#         "decayhint": 0.010,
#     },
#     "M400_CT1": {
#         "files": [
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151149/0000/stop_dbar_miniAOD_10.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151149/0000/stop_dbar_miniAOD_8.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151149/0000/stop_dbar_miniAOD_4.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151149/0000/stop_dbar_miniAOD_5.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151149/0000/stop_dbar_miniAOD_9.root",
#         ],
#         "masshint": 400.0,
#         "decayhint": 0.001,
#     },
#     "M400_CT3": {
#         "files": [
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151323/0000/stop_dbar_miniAOD_2.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151323/0000/stop_dbar_miniAOD_7.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151323/0000/stop_dbar_miniAOD_4.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151323/0000/stop_dbar_miniAOD_9.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151323/0000/stop_dbar_miniAOD_10.root",
#         ],
#         "masshint": 400.0,
#         "decayhint": 0.003,
#     },
#     "M400_CT10": {
#         "files": [
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151028/0000/stop_dbar_miniAOD_2.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151028/0000/stop_dbar_miniAOD_4.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151028/0000/stop_dbar_miniAOD_6.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151028/0000/stop_dbar_miniAOD_7.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-400_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-400_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151028/0000/stop_dbar_miniAOD_9.root",
#         ],
#         "masshint": 400.0,
#         "decayhint": 0.010,
#     },
#     "M600_CT1": {
#         "files": [
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151500/0000/stop_dbar_miniAOD_8.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151500/0000/stop_dbar_miniAOD_5.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151500/0000/stop_dbar_miniAOD_3.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151500/0000/stop_dbar_miniAOD_7.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151500/0000/stop_dbar_miniAOD_2.root",
#         ],
#         "masshint": 600.0,
#         "decayhint": 0.001,
#     },
#     "M600_CT3": {
#         "files": [
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151548/0000/stop_dbar_miniAOD_7.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151548/0000/stop_dbar_miniAOD_9.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151548/0000/stop_dbar_miniAOD_8.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151548/0000/stop_dbar_miniAOD_4.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151548/0000/stop_dbar_miniAOD_3.root",
#         ],
#         "masshint": 600.0,
#         "decayhint": 0.003,
#     },
#     "M600_CT10": {
#         "files": [
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151410/0000/stop_dbar_miniAOD_3.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151410/0000/stop_dbar_miniAOD_6.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151410/0000/stop_dbar_miniAOD_10.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151410/0000/stop_dbar_miniAOD_4.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-600_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-600_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151410/0000/stop_dbar_miniAOD_2.root",
#         ],
#         "masshint": 600.0,
#         "decayhint": 0.010,
#     },
#     "M800_CT1": {
#         "files": [
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151734/0000/stop_dbar_miniAOD_9.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151734/0000/stop_dbar_miniAOD_5.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151734/0000/stop_dbar_miniAOD_10.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151734/0000/stop_dbar_miniAOD_8.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-1mm_Summer24_100k_miniAOD_v2/250214_151734/0000/stop_dbar_miniAOD_2.root",
#         ],
#         "masshint": 800.0,
#         "decayhint": 0.001,
#     },
#     "M800_CT3": {
#         "files": [
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151828/0000/stop_dbar_miniAOD_4.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151828/0000/stop_dbar_miniAOD_2.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151828/0000/stop_dbar_miniAOD_3.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151828/0000/stop_dbar_miniAOD_1.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-3mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-3mm_Summer24_100k_miniAOD_v2/250214_151828/0000/stop_dbar_miniAOD_6.root",
#         ],
#         "masshint": 800.0,
#         "decayhint": 0.003,
#     },
#     "M800_CT10": {
#         "files": [
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151643/0000/stop_dbar_miniAOD_3.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151643/0000/stop_dbar_miniAOD_10.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151643/0000/stop_dbar_miniAOD_7.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151643/0000/stop_dbar_miniAOD_8.root",
#             "root://cms-xrd-global.cern.ch//store/user/brlopesd/StopStopbarTo2Dbar2D_M-800_CTau-10mm_Summer24_100k_v2/StopStopbarTo2Dbar2D_M-800_CTau-10mm_Summer24_100k_miniAOD_v2/250214_151643/0000/stop_dbar_miniAOD_6.root",
#         ],
#         "masshint": 800.0,
#         "decayhint": 0.010,
#     },
# }

# sample = SAMPLES[SAMPLE_KEY]
# ACTIVE_FILES = sample["files"]
# MASS_HINT = sample["masshint"] # GeV
# DECAY_HINT = 100 * sample["decayhint"] # sample has units of meters (SI). Decay hint is in cm (Geant4).


SAMPLE_KEY = os.environ.get("SAMPLE_KEY", "H_MS30_CT0p1")

# All decay hints below are stored in meters (SI).
# 0p1 mm = 0.0001 m, 1 mm = 0.001 m, 10 mm = 0.01 m.
# The analyzer converts to cm where needed via DECAY_HINT_CM = 100 * DECAY_HINT_M.

SAMPLES = {
    "H_MS30_CT0p1": {
        "display_name": "GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-30",
        "files": [
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/8c69c404-096f-4320-9e45-14c36a83c30c.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/06967f31-a721-407e-a80e-9bd5cb99a7e8.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/c55218b2-b07e-4ede-a0b9-e09dd40fc23a.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/5b6d5816-2d30-4995-be88-c0c464c7cfa0.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/a8699972-6afb-4d64-b374-b98db3ee4e42.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/ccd7f6b3-a43e-4dbb-9e5f-70c690f1d444.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/6bc5739c-2785-43e0-9c64-91c356ddb068.root",
        ],
        "masshint": 30.0,
        "decayhint": 0.0001,
    },
    "H_MS30_CT1": {
        "display_name": "GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-30",
        "files": [
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/130000/b249ac21-9cf0-4f63-8351-affba49b2d27.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/130000/ced3f354-5d58-4a76-9cda-918d89904881.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/130000/138c7dd5-fe3f-42aa-be77-58c4d44ce69d.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/130000/3f6bbcd2-3a79-4b32-bce6-965fdeefb232.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/130000/96decbcf-0b40-4773-86f2-e636cb3fb6bc.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/130000/bc1f20e7-c342-421e-9e12-61065dc84aaf.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/130000/adbfafc7-eeb7-4caf-a5a1-293fc70b1c5c.root",
        ],
        "masshint": 30.0,
        "decayhint": 0.001,
    },
    "H_MS30_CT10": {
        "display_name": "GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-30",
        "files": [
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/ac19333f-54bd-41d2-9221-303417caa432.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/c4f09dbf-6efa-416b-bec2-72c95e5abfd9.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/a687d649-7d56-48e6-88e9-247a06ce538b.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/130000/c18be0ea-8a70-4094-92c1-96ad127644c6.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/130000/fbab8e6d-3e4c-4a66-a70b-f8b99cf20cf9.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/130000/cdfb1570-764e-4a6b-aaf1-aa5ce15f35a1.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-30_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/e66bfbea-85f8-4a1f-90b3-795803252f95.root",
        ],
        "masshint": 30.0,
        "decayhint": 0.01,
    },

    "H_MS40_CT0p1": {
        "display_name": "GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-40",
        "files": [
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/140000/40cd250c-a3a4-4308-b2b6-3b31f7830358.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/140000/7dd75ce8-1347-4be0-911a-a2164275182c.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/140000/eb89ec36-5d73-4fdf-a627-05e320eb282f.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/140000/8ca7b156-95ff-4bae-9e3f-485668e95738.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/140000/8c238b6c-7b0b-451e-839d-cd50e33cd2d5.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/140000/f48b456f-07eb-49ca-b90d-d90ec23027c1.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/140000/f4930780-397f-4584-a713-6c57db3e40df.root",
        ],
        "masshint": 40.0,
        "decayhint": 0.0001,
    },
    "H_MS40_CT1": {
        "display_name": "GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-40",
        "files": [
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/f8bfbf37-a7f9-44a0-8899-1fbff7b2ae24.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/f9db1999-aa5f-479d-a477-cddaccffe776.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/e7657e73-31ad-4551-a1c8-277685ba5a21.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/110000/36c57160-c568-4661-8850-bdfb52e6295f.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/110000/b7bce6de-9e28-49f2-8a80-436f6bf66886.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/110000/451d4e09-37df-4d44-8cbd-3a971185d063.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/110000/bb89beca-cbda-43da-be78-07aa2156aba4.root",
        ],
        "masshint": 40.0,
        "decayhint": 0.001,
    },
    "H_MS40_CT10": {
        "display_name": "GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-40",
        "files": [
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2820000/3d8a6b66-a7f2-4115-b1c1-dcaf640c50a1.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2820000/94c90fe7-3025-4a51-aa73-fabb7c27be2f.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2820000/fcdeb004-fbc9-40e7-8efc-e9d623fca687.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/130000/b616d170-a193-4589-8ff2-eed3b302f15e.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2820000/ce20ee10-09c1-400e-8676-7ae32c39b070.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/130000/5ef28433-4b52-43c6-b9a5-3d80923c904e.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-40_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2820000/8dc47d0a-6f82-4773-8bbb-ef1bd0313911.root",
        ],
        "masshint": 40.0,
        "decayhint": 0.01,
    },

    "H_MS55_CT0p1": {
        "display_name": "GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-55",
        "files": [
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/0cb7c520-c385-445a-a620-2499437d8200.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/110063bb-019f-4aaa-a685-6eb6f95140c7.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/4b489a86-42f9-4243-ae8e-9c1aa683b459.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/68499237-ea35-44d5-8c71-0e34de9a7ba0.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/ec31d504-4a82-4c1c-9431-f58220d32610.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/00a05bd2-9dda-4561-9299-1656af6e358b.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-0p1-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/2e235f07-aed4-45af-a690-3c16248d3ba9.root",
        ],
        "masshint": 55.0,
        "decayhint": 0.0001,
    },
    "H_MS55_CT1": {
        "display_name": "GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-55",
        "files": [
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/d81615ab-42c5-425d-9fc9-81aec3a984a1.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/4c485222-7278-40be-9d40-c30f7a548e08.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/9ff16381-a389-484e-b5ff-49a00b97f9c7.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/0e495b1e-675c-4044-9a2d-eda96d70440e.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/7043e792-e639-4774-8994-c834cbf3ce91.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/70c8f8f0-6e29-479b-a728-fe0afa650a5b.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-1-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2810000/67eb7b29-f660-47bb-842e-40f8461adc5b.root",
        ],
        "masshint": 55.0,
        "decayhint": 0.001,
    },
    "H_MS55_CT10": {
        "display_name": "GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-55",
        "files": [
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2820000/819118e7-0a80-4e33-a724-15ed2ee3f4c3.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2820000/7dbe5bc0-71cd-4910-801e-f144d293b805.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2820000/1ce3db5d-cb11-4b77-8b1d-f09e8ee7f115.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2820000/40af300d-a570-4ba1-8af6-5337b539798e.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2820000/2b971a06-efd7-4f53-9015-b2093525dd2d.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/2820000/a52dd5cb-d250-4448-b797-06bcca11056b.root",
            "root://cms-xrd-global.cern.ch//store/mc/RunIII2024Summer24MiniAOD/GluGluH-Hto2Sto4D_Par-ctauS-10-MH-125-MS-55_TuneCP5_13p6TeV_powheg-pythia8/MINIAODSIM/140X_mcRun3_2024_realistic_v26-v2/130000/3420bb12-6048-4324-a11a-4e2735f24f3a.root",
        ],
        "masshint": 55.0,
        "decayhint": 0.01,
    },
}

sample = SAMPLES[SAMPLE_KEY]
ACTIVE_FILES = sample["files"]
MASS_HINT = sample["masshint"]  # GeV
DECAY_HINT_M = sample["decayhint"]  # meters
DECAY_HINT = 5 * 100.0 * DECAY_HINT_M  # cm (Geant4), 5 times scaling factor for higgs to scalar

print("="*80)
print("SAMPLE_KEY =", SAMPLE_KEY)
print("MASS_HINT =", MASS_HINT)
print("DECAY_HINT =", DECAY_HINT)
print("NFILES =", len(ACTIVE_FILES))
print("="*80)

# -------------------- Message Logger --------------------
process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = 1000

# -------------------- Processing Options --------------------
process.options = cms.untracked.PSet(
    wantSummary = cms.untracked.bool(True),
    numberOfThreads = cms.untracked.uint32(8),
    numberOfStreams = cms.untracked.uint32(0),
)

# -------------------- Histogram Output Target --------------------
process.TFileService = cms.Service(
    "TFileService",
    # fileName=cms.string(f"GenScoutCompare_Stop_g2l5Tracks_{SAMPLE_KEY}.root")
    fileName = cms.string(f"GenScoutCompare_H2SGlu_g2Tracks_NoJets_{SAMPLE_KEY}.root"),
)

# -------------------- Event Limits --------------------
process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(-1))

# -------------------- Input Source Pool --------------------
process.source = cms.Source(
    "PoolSource",
    fileNames = cms.untracked.vstring(*ACTIVE_FILES),
)

# -------------------- Scouting Unpacker Setup --------------------
process.hltScoutingUnpackProducer = cms.EDProducer(
    "HLTScoutingUnpackProducer",
    scoutingTrack = cms.InputTag("hltScoutingTrackPacker"),
    scoutingPrimaryVertex = cms.InputTag("hltScoutingPrimaryVertexPacker", "primaryVtx"),
    pfCand = cms.InputTag(""),
    lostTrack = cms.InputTag(""),
    isScouting = cms.bool(True),
    producePFCHSCandidate = cms.bool(False),
)

# Standard Geometry, Field and Global Tag Records
process.load("Configuration.StandardSequences.GeometryRecoDB_cff")
process.load("Configuration.StandardSequences.MagneticField_cff")
process.load("TrackingTools.TransientTrack.TransientTrackBuilder_cfi")
process.load("RecoVertex.BeamSpotProducer.BeamSpot_cfi")
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")

from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, "140X_mcRun3_2024_realistic_v26", "")

# -------------------- Vertexer Configuration --------------------
process.Vertexer = cms.EDProducer(
    "Vertexer",
    seed_tracks_src = cms.InputTag("hltScoutingUnpackProducer", "Track"),
    seed_jets_src = cms.InputTag("hltScoutingPFPacker"),
    primaryVertices = cms.InputTag("hltScoutingUnpackProducer", "PrimaryVertex"),
    trackToScoutingMap = cms.InputTag("hltScoutingUnpackProducer", "Track-RefToOriginal"),
    beamspot_src = cms.InputTag("offlineBeamSpot"),

    useOnlineBeamSpot = cms.untracked.bool(True),
    logBeamspotSource = cms.untracked.bool(False),

    minSeedPt = cms.untracked.double(0.9),
    minSeedIPSig = cms.untracked.double(4.0),
    maxSeedEta = cms.untracked.double(2.4),
    minSeedPixelHits = cms.int32(2),
    minSeedStripHits = cms.int32(1),
    minSeedTrackerLayers = cms.int32(5),

    jet_pt_min = cms.double(-1),
    jet_eta_max = cms.double(1000),
    min_selected_jets = cms.int32(-1),
    jet_dr_max = cms.untracked.double(1000),
    # jet_pt_min = cms.double(20.0),
    # jet_eta_max = cms.double(5.0),
    # min_selected_jets = cms.int32(3),
    # jet_dr_max = cms.untracked.double(0.4),

    n_tracks_per_seed_vertex = cms.int32(2),
    max_seed_vertex_chi2 = cms.double(5.0),
    use_2d_vv_dist_forReco = cms.bool(False),
    use_2d_tv_dist_bsIPpresel = cms.bool(True),
    use_2d_tv_dist_forReco = cms.bool(False),

    merge_shared_dist = cms.double(-1.0),
    merge_shared_sig = cms.double(3.0),
    max_track_vertex_dist = cms.double(-1.0),
    max_track_vertex_sig = cms.double(5.0),
    min_track_vertex_sig_to_remove = cms.double(2.0),
    remove_one_track_at_a_time = cms.bool(True),

    max_nm1_refit_dist3 = cms.double(0.003),
    max_nm1_refit_distz = cms.double(-1.0),

    resolve_split_vertices_loose = cms.bool(False),
    merge_anyway_dist = cms.double(0.05),
    merge_anyway_sig = cms.double(3.0),

    resolve_split_vertices_tight = cms.bool(True),
    split_merge_dbv_min = cms.double(0.01),
    split_merge_dvv2d_max = cms.double(0.05),
    split_merge_dphi_max = cms.double(0.5),
    verbose = cms.bool(False),
)

# -------------------- Custom Analyzer Module --------------------
process.scoutingComparer = cms.EDAnalyzer(
    "GenScoutingComparer",
    genParticles = cms.InputTag("prunedGenParticles"),
    scoutingVertices = cms.InputTag("Vertexer"),
    beamspot = cms.InputTag("offlineBeamSpot"),
    scoutingJets = cms.InputTag("hltScoutingPFPacker"),
    trackToScoutingMap = cms.InputTag("hltScoutingUnpackProducer", "Track-RefToOriginal"),
    
    # Enforce global beamspot matching with Vertexer setup
    useOnlineBeamSpot = cms.untracked.bool(True),

    masshint = cms.untracked.double(MASS_HINT),
    decayhint = cms.untracked.double(DECAY_HINT),

    # Matching Configuration
    match_distance_mode = cms.untracked.string("2D"),

    # Signal Truth Decay Configuration
    # parent_pdgids = cms.untracked.vint32(1000006, -1000006), #STOP
    parent_pdgids = cms.untracked.vint32(9000006, -9000006), #Scalar
    daughter_pdgids = cms.untracked.vint32(1, -1),

    # Offline Signal Region Definition Vertex Cuts
    vtx_chi2_max = cms.double(2.5),
    vtx_dbv_min = cms.double(0.01),
    vtx_dbv_max = cms.double(2.0),
    vtx_tracks_min = cms.uint32(2),
    vtx_tracks_max = cms.uint32(100),
    vtx_ddbv_max = cms.double(0.005),
    vtx_cosT_min = cms.double(0.0),

    # Analysis Event Level Jet Gate Configuration
    jet_pt_min = cms.double(-1),
    jet_eta_max = cms.double(1000),
    min_selected_jets = cms.uint32(-1),
    require_jet_selection = cms.untracked.bool(False),

    # jet_pt_min = cms.double(30),
    # jet_eta_max = cms.double(2.5),
    # min_selected_jets = cms.uint32(3),
    # require_jet_selection = cms.untracked.bool(True),

    verbose = cms.untracked.bool(False),
    verbose_unselected = cms.untracked.bool(False),
)

# -------------------- Execution Path --------------------
process.p = cms.Path(
    process.hltScoutingUnpackProducer +
    process.offlineBeamSpot +
    process.Vertexer +
    process.scoutingComparer
)