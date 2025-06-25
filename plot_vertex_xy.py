# filepath: /afs/cern.ch/user/a/amalhotr/CMSSW_14_0_18_patch1/src/Run3ScoutingAnalysisTools/plot_vertex_xy.py
import ROOT;

# Open the ROOT file
file = ROOT.TFile("treeWvertexXY.root");

# Get the histogram
h_scoutVert_x_y = file.Get("h_scoutVert_x_y");

# Draw the histogram
c1 = ROOT.TCanvas("c1", "Vertex x vs y", 800, 600);
h_scoutVert_x_y.Draw("COLZ");

# Save the canvas as an image
c1.SaveAs("vertex_xy.png");