void checkBranches(const char* filePath) {
    // Open the ROOT file
    TFile *file = TFile::Open(filePath);
    if (!file || file->IsZombie()) {
        std::cerr << "Error: Could not open file " << filePath << std::endl;
        return;
    }

    // Navigate to the directory and tree
    TDirectoryFile *dir = (TDirectoryFile*)file->Get("scoutingTree");
    if (!dir) {
        std::cerr << "Error: Could not find 'scoutingTree' in " << filePath << std::endl;
        file->Close();
        return;
    }
    TTree *tree = (TTree*)dir->Get("histTree");
    if (!tree) {
        std::cerr << "Error: Could not find 'histTree' in " << filePath << std::endl;
        file->Close();
        return;
    }

    // Print the tree structure to check available branches
    std::cout << "Tree structure for file: " << filePath << std::endl;
    tree->Print();

    // Try accessing a specific branch (e.g., h_vertex_pt)
    TH1F *h_vertex_pt = nullptr;
    if (tree->SetBranchAddress("h_vertex_pt", &h_vertex_pt) == 0) {
        tree->GetEntry(0); // Load the first entry
        if (h_vertex_pt) {
            std::cout << "Successfully accessed branch 'h_vertex_pt'" << std::endl;
            h_vertex_pt->Print();
        } else {
            std::cerr << "Branch 'h_vertex_pt' is null!" << std::endl;
        }
    } else {
        std::cerr << "Error: Branch 'h_vertex_pt' not found!" << std::endl;
    }

    // Close the file
    file->Close();
}