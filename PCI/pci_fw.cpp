#include <bits/stdc++.h>
#include <chrono>
#include <cfloat>
#include <cassert>
using namespace std;
typedef long long ll; typedef long double ld;
const ld INF = 1e100L, FEPS = 1e-14L, SEPS = 1e-12L, SORT_EPS = 1e-12L;

inline bool read_number(FILE* in, int& x) { int ch = fgetc(in); while (ch != EOF && (ch < '0' || ch > '9')) ch = fgetc(in); if (ch == EOF) return false; x = 0; while (ch >= '0' && ch <= '9') x = x * 10 + ch - '0', ch = fgetc(in); return true; }
inline void check(bool flag, const char* message) { if (!flag) { printf("!!!!! CHECK ERROR !!!!!\n Error message: %s\n", message); fflush(stdout); fflush(stderr); assert(0); } }
inline ld slack(ld scale) { if (scale < 1) scale = 1; return 256.0L * LDBL_EPSILON * scale; }
inline bool sig_neg(ld x, ld scale) { return x < -slack(scale); }
inline bool sig_pos(ld x, ld scale) { return x > slack(scale); }
struct Timer { chrono::high_resolution_clock::time_point s, t; void start() { s = chrono::high_resolution_clock::now(); } void end() { t = chrono::high_resolution_clock::now(); } double time() { return chrono::duration<double>(t - s).count(); } };

struct Edge { int u, v; };
struct Graph {
	int n, m; Edge* e; int* deg; int** adj; int** adj_id; ll total_vol;
	Graph() { n = m = 0, e = NULL, deg = NULL, adj = adj_id = NULL, total_vol = 0; }
	~Graph() {
		if (e) free(e); if (deg) free(deg);
		if (adj) { for (int i = 0; i < n; i++) if (adj[i]) free(adj[i]); free(adj); }
		if (adj_id) { for (int i = 0; i < n; i++) if (adj_id[i]) free(adj_id[i]); free(adj_id); }
	}
	void read_graph_from_dataset(char* dataset_address) {
		FILE* in = fopen(dataset_address, "r"); check(in != NULL, "Can not open file dataset_address");
		int header_n; check(read_number(in, m) && read_number(in, header_n), "Bad dataset header");
		e = (Edge*)malloc(m * sizeof(Edge)); int mx = INT_MIN;
		for (int i = 0; i < m; i++) { check(read_number(in, e[i].u) && read_number(in, e[i].v), "Bad edge line"); mx = max(mx, max(e[i].u, e[i].v)); }
		n = header_n + 1; check(mx < n, "Vertex id exceeds header bound");
		deg = (int*)malloc(n * sizeof(int)); memset(deg, 0, n * sizeof(int)); adj = (int**)malloc(n * sizeof(int*)); adj_id = (int**)malloc(n * sizeof(int*));
		for (int i = 0; i < m; i++) { check(0 <= e[i].u && e[i].u < n && 0 <= e[i].v && e[i].v < n, "Vertex id out of range"); deg[e[i].u]++, deg[e[i].v]++; }
		for (int i = 0; i < n; i++) {
			adj[i] = (int*)malloc(max(1, deg[i]) * (int)sizeof(int));
			adj_id[i] = (int*)malloc(max(1, deg[i]) * (int)sizeof(int));
		}
		int* cur = (int*)malloc(n * sizeof(int)); memset(cur, 0, n * sizeof(int));
		for (int i = 0; i < m; i++) {
			int u = e[i].u, v = e[i].v;
			adj[u][cur[u]] = v, adj_id[u][cur[u]++] = i;
			adj[v][cur[v]] = u, adj_id[v][cur[v]++] = i;
		}
		free(cur); fclose(in); total_vol = 0; for (int i = 0; i < n; i++) total_vol += deg[i];
	}
};

int parse_query_line(const string& line, Graph& G, int* R) {
	int len = (int)line.size(), R_size = 0;
	for (int i = 0; i < len; ) {
		while (i < len && (line[i] == ' ' || line[i] == '\t' || line[i] == ',' || line[i] == '\r' || line[i] == '\n')) i++;
		if (i >= len) break;
		check(line[i] >= '0' && line[i] <= '9', "Bad query line");
		int x = 0; while (i < len && line[i] >= '0' && line[i] <= '9') x = x * 10 + line[i++] - '0';
		check(0 <= x && x < G.n, "Query vertex out of range"); R[R_size++] = x;
		while (i < len && (line[i] == ' ' || line[i] == '\t')) i++;
		if (i < len && line[i] != ',' && line[i] != '\r' && line[i] != '\n') check(false, "Bad query separator");
	}
	check(R_size > 0, "Query R is empty"); return R_size;
}

struct MFEdge { int v, rev; double flow, cap; };
struct LocalDinic {
	int N, S, T, build_stamp, level_stamp, vis_stamp;
	vector<vector<MFEdge>> adj;
	vector<int> build_mark, level, level_mark, it, vis_mark, touched, bfs_nodes, q;
	void init(int n) {
		N = n; build_stamp = level_stamp = vis_stamp = 0;
		adj.resize(N); build_mark.assign(N, 0); level.resize(N); level_mark.assign(N, 0); it.resize(N); vis_mark.assign(N, 0);
		touched.reserve(N); bfs_nodes.reserve(N); q.reserve(N);
	}
	void begin_build() { build_stamp++; touched.clear(); }
	inline void touch(int u) { if (build_mark[u] != build_stamp) build_mark[u] = build_stamp, adj[u].clear(), touched.push_back(u); }
	void add_undirected(int u, int v, double c) { touch(u); touch(v); MFEdge a{ v, (int)adj[v].size(), 0, c }, b{ u, (int)adj[u].size(), 0, c }; adj[u].push_back(a); adj[v].push_back(b); }
	bool bfs(int s, int t) {
		S = s, T = t; level_stamp++; bfs_nodes.clear(); q.clear(); q.push_back(S); bfs_nodes.push_back(S); level_mark[S] = level_stamp; level[S] = 0;
		for (int qi = 0; qi < (int)q.size(); qi++) {
			int u = q[qi];
			for (int i = 0; i < (int)adj[u].size(); i++) { MFEdge& e = adj[u][i]; if (level_mark[e.v] == level_stamp || e.flow + FEPS >= e.cap) continue; level_mark[e.v] = level_stamp; level[e.v] = level[u] + 1; q.push_back(e.v); bfs_nodes.push_back(e.v); }
		}
		for (int i = 0; i < (int)bfs_nodes.size(); i++) it[bfs_nodes[i]] = 0;
		return level_mark[T] == level_stamp;
	}
	double dfs(int u, double pushed) {
		if (u == T || pushed <= FEPS) return pushed;
		for (int& i = it[u]; i < (int)adj[u].size(); i++) {
			MFEdge& e = adj[u][i];
			if (level_mark[e.v] != level_stamp || level[e.v] != level[u] + 1 || e.flow + FEPS >= e.cap) continue;
			double tr = dfs(e.v, min(pushed, e.cap - e.flow));
			if (tr > FEPS) { e.flow += tr; adj[e.v][e.rev].flow -= tr; return tr; }
		}
		return 0;
	}
	pair<double, int> maxflow(int s, int t) {
		double total = 0;
		while (bfs(s, t)) while (1) { double pushed = dfs(s, INF); if (pushed <= FEPS) break; total += pushed; }
		vis_stamp++; q.clear(); q.push_back(s); vis_mark[s] = vis_stamp; int cnt = 0;
		for (int qi = 0; qi < (int)q.size(); qi++) {
			int u = q[qi]; cnt++;
			for (int i = 0; i < (int)adj[u].size(); i++) {
				MFEdge& e = adj[u][i];
				if (vis_mark[e.v] == vis_stamp || e.flow + FEPS >= e.cap) continue;
				vis_mark[e.v] = vis_stamp; q.push_back(e.v);
			}
		}
		return make_pair(total, cnt);
	}
	inline bool on_source_side(int u) const { return vis_mark[u] == vis_stamp; }
};

struct SetStats {
	int size; ll cut, vol_in_R, vol_out_R; ld g, ratio; vector<char> in_set; vector<int> nodes;
	SetStats() { size = 0, cut = vol_in_R = vol_out_R = 0, g = 0, ratio = INF; }
};

struct ThresholdSweepResult {
	bool has_feasible, has_negative; ld best_residual; SetStats best_ratio_stats, best_negative_stats;
	ThresholdSweepResult() { has_feasible = has_negative = false; best_residual = INF; }
};

struct GreedyNeighborResult {
	bool valid, improved, found_negative; int steps, add_steps, remove_steps; ld residual; SetStats stats;
	GreedyNeighborResult() { valid = improved = found_negative = false; steps = add_steps = remove_steps = 0; residual = INF; }
};

struct PeelingInitResult {
	bool valid; int best_step, peel_steps; ld residual; SetStats stats;
	PeelingInitResult() { valid = false; best_step = peel_steps = 0; residual = INF; }
};

enum FwStopReason { FW_NEGATIVE_WITNESS, FW_NO_NEGATIVE_IN_BUDGET, FW_MAX_OUTER };
const char* fw_stop_reason_name(FwStopReason reason) {
	if (reason == FW_NEGATIVE_WITNESS) return "NEGATIVE_WITNESS";
	if (reason == FW_NO_NEGATIVE_IN_BUDGET) return "NO_NEGATIVE_IN_BUDGET";
	return "MAX_OUTER";
}

enum SolveMode { MODE_APPROX, MODE_EXACT };
const char* solve_mode_name(SolveMode mode) { return mode == MODE_APPROX ? "approx" : "exact"; }
bool parse_mode(const char* s, SolveMode& mode) {
	if (!strcmp(s, "approx")) return mode = MODE_APPROX, true;
	if (!strcmp(s, "exact")) return mode = MODE_EXACT, true;
	return false;
}
const ld FW_GAMMA_C = 2.0L;

struct FwSettings {
	int max_outer, max_iters, check_period, greedy_max_steps;
	ld theta_offset;
	bool has_theta_offset;
	FwSettings() { max_outer = 10, max_iters = 500, check_period = 100, greedy_max_steps = 5, theta_offset = 0, has_theta_offset = false; }
};

bool parse_int_option(const char* arg, const char* name, int& value) {
	int len = (int)strlen(name);
	if (strncmp(arg, name, len) != 0 || arg[len] != '=') return false;
	value = atoi(arg + len + 1);
	return true;
}

bool parse_ld_option(const char* arg, const char* name, ld& value) {
	int len = (int)strlen(name);
	if (strncmp(arg, name, len) != 0 || arg[len] != '=') return false;
	value = strtold(arg + len + 1, NULL);
	return true;
}

void parse_fw_setting_option(const char* arg, FwSettings& settings) {
	int ivalue; ld ldvalue;
	if (parse_int_option(arg, "--max_outer", ivalue)) { settings.max_outer = ivalue; return; }
	if (parse_int_option(arg, "--max_iters", ivalue)) { settings.max_iters = ivalue; return; }
	if (parse_int_option(arg, "--check_period", ivalue)) { settings.check_period = ivalue; return; }
	if (parse_int_option(arg, "--greedy_steps", ivalue)) { settings.greedy_max_steps = ivalue; return; }
	if (parse_ld_option(arg, "--theta_offset", ldvalue)) { settings.theta_offset = ldvalue; settings.has_theta_offset = true; return; }
	check(false, "Unknown FW option");
}

void validate_fw_settings(const FwSettings& settings) {
	check(settings.max_outer > 0, "max_outer must be positive");
	check(settings.max_iters > 0, "max_iters must be positive");
	check(settings.check_period > 0, "check_period must be positive");
	check(settings.greedy_max_steps >= 0, "greedy_steps must be nonnegative");
	check(settings.has_theta_offset, "theta_offset is required");
}

struct FwInnerResult {
 bool found_negative, has_last_h_sweep;
	int iters, checks, sweep_calls, greedy_calls;
	long long active_node_sum, active_edge_sum, active_samples;
	int max_active_nodes, max_active_edges;
	double state_time, step_time, sweep_time, greedy_time;
	ld final_H, best_H, best_residual;
	const char* witness_source; SetStats witness_stats; ThresholdSweepResult last_h_sweep;
 FwInnerResult() { found_negative = has_last_h_sweep = false; iters = checks = sweep_calls = greedy_calls = 0; active_node_sum = active_edge_sum = active_samples = 0; max_active_nodes = max_active_edges = 0; state_time = step_time = sweep_time = greedy_time = 0; final_H = 0; best_H = best_residual = INF; witness_source = "none"; witness_stats.ratio = INF; }
};

struct DinkelbachTrace {
	int round, inner_iters, checks, witness_size, sweep_calls, greedy_calls, max_active_nodes, max_active_edges;
	int init_greedy_steps, init_greedy_add_steps, init_greedy_remove_steps;
	long long active_node_sum, active_edge_sum, active_samples;
	bool found_negative;
	double round_time, state_time, step_time, sweep_time, greedy_time, init_greedy_time;
	ld delta_before, delta_after, witness_ratio, delta_improve, witness_residual, final_H;
	const char* witness_source;
	DinkelbachTrace() { round = inner_iters = checks = witness_size = sweep_calls = greedy_calls = max_active_nodes = max_active_edges = 0; init_greedy_steps = init_greedy_add_steps = init_greedy_remove_steps = 0; active_node_sum = active_edge_sum = active_samples = 0; found_negative = false; round_time = state_time = step_time = sweep_time = greedy_time = init_greedy_time = 0; delta_before = delta_after = witness_ratio = delta_improve = witness_residual = final_H = 0; witness_source = "none"; }
};

struct Solver {
	Graph* G;
	int* R, R_size; unsigned char* in_R, * cur_set;
	double* p; double* h; int* neighbors_in_set;
	unsigned char* active; int* active_nodes; int active_size;
	int active_edge_size; int* edge_mark; int edge_stamp; int* p_mark; int p_stamp;
	int* candidate_nodes; int candidate_size; int* node_mark; int node_stamp;
	int* fw_node_mark; int fw_node_stamp;
	int* set_mark; int set_stamp; int* neighbor_mark; int neighbor_stamp;
	int* peel_inside; int* peel_version; unsigned char* peel_alive;
	int* tag_W, * tag_S, * tag_B, * tag_local, * local_id;
	int W_stamp, S_stamp, B_stamp, local_stamp;
	ll vol_R, vol_out_bar; ld theta;
	LocalDinic exact_dinic;
	SetStats best_stats;
	int outer_iters, total_inner_iters, exact_outer_iters, exact_flow_calls;
	int exact_stage_rounds, exact_peak_W_size, exact_last_W_size, exact_peak_local_size, exact_last_local_size;
	ll exact_peak_W_vol, exact_last_W_vol;
	FwStopReason final_reason;
	ld final_delta, final_H;
	vector<int> R_nodes, touched_edges, changed_edges, greedy_candidates;
	vector<int> Wv, Sv, Ev, local_orig, boundary, exact_ret, exact_tmp;
	vector<DinkelbachTrace> traces;
	Solver() {
		G = NULL, R = NULL, R_size = 0, in_R = cur_set = NULL, p = h = NULL, neighbors_in_set = NULL;
		active = NULL, active_nodes = edge_mark = p_mark = candidate_nodes = node_mark = fw_node_mark = set_mark = neighbor_mark = peel_inside = peel_version = NULL;
		peel_alive = NULL;
		tag_W = tag_S = tag_B = tag_local = local_id = NULL;
		active_size = active_edge_size = candidate_size = edge_stamp = p_stamp = node_stamp = fw_node_stamp = set_stamp = neighbor_stamp = 0;
		W_stamp = S_stamp = B_stamp = local_stamp = 1;
		vol_R = vol_out_bar = 0, theta = 0, outer_iters = total_inner_iters = exact_outer_iters = exact_flow_calls = 0;
		exact_stage_rounds = exact_peak_W_size = exact_last_W_size = exact_peak_local_size = exact_last_local_size = 0;
		exact_peak_W_vol = exact_last_W_vol = 0;
		final_reason = FW_MAX_OUTER; final_delta = final_H = 0;
		traces.reserve(64);
	}
	~Solver() { if (in_R) free(in_R); if (cur_set) free(cur_set); if (p) free(p); if (h) free(h); if (neighbors_in_set) free(neighbors_in_set); if (active) free(active); if (active_nodes) free(active_nodes); if (edge_mark) free(edge_mark); if (p_mark) free(p_mark); if (candidate_nodes) free(candidate_nodes); if (node_mark) free(node_mark); if (fw_node_mark) free(fw_node_mark); if (set_mark) free(set_mark); if (neighbor_mark) free(neighbor_mark); if (peel_inside) free(peel_inside); if (peel_version) free(peel_version); if (peel_alive) free(peel_alive); if (tag_W) free(tag_W); if (tag_S) free(tag_S); if (tag_B) free(tag_B); if (tag_local) free(tag_local); if (local_id) free(local_id); }
	void bind_graph(Graph& g) {
		G = &g;
		if (in_R) free(in_R); if (cur_set) free(cur_set); if (p) free(p); if (h) free(h); if (neighbors_in_set) free(neighbors_in_set);
		if (active) free(active); if (active_nodes) free(active_nodes); if (edge_mark) free(edge_mark); if (p_mark) free(p_mark); if (candidate_nodes) free(candidate_nodes); if (node_mark) free(node_mark); if (fw_node_mark) free(fw_node_mark); if (set_mark) free(set_mark); if (neighbor_mark) free(neighbor_mark); if (peel_inside) free(peel_inside); if (peel_version) free(peel_version); if (peel_alive) free(peel_alive);
		if (tag_W) free(tag_W); if (tag_S) free(tag_S); if (tag_B) free(tag_B); if (tag_local) free(tag_local); if (local_id) free(local_id);
		in_R = (unsigned char*)malloc(G->n * sizeof(unsigned char));
		cur_set = (unsigned char*)malloc((G->n + 2) * sizeof(unsigned char));
		p = (double*)malloc(max(1, G->m) * sizeof(double));
		h = (double*)malloc(G->n * sizeof(double));
		neighbors_in_set = (int*)malloc(G->n * sizeof(int));
		active = (unsigned char*)malloc(G->n * sizeof(unsigned char));
		active_nodes = (int*)malloc(G->n * sizeof(int));
		edge_mark = (int*)malloc(max(1, G->m) * sizeof(int));
		p_mark = (int*)malloc(max(1, G->m) * sizeof(int));
		candidate_nodes = (int*)malloc(G->n * sizeof(int));
		node_mark = (int*)malloc(G->n * sizeof(int));
		fw_node_mark = (int*)malloc(G->n * sizeof(int));
		set_mark = (int*)malloc(G->n * sizeof(int));
		neighbor_mark = (int*)malloc(G->n * sizeof(int));
		peel_inside = (int*)malloc(G->n * sizeof(int));
		peel_version = (int*)malloc(G->n * sizeof(int));
		peel_alive = (unsigned char*)malloc(G->n * sizeof(unsigned char));
		tag_W = (int*)malloc(G->n * sizeof(int));
		tag_S = (int*)malloc(G->n * sizeof(int));
		tag_B = (int*)malloc(G->n * sizeof(int));
		tag_local = (int*)malloc(G->n * sizeof(int));
		local_id = (int*)malloc(G->n * sizeof(int));
		exact_dinic.init(G->n + 2);
		memset(in_R, 0, G->n * sizeof(unsigned char));
		memset(p, 0, max(1, G->m) * sizeof(double));
		memset(active, 0, G->n * sizeof(unsigned char));
		memset(edge_mark, 0, max(1, G->m) * sizeof(int));
		memset(p_mark, 0, max(1, G->m) * sizeof(int));
		memset(node_mark, 0, G->n * sizeof(int));
		memset(fw_node_mark, 0, G->n * sizeof(int));
		memset(set_mark, 0, G->n * sizeof(int));
		memset(neighbor_mark, 0, G->n * sizeof(int));
		memset(peel_inside, 0, G->n * sizeof(int));
		memset(peel_version, 0, G->n * sizeof(int));
		memset(peel_alive, 0, G->n * sizeof(unsigned char));
		memset(tag_W, 0, G->n * sizeof(int));
		memset(tag_S, 0, G->n * sizeof(int));
		memset(tag_B, 0, G->n * sizeof(int));
		memset(tag_local, 0, G->n * sizeof(int));
		active_size = active_edge_size = candidate_size = edge_stamp = p_stamp = node_stamp = fw_node_stamp = set_stamp = neighbor_stamp = 0;
		W_stamp = S_stamp = B_stamp = local_stamp = 1;
		R_nodes.reserve(G->n); touched_edges.reserve(max(1, G->m)); changed_edges.reserve(1 << 20); greedy_candidates.reserve(G->n);
		Wv.reserve(G->n); Sv.reserve(G->n); Ev.reserve(G->n); local_orig.reserve(G->n); boundary.reserve(G->n); exact_ret.reserve(G->n); exact_tmp.reserve(G->n);
	}
	void clear_answer() { best_stats = SetStats(); outer_iters = total_inner_iters = exact_outer_iters = exact_flow_calls = 0; exact_stage_rounds = exact_peak_W_size = exact_last_W_size = exact_peak_local_size = exact_last_local_size = 0; exact_peak_W_vol = exact_last_W_vol = 0; final_reason = FW_MAX_OUTER; final_delta = final_H = 0; for (int i = 0; i < (int)touched_edges.size(); i++) p[touched_edges[i]] = 0; touched_edges.clear(); p_stamp++; for (int i = 0; i < active_size; i++) active[active_nodes[i]] = 0; active_size = active_edge_size = candidate_size = 0; }
	void prepare_query(int* R_, int R_size_, ld theta_offset) {
		R = R_, R_size = R_size_, vol_R = 0;
		for (int i = 0; i < (int)R_nodes.size(); i++) in_R[R_nodes[i]] = 0;
		R_nodes.clear();
		for (int i = 0; i < R_size; i++) {
			int u = R[i]; check(0 <= u && u < G->n, "Seed vertex out of range");
			in_R[u] = 1; R_nodes.push_back(u); vol_R += G->deg[u];
		}
		check(vol_R > 0, "R must be non-empty"); check(vol_R < G->total_vol, "R covers all volume");
		vol_out_bar = G->total_vol - vol_R; theta = (ld)vol_R / (ld)vol_out_bar + theta_offset;
		clear_answer();
	}
	inline ld node_c(int u) { return in_R[u] ? (ld)G->deg[u] : -theta * (ld)G->deg[u]; }
	inline bool in_W(int u) { return tag_W[u] == W_stamp; }
	inline bool in_exact_S(int u) { return tag_S[u] == S_stamp; }
	SetStats stats_from_nodes(const vector<int>& nodes) {
		SetStats stats; stats.nodes = nodes; stats.size = (int)nodes.size();
		int stamp = ++set_stamp;
		for (int i = 0; i < (int)nodes.size(); i++) {
			int u = nodes[i]; set_mark[u] = stamp;
			if (in_R[u]) stats.vol_in_R += G->deg[u]; else stats.vol_out_R += G->deg[u];
		}
		for (int i = 0; i < (int)nodes.size(); i++) {
			int u = nodes[i];
			for (int t = 0; t < G->deg[u]; t++) if (set_mark[G->adj[u][t]] != stamp) stats.cut++;
		}
		stats.g = (ld)stats.vol_in_R - theta * (ld)stats.vol_out_R;
		if (stats.g > SEPS) stats.ratio = (ld)stats.cut / stats.g;
		return stats;
	}
	SetStats evaluate_R_local() { return stats_from_nodes(R_nodes); }
	vector<char> indicator_from_cur_set() { vector<char> in_set(G->n, 0); for (int u = 0; u < G->n; u++) if (cur_set[u]) in_set[u] = 1; return in_set; }
	SetStats evaluate_indicator(const vector<char>& in_set) {
		SetStats stats; stats.in_set = in_set;
		for (int u = 0; u < G->n; u++) if (in_set[u]) { stats.nodes.push_back(u); stats.size++; if (in_R[u]) stats.vol_in_R += G->deg[u]; else stats.vol_out_R += G->deg[u]; }
		for (int i = 0; i < G->m; i++) if ((bool)in_set[G->e[i].u] ^ (bool)in_set[G->e[i].v]) stats.cut++;
		stats.g = (ld)stats.vol_in_R - theta * (ld)stats.vol_out_R;
		if (stats.g > SEPS) stats.ratio = (ld)stats.cut / stats.g;
		return stats;
	}
	inline ld residual_objective(const SetStats& stats, ld delta) { if (stats.g <= SEPS) return INF; return (ld)stats.cut - delta * stats.g; }
	struct PeelHeapNode {
		ld score; int u, version;
		bool operator<(const PeelHeapNode& other) const {
			if (fabsl(score - other.score) > 1e-18L) return score < other.score;
			return u > other.u;
		}
	};
	inline ld peeling_score(int u) {
		int deg_u = G->deg[u];
		return (ld)((ll)deg_u - 2LL * (ll)peel_inside[u]) / (ld)deg_u;
	}
	inline void peeling_push(priority_queue<PeelHeapNode>& heap, int u) {
		peel_version[u]++;
		heap.push({ peeling_score(u), u, peel_version[u] });
	}
	PeelingInitResult peeling_init_from_R() {
		PeelingInitResult result;
		for (int i = 0; i < (int)R_nodes.size(); i++) {
			int u = R_nodes[i];
			peel_alive[u] = 1, peel_inside[u] = 0, peel_version[u] = 0;
		}
		for (int i = 0; i < (int)R_nodes.size(); i++) {
			int u = R_nodes[i], deg_u = G->deg[u], *adj_u = G->adj[u];
			for (int t = 0; t < deg_u; t++) if (in_R[adj_u[t]]) peel_inside[u]++;
		}
		SetStats best = evaluate_R_local();
		priority_queue<PeelHeapNode> heap;
		for (int i = 0; i < (int)R_nodes.size(); i++) peeling_push(heap, R_nodes[i]);
		ll cur_cut = best.cut, cur_vol_in = best.vol_in_R;
		int best_step = 0, peel_steps = 0;
		while (!heap.empty()) {
			PeelHeapNode top = heap.top(); heap.pop();
			int u = top.u;
			if (!peel_alive[u] || top.version != peel_version[u]) continue;
			int deg_u = G->deg[u], inside_u = peel_inside[u], outside_u = deg_u - inside_u, *adj_u = G->adj[u];
			cur_cut += (ll)inside_u - (ll)outside_u;
			cur_vol_in -= deg_u;
			peel_alive[u] = 0, peel_steps++;
			for (int t = 0; t < deg_u; t++) {
				int v = adj_u[t];
				if (!peel_alive[v]) continue;
				peel_inside[v]--;
				peeling_push(heap, v);
			}
			if (cur_vol_in <= 0) continue;
			ld ratio = (ld)cur_cut / (ld)cur_vol_in;
			if (ratio + 1e-18L < best.ratio) best.ratio = ratio, best.cut = cur_cut, best.vol_in_R = cur_vol_in, best.vol_out_R = 0, best.g = (ld)cur_vol_in, best_step = peel_steps;
		}
		for (int i = 0; i < (int)R_nodes.size(); i++) {
			int u = R_nodes[i];
			peel_alive[u] = 1, peel_inside[u] = 0, peel_version[u] = 0;
		}
		for (int i = 0; i < (int)R_nodes.size(); i++) {
			int u = R_nodes[i], deg_u = G->deg[u], *adj_u = G->adj[u];
			for (int t = 0; t < deg_u; t++) if (in_R[adj_u[t]]) peel_inside[u]++;
		}
		priority_queue<PeelHeapNode> replay_heap;
		for (int i = 0; i < (int)R_nodes.size(); i++) peeling_push(replay_heap, R_nodes[i]);
		for (int removed = 0; removed < best_step && !replay_heap.empty(); ) {
			PeelHeapNode top = replay_heap.top(); replay_heap.pop();
			int u = top.u;
			if (!peel_alive[u] || top.version != peel_version[u]) continue;
			peel_alive[u] = 0, removed++;
			int deg_u = G->deg[u], *adj_u = G->adj[u];
			for (int t = 0; t < deg_u; t++) {
				int v = adj_u[t];
				if (!peel_alive[v]) continue;
				peel_inside[v]--;
				peeling_push(replay_heap, v);
			}
		}
		vector<int> nodes;
		nodes.reserve(max(0, (int)R_nodes.size() - best_step));
		for (int i = 0; i < (int)R_nodes.size(); i++) if (peel_alive[R_nodes[i]]) nodes.push_back(R_nodes[i]);
		for (int i = 0; i < (int)R_nodes.size(); i++) peel_alive[R_nodes[i]] = 0;
		result.valid = true, result.best_step = best_step, result.peel_steps = peel_steps;
		result.stats = stats_from_nodes(nodes);
		result.residual = residual_objective(result.stats, result.stats.ratio);
		return result;
	}
	inline double fw_base_h(int u, ld delta) { return (double)(delta * node_c(u)); }
	ThresholdSweepResult threshold_sweep(ld delta) {
		vector<int> order(active_nodes, active_nodes + active_size);
		sort(order.begin(), order.end(), [&](int a, int b) {
			if (fabsl(h[a] - h[b]) > SORT_EPS) return h[a] > h[b];
			return a < b;
		});
		vector<int> nodes;
		ll cut = 0, vol_in = 0, vol_out = 0; int size = 0;
		ThresholdSweepResult result;
		int stamp = ++set_stamp, i = 0, order_size = (int)order.size();
		nodes.reserve(order_size);
		while (i < order_size) {
			int j = i + 1;
			while (j < order_size && fabsl(h[order[j]] - h[order[i]]) <= SORT_EPS) j++;
			for (int k = i; k < j; k++) {
				int u = order[k];
				if (set_mark[u] == stamp) continue;
				for (int t = 0; t < G->deg[u]; t++) {
					int v = G->adj[u][t];
					if (set_mark[v] == stamp) cut--;
					else cut++;
				}
				set_mark[u] = stamp; nodes.push_back(u); size++;
				if (in_R[u]) vol_in += G->deg[u]; else vol_out += G->deg[u];
			}
			ld g_value = (ld)vol_in - theta * (ld)vol_out;
			ld residual = (ld)cut - delta * g_value;
			if (g_value > SEPS) {
				SetStats stats;
				stats.size = size, stats.cut = cut, stats.vol_in_R = vol_in, stats.vol_out_R = vol_out, stats.g = g_value, stats.ratio = (ld)cut / g_value, stats.nodes = nodes;
				result.has_feasible = true;
				if (stats.ratio + 1e-18L < result.best_ratio_stats.ratio) result.best_ratio_stats = stats;
				if (residual + 1e-18L < result.best_residual) {
					result.best_residual = residual;
					result.best_negative_stats = stats;
					result.has_negative = residual < -SEPS;
				} else if (residual < -SEPS) result.has_negative = true;
			}
			i = j;
		}
		if (result.best_residual < -SEPS && result.best_negative_stats.g > SEPS) result.has_negative = true;
		return result;
	}
	GreedyNeighborResult greedy_neighbor_improve(const SetStats& start, ld delta, int max_steps) {
		GreedyNeighborResult result; if (start.g <= SEPS || start.nodes.empty()) return result;
		vector<int> nodes = start.nodes;
		int cur_set_stamp = ++set_stamp, cur_neighbor_stamp = ++neighbor_stamp; node_stamp++; greedy_candidates.clear();
		for (int i = 0; i < (int)nodes.size(); i++) set_mark[nodes[i]] = cur_set_stamp;
		auto ensure_candidate = [&](int u) {
			if (node_mark[u] == node_stamp) return;
			node_mark[u] = node_stamp; greedy_candidates.push_back(u);
		};
		auto ensure_neighbor_count = [&](int u) {
			if (neighbor_mark[u] != cur_neighbor_stamp) neighbor_mark[u] = cur_neighbor_stamp, neighbors_in_set[u] = 0;
		};
		for (int i = 0; i < (int)nodes.size(); i++) {
			int u = nodes[i]; ensure_candidate(u); ensure_neighbor_count(u);
			for (int t = 0; t < G->deg[u]; t++) {
				int v = G->adj[u][t]; ensure_candidate(v); ensure_neighbor_count(v); neighbors_in_set[v]++;
			}
		}
		SetStats current = start; ld current_residual = residual_objective(current, delta);
		result.valid = true, result.stats = current, result.residual = current_residual, result.found_negative = sig_neg(current_residual, fabsl((ld)current.cut) + fabsl(delta * current.g));
		max_steps = max(0, max_steps);
		for (int step = 0; step < max_steps; step++) {
			bool found_move = false, best_add = false; int best_node = -1; ld best_delta_cut = 0, best_delta_g = 0, best_delta_F = 0, best_new_g = 0, best_new_residual = 0;
			for (int ci = 0; ci < (int)greedy_candidates.size(); ci++) {
				int u = greedy_candidates[ci]; ensure_neighbor_count(u);
				int inside = neighbors_in_set[u], degree = G->deg[u], outside = degree - inside;
				if (set_mark[u] == cur_set_stamp) {
					ld delta_cut = (ld)inside - (ld)outside;
					ld delta_g = in_R[u] ? -(ld)degree : theta * (ld)degree;
					ld new_g = current.g + delta_g; if (new_g <= SEPS) continue;
					ld delta_F = delta_cut - delta * delta_g, new_residual = current_residual + delta_F, scale = fabsl(current_residual) + fabsl(new_residual) + (ld)1.0;
					if (sig_neg(delta_F, scale) && (!found_move || delta_F + slack(scale) < best_delta_F)) found_move = true, best_add = false, best_node = u, best_delta_cut = delta_cut, best_delta_g = delta_g, best_delta_F = delta_F, best_new_g = new_g, best_new_residual = new_residual;
				} else if (inside > 0) {
					ld delta_cut = (ld)outside - (ld)inside;
					ld delta_g = in_R[u] ? (ld)degree : -theta * (ld)degree;
					ld new_g = current.g + delta_g; if (new_g <= SEPS) continue;
					ld delta_F = delta_cut - delta * delta_g, new_residual = current_residual + delta_F, scale = fabsl(current_residual) + fabsl(new_residual) + (ld)1.0;
					if (sig_neg(delta_F, scale) && (!found_move || delta_F + slack(scale) < best_delta_F)) found_move = true, best_add = true, best_node = u, best_delta_cut = delta_cut, best_delta_g = delta_g, best_delta_F = delta_F, best_new_g = new_g, best_new_residual = new_residual;
				}
			}
			if (!found_move) break;
			int u = best_node, degree = G->deg[u];
			current.cut = llround((long double)current.cut + (long double)best_delta_cut);
			if (best_add) {
				set_mark[u] = cur_set_stamp; nodes.push_back(u); current.size++; if (in_R[u]) current.vol_in_R += degree; else current.vol_out_R += degree; result.add_steps++;
			} else {
				set_mark[u] = 0; current.size--; if (in_R[u]) current.vol_in_R -= degree; else current.vol_out_R -= degree; result.remove_steps++;
				for (int i = 0; i < (int)nodes.size(); i++) if (nodes[i] == u) { nodes[i] = nodes.back(); nodes.pop_back(); break; }
			}
			current.g = best_new_g; current.ratio = (current.g > SEPS) ? (ld)current.cut / current.g : INF; current_residual = best_new_residual;
			ensure_candidate(u); ensure_neighbor_count(u);
			for (int t = 0; t < G->deg[u]; t++) {
				int v = G->adj[u][t]; ensure_candidate(v); ensure_neighbor_count(v);
				if (best_add) neighbors_in_set[v]++; else neighbors_in_set[v]--;
			}
			result.steps++; result.improved = true; current.nodes = nodes; result.stats = current; result.residual = current_residual;
			if (sig_neg(current_residual, fabsl((ld)current.cut) + fabsl(delta * current.g))) result.found_negative = true;
		}
		result.stats.nodes = nodes; result.stats.g = (ld)result.stats.vol_in_R - theta * (ld)result.stats.vol_out_R; result.stats.ratio = (result.stats.g > SEPS) ? (ld)result.stats.cut / result.stats.g : INF;
		result.residual = residual_objective(result.stats, delta);
		result.found_negative = sig_neg(result.residual, fabsl((ld)result.stats.cut) + fabsl(delta * result.stats.g));
		return result;
	}
	void initialize_fw_state(bool keep_p) {
		if (!keep_p) {
			for (int i = 0; i < (int)touched_edges.size(); i++) p[touched_edges[i]] = 0;
			touched_edges.clear(); p_stamp++;
		}
		for (int i = 0; i < active_size; i++) active[active_nodes[i]] = 0;
		active_size = active_edge_size = candidate_size = 0;
	}
	inline void ensure_fw_node(int u, ld delta) {
		if (fw_node_mark[u] == fw_node_stamp) return;
		fw_node_mark[u] = fw_node_stamp;
		h[u] = fw_base_h(u, delta);
	}
	void initialize_local_fw_state_warm(ld delta) {
		active_size = active_edge_size = candidate_size = 0;
		fw_node_stamp++;
		for (int i = 0; i < (int)R_nodes.size(); i++) {
			int u = R_nodes[i];
			ensure_fw_node(u, delta);
		}
		for (int i = 0; i < (int)touched_edges.size(); i++) {
			int id = touched_edges[i];
			double flow = p[id];
			if (flow == 0) continue;
			const Edge& e = G->e[id];
			ensure_fw_node(e.u, delta);
			ensure_fw_node(e.v, delta);
			h[e.u] -= flow;
			h[e.v] += flow;
		}
		for (int i = 0; i < (int)R_nodes.size(); i++) {
			int u = R_nodes[i];
			if (!active[u] && h[u] > (double)SEPS) active[u] = 1, active_nodes[active_size++] = u;
		}
		for (int i = 0; i < (int)touched_edges.size(); i++) {
			int id = touched_edges[i];
			const Edge& e = G->e[id];
			int u = e.u, v = e.v;
			if (!active[u] && fw_node_mark[u] == fw_node_stamp && h[u] > (double)SEPS) active[u] = 1, active_nodes[active_size++] = u;
			if (!active[v] && fw_node_mark[v] == fw_node_stamp && h[v] > (double)SEPS) active[v] = 1, active_nodes[active_size++] = v;
		}
	}
	ld compute_local_H() {
		ld H = 0;
		for (int i = 0; i < active_size; i++) {
			int u = active_nodes[i];
			double hu = h[u];
			H += (ld)0.5L * (ld)hu * (ld)hu;
		}
		return H;
	}
	void local_fw_step_online(ld gamma, ld delta, FwInnerResult* result) {
		node_stamp++; candidate_size = 0;
		edge_stamp++; active_edge_size = 0;
		double gamma_r = (double)gamma;
		for (int i = 0; i < active_size; i++) {
			int u = active_nodes[i];
			node_mark[u] = node_stamp;
			candidate_nodes[candidate_size++] = u;
		}
		for (int i = 0; i < active_size; i++) {
			int u = active_nodes[i];
			int deg_u = G->deg[u], *adj_id_u = G->adj_id[u];
			for (int t = 0; t < deg_u; t++) {
				int id = adj_id_u[t];
				if (edge_mark[id] == edge_stamp) continue;
				edge_mark[id] = edge_stamp;
				active_edge_size++;
				const Edge& e = G->e[id];
				int a = e.u, b = e.v;
				if (fw_node_mark[a] != fw_node_stamp) {
					fw_node_mark[a] = fw_node_stamp;
					h[a] = fw_base_h(a, delta);
				}
				if (fw_node_mark[b] != fw_node_stamp) {
					fw_node_mark[b] = fw_node_stamp;
					h[b] = fw_base_h(b, delta);
				}
				double ha = h[a], hb = h[b];
				double dp;
				if (ha > hb) dp = gamma_r * (1.0 - p[id]);
				else if (ha < hb) dp = gamma_r * (-1.0 - p[id]);
				else continue;
				if (dp == 0) continue;
				if (p_mark[id] != p_stamp) p_mark[id] = p_stamp, touched_edges.push_back(id);
				p[id] += dp;
				// Online/Gauss-Seidel style: h is the only live residual state here.
				h[a] -= dp; h[b] += dp;
				if (node_mark[a] != node_stamp) {
					node_mark[a] = node_stamp;
					candidate_nodes[candidate_size++] = a;
				}
				if (node_mark[b] != node_stamp) {
					node_mark[b] = node_stamp;
					candidate_nodes[candidate_size++] = b;
				}
			}
		}
		if (result) {
			result->active_node_sum += active_size;
			result->active_edge_sum += active_edge_size;
			result->active_samples++;
			result->max_active_nodes = max(result->max_active_nodes, active_size);
			result->max_active_edges = max(result->max_active_edges, active_edge_size);
		}
		for (int i = 0; i < active_size; i++) active[active_nodes[i]] = 0;
		active_size = 0;
		for (int i = 0; i < candidate_size; i++) {
			int u = candidate_nodes[i];
			if (h[u] > (double)SEPS) active[u] = 1, active_nodes[active_size++] = u;
		}
	}
	bool maybe_take_fw_witness(const ThresholdSweepResult& sweep, const char* source, ld delta, FwInnerResult& result) {
		if (!sweep.has_negative || sweep.best_negative_stats.g <= SEPS) return false;
		ld residual = (ld)sweep.best_negative_stats.cut - delta * sweep.best_negative_stats.g;
		ld scale = fabsl((ld)sweep.best_negative_stats.cut) + fabsl(delta * sweep.best_negative_stats.g);
		if (!sig_neg(residual, scale)) return false;
		bool take = !result.found_negative;
		if (!take && sweep.best_negative_stats.ratio + 1e-18L < result.witness_stats.ratio) take = true;
		if (take) result.found_negative = true, result.witness_stats = sweep.best_negative_stats, result.witness_source = source;
		if (residual < result.best_residual) result.best_residual = residual;
		return true;
	}
	bool maybe_take_set_witness(const SetStats& stats, const char* source, ld delta, FwInnerResult& result) {
		if (stats.g <= SEPS) return false;
		ld residual = residual_objective(stats, delta), scale = fabsl((ld)stats.cut) + fabsl(delta * stats.g);
		if (!sig_neg(residual, scale)) return false;
		bool take = !result.found_negative;
		if (!take && stats.ratio + 1e-18L < result.witness_stats.ratio) take = true;
		if (take) result.found_negative = true, result.witness_stats = stats, result.witness_source = source;
		if (residual < result.best_residual) result.best_residual = residual;
		return true;
	}
	FwInnerResult run_inner_negative_search(ld delta, const FwSettings& settings, int gamma_offset) {
		FwInnerResult result;
		initialize_local_fw_state_warm(delta);
		for (int iter = 0; iter < settings.max_iters; iter++) {
			ld gamma = FW_GAMMA_C / ((ld)gamma_offset + (ld)iter + FW_GAMMA_C);
			Timer part_timer; part_timer.start();
			local_fw_step_online(gamma, delta, &result);
			part_timer.end(); result.step_time += part_timer.time();
			result.iters = iter + 1;
			if (result.iters % settings.check_period == 0 || result.iters == settings.max_iters) {
				part_timer.start();
				ld H = compute_local_H();
				part_timer.end(); result.state_time += part_timer.time();
				result.final_H = H; result.best_H = min(result.best_H, H);
				result.checks++;
				part_timer.start();
				ThresholdSweepResult hs = threshold_sweep(delta);
				part_timer.end(); result.sweep_time += part_timer.time(); result.sweep_calls++;
				result.last_h_sweep = hs; result.has_last_h_sweep = true;
				maybe_take_fw_witness(hs, "h_sweep", delta, result);
				result.best_residual = min(result.best_residual, hs.best_residual);
				if (hs.best_ratio_stats.g > SEPS) {
					part_timer.start();
					GreedyNeighborResult greedy = greedy_neighbor_improve(hs.best_ratio_stats, delta, settings.greedy_max_steps);
					part_timer.end(); result.greedy_time += part_timer.time(); result.greedy_calls++;
					if (greedy.valid) {
						if (greedy.residual < result.best_residual) result.best_residual = greedy.residual;
						maybe_take_set_witness(greedy.stats, "h_sweep_greedy", delta, result);
					}
					if (hs.best_negative_stats.g > SEPS && hs.best_negative_stats.nodes != hs.best_ratio_stats.nodes) {
						part_timer.start();
						GreedyNeighborResult greedy_negative = greedy_neighbor_improve(hs.best_negative_stats, delta, settings.greedy_max_steps);
						part_timer.end(); result.greedy_time += part_timer.time(); result.greedy_calls++;
						if (greedy_negative.valid) {
							if (greedy_negative.residual < result.best_residual) result.best_residual = greedy_negative.residual;
							maybe_take_set_witness(greedy_negative.stats, "h_neg_greedy", delta, result);
						}
					}
				}
				if (result.found_negative) break;
			}
		}
		if (!result.has_last_h_sweep) {
			Timer part_timer; part_timer.start();
			ld H = compute_local_H();
			part_timer.end(); result.state_time += part_timer.time();
			result.final_H = H, result.best_H = min(result.best_H, H);
			part_timer.start();
			result.last_h_sweep = threshold_sweep(delta);
			part_timer.end(); result.sweep_time += part_timer.time(); result.sweep_calls++;
			result.has_last_h_sweep = true;
			result.best_residual = min(result.best_residual, result.last_h_sweep.best_residual);
		}
		if (!isfinite((double)result.best_residual)) result.best_residual = 0;
		return result;
	}
	void solve_dinkelbach_fw(const FwSettings& settings) {
		best_stats = evaluate_R_local();
		ld delta = best_stats.ratio; outer_iters = total_inner_iters = 0; final_reason = FW_MAX_OUTER; traces.clear();

		Timer peel_timer; peel_timer.start();
		PeelingInitResult init_peel = peeling_init_from_R();
		peel_timer.end();
		if (init_peel.valid && init_peel.stats.g > SEPS && init_peel.stats.ratio + 1e-18L < best_stats.ratio) {
			DinkelbachTrace tr;
			tr.round = 0, tr.round_time = 0, tr.delta_before = delta, tr.inner_iters = 0, tr.checks = 0;
			tr.found_negative = true, tr.witness_ratio = init_peel.stats.ratio, tr.delta_after = init_peel.stats.ratio;
			tr.delta_improve = delta - init_peel.stats.ratio, tr.witness_residual = init_peel.residual, tr.witness_size = init_peel.stats.size;
			tr.witness_source = "R_peeling"; tr.greedy_calls = 0;
			tr.init_greedy_time = peel_timer.time();
			tr.init_greedy_steps = init_peel.best_step;
			tr.init_greedy_add_steps = 0;
			tr.init_greedy_remove_steps = init_peel.best_step;
			traces.push_back(tr);
			best_stats = init_peel.stats;
			delta = best_stats.ratio;
		}
		
		for (int outer = 0; outer < settings.max_outer; outer++) {
			Timer round_timer; round_timer.start();
			initialize_fw_state(true);
			FwInnerResult inner = run_inner_negative_search(delta, settings, total_inner_iters);
			round_timer.end();
			outer_iters = outer + 1; total_inner_iters += inner.iters;
			DinkelbachTrace tr;
			tr.round = outer + 1, tr.round_time = round_timer.time(), tr.delta_before = delta, tr.inner_iters = inner.iters, tr.checks = inner.checks;
			tr.final_H = inner.final_H;
			tr.state_time = inner.state_time, tr.step_time = inner.step_time, tr.sweep_time = inner.sweep_time, tr.greedy_time = inner.greedy_time;
			tr.sweep_calls = inner.sweep_calls, tr.greedy_calls = inner.greedy_calls;
			tr.active_node_sum = inner.active_node_sum, tr.active_edge_sum = inner.active_edge_sum, tr.active_samples = inner.active_samples;
			tr.max_active_nodes = inner.max_active_nodes, tr.max_active_edges = inner.max_active_edges;
			if (!inner.found_negative) {
				tr.found_negative = false, tr.delta_after = delta, tr.witness_ratio = delta, tr.delta_improve = 0, tr.witness_residual = inner.best_residual, tr.witness_source = inner.witness_source;
				traces.push_back(tr);
				final_delta = delta, final_H = inner.final_H;
				final_reason = FW_NO_NEGATIVE_IN_BUDGET;
				return;
			}
			SetStats witness = inner.witness_stats;
			ld new_delta = witness.ratio, witness_residual = residual_objective(witness, delta);
			tr.found_negative = true, tr.witness_ratio = witness.ratio, tr.delta_after = new_delta, tr.delta_improve = delta - new_delta, tr.witness_residual = witness_residual, tr.witness_size = witness.size, tr.witness_source = inner.witness_source;
			traces.push_back(tr);
			if (witness.ratio + 1e-18L < best_stats.ratio) best_stats = witness;
			delta = new_delta;
			final_reason = FW_NEGATIVE_WITNESS;
		}
		final_delta = best_stats.ratio;
		final_reason = FW_MAX_OUTER;
	}
	void exact_set_to_tag(const vector<int>& S) {
		S_stamp++;
		for (int i = 0; i < (int)S.size(); i++) tag_S[S[i]] = S_stamp;
	}
	SetStats exact_stats_on(const vector<int>& S) {
		exact_set_to_tag(S);
		SetStats st; st.nodes = S; st.size = (int)S.size();
		for (int i = 0; i < (int)S.size(); i++) {
			int u = S[i];
			if (in_R[u]) st.vol_in_R += G->deg[u]; else st.vol_out_R += G->deg[u];
			for (int t = 0; t < G->deg[u]; t++) if (!in_exact_S(G->adj[u][t])) st.cut++;
		}
		st.g = (ld)st.vol_in_R - theta * (ld)st.vol_out_R;
		if (st.g > SEPS) st.ratio = (ld)st.cut / st.g;
		return st;
	}
	ll exact_volume_of(const vector<int>& S) { ll ans = 0; for (int i = 0; i < (int)S.size(); i++) ans += G->deg[S[i]]; return ans; }
	void exact_reset_W_to_R() { W_stamp++; Wv = R_nodes; for (int i = 0; i < (int)Wv.size(); i++) tag_W[Wv[i]] = W_stamp; }
	void exact_build_local_vertices() {
		local_stamp++;
		local_orig.clear();
		for (int i = 0; i < (int)Wv.size(); i++) {
			int u = Wv[i];
			if (tag_local[u] != local_stamp) tag_local[u] = local_stamp, local_id[u] = (int)local_orig.size(), local_orig.push_back(u);
		}
		for (int i = 0; i < (int)Wv.size(); i++) {
			int u = Wv[i];
			for (int t = 0; t < G->deg[u]; t++) {
				int v = G->adj[u][t];
				if (tag_local[v] != local_stamp) tag_local[v] = local_stamp, local_id[v] = (int)local_orig.size(), local_orig.push_back(v);
			}
		}
	}
	void exact_stage_flow(double lambda) {
		double sink_penalty = lambda * (double)theta;
		Sv.clear(); exact_stage_rounds = 0; exact_peak_W_size = exact_last_W_size = 0; exact_peak_local_size = exact_last_local_size = 0; exact_peak_W_vol = exact_last_W_vol = 0;
		while (1) {
			exact_stage_rounds++;
			exact_build_local_vertices(); boundary.clear(); B_stamp++;
			int V = (int)local_orig.size(), s = V, t = V + 1;
			exact_last_W_size = (int)Wv.size(); exact_peak_W_size = max(exact_peak_W_size, exact_last_W_size);
			exact_last_W_vol = exact_volume_of(Wv); exact_peak_W_vol = max(exact_peak_W_vol, exact_last_W_vol);
			exact_last_local_size = V; exact_peak_local_size = max(exact_peak_local_size, exact_last_local_size);
			exact_dinic.begin_build();
			for (int i = 0; i < (int)Wv.size(); i++) {
				int u = Wv[i], iu = local_id[u];
				if (!in_R[u]) exact_dinic.add_undirected(iu, t, (double)G->deg[u] * sink_penalty), exact_dinic.add_undirected(iu, t, (double)G->deg[u] * sink_penalty);
				for (int j = 0; j < G->deg[u]; j++) {
					int v = G->adj[u][j], iv = local_id[v];
					if (in_W(v)) exact_dinic.add_undirected(iu, iv, 1.0);
					else {
						exact_dinic.add_undirected(iu, iv, 1.0), exact_dinic.add_undirected(iu, iv, 1.0);
						if (tag_B[v] != B_stamp) tag_B[v] = B_stamp, boundary.push_back(v);
					}
				}
			}
			for (int i = 0; i < (int)R_nodes.size(); i++) {
				int u = R_nodes[i];
				exact_dinic.add_undirected(s, local_id[u], (double)G->deg[u] * lambda);
				exact_dinic.add_undirected(s, local_id[u], (double)G->deg[u] * lambda);
			}
			for (int i = 0; i < (int)boundary.size(); i++) {
				int u = boundary[i];
				exact_dinic.add_undirected(local_id[u], t, (double)G->deg[u] * sink_penalty);
				exact_dinic.add_undirected(local_id[u], t, (double)G->deg[u] * sink_penalty);
			}
			pair<double, int> ret_mf = exact_dinic.maxflow(s, t);
			exact_flow_calls++;
			Sv.clear(); Ev.clear();
			for (int i = 0; i < V; i++) if (exact_dinic.on_source_side(i)) {
				int u = local_orig[i];
				Sv.push_back(u);
				if (!in_W(u)) Ev.push_back(u), tag_W[u] = W_stamp, Wv.push_back(u);
			}
			if (Ev.empty() || ret_mf.second <= 1) break;
		}
	}
	void exact_connectivity_cleanup(vector<int>& S) {
		if (S.empty()) return;
		exact_set_to_tag(S);
		exact_tmp.clear(); vector<int> stk; stk.push_back(S[0]);
		int old_stamp = S_stamp, keep_stamp = ++S_stamp;
		while (!stk.empty()) {
			int u = stk.back(); stk.pop_back();
			if (tag_S[u] != old_stamp) continue;
			tag_S[u] = keep_stamp; exact_tmp.push_back(u);
			for (int t = 0; t < G->deg[u]; t++) {
				int v = G->adj[u][t];
				if (tag_S[v] == old_stamp) stk.push_back(v);
			}
		}
		S.swap(exact_tmp);
	}
	void solve_exact_after_fw() {
		int max_outer = 1000000;
		exact_outer_iters = exact_flow_calls = 0;
		exact_ret = best_stats.nodes.empty() ? R_nodes : best_stats.nodes;
		ld lambda0 = best_stats.ratio;
		for (int round = 1; round <= max_outer; round++) {
			exact_reset_W_to_R();
			exact_stage_flow((double)lambda0);
			SetStats cand = exact_stats_on(Sv);
			if (!(cand.g > SEPS && cand.ratio + 1e-18L < lambda0)) break;
			exact_ret = Sv;
			if (cand.ratio + 1e-18L < best_stats.ratio) best_stats = cand;
			lambda0 = cand.ratio;
			exact_outer_iters = round;
		}
		exact_connectivity_cleanup(exact_ret);
		SetStats final_st = exact_stats_on(exact_ret);
		if (final_st.g > SEPS && final_st.ratio <= best_stats.ratio + 1e-18L) best_stats = final_st;
	}
};

void print_set_sample(const SetStats& s) { int shown = 0; printf("- %-20s:", "Best set sample"); if (!s.nodes.empty()) for (int i = 0; i < (int)s.nodes.size() && shown < 30; i++) printf(" %d", s.nodes[i]), shown++; else for (int u = 0; u < (int)s.in_set.size() && shown < 30; u++) if (s.in_set[u]) printf(" %d", u), shown++; if (!shown) printf(" (empty)"); if ((int)s.nodes.size() > 30 || shown == 30) printf(" ..."); printf("\n"); }

int main(int argc, char** argv) {
	if (argc < 4) {
		printf("Usage:\n./main <dataset> <R_query_file> <approx|exact> [--max_outer=N] [--max_iters=N] [--check_period=N] [--greedy_steps=N] --theta_offset=X\n");
		printf("R file format:\nline 1: number of queries\nnext lines: comma-separated node ids for each R\ntheta is always set to vol(R)/vol(V\\\\R)+theta_offset for each query.\n");
		return 0;
	}
	char dataset_address[256], query_address[256]; strcpy(dataset_address, argv[1]); strcpy(query_address, argv[2]);
	SolveMode mode; check(parse_mode(argv[3], mode), "Mode must be approx or exact");
	FwSettings settings;
	for (int argi = 4; argi < argc; argi++) parse_fw_setting_option(argv[argi], settings);
	validate_fw_settings(settings);
	printf("----------Local Dinkelbach + Frank-Wolfe----------\n");
	printf("- %-20s: %s\n- %-20s: %s\n- %-20s: %s\n- %-20s: %s\n", "Dataset address", dataset_address, "Query file", query_address, "Mode", solve_mode_name(mode), "FW kernel", "local_online");
	printf("- %-20s: %d\n- %-20s: %d\n- %-20s: %d\n- %-20s: %d\n- %-20s: %.12Le\n", "FW max_outer", settings.max_outer, "FW max_iters", settings.max_iters, "FW check_period", settings.check_period, "FW greedy_steps", settings.greedy_max_steps, "theta_offset", settings.theta_offset);

	Timer timer; timer.start(); Graph G; G.read_graph_from_dataset(dataset_address); timer.end();
	printf("- %-20s: %d, %d\n", "|E|, |V|", G.m, G.n);
	printf("- %-20s: %.6lf\n", "Read graph time", timer.time());

	ifstream fin(query_address); check(fin.good(), "Can not open query file");
	string line; check((bool)getline(fin, line), "Query file is empty");
	int query_num = atoi(line.c_str()); check(query_num > 0, "Bad query number");
	printf("- %-20s: %d\n", "#queries", query_num);

	int* R = (int*)malloc(G.n * sizeof(int));
	Solver solver; solver.bind_graph(G);
	for (int qid = 1; qid <= query_num; qid++) {
		check((bool)getline(fin, line), "Query lines are fewer than declared");
		int R_size = parse_query_line(line, G, R);
		solver.prepare_query(R, R_size, settings.theta_offset);
		timer.start();
		solver.solve_dinkelbach_fw(settings);
		SetStats fw_stats = solver.best_stats;
		if (mode == MODE_EXACT) solver.solve_exact_after_fw();
		timer.end();

		printf("\n[Query %d]\n", qid);
		printf("- %-20s: %d\n", "|R|", solver.R_size);
		printf("- %-20s: %lld\n", "vol(R)", solver.vol_R);
		printf("- %-20s: %.12Lf\n", "theta", solver.theta);
		printf("- %-20s: %.12Lf\n", "FW ratio", fw_stats.ratio);
		printf("- %-20s: %d\n", "FW set size", fw_stats.size);
		if (mode == MODE_APPROX) printf("- %-20s: %lld\n- %-20s: %lld\n- %-20s: %lld\n- %-20s: %.12Lf\n", "cut(S)", fw_stats.cut, "vol(S cap R)", fw_stats.vol_in_R, "vol(S \\ R)", fw_stats.vol_out_R, "g(S)", fw_stats.g);
		else printf("- %-20s: %.12Lf\n- %-20s: %d\n- %-20s: %lld\n- %-20s: %lld\n- %-20s: %lld\n- %-20s: %.12Lf\n- %-20s: %d\n- %-20s: %d\n- %-20s: %d\n- %-20s: %d\n- %-20s: %d\n- %-20s: %lld\n- %-20s: %lld\n- %-20s: %d\n- %-20s: %d\n",
			"Exact ratio", solver.best_stats.ratio, "Best set size", solver.best_stats.size, "cut(S)", solver.best_stats.cut, "vol(S cap R)", solver.best_stats.vol_in_R, "vol(S \\ R)", solver.best_stats.vol_out_R, "g(S)", solver.best_stats.g,
			"Exact outer iters", solver.exact_outer_iters, "Exact flow calls", solver.exact_flow_calls, "Exact stage rounds", solver.exact_stage_rounds, "Exact W size(last)", solver.exact_last_W_size, "Exact W size(peak)", solver.exact_peak_W_size,
			"Exact W vol(last)", solver.exact_last_W_vol, "Exact W vol(peak)", solver.exact_peak_W_vol, "Exact local V(last)", solver.exact_last_local_size, "Exact local V(peak)", solver.exact_peak_local_size);
		printf("- %-20s: %d\n", "FW outer iters", solver.outer_iters);
		printf("- %-20s: %d\n", "FW inner iters", solver.total_inner_iters);
		printf("- %-20s: %s\n", "FW final reason", fw_stop_reason_name(solver.final_reason));
		printf("- %-20s: %.12Lf\n", "FW final delta", solver.final_delta);
		printf("- %-20s: %.12Le\n", "FW final H", solver.final_H);
		printf("- %-20s: %.6lf\n", "Solve time", timer.time());
		printf("- %-20s:\n", "Dinkelbach trace");
		for (int i = 0; i < (int)solver.traces.size(); i++) {
			DinkelbachTrace& tr = solver.traces[i];
			double avg_active_nodes = tr.active_samples ? (double)tr.active_node_sum / tr.active_samples : 0;
			double avg_active_edges = tr.active_samples ? (double)tr.active_edge_sum / tr.active_samples : 0;
			printf("  - round=%d, time=%.6lf, delta_before=%.12Lf, found_negative=%d, witness_ratio=%.12Lf, delta_after=%.12Lf, improve=%.12Lf, witness_residual=%.12Le, witness_source=%s, witness_size=%d, inner_iters=%d, checks=%d, sweep_calls=%d, greedy_calls=%d, init_greedy_steps=%d, init_greedy_add_steps=%d, init_greedy_remove_steps=%d, init_greedy_time=%.6lf, state_time=%.6lf, step_time=%.6lf, sweep_time=%.6lf, greedy_time=%.6lf, active_samples=%lld, avg_active_nodes=%.2lf, avg_active_edges=%.2lf, max_active_nodes=%d, max_active_edges=%d, final_H=%.12Le\n",
				tr.round, tr.round_time, tr.delta_before, (int)tr.found_negative, tr.witness_ratio, tr.delta_after, tr.delta_improve, tr.witness_residual, tr.witness_source, tr.witness_size, tr.inner_iters, tr.checks, tr.sweep_calls, tr.greedy_calls, tr.init_greedy_steps, tr.init_greedy_add_steps, tr.init_greedy_remove_steps, tr.init_greedy_time, tr.state_time, tr.step_time, tr.sweep_time, tr.greedy_time, tr.active_samples, avg_active_nodes, avg_active_edges, tr.max_active_nodes, tr.max_active_edges, tr.final_H);
		}
		print_set_sample(mode == MODE_APPROX ? fw_stats : solver.best_stats);
	}
	check(!getline(fin, line), "Query lines are more than declared");
	free(R);
	return 0;
}
