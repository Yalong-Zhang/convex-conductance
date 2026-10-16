"""Reproduce the two refinement examples and export Gephi CSV files."""
import csv
from collections import Counter
from pathlib import Path

import networkx as nx

ROOT = Path(__file__).resolve().parent
RCI_EXTRA = {687,685,711,335,333,334,710,709,807,686,651,63,318,320,650,911,910,37,569,915,913,567,35,772,762,829,670,672,559,551,549,560,72,811,74,69,799,71,558}
PCI_MISSING = {930,932,324,326,32,931,946,76,530,532,33,948}
PCI_EXTRA = {807,711,709,335,333,685,686,687}


def read_csv(name):
    with (ROOT / name).open(encoding="utf-8-sig", newline="") as f:
        return list(csv.DictReader(f))


def write_csv(name, fields, rows):
    with (ROOT / name).open("w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def solve(graph, initial, restricted):
    """Exact Dinkelbach/min-cut solver; PCI uses theta = 1.

    Integer capacities avoid roundoff. With lambda = cut/denominator,
    the network minimizes denominator * cut(S) - cut * g(S), up to
    the constant source capacity. MQI restricts S to the initial set.
    """
    best = set(initial)
    degree = dict(graph.degree())
    allowed = set(initial) if restricted else set(graph)
    source, sink = "source", "sink"

    def stats(nodes):
        cut = sum((u in nodes) != (v in nodes) for u, v in graph.edges())
        denominator = sum(degree[u] * (1 if restricted or u in initial else -1) for u in nodes)
        return cut, denominator

    while True:
        cut, denominator = stats(best)
        network = nx.DiGraph()
        network.add_nodes_from([source, sink])
        network.add_nodes_from(allowed)
        for u in allowed:
            network.add_edge(source, u, capacity=cut * degree[u] if u in initial else 0)
            boundary = sum(v not in allowed for v in graph[u])
            penalty = 0 if u in initial else cut * degree[u]
            network.add_edge(u, sink, capacity=denominator * boundary + penalty)
        for u, v in graph.edges():
            if u in allowed and v in allowed:
                network.add_edge(u, v, capacity=denominator)
                network.add_edge(v, u, capacity=denominator)
        _, (reachable, _) = nx.minimum_cut(network, source, sink)
        candidate = set(reachable) - {source}
        next_cut, next_denominator = stats(candidate)
        if next_denominator <= 0 or next_cut * denominator >= cut * next_denominator:
            return best
        best = candidate


def main():
    # Preserve the exact hand-selected subgraph used for the figure.
    raw = read_csv("subgraph.csv")
    original_edges = sorted({tuple(sorted((int(r["Source"]), int(r["Target"])))) for r in raw if r["Source"] != r["Target"]})
    original_ids = sorted({u for edge in original_edges for u in edge})
    mapping = {old: new for new, old in enumerate(original_ids)}
    edges = [(mapping[u], mapping[v]) for u, v in original_edges]
    graph = nx.Graph()
    graph.add_nodes_from(range(len(original_ids)))
    graph.add_edges_from(edges)
    reference = read_csv("communities.csv")
    labels = {int(r["Id"]): int(r["LouvainCommunity"]) for r in reference}
    assert {int(r["Id"]): int(r["OriginalId"]) for r in reference} == dict(enumerate(original_ids))
    target = {u for u in graph if labels[u] == 0}
    initial_rci = target | RCI_EXTRA
    initial_pci = (target - PCI_MISSING) | PCI_EXTRA
    returned_rci = solve(graph, initial_rci, restricted=True)
    returned_pci = solve(graph, initial_pci, restricted=False)
    assert returned_rci == returned_pci == target, "Refinement did not recover community 0"
    memberships = {"rci_initial": initial_rci, "rci_return": returned_rci, "pci_initial": initial_pci, "pci_return": returned_pci}
    sizes = Counter(labels.values())
    nodes = []
    for u, old in enumerate(original_ids):
        row = {"Id": u, "Label": old, "OriginalId": old, "Degree": graph.degree(u), "LouvainCommunity": labels[u], "LouvainCommunitySize": sizes[labels[u]], "target_community": int(u in target)}
        row.update({name: int(u in group) for name, group in memberships.items()})
        nodes.append(row)
    edge_rows = []
    for u, v in edges:
        row = {"Source": u, "Target": v, "Type": "Undirected", "Weight": 1}
        row.update({name: 2 - int(u in group) - int(v in group) for name, group in memberships.items()})
        edge_rows.append(row)
    write_csv("nodes.csv", list(nodes[0]), nodes)
    write_csv("edges.csv", list(edge_rows[0]), edge_rows)
    print(f"Graph: {len(graph)} nodes, {len(edges)} undirected edges")
    print(f"RCI: {len(initial_rci)} -> {len(returned_rci)} nodes")
    print(f"PCI (theta=1): {len(initial_pci)} -> {len(returned_pci)} nodes")
    print("Both results exactly recover the 96-node community 0.")
    print("Import nodes.csv and edges.csv into Gephi, or open the prepared graph.gephi.")


if __name__ == "__main__":
    main()
