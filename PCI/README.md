# PCI-FW and PCI-FW-Exact

`pci_fw.cpp` implements PCI-FW and its exact-certification version, PCI-FW-Exact, from the paper.

## Input format

The first line of a graph file is `m n`, followed by `m` undirected edges `u v`. The first line of a query file is the number of queries; each subsequent line is one comma-separated reference set `R`. The root-level `process_adjacent_list.cpp` converts downloaded raw edge lists to this graph format, and the root-level `generate_R.cpp` produces compatible query files.

## Compile

Run the following command from the repository root:

```bash
g++ -O3 -march=native -std=c++17 PCI/pci_fw.cpp -o pci_fw
```

## Run

Run PCI-FW:

```bash
./pci_fw graph.txt queries/pci.txt approx --theta_offset=0.5 \
  --max_outer=10 --max_iters=500 --check_period=100
```

Run PCI-FW-Exact:

```bash
./pci_fw graph.txt queries/pci.txt exact --theta_offset=0.5 \
  --max_outer=10 --max_iters=500 --check_period=100
```

For each query, the program uses:

```text
theta = vol(R) / vol(V \ R) + theta_offset
```

The `--theta_offset` parameter is required.

## Parameters

| Parameter | Meaning | Default |
| --- | --- | ---: |
| `approx` / `exact` | The third positional argument: `approx` runs PCI-FW; `exact` runs PCI-FW-Exact. | Required |
| `--theta_offset=X` | Offset in the penalty parameter defined above. | Required |
| `--max_outer=N` | Maximum number of outer Dinkelbach iterations. | `10` |
| `--max_iters=N` | Maximum number of Frank--Wolfe iterations in each outer round. | `500` |
| `--check_period=N` | Run a threshold sweep every `N` Frank--Wolfe iterations. | `100` |
