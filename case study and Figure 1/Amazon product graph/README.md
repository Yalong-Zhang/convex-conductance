# Amazon Product-Graph Case Study

This directory is a focused reproduction package for the Chess-category case study in the paper. It retains the processed Amazon co-purchasing graph, the single final APPR/RCI query, the four expected refined sets, and the visualization artifacts used in the paper.

## Data source

The graph and ground-truth product communities originate from the [SNAP Amazon product co-purchasing dataset](https://snap.stanford.edu/data/com-Amazon.html). The packaged graph is the processed largest connected component used in the experiments:

| Quantity | Value |
| --- | ---: |
| Products | 334,863 |
| Undirected edges | 925,872 |
| Ground-truth category | Chess |
| Ground-truth community ID | 41408 |
| Ground-truth products | 284 |

Vertex IDs in every packaged file are zero-based and refer to the same processed graph.

## Case-study query

The selected APPR seed is node `312245`, corresponding to the book *Meeting 1d4 (Everyman Chess)* (original SNAP node `517102`, ASIN `1857442245`). APPR returns 418 products and covers all 284 Chess products, but includes 134 false positives.

The two refinement inputs are:

- RCI: `queries/rci.txt`, the union of the APPR set and all its one-hop neighbours, containing 454 products.
- PCI: `queries/appr.txt`, the 418-product APPR set itself, with `theta_offset = 0.5`.

The fixed APPR set is included so that the case study always starts from the exact seed-level query used in the paper rather than from a new random seed selection.

## Directory contents

| Path | Purpose |
| --- | --- |
| `data/amazon.txt` | Processed full Amazon graph. |
| `data/chess-ground-truth.txt` | The 284-product Chess ground-truth set. |
| `queries/appr.txt` | APPR output used as the PCI reference set. |
| `queries/rci.txt` | APPR plus one-hop neighbours, used as the RCI reference set. |
| `expected/*.txt` | Full expected output sets for all four algorithms. |
| `expected/set-summary.csv` | Precision, recall, and F1 summary for the final query. |
| `visualization/nodes.csv` | Metadata and set-membership attributes for the 3,000-node visualization subgraph. |
| `visualization/edges.csv` | Its 7,149 induced edges. |
| `visualization/amazon-chess.gephi` | Gephi 0.11.2 project containing the saved layout and styling data. |
| `figures/` | Raw SVG exports and the three PDF panels used in the paper. |
| `validate_case_study.py` | Dependency-free validation of graph, queries, solutions, and paper metrics. |
| `run_algorithms.ps1` | Windows script that compiles and runs all four algorithms. |

## Requirements

- A C++17 compiler; the supplied commands use `g++`.
- Python 3.9 or later for deterministic validation; no third-party Python packages are required.
- Windows PowerShell to use `run_algorithms.ps1`. The listed commands can be run manually on other systems.
- Gephi 0.11.2 only if the saved visualization project needs to be inspected or re-exported.

## Run the algorithms

From this directory on Windows PowerShell, run:

```powershell
.\run_algorithms.ps1
```

The script compiles the RCI and PCI implementations from the repository root, runs RCI-FW, RCI-FW-Exact, PCI-FW, and PCI-FW-Exact on the packaged query, and stores their logs under `results/`.

Equivalent manual compilation commands are:

```bash
g++ -O3 -march=native -std=c++17 ../../RCI/rci_fw.cpp -o rci_fw
g++ -O3 -march=native -std=c++17 ../../PCI/pci_fw.cpp -o pci_fw
```

Run RCI-FW and RCI-FW-Exact:

```bash
./rci_fw data/amazon.txt queries/rci.txt --max_iters=500 --check_period=50 --exact=0
./rci_fw data/amazon.txt queries/rci.txt --max_iters=500 --check_period=50 --exact=1
```

Run PCI-FW and PCI-FW-Exact:

```bash
./pci_fw data/amazon.txt queries/appr.txt approx --theta_offset=0.5 \
  --max_outer=10 --max_iters=500 --check_period=100

./pci_fw data/amazon.txt queries/appr.txt exact --theta_offset=0.5 \
  --max_outer=10 --max_iters=500 --check_period=100
```

## Validate the complete result

The validator uses only the Python standard library:

```bash
python validate_case_study.py
```

It verifies the graph header, checks that the RCI query is exactly APPR plus its one-hop neighbours, confirms that all four refined sets are identical, and recomputes set sizes, cut sizes, conductance, precision, recall, and F1 from the full graph.

The expected result is:

| Set | Size | TP | FP | FN | F1 | Cut | Conductance |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| APPR | 418 | 284 | 134 | 0 | 0.8091 | 48 | 0.0212 |
| RCI-FW / RCI-FW-Exact / PCI-FW / PCI-FW-Exact | 327 | 282 | 45 | 2 | 0.9231 | 10 | 0.0058 |

## Reproduce the figures

The exact PDF panels included in the paper are:

- `figures/amazon-case-ground-truth.pdf`
- `figures/amazon-case-appr.pdf`
- `figures/amazon-case-rci-pci.pdf`

To inspect or re-export the graph visualization, open `visualization/amazon-chess.gephi` with Gephi 0.11.2. The project stores the layout. The corresponding CSV files retain the semantic category, ground-truth, APPR, RCI, and PCI membership attributes. Use `in_ground_truth`, `in_appr`, or one of the refined-set membership columns to reproduce the three panels; the raw SVG exports in `figures/` preserve the exact vector renderings used to prepare the PDFs.
