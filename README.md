# Strongly Local Convex-Programming Algorithms for Cluster Refinement on Large Graphs

## Dataset Sources

The experimental datasets are large and are not included in the GitHub repository. The table below summarizes the datasets used in the paper and provides their download pages. Dataset names follow the abbreviations used in the paper.

| Dataset | Source |
| --- | --- |
| Retweet | <https://networkrepository.com/rt-retweet-crawl.php> |
| DBLP | <http://www.konect.cc/networks/dblp_coauthor/> |
| YouTube | <http://www.konect.cc/networks/youtube-u-growth/> |
| DBpedia | <http://www.konect.cc/networks/dbpedia-all/> |
| Stack | <http://www.konect.cc/networks/sx-stackoverflow/> |
| LiveJ | <http://www.konect.cc/networks/soc-LiveJournal1/> |
| WikiEn | <https://networkrepository.com/web-wikipedia-link-en13-all.php> |
| AMiner | <https://open.aminer.cn/open/article?id=5965cf249ed5db41ed4f52bf> |
| Twitter | <http://www.konect.cc/networks/twitter_mpi/> |
| Friend | <https://snap.stanford.edu/data/com-Friendster.html> |

## Graph preprocessing: `process_adjacent_list.cpp`

`process_adjacent_list.cpp` converts a downloaded raw edge list into the graph format used by the algorithms. It reads from the first input line and assumes that every line contains two non-negative integers separated by spaces:

```text
u v
```

The program removes self-loops and duplicate edges, extracts the largest connected component, and renumbers its vertices consecutively as `1, 2, ..., n`. The output format is:

```text
m n
u_1 v_1
u_2 v_2
...
u_m v_m
```

Here, `m` is the number of unique undirected edges in the largest connected component, and `n` is its largest vertex ID.

### Compile

Run the following command in this directory:

```bash
g++ -O3 -std=c++17 process_adjacent_list.cpp -o process_adjacent_list
```

### Run

```bash
./process_adjacent_list raw_edges.txt graph.txt
```

Note: The input file should not contain a header or other metadata lines; remove them separately if they are present in the downloaded file.

## Query generation: `generate_R.cpp`

`generate_R.cpp` produces the reference-set queries used by the algorithms. It runs APPR from randomly selected seed vertices and writes one reference set `R` per query. The output query file starts with the number of queries; every following line is a comma-separated list of vertex IDs.

### Compile

Run the following command in this directory:

```bash
g++ -O3 -march=native -std=c++17 generate_R.cpp -o generate_R
```

### Run

Generate 64 PCI queries:

```bash
./generate_R graph.txt -appr 64 queries/pci.txt 2606042100 \
  --alpha=0.05 --eps=1e-6 --prefix_expand=1
```

Generate 64 RCI queries. RCI uses the APPR sweep set together with its one-hop neighbors:

```bash
./generate_R graph.txt -appr 64 queries/rci.txt 2606042100 \
  --alpha=0.05 --eps=1e-6 --prefix_expand=1 --add_neighbors=1
```

After `-appr`, the positional arguments are the number of queries, the output file, and an optional random seed.

| Parameter | Meaning | Default |
| --- | --- | ---: |
| `--alpha=X` | APPR stopping probability. | `0.05` |
| `--eps=X` | APPR approximation tolerance. | `1e-6` |
| `--prefix_expand=N` | Expansion factor for the output sweep prefix relative to the minimum-conductance prefix. | `1` |
| `--add_neighbors=0\|1` | Add one-hop neighbors of the reference set. Set this to `1` for RCI queries and keep it at `0` for PCI queries. | `0` |

## Algorithm implementations

The implementations of our RCI-FW, RCI-FW-Exact, PCI-FW, and PCI-FW-Exact algorithms are provided in the [`RCI`](RCI/README.md) and [`PCI`](PCI/README.md) directories, respectively.

The reproduction packages for the case studies and the Figure 1 example graph are provided in the [`case study and Figure 1`](<case study and Figure 1/>) directory.
