$ErrorActionPreference = "Stop"

$caseRoot = $PSScriptRoot
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $caseRoot "..\..")).Path
$resultRoot = Join-Path $caseRoot "results"
$binaryRoot = Join-Path $resultRoot "bin"
New-Item -ItemType Directory -Force -Path $binaryRoot | Out-Null

$rciSource = Join-Path $repoRoot "RCI\rci_fw.cpp"
$pciSource = Join-Path $repoRoot "PCI\pci_fw.cpp"
$rciBinary = Join-Path $binaryRoot "rci_fw.exe"
$pciBinary = Join-Path $binaryRoot "pci_fw.exe"
$graph = Join-Path $caseRoot "data\amazon.txt"
$rciQuery = Join-Path $caseRoot "queries\rci.txt"
$pciQuery = Join-Path $caseRoot "queries\appr.txt"

& g++ -O3 -march=native -std=c++17 $rciSource -o $rciBinary
if ($LASTEXITCODE -ne 0) { throw "Failed to compile RCI" }
& g++ -O3 -march=native -std=c++17 $pciSource -o $pciBinary
if ($LASTEXITCODE -ne 0) { throw "Failed to compile PCI" }

& $rciBinary $graph $rciQuery --max_iters=500 --check_period=50 --exact=0 |
    Tee-Object -FilePath (Join-Path $resultRoot "rci-fw.log")
if ($LASTEXITCODE -ne 0) { throw "RCI-FW failed" }

& $rciBinary $graph $rciQuery --max_iters=500 --check_period=50 --exact=1 |
    Tee-Object -FilePath (Join-Path $resultRoot "rci-fw-exact.log")
if ($LASTEXITCODE -ne 0) { throw "RCI-FW-Exact failed" }

& $pciBinary $graph $pciQuery approx --theta_offset=0.5 --max_outer=10 --max_iters=500 --check_period=100 |
    Tee-Object -FilePath (Join-Path $resultRoot "pci-fw.log")
if ($LASTEXITCODE -ne 0) { throw "PCI-FW failed" }

& $pciBinary $graph $pciQuery exact --theta_offset=0.5 --max_outer=10 --max_iters=500 --check_period=100 |
    Tee-Object -FilePath (Join-Path $resultRoot "pci-fw-exact.log")
if ($LASTEXITCODE -ne 0) { throw "PCI-FW-Exact failed" }

& python (Join-Path $caseRoot "validate_case_study.py")
if ($LASTEXITCODE -ne 0) { throw "Case-study validation failed" }
