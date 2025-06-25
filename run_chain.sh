#!/bin/bash
set -e  # Exit on any error
cmsRun TrackProducer.py
cmsRun VertexProducer.py
rm scout.root
cmsRun TREE.py
rm DY2M_VertexerOutput.root
mv DY2M_ScoutingTree_Output.root $1

# Update FJR with success status
exitCode=0
exitMessage="Job completed successfully"
errorType="None"

cat << EOF > FrameworkJobReport.xml.tmp
<FrameworkJobReport>
<FrameworkError ExitStatus="$exitCode" Type="$errorType" >
$exitMessage
</FrameworkError>
EOF
tail -n+2 FrameworkJobReport.xml >> FrameworkJobReport.xml.tmp
mv FrameworkJobReport.xml.tmp FrameworkJobReport.xml