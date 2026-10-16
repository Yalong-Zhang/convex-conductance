# WebBase refinement figure

This example shows RCI removing false positives and PCI recovering false negatives while removing false positives. Both recover the same 96-node target community.

1. Download the [WebBase-2001 dataset](https://nrvis.com/download/data/web/web-webbase-2001.zip). We extracted the subgraph used in the figure and provide it as `subgraph.csv`. The graph is undirected and unweighted. The supplied `nodes.csv` marks the target community and the RCI and PCI initial sets in the columns `target_community`, `rci_initial`, and `pci_initial` (1 = inside, 0 = outside).
2. To reproduce the results, open a terminal in `WebBase-Reproduce` and run:

   ```sh
   python -m pip install networkx==3.6.1
   python reproduce.py
   ```

   The script processes `subgraph.csv`, constructs the initial sets, runs RCI (MQI) and PCI using exact min-cut iterations, and regenerates `nodes.csv` and `edges.csv`. RCI starts with 135 nodes (39 false positives); PCI starts with 92 nodes (12 false negatives and 8 false positives), with **theta = 1.0**. Both recover the 96-node target community. Keep the supplied input files together with `reproduce.py`.
3. Import `nodes.csv` as a node table and `edges.csv` as an undirected edge table in Gephi's Data Laboratory, in the same workspace. The generated files are also included, ready to import.

   Node columns `rci_initial`, `rci_return`, `pci_initial`, and `pci_return` use **1 = inside**, **0 = outside**. The same edge columns use **0 = internal edge**, **1 = cut edge**, **2 = external edge**. Use these columns to color the initial and refined clusters while keeping the same layout.
4. Alternatively, open **`graph.gephi`**, the finished Gephi project with the prepared layout. The two finished figure PDFs are included as visual references. The CSV files use consecutive zero-based IDs; `OriginalId` retains the original IDs for matching nodes in the prepared project.
