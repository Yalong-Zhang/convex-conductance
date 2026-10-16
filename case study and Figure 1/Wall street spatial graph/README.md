# Wall Street Financial-Region Refinement

This directory is a clean, self-contained reproduction of the Wall Street case study in the paper. It constructs a symmetric 10-nearest-neighbour graph over financial places in Lower Manhattan, exactly solves the final RCI and PCI instances, and recreates the three figure panels.

## Contents

| Path | Purpose |
| --- | --- |
| `data/financial-places-lower-manhattan.geojson` | Prepared financial-place points used to construct the graph. |
| `data/buildings-lower-manhattan.parquet` | Building basemap for the figure. |
| `data/roads-lower-manhattan.parquet` | Road basemap for the figure. |
| `reproduce_wall_street.py` | Builds the graph, solves both final instances exactly, and writes the results and figures. |
| `requirements.txt` | Python dependencies. |

The prepared geographic inputs originate from the Overture Maps data used for the original case study. They are included here because they are small enough to make the final figure directly reproducible.

## Requirements

Use Python 3.10 or later. Create an environment and install the dependencies:

```bash
python -m venv .venv
```

On Windows PowerShell:

```powershell
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
python reproduce_wall_street.py
```

On macOS or Linux:

```bash
source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
python reproduce_wall_street.py
```

## Reproduction procedure

The script performs the following deterministic steps:

1. Project the financial places to UTM zone 18N and connect each point to its 10 nearest other points. The graph is symmetrised before use.
2. Retain the largest connected component.
3. Solve the RCI objective exactly for the large spatial reference circle.
4. Solve the PCI objective exactly for the small spatial reference circle with fixed `theta = 0.5`.
5. Verify that the two solutions coincide, then render the graph, RCI, and PCI panels.

The exact optimisation uses Dinkelbach iterations whose fixed-ratio subproblems are solved with minimum cuts. This is appropriate for the small visual case study and reproduces the exact solutions reported in the paper.

## Final instances

| Method | Reference-set centre (longitude, latitude) | Radius | Other setting |
| --- | --- | ---: | --- |
| RCI | `(-74.0090, 40.7095)` | 1000 m | RCI can only remove vertices from the reference set. |
| PCI | `(-74.0100200102, 40.7073488977)` | 300 m | `theta = 0.5`; PCI can expand beyond the reference set. |

## Expected output

Running the script creates `results/` containing:

- `wall-street-network.pdf` and `.png`
- `wall-street-rci.pdf` and `.png`
- `wall-street-pci.pdf` and `.png`
- `rci-reference.txt`, `pci-reference.txt`, `rci-solution.txt`, and `pci-solution.txt`
- `summary.json`

The two exact methods should return the same 526-place region. Its 3,244 internal graph edges and 19 cut edges give conductance approximately `0.002916`.

The generated node IDs are zero-based row indices of `financial-places-lower-manhattan.geojson`; they are stable because the script preserves the input ordering.
