"""Validate the packaged Amazon Chess case-study queries and solution sets."""

from __future__ import annotations

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent
GRAPH = ROOT / "data" / "amazon.txt"
GROUND_TRUTH = ROOT / "data" / "chess-ground-truth.txt"
APPR_QUERY = ROOT / "queries" / "appr.txt"
RCI_QUERY = ROOT / "queries" / "rci.txt"
OUTPUT = ROOT / "results"
SEED_NODE = 312245


def read_single_set(path: Path) -> set[int]:
    with path.open("r", encoding="utf-8") as handle:
        count = int(handle.readline().strip())
        rows = [line.strip() for line in handle if line.strip()]
    if count != 1 or len(rows) != 1:
        raise ValueError(f"{path}: expected exactly one set")
    return {int(value) for value in rows[0].split(",") if value}


def classification_metrics(candidate: set[int], truth: set[int]) -> dict[str, float | int]:
    true_positive = len(candidate & truth)
    false_positive = len(candidate - truth)
    false_negative = len(truth - candidate)
    precision = true_positive / len(candidate)
    recall = true_positive / len(truth)
    f1 = 2 * true_positive / (2 * true_positive + false_positive + false_negative)
    return {
        "size": len(candidate),
        "true_positive": true_positive,
        "false_positive": false_positive,
        "false_negative": false_negative,
        "precision": precision,
        "recall": recall,
        "f1": f1,
    }


def main() -> None:
    sets = {
        "ground_truth": read_single_set(GROUND_TRUTH),
        "appr": read_single_set(APPR_QUERY),
        "rci_query": read_single_set(RCI_QUERY),
        "rci_fw": read_single_set(ROOT / "expected" / "rci-fw.txt"),
        "rci_fw_exact": read_single_set(ROOT / "expected" / "rci-fw-exact.txt"),
        "pci_fw": read_single_set(ROOT / "expected" / "pci-fw.txt"),
        "pci_fw_exact": read_single_set(ROOT / "expected" / "pci-fw-exact.txt"),
    }

    refined = sets["rci_fw"]
    for name in ("rci_fw_exact", "pci_fw", "pci_fw_exact"):
        if sets[name] != refined:
            raise RuntimeError(f"{name} does not match the common refined solution")

    with GRAPH.open("r", encoding="utf-8") as handle:
        declared_edges, maximum_node_id = map(int, handle.readline().split())
        degrees = [0] * (maximum_node_id + 1)
        cuts = {name: 0 for name in sets}
        expanded_appr = set(sets["appr"])
        observed_edges = 0
        for line in handle:
            if not line.strip():
                continue
            u, v = map(int, line.split())
            observed_edges += 1
            degrees[u] += 1
            degrees[v] += 1
            if u in sets["appr"]:
                expanded_appr.add(v)
            if v in sets["appr"]:
                expanded_appr.add(u)
            for name, nodes in sets.items():
                if (u in nodes) != (v in nodes):
                    cuts[name] += 1

    if observed_edges != declared_edges:
        raise RuntimeError(
            f"Graph header declares {declared_edges} edges, but {observed_edges} were read"
        )
    if sets["rci_query"] != expanded_appr:
        raise RuntimeError("The RCI query is not APPR union its one-hop neighbours")

    total_volume = 2 * observed_edges
    metrics: dict[str, dict[str, float | int]] = {}
    for name, nodes in sets.items():
        volume = sum(degrees[node] for node in nodes)
        denominator = min(volume, total_volume - volume)
        row: dict[str, float | int] = {
            "size": len(nodes),
            "volume": volume,
            "cut": cuts[name],
            "conductance": cuts[name] / denominator if denominator else 0.0,
        }
        if name not in {"ground_truth", "rci_query"}:
            row.update(classification_metrics(nodes, sets["ground_truth"]))
        metrics[name] = row

    expected = {
        "ground_truth": 284,
        "appr": 418,
        "rci_query": 454,
        "rci_fw": 327,
        "rci_fw_exact": 327,
        "pci_fw": 327,
        "pci_fw_exact": 327,
    }
    for name, expected_size in expected.items():
        if len(sets[name]) != expected_size:
            raise RuntimeError(
                f"{name}: expected {expected_size} nodes, found {len(sets[name])}"
            )
    if SEED_NODE not in sets["ground_truth"] or SEED_NODE not in sets["appr"]:
        raise RuntimeError("The selected seed is missing from the ground truth or APPR set")
    if metrics["appr"]["cut"] != 48 or metrics["rci_fw"]["cut"] != 10:
        raise RuntimeError("The APPR or refined cut no longer matches the paper result")

    summary = {
        "dataset": {
            "nodes": maximum_node_id + 1,
            "edges": observed_edges,
        },
        "case_study": {
            "community": "Chess",
            "community_id": 41408,
            "seed_node": SEED_NODE,
            "seed_original_node_id": 517102,
            "seed_asin": "1857442245",
            "seed_product_title": "Meeting 1d4 (Everyman Chess)",
            "pci_theta_offset": 0.5,
            "all_four_refined_sets_identical": True,
        },
        "metrics": metrics,
    }
    OUTPUT.mkdir(exist_ok=True)
    output_path = OUTPUT / "validation-summary.json"
    output_path.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))
    print(f"Validation passed; wrote {output_path}")


if __name__ == "__main__":
    main()
