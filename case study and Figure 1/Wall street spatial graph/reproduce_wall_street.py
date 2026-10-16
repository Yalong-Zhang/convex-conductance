"""Reproduce the Wall Street financial-region refinement case study.

The script builds the symmetric 10-nearest-neighbour graph over the prepared
financial-place data, exactly solves the final RCI and PCI instances, and
renders the three panels used for the case study.
"""

from __future__ import annotations

import json
import os
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent
# Keep Matplotlib's cache within this reproducibility directory.
os.environ.setdefault("MPLCONFIGDIR", str(ROOT / ".matplotlib"))

import geopandas as gpd
import matplotlib.pyplot as plt
import networkx as nx
import numpy as np
from matplotlib.collections import LineCollection
from matplotlib.patches import Circle
from scipy.spatial import cKDTree
from shapely.geometry import MultiPoint, Point


DATA = ROOT / "data"
OUTPUT = ROOT / "results"
PLACES = DATA / "financial-places-lower-manhattan.geojson"
BUILDINGS = DATA / "buildings-lower-manhattan.parquet"
ROADS = DATA / "roads-lower-manhattan.parquet"
PROJECTED_CRS = "EPSG:32618"

K_NEIGHBOURS = 10

# Parameters of the final RCI panel. The paper describes this as the large
# circle around Wall Street; these are the exact coordinates used for the
# published visualisation.
RCI_CENTER_LONLAT = (-74.0090, 40.7095)
RCI_RADIUS_M = 1050

# Parameters of the final PCI panel.
PCI_CENTER_LONLAT = (-74.0100200102, 40.7073488977)
PCI_RADIUS_M = 300
PCI_THETA = 0.5

TOLERANCE = 1e-9


@dataclass(frozen=True)
class SetStats:
    nodes: set[int]
    cut: int
    volume: int
    denominator: float
    ratio: float


def build_symmetric_knn(xy: np.ndarray, k: int) -> list[set[int]]:
    """Connect every point to its k nearest other points, then symmetrise."""
    _, nearest = cKDTree(xy).query(xy, k=k + 1)
    edges: set[tuple[int, int]] = set()
    for u, neighbours in enumerate(nearest):
        for v in neighbours[1:]:
            v = int(v)
            # Equal coordinates can put the query point at a position other
            # than index zero in a tied nearest-neighbour result.
            if u != v:
                edges.add(tuple(sorted((u, v))))
    adjacency = [set() for _ in range(len(xy))]
    for u, v in edges:
        adjacency[u].add(v)
        adjacency[v].add(u)
    return adjacency


def largest_component(adjacency: list[set[int]]) -> set[int]:
    """Return the vertices of the largest connected component."""
    unseen = set(range(len(adjacency)))
    largest: set[int] = set()
    while unseen:
        start = unseen.pop()
        stack = [start]
        component = {start}
        while stack:
            u = stack.pop()
            for v in adjacency[u]:
                if v in unseen:
                    unseen.remove(v)
                    component.add(v)
                    stack.append(v)
        if len(component) > len(largest):
            largest = component
    return largest


def relabel_component(
    component: set[int], adjacency: list[set[int]]
) -> tuple[list[int], list[set[int]]]:
    """Induce a connected graph and map its local IDs to input-row IDs."""
    local_to_original = sorted(component)
    original_to_local = {u: i for i, u in enumerate(local_to_original)}
    local_adjacency = [
        {original_to_local[v] for v in adjacency[u] if v in original_to_local}
        for u in local_to_original
    ]
    return local_to_original, local_adjacency


def set_stats(
    nodes: set[int],
    adjacency: list[set[int]],
    reference: set[int] | None = None,
    theta: float | None = None,
) -> SetStats:
    """Evaluate conductance, or penalised conductance when reference is set."""
    cut = sum(1 for u in nodes for v in adjacency[u] if v not in nodes)
    volume = sum(len(adjacency[u]) for u in nodes)
    if reference is None:
        denominator = float(volume)
    else:
        assert theta is not None
        volume_inside = sum(len(adjacency[u]) for u in nodes & reference)
        volume_outside = sum(len(adjacency[u]) for u in nodes - reference)
        denominator = float(volume_inside - theta * volume_outside)
    ratio = cut / denominator if denominator > 0 else float("inf")
    return SetStats(nodes, cut, volume, denominator, ratio)


def minimum_fixed_delta(
    adjacency: list[set[int]], unary: np.ndarray, universe: set[int]
) -> tuple[float, set[int]]:
    """Exactly minimise cut(S) + sum(unary[u] for u in S) over ``universe``."""
    source, sink = "source", "sink"
    graph = nx.DiGraph()
    graph.add_nodes_from(universe)
    graph.add_nodes_from((source, sink))
    constant = 0.0
    for u in universe:
        coefficient = float(unary[u])
        if coefficient >= 0.0:
            graph.add_edge(u, sink, capacity=coefficient)
        else:
            graph.add_edge(source, u, capacity=-coefficient)
            constant += coefficient
        for v in adjacency[u]:
            if v in universe and u < v:
                graph.add_edge(u, v, capacity=1.0)
                graph.add_edge(v, u, capacity=1.0)
    value, partition = nx.minimum_cut(
        graph,
        source,
        sink,
        capacity="capacity",
        flow_func=nx.algorithms.flow.preflow_push,
    )
    return value + constant, set(partition[0]) - {source}


def solve_rci_exact(reference: set[int], adjacency: list[set[int]]) -> SetStats:
    """Solve the RCI objective exactly with Dinkelbach iterations and min-cut."""
    current = set_stats(reference, adjacency)
    boundary = np.zeros(len(adjacency), dtype=float)
    for u in reference:
        boundary[u] = sum(v not in reference for v in adjacency[u])
    for _ in range(100):
        unary = boundary.copy()
        for u in reference:
            unary[u] -= current.ratio * len(adjacency[u])
        residual, candidate = minimum_fixed_delta(adjacency, unary, reference)
        if residual >= -TOLERANCE or not candidate:
            return current
        current = set_stats(candidate, adjacency)
    raise RuntimeError("RCI Dinkelbach iterations did not converge")


def solve_pci_exact(
    reference: set[int], adjacency: list[set[int]], theta: float
) -> SetStats:
    """Solve the PCI objective for a fixed absolute penalty parameter theta."""
    current = set_stats(reference, adjacency, reference, theta)
    universe = set(range(len(adjacency)))
    for _ in range(100):
        unary = np.empty(len(adjacency), dtype=float)
        for u in universe:
            if u in reference:
                unary[u] = -current.ratio * len(adjacency[u])
            else:
                unary[u] = current.ratio * theta * len(adjacency[u])
        residual, candidate = minimum_fixed_delta(adjacency, unary, universe)
        if residual >= -TOLERANCE or not candidate:
            return current
        candidate_stats = set_stats(candidate, adjacency, reference, theta)
        if candidate_stats.denominator <= 0:
            raise RuntimeError("Min-cut returned an infeasible PCI solution")
        current = candidate_stats
    raise RuntimeError("PCI Dinkelbach iterations did not converge")


def project_point(lonlat: tuple[float, float]) -> Point:
    return gpd.GeoSeries([Point(*lonlat)], crs="EPSG:4326").to_crs(PROJECTED_CRS).iloc[0]


def save_node_set(path: Path, nodes: set[int]) -> None:
    np.savetxt(path, np.array(sorted(nodes), dtype=int), fmt="%d")


def add_basemap(ax, buildings: gpd.GeoDataFrame, roads: gpd.GeoDataFrame) -> None:
    ax.set_facecolor("#E8ECEF")
    buildings.plot(ax=ax, facecolor="#CFD5D9", edgecolor="none", zorder=0)
    roads.plot(ax=ax, color="#A8B3BA", linewidth=0.38, alpha=0.90, zorder=1)


def add_solution_hull(ax, xy: np.ndarray, nodes: set[int]) -> None:
    hull = MultiPoint(xy[sorted(nodes)]).convex_hull
    if hull.geom_type != "Polygon":
        return
    coordinates = np.asarray(hull.exterior.coords)
    ax.fill(
        coordinates[:, 0],
        coordinates[:, 1],
        facecolor="#ED9893",
        edgecolor="#C94A45",
        linewidth=1.35,
        alpha=0.25,
        zorder=2.6,
    )


def export_panel(fig, stem: str) -> None:
    fig.savefig(OUTPUT / f"{stem}.pdf", bbox_inches="tight", pad_inches=0)
    fig.savefig(OUTPUT / f"{stem}.png", dpi=600, bbox_inches="tight", pad_inches=0)
    plt.close(fig)


def draw_panels(
    xy: np.ndarray,
    adjacency: list[set[int]],
    buildings: gpd.GeoDataFrame,
    roads: gpd.GeoDataFrame,
    rci_solution: set[int],
    pci_solution: set[int],
) -> None:
    """Write the graph, RCI, and PCI panels of the case-study figure."""
    pairs = np.array(
        [(u, v) for u in range(len(adjacency)) for v in adjacency[u] if u < v],
        dtype=int,
    )
    segments = np.stack((xy[pairs[:, 0]], xy[pairs[:, 1]]), axis=1)
    rci_center = project_point(RCI_CENTER_LONLAT)
    pci_center = project_point(PCI_CENTER_LONLAT)
    rci_center_xy = np.array([rci_center.x, rci_center.y])
    pci_center_xy = np.array([pci_center.x, pci_center.y])

    def new_panel():
        fig, ax = plt.subplots(figsize=(5.7, 5.25))
        add_basemap(ax, buildings, roads)
        ax.add_collection(
            LineCollection(
                segments,
                colors="#5D737F",
                linewidths=0.58,
                alpha=0.48,
                zorder=2,
            )
        )
        ax.set_xlim(pci_center_xy[0] - 1150, pci_center_xy[0] + 1950)
        ax.set_ylim(pci_center_xy[1] - 950, pci_center_xy[1] + 1950)
        ax.set_aspect("equal")
        ax.set_axis_off()
        fig.subplots_adjust(left=0, right=1, bottom=0, top=1)
        return fig, ax

    fig, ax = new_panel()
    ax.scatter(xy[:, 0], xy[:, 1], s=17, c="#2878B5", alpha=0.88, linewidths=0, zorder=3)
    export_panel(fig, "wall-street-network")

    fig, ax = new_panel()
    outside = np.array(sorted(set(range(len(xy))) - rci_solution), dtype=int)
    selected = np.array(sorted(rci_solution), dtype=int)
    add_solution_hull(ax, xy, rci_solution)
    ax.scatter(xy[outside, 0], xy[outside, 1], s=15, c="#2878B5", alpha=0.80, linewidths=0, zorder=3)
    ax.scatter(xy[selected, 0], xy[selected, 1], s=23, c="#D84A4A", alpha=0.94, edgecolors="white", linewidths=0.12, zorder=5)
    ax.add_patch(Circle(rci_center_xy, RCI_RADIUS_M, fill=False, color="#843C96", linewidth=1.8, linestyle=(0, (4, 2)), zorder=6))
    export_panel(fig, "wall-street-rci")

    fig, ax = new_panel()
    outside = np.array(sorted(set(range(len(xy))) - pci_solution), dtype=int)
    selected = np.array(sorted(pci_solution), dtype=int)
    add_solution_hull(ax, xy, pci_solution)
    ax.scatter(xy[outside, 0], xy[outside, 1], s=15, c="#2878B5", alpha=0.80, linewidths=0, zorder=3)
    ax.scatter(xy[selected, 0], xy[selected, 1], s=23, c="#D84A4A", alpha=0.94, edgecolors="white", linewidths=0.12, zorder=5)
    ax.add_patch(Circle(pci_center_xy, PCI_RADIUS_M, fill=False, color="#843C96", linewidth=2.8, linestyle=(0, (5, 2.2)), zorder=6))
    export_panel(fig, "wall-street-pci")


def main() -> None:
    OUTPUT.mkdir(exist_ok=True)

    places = gpd.read_file(PLACES).to_crs(PROJECTED_CRS).reset_index(drop=True)
    buildings = gpd.read_parquet(BUILDINGS).to_crs(PROJECTED_CRS)
    roads = gpd.read_parquet(ROADS).to_crs(PROJECTED_CRS)
    roads = roads[roads["subtype"].eq("road")]
    roads = roads[
        roads["class"].isin(
            {
                "motorway", "trunk", "primary", "secondary", "tertiary",
                "residential", "service", "unclassified", "pedestrian", "living_street",
            }
        )
    ]
    xy = np.c_[places.geometry.x.to_numpy(), places.geometry.y.to_numpy()]

    global_adjacency = build_symmetric_knn(xy, K_NEIGHBOURS)
    component = largest_component(global_adjacency)
    local_to_original, adjacency = relabel_component(component, global_adjacency)
    local_xy = xy[local_to_original]

    rci_center = project_point(RCI_CENTER_LONLAT)
    rci_distance = np.linalg.norm(local_xy - [rci_center.x, rci_center.y], axis=1)
    rci_reference = set(np.flatnonzero(rci_distance <= RCI_RADIUS_M).tolist())
    rci_result = solve_rci_exact(rci_reference, adjacency)

    pci_center = project_point(PCI_CENTER_LONLAT)
    pci_distance = np.linalg.norm(local_xy - [pci_center.x, pci_center.y], axis=1)
    pci_reference = set(np.flatnonzero(pci_distance <= PCI_RADIUS_M).tolist())
    pci_result = solve_pci_exact(pci_reference, adjacency, PCI_THETA)

    rci_original = {local_to_original[u] for u in rci_result.nodes}
    pci_original = {local_to_original[u] for u in pci_result.nodes}
    rci_reference_original = {local_to_original[u] for u in rci_reference}
    pci_reference_original = {local_to_original[u] for u in pci_reference}
    common_stats = set_stats(rci_result.nodes, adjacency)

    if rci_result.nodes != pci_result.nodes:
        raise RuntimeError("The configured RCI and PCI instances no longer have the same solution")
    if len(rci_result.nodes) != 526 or common_stats.cut != 19:
        raise RuntimeError(
            "Unexpected final solution; expected 526 nodes and 19 cut edges, "
            f"got {len(rci_result.nodes)} nodes and {common_stats.cut} cut edges"
        )

    save_node_set(OUTPUT / "rci-reference.txt", rci_reference_original)
    save_node_set(OUTPUT / "pci-reference.txt", pci_reference_original)
    save_node_set(OUTPUT / "rci-solution.txt", rci_original)
    save_node_set(OUTPUT / "pci-solution.txt", pci_original)
    summary = {
        "input_places": len(places),
        "largest_component_nodes": len(adjacency),
        "knn_k": K_NEIGHBOURS,
        "knn_edges": sum(map(len, global_adjacency)) // 2,
        "rci_reference_nodes": len(rci_reference),
        "rci_solution_nodes": len(rci_result.nodes),
        "rci_conductance": rci_result.ratio,
        "pci_reference_nodes": len(pci_reference),
        "pci_solution_nodes": len(pci_result.nodes),
        "pci_penalized_conductance": pci_result.ratio,
        "common_solution_internal_edges": (common_stats.volume - common_stats.cut) // 2,
        "common_solution_cut_edges": common_stats.cut,
        "common_solution_conductance": common_stats.ratio,
    }
    (OUTPUT / "summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    draw_panels(xy, global_adjacency, buildings, roads, rci_original, pci_original)

    print(json.dumps(summary, indent=2))
    print(f"Wrote figures and node sets to {OUTPUT}")


if __name__ == "__main__":
    main()
