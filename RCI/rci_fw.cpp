#include <bits/stdc++.h>
#include <chrono>
#include <cassert>
using namespace std;
typedef long long ll;

inline bool read_number(FILE* in, int& x) {
	int ch = fgetc(in);
	while (ch != EOF && (ch < '0' || ch > '9')) ch = fgetc(in);
	if (ch == EOF) return false;
	x = 0;
	while (ch >= '0' && ch <= '9') x = x * 10 + ch - '0', ch = fgetc(in);
	return true;
}
inline void check(bool flag, const char* message) {
	if (!flag) { printf("!!!!! CHECK ERROR !!!!!\n Error message: %s\n", message); fflush(stdout); assert(0); }
}
struct Timer {
	chrono::high_resolution_clock::time_point s, t;
	void start() { s = chrono::high_resolution_clock::now(); }
	void end() { t = chrono::high_resolution_clock::now(); }
	double time() const { return chrono::duration<double>(t - s).count(); }
};

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
		FILE* in = fopen(dataset_address, "r"); check(in != NULL, "Can not open dataset");
		int header_n; check(read_number(in, m) && read_number(in, header_n), "Bad dataset header");
		e = (Edge*)malloc((size_t)m * sizeof(Edge)); int mx = INT_MIN;
		for (int i = 0; i < m; i++) { check(read_number(in, e[i].u) && read_number(in, e[i].v), "Bad edge line"); mx = max(mx, max(e[i].u, e[i].v)); }
		n = header_n + 1; check(mx < n, "Vertex id exceeds header bound");
		deg = (int*)malloc((size_t)n * sizeof(int)); memset(deg, 0, (size_t)n * sizeof(int));
		adj = (int**)malloc((size_t)n * sizeof(int*)); adj_id = (int**)malloc((size_t)n * sizeof(int*));
		for (int i = 0; i < m; i++) deg[e[i].u]++, deg[e[i].v]++;
		for (int i = 0; i < n; i++) adj[i] = (int*)malloc((size_t)max(1, deg[i]) * sizeof(int)), adj_id[i] = (int*)malloc((size_t)max(1, deg[i]) * sizeof(int));
		int* cur = (int*)malloc((size_t)n * sizeof(int)); memset(cur, 0, (size_t)n * sizeof(int));
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
	for (int i = 0; i < len;) {
		while (i < len && (line[i] == ' ' || line[i] == '\t' || line[i] == ',' || line[i] == '\r' || line[i] == '\n')) i++;
		if (i >= len) break;
		check(line[i] >= '0' && line[i] <= '9', "Bad query token");
		int x = 0; while (i < len && line[i] >= '0' && line[i] <= '9') x = x * 10 + line[i++] - '0';
		check(0 <= x && x < G.n, "Query vertex out of range"); R[R_size++] = x;
		while (i < len && (line[i] == ' ' || line[i] == '\t')) i++;
		if (i < len && line[i] != ',' && line[i] != '\r' && line[i] != '\n') check(false, "Bad query separator");
	}
	check(R_size > 0, "Query R is empty"); return R_size;
}

struct SetStats {
	int size; ll cut, volS; double ratio, conductance;
	SetStats() { size = 0; cut = volS = 0; ratio = conductance = 1e100; }
};
struct FwResult {
	SetStats stats; int iterations, sweep_calls, internal_edges; double runtime; const char* stop_reason;
	FwResult() { iterations = sweep_calls = internal_edges = 0; runtime = 0; stop_reason = "MAX_ITERS"; }
};
struct ExactResult {
	SetStats stats; int rounds, flow_calls; double runtime;
	ExactResult() { rounds = flow_calls = 0; runtime = 0; }
};
struct Settings {
	int max_iters, check_period, exact;
	Settings() { max_iters = 500; check_period = 50; exact = 0; }
};
bool parse_int_option(const char* arg, const char* key, int& value) {
	int len = (int)strlen(key);
	if (strncmp(arg, key, len) || arg[len] != '=') return false;
	value = atoi(arg + len + 1); return true;
}
struct PeelHeapNode {
	double score; int u, version;
	bool operator<(const PeelHeapNode& other) const {
		return score < other.score || (score == other.score && u > other.u);
	}
};

struct FlowEdge { int v, rev; double cap; };
struct Dinic {
	int n, s, t; vector<vector<FlowEdge>> adj; vector<int> level, it, q, vis;
	Dinic() { n = s = t = 0; }
	void init(int n_) { n = n_; adj.assign(n, vector<FlowEdge>()); level.resize(n); it.resize(n); q.resize(n); vis.resize(n); }
	void clear_graph() { for (int i = 0; i < n; i++) adj[i].clear(); }
	void add_edge(int u, int v, double c) { FlowEdge a = { v, (int)adj[v].size(), c }, b = { u, (int)adj[u].size(), 0 }; adj[u].push_back(a); adj[v].push_back(b); }
	void add_undirected(int u, int v, double c) { add_edge(u, v, c); add_edge(v, u, c); }
	bool bfs() {
		fill(level.begin(), level.end(), -1); int head = 0, tail = 0; level[s] = 0; q[tail++] = s;
		while (head < tail) {
			int u = q[head++];
			for (int i = 0; i < (int)adj[u].size(); i++) { FlowEdge& e = adj[u][i]; if (level[e.v] < 0 && e.cap > 1e-12) level[e.v] = level[u] + 1, q[tail++] = e.v; }
		}
		return level[t] >= 0;
	}
	double dfs(int u, double f) {
		if (u == t) return f;
		for (int& i = it[u]; i < (int)adj[u].size(); i++) {
			FlowEdge& e = adj[u][i];
			if (level[e.v] != level[u] + 1 || e.cap <= 1e-12) continue;
			double ret = dfs(e.v, min(f, e.cap));
			if (ret > 1e-12) { e.cap -= ret; adj[e.v][e.rev].cap += ret; return ret; }
		}
		return 0;
	}
	double maxflow(int s_, int t_) {
		s = s_; t = t_; double flow = 0;
		while (bfs()) { fill(it.begin(), it.end(), 0); while (1) { double pushed = dfs(s, 1e100); if (pushed <= 1e-12) break; flow += pushed; } }
		return flow;
	}
	void mark_source_side() {
		fill(vis.begin(), vis.end(), 0); int head = 0, tail = 0; vis[s] = 1; q[tail++] = s;
		while (head < tail) {
			int u = q[head++];
			for (int i = 0; i < (int)adj[u].size(); i++) { FlowEdge& e = adj[u][i]; if (!vis[e.v] && e.cap > 1e-12) vis[e.v] = 1, q[tail++] = e.v; }
		}
	}
};

struct Solver {
	Graph* G;
	int *in_R, *alive, *set_mark, *peel_inside, *peel_version, in_R_stamp, alive_stamp, set_stamp;
	int *local_id;
	double *r;
	int *R_nodes, R_cnt, orig_R_cnt, *order, *peel_order, peel_steps;
	int *edge_u, *edge_v, edge_cnt, edge_cap; double* edge_p;
	int peel_best_steps, peel_removed;
	double peel_alpha, original_R_ratio, peeling_time, preprocess_time, *peel_score_at;
	ll vol_R, orig_vol_R, core_vol;
	SetStats peel_best_stats;
	Dinic dinic;
	Solver() {
		G = NULL; in_R = alive = set_mark = peel_inside = peel_version = local_id = NULL; r = NULL; R_nodes = order = peel_order = NULL; peel_score_at = NULL;
		edge_u = edge_v = NULL; edge_p = NULL; in_R_stamp = alive_stamp = set_stamp = 1;
		R_cnt = orig_R_cnt = edge_cnt = edge_cap = peel_best_steps = peel_removed = peel_steps = 0;
		vol_R = orig_vol_R = core_vol = 0; peel_alpha = original_R_ratio = peeling_time = preprocess_time = 0;
	}
	~Solver() {
		if (in_R) free(in_R); if (alive) free(alive); if (set_mark) free(set_mark); if (peel_inside) free(peel_inside); if (peel_version) free(peel_version); if (local_id) free(local_id); if (r) free(r);
		if (R_nodes) free(R_nodes); if (order) free(order); if (peel_order) free(peel_order); if (peel_score_at) free(peel_score_at); if (edge_u) free(edge_u); if (edge_v) free(edge_v); if (edge_p) free(edge_p);
	}
	void bind_graph(Graph& g) {
		G = &g;
		in_R = (int*)malloc((size_t)G->n * sizeof(int)); alive = (int*)malloc((size_t)G->n * sizeof(int)); set_mark = (int*)malloc((size_t)G->n * sizeof(int));
		peel_inside = (int*)malloc((size_t)G->n * sizeof(int)); peel_version = (int*)malloc((size_t)G->n * sizeof(int));
		local_id = (int*)malloc((size_t)G->n * sizeof(int));
		r = (double*)malloc((size_t)G->n * sizeof(double));
		R_nodes = (int*)malloc((size_t)G->n * sizeof(int)); order = (int*)malloc((size_t)G->n * sizeof(int));
		peel_order = (int*)malloc((size_t)G->n * sizeof(int)); peel_score_at = (double*)malloc((size_t)G->n * sizeof(double));
		memset(in_R, 0, (size_t)G->n * sizeof(int)); memset(alive, 0, (size_t)G->n * sizeof(int)); memset(set_mark, 0, (size_t)G->n * sizeof(int));
		edge_cap = 1024; edge_u = (int*)malloc((size_t)edge_cap * sizeof(int)); edge_v = (int*)malloc((size_t)edge_cap * sizeof(int)); edge_p = (double*)malloc((size_t)edge_cap * sizeof(double));
	}
	inline void ensure_edge_cap(int need) {
		if (need <= edge_cap) return;
		while (edge_cap < need) edge_cap <<= 1;
		edge_u = (int*)realloc(edge_u, (size_t)edge_cap * sizeof(int)); edge_v = (int*)realloc(edge_v, (size_t)edge_cap * sizeof(int)); edge_p = (double*)realloc(edge_p, (size_t)edge_cap * sizeof(double));
		check(edge_u != NULL && edge_v != NULL && edge_p != NULL, "edge realloc failed");
	}
	inline double peeling_score(int u) const {
		int du = G->deg[u];
		return (double)(du - 2 * peel_inside[u]) / (double)du;
	}
	void init_peeling_marks() {
		alive_stamp++;
		for (int i = 0; i < orig_R_cnt; i++) {
			int u = R_nodes[i]; alive[u] = alive_stamp; peel_inside[u] = peel_version[u] = 0;
		}
		for (int i = 0; i < orig_R_cnt; i++) {
			int u = R_nodes[i], du = G->deg[u], *adj = G->adj[u];
			for (int j = 0; j < du; j++) if (alive[adj[j]] == alive_stamp) peel_inside[u]++;
		}
	}
	void heap_push(priority_queue<PeelHeapNode>& heap, int u) {
		peel_version[u]++;
		heap.push({ peeling_score(u), u, peel_version[u] });
	}
	SetStats first_peeling() {
		init_peeling_marks(); priority_queue<PeelHeapNode> heap;
		for (int i = 0; i < orig_R_cnt; i++) heap_push(heap, R_nodes[i]);
		ll cur_cut = 0, cur_vol = orig_vol_R;
		for (int i = 0; i < orig_R_cnt; i++) cur_cut += (ll)G->deg[R_nodes[i]] - (ll)peel_inside[R_nodes[i]];
		SetStats best; best.size = orig_R_cnt; best.cut = cur_cut; best.volS = cur_vol; best.ratio = cur_vol > 0 ? (double)cur_cut / (double)cur_vol : 1e100;
		{ ll side = min(cur_vol, G->total_vol - cur_vol); best.conductance = side > 0 ? (double)cur_cut / (double)side : 1e100; }
		original_R_ratio = best.ratio; peel_best_steps = 0; peel_steps = 0; int alive_cnt = orig_R_cnt, steps = 0;
		while (alive_cnt > 1 && !heap.empty()) {
			PeelHeapNode top = heap.top(); heap.pop(); int u = top.u;
			if (alive[u] != alive_stamp || top.version != peel_version[u]) continue;
			peel_order[peel_steps] = u; peel_score_at[peel_steps] = top.score; peel_steps++;
			int du = G->deg[u], inside = peel_inside[u], outside = du - inside, *adj = G->adj[u];
			cur_cut += (ll)inside - (ll)outside; cur_vol -= du; alive[u] = 0; alive_cnt--; steps++;
			for (int j = 0; j < du; j++) {
				int v = adj[j];
				if (alive[v] != alive_stamp) continue;
				peel_inside[v]--; heap_push(heap, v);
			}
			if (cur_vol <= 0) continue;
			double ratio = (double)cur_cut / (double)cur_vol;
			if (ratio < best.ratio) {
				best.ratio = ratio; best.cut = cur_cut; best.volS = cur_vol; best.size = alive_cnt; peel_best_steps = steps;
				ll side = min(cur_vol, G->total_vol - cur_vol); best.conductance = side > 0 ? (double)cur_cut / (double)side : 1e100;
			}
		}
		return best;
	}
	void mark_core_from_peeling_prefix(double alpha) {
		alive_stamp++; peel_removed = 0;
		for (int i = 0; i < orig_R_cnt; i++) alive[R_nodes[i]] = alive_stamp;
		while (peel_removed < peel_steps && peel_score_at[peel_removed] > alpha) {
			alive[peel_order[peel_removed]] = 0; peel_removed++;
		}
	}
	void prepare_query(int* R, int R_size) {
		Timer timer; timer.start();
		in_R_stamp++; R_cnt = edge_cnt = 0; vol_R = orig_vol_R = core_vol = 0; peel_removed = peel_best_steps = 0;
		for (int i = 0; i < R_size; i++) {
			int u = R[i];
			in_R[u] = in_R_stamp; R_nodes[R_cnt++] = u; orig_vol_R += G->deg[u];
		}
		check(R_cnt > 0, "empty query after duplicate removal"); orig_R_cnt = R_cnt; vol_R = orig_vol_R;
		Timer peel_timer; peel_timer.start();
		peel_best_stats = first_peeling(); peel_alpha = peel_best_stats.ratio; mark_core_from_peeling_prefix(peel_alpha);
		peel_timer.end(); peeling_time = peel_timer.time();
		int core_cnt = 0; core_vol = 0;
		for (int i = 0; i < orig_R_cnt; i++) {
			int u = R_nodes[i];
			if (alive[u] == alive_stamp) R_nodes[core_cnt++] = u, core_vol += G->deg[u];
		}
		check(core_cnt > 0, "peeling pruned empty core");
		R_cnt = core_cnt; vol_R = core_vol; in_R_stamp++;
		for (int i = 0; i < R_cnt; i++) {
			int u = R_nodes[i]; in_R[u] = in_R_stamp; r[u] = 0;
		}
		ll need_ll = vol_R / 2 + 1; if (need_ll > G->m) need_ll = G->m; ensure_edge_cap((int)max(1LL, need_ll));
		for (int i = 0; i < R_cnt; i++) {
			int u = R_nodes[i], out = 0, du = G->deg[u], *adj = G->adj[u];
			for (int j = 0; j < du; j++) {
				int v = adj[j];
				if (in_R[v] == in_R_stamp) {
					if (u < v) edge_u[edge_cnt] = u, edge_v[edge_cnt] = v, edge_p[edge_cnt++] = 0;
				} else out++;
			}
			r[u] = (double)out / (double)du;
		}
		timer.end(); preprocess_time = timer.time();
	}
	inline bool less_node(int a, int b) const {
		double x = r[a], y = r[b];
		return x < y || (x == y && a < b);
	}
	SetStats sweep_by_r() {
		memcpy(order, R_nodes, (size_t)R_cnt * sizeof(int));
		if (R_cnt > 1) sort(order, order + R_cnt, [&](int a, int b) { return less_node(a, b); });
		int stamp = ++set_stamp; ll cut = 0, vol = 0; SetStats best;
		for (int i = 0; i < R_cnt; i++) {
			int u = order[i], du = G->deg[u], *adj = G->adj[u]; set_mark[u] = stamp; vol += du;
			for (int j = 0; j < du; j++) cut += (set_mark[adj[j]] == stamp ? -1 : 1);
			double ratio = (double)cut / (double)vol;
			if (ratio < best.ratio) {
				best.ratio = ratio; best.cut = cut; best.volS = vol; best.size = i + 1;
				ll side = min(vol, G->total_vol - vol); best.conductance = side > 0 ? (double)cut / (double)side : 1e100;
			}
		}
		return best;
	}
	SetStats stats_from_source_side() {
		int stamp = ++set_stamp; SetStats st;
		for (int i = 0; i < R_cnt; i++) if (dinic.vis[i]) { int u = R_nodes[i]; set_mark[u] = stamp; st.size++; st.volS += G->deg[u]; }
		for (int i = 0; i < R_cnt; i++) if (dinic.vis[i]) {
			int u = R_nodes[i], du = G->deg[u], *adj = G->adj[u];
			for (int j = 0; j < du; j++) if (set_mark[adj[j]] != stamp) st.cut++;
		}
		if (st.volS > 0) st.ratio = (double)st.cut / (double)st.volS;
		ll side = min(st.volS, G->total_vol - st.volS); st.conductance = side > 0 ? (double)st.cut / (double)side : 1e100;
		return st;
	}
	ExactResult solve_exact(SetStats init_stats) {
		ExactResult result; Timer timer; timer.start(); result.stats = init_stats; double lambda = init_stats.ratio;
		dinic.init(R_cnt + 2);
		for (int i = 0; i < R_cnt; i++) local_id[R_nodes[i]] = i;
		for (int round = 1; round <= 1000000; round++) {
			int s = R_cnt, t = R_cnt + 1; double constant = 0; dinic.clear_graph();
			for (int i = 0; i < R_cnt; i++) {
				int u = R_nodes[i], out = 0, du = G->deg[u], *adj = G->adj[u];
				for (int j = 0; j < du; j++) if (in_R[adj[j]] != in_R_stamp) out++;
				double c = (double)out - lambda * (double)du;
				if (c < 0) dinic.add_edge(s, i, -c), constant += -c;
				else if (c > 0) dinic.add_edge(i, t, c);
			}
			for (int i = 0; i < edge_cnt; i++) dinic.add_undirected(local_id[edge_u[i]], local_id[edge_v[i]], 1.0);
			double flow = dinic.maxflow(s, t); result.flow_calls++; dinic.mark_source_side();
			SetStats cand = stats_from_source_side();
			if (!(cand.size > 0 && flow - constant < -1e-9 && cand.ratio + 1e-12 < lambda)) break;
			result.stats = cand; lambda = cand.ratio; result.rounds = round;
		}
		timer.end(); result.runtime = timer.time(); return result;
	}
	void fw_step(int iter) {
		double gamma = 2.0 / (double)(iter + 2);
		for (int i = 0, ec = edge_cnt; i < ec; i++) {
			int a = edge_u[i], b = edge_v[i]; double ra = r[a], rb = r[b], target;
			if (ra > rb) target = -1.0; else if (ra < rb) target = 1.0; else continue;
			double dp = gamma * (target - edge_p[i]);
			edge_p[i] += dp; r[a] += dp / (double)G->deg[a]; r[b] -= dp / (double)G->deg[b];
		}
	}
	FwResult solve(const Settings& settings) {
		FwResult result; Timer timer; timer.start(); result.internal_edges = edge_cnt;
		SetStats best = peel_best_stats, last = sweep_by_r(); result.sweep_calls = 1;
		if (last.ratio < best.ratio) best = last;
		for (int it = 1; it <= settings.max_iters; it++) {
			fw_step(it); result.iterations = it;
			if (it % settings.check_period) continue;
			SetStats cur = sweep_by_r(); result.sweep_calls++;
			if (cur.ratio < best.ratio) best = cur;
			if (cur.ratio < last.ratio) last = cur;
			else { result.stop_reason = "NO_SWEEP_IMPROVE"; break; }
		}
		result.stats = best; timer.end(); result.runtime = timer.time(); return result;
	}
};

int main(int argc, char** argv) {
	if (argc < 3) { printf("Usage: %s <dataset> <R_query_file> [--max_iters=N] [--check_period=N] [--exact=0|1]\n", argv[0]); return 0; }
	char dataset_address[256], query_address[256]; strcpy(dataset_address, argv[1]); strcpy(query_address, argv[2]);
	Settings settings;
	for (int i = 3; i < argc; i++) {
		if (parse_int_option(argv[i], "--max_iters", settings.max_iters)) continue;
		if (parse_int_option(argv[i], "--check_period", settings.check_period)) continue;
		if (parse_int_option(argv[i], "--exact", settings.exact)) continue;
		check(false, "Unknown option");
	}
	check(settings.max_iters > 0, "max_iters must be positive"); check(settings.check_period > 0, "check_period must be positive");
	Graph G; Timer timer; timer.start(); G.read_graph_from_dataset(dataset_address); timer.end();
	fprintf(stderr, "graph_read_sec=%.6f n=%d m=%d total_vol=%lld\n", timer.time(), G.n, G.m, G.total_vol);
	ifstream fin(query_address); check(fin.good(), "Can not open query file");
	string line; check((bool)getline(fin, line), "Empty query file"); int qnum = atoi(line.c_str()); check(qnum > 0, "Bad query count");
	int* R = (int*)malloc((size_t)G.n * sizeof(int)); Solver solver; solver.bind_graph(G);
	printf("query_id,query_size,volR,original_R_ratio,peeling_sec,peel_best_size,peel_best_ratio,peel_removed,core_size,core_vol,internal_edges,return_size,cut,volS,ratio,conductance,preprocess_sec,fw_sec,runtime_sec,iterations,sweep_calls,stop_reason,exact_return_size,exact_cut,exact_volS,exact_ratio,exact_conductance,exact_sec,exact_rounds,exact_flow_calls,total_sec\n");
	for (int qid = 1; qid <= qnum; qid++) {
		check((bool)getline(fin, line), "Not enough query lines");
		int R_size = parse_query_line(line, G, R);
		solver.prepare_query(R, R_size);
		FwResult ans = solver.solve(settings);
		ExactResult ex; if (settings.exact) ex = solver.solve_exact(ans.stats);
		SetStats es = settings.exact ? ex.stats : SetStats();
		printf("%d,%d,%lld,%.17g,%.9f,%d,%.17g,%d,%d,%lld,%d,%d,%lld,%lld,%.17g,%.17g,%.9f,%.9f,%.9f,%d,%d,%s,%d,%lld,%lld,%.17g,%.17g,%.9f,%d,%d,%.9f\n", qid, solver.orig_R_cnt, solver.orig_vol_R, solver.original_R_ratio, solver.peeling_time, solver.peel_best_stats.size, solver.peel_best_stats.ratio, solver.peel_removed, solver.R_cnt, solver.vol_R, ans.internal_edges, ans.stats.size, ans.stats.cut, ans.stats.volS, ans.stats.ratio, ans.stats.conductance, solver.preprocess_time, ans.runtime, solver.preprocess_time + ans.runtime, ans.iterations, ans.sweep_calls, ans.stop_reason, es.size, es.cut, es.volS, es.ratio, es.conductance, ex.runtime, ex.rounds, ex.flow_calls, solver.preprocess_time + ans.runtime + ex.runtime);
	}
	free(R); return 0;
}
