#include <bits/stdc++.h>
#include <chrono>
#include <cassert>
using namespace std;
typedef long long ll;
typedef long double ld;

const ld FEPS = 1e-18L;
const int DEFAULT_LOCAL_SET_SIZE = 1000;

inline bool read_number(FILE* in, int& x) {
	int ch = fgetc(in);
	while (ch != EOF && (ch < '0' || ch > '9')) ch = fgetc(in);
	if (ch == EOF) return false;
	x = 0;
	while (ch >= '0' && ch <= '9') x = x * 10 + ch - '0', ch = fgetc(in);
	return true;
}
inline void check(bool flag, const char* message) {
	if (!flag) {
		printf("!!!!! CHECK ERROR !!!!!\n Error message: %s\n", message);
		assert(0);
	}
}
bool is_positive_int_string(const char* s) {
	if (s == NULL || *s == 0) return false;
	for (int i = 0; s[i]; i++) if (s[i] < '0' || s[i] > '9') return false;
	return atoi(s) > 0;
}
struct Timer {
	chrono::high_resolution_clock::time_point s, t;
	void start() { s = chrono::high_resolution_clock::now(); }
	void end() { t = chrono::high_resolution_clock::now(); }
	double time() { return chrono::duration<double>(t - s).count(); }
};

struct Edge { int u, v; };
struct Graph {
	int n, m, header_n;
	Edge* e;
	int* deg;
	int** adj;
	ll total_vol;
	Graph() { n = m = header_n = 0; e = NULL; deg = NULL; adj = NULL; total_vol = 0; }
	~Graph() {
		if (e) free(e);
		if (deg) free(deg);
		if (adj) {
			if (adj[0]) free(adj[0]);
			free(adj);
		}
	}
	void read_graph_from_dataset(char* dataset_address) {
		FILE* in = fopen(dataset_address, "r");
		check(in != NULL, "Can not open file dataset_address");
		check(read_number(in, m) && read_number(in, header_n), "Bad dataset header");
		e = (Edge*)malloc((size_t)m * sizeof(Edge));
		int mx = INT_MIN;
		for (int i = 0; i < m; i++) {
			check(read_number(in, e[i].u) && read_number(in, e[i].v), "Bad edge line");
			mx = max(mx, max(e[i].u, e[i].v));
		}
		n = header_n + 1;
		check(mx < n, "Vertex id exceeds header bound");
		deg = (int*)malloc((size_t)n * sizeof(int));
		memset(deg, 0, (size_t)n * sizeof(int));
		for (int i = 0; i < m; i++) {
			check(0 <= e[i].u && e[i].u < n && 0 <= e[i].v && e[i].v < n, "Vertex id out of range");
			deg[e[i].u]++, deg[e[i].v]++;
		}
		adj = (int**)malloc((size_t)n * sizeof(int*));
		size_t* offset = (size_t*)malloc((size_t)(n + 1) * sizeof(size_t));
		offset[0] = 0;
		for (int i = 0; i < n; i++) offset[i + 1] = offset[i] + deg[i];
		size_t edge_slots = max<size_t>(1, (size_t)m * 2);
		int* flat_adj = (int*)malloc(edge_slots * sizeof(int));
		for (int i = 0; i < n; i++) adj[i] = flat_adj + offset[i];
		size_t* cur = (size_t*)malloc((size_t)n * sizeof(size_t));
		memcpy(cur, offset, (size_t)n * sizeof(size_t));
		for (int i = 0; i < m; i++) {
			int u = e[i].u, v = e[i].v;
			flat_adj[cur[u]++] = v;
			flat_adj[cur[v]++] = u;
		}
		free(cur);
		free(offset);
		fclose(in);
		total_vol = 0;
		for (int i = 0; i < n; i++) total_vol += deg[i];
	}
};

struct NodeScore { int u; ld key; };
bool cmp_score(NodeScore a, NodeScore b) {
	if (fabsl(a.key - b.key) > 1e-30L) return a.key > b.key;
	return a.u < b.u;
}

struct Workspace {
	Graph& G;
	int* que;
	int* mark_stamp;
	int* in_stamp;
	int cur_mark_stamp, cur_in_stamp;
	ld* p;
	ld* q;
	ld* r;
	int* val_stamp;
	int* inq_stamp;
	int* support;
	NodeScore* ord;
	int support_size;
	Workspace(Graph& graph) : G(graph) {
		que = (int*)malloc((size_t)G.n * sizeof(int));
		mark_stamp = (int*)malloc((size_t)G.n * sizeof(int));
		in_stamp = (int*)malloc((size_t)G.n * sizeof(int));
		p = (ld*)malloc((size_t)G.n * sizeof(ld));
		q = (ld*)malloc((size_t)G.n * sizeof(ld));
		r = (ld*)malloc((size_t)G.n * sizeof(ld));
		val_stamp = (int*)malloc((size_t)G.n * sizeof(int));
		inq_stamp = (int*)malloc((size_t)G.n * sizeof(int));
		support = (int*)malloc((size_t)G.n * sizeof(int));
		ord = (NodeScore*)malloc((size_t)G.n * sizeof(NodeScore));
		memset(mark_stamp, 0, (size_t)G.n * sizeof(int));
		memset(in_stamp, 0, (size_t)G.n * sizeof(int));
		memset(val_stamp, 0, (size_t)G.n * sizeof(int));
		memset(inq_stamp, 0, (size_t)G.n * sizeof(int));
		cur_mark_stamp = cur_in_stamp = 0;
		support_size = 0;
	}
	~Workspace() {
		free(que);
		free(mark_stamp);
		free(in_stamp);
		free(p);
		free(q);
		free(r);
		free(val_stamp);
		free(inq_stamp);
		free(support);
		free(ord);
	}
	inline void new_mark_round() {
		cur_mark_stamp++;
		if (cur_mark_stamp == INT_MAX) {
			memset(mark_stamp, 0, (size_t)G.n * sizeof(int));
			cur_mark_stamp = 1;
		}
	}
	inline void new_in_round() {
		cur_in_stamp++;
		if (cur_in_stamp == INT_MAX) {
			memset(in_stamp, 0, (size_t)G.n * sizeof(int));
			cur_in_stamp = 1;
		}
	}
	inline void new_value_round() {
		cur_mark_stamp++;
		if (cur_mark_stamp == INT_MAX) {
			memset(mark_stamp, 0, (size_t)G.n * sizeof(int));
			memset(val_stamp, 0, (size_t)G.n * sizeof(int));
			memset(inq_stamp, 0, (size_t)G.n * sizeof(int));
			cur_mark_stamp = 1;
		}
		support_size = 0;
	}
	inline void touch_value(int u) {
		if (val_stamp[u] != cur_mark_stamp) {
			val_stamp[u] = cur_mark_stamp;
			p[u] = 0;
			r[u] = 0;
		}
	}
	inline bool in_queue(int u) { return inq_stamp[u] == cur_mark_stamp; }
	inline void set_in_queue(int u) { inq_stamp[u] = cur_mark_stamp; }
	inline void clear_in_queue(int u) { inq_stamp[u] = 0; }
};

ll volume_of_set(Graph& G, int* S, int S_size) {
	ll vol = 0;
	for (int i = 0; i < S_size; i++) vol += G.deg[S[i]];
	return vol;
}

ld conductance_of_set_local(Graph& G, Workspace& ws, int* S, int S_size, ll* out_vol = NULL) {
	ws.new_in_round();
	ll vol = 0, cut = 0;
	for (int i = 0; i < S_size; i++) {
		int u = S[i];
		ws.in_stamp[u] = ws.cur_in_stamp;
		vol += G.deg[u];
	}
	for (int i = 0; i < S_size; i++) {
		int u = S[i];
		for (int j = 0; j < G.deg[u]; j++) {
			int v = G.adj[u][j];
			if (ws.in_stamp[v] != ws.cur_in_stamp) cut++;
		}
	}
	if (out_vol) *out_vol = vol;
	ll den = min(vol, G.total_vol - vol);
	if (den <= 0) return 0;
	return (ld)cut / (ld)den;
}

int compute_ppr_power(Graph& G, int seed, ld alpha, int max_iter, ld tol, Workspace& ws) {
	for (int i = 0; i < G.n; i++) ws.p[i] = ws.q[i] = 0;
	ws.p[seed] = 1;
	for (int it = 0; it < max_iter; it++) {
		for (int i = 0; i < G.n; i++) ws.q[i] = 0;
		ws.q[seed] += alpha;
		for (int u = 0; u < G.n; u++) if (ws.p[u] > FEPS && G.deg[u] > 0) {
			ld add = (1.0L - alpha) * ws.p[u] / (ld)G.deg[u];
			for (int j = 0; j < G.deg[u]; j++) ws.q[G.adj[u][j]] += add;
		}
		ld diff = 0;
		for (int i = 0; i < G.n; i++) diff += fabsl(ws.q[i] - ws.p[i]), ws.p[i] = ws.q[i];
		if (diff <= tol) return it + 1;
	}
	return max_iter;
}

int compute_ppr_push(Graph& G, int seed, ld alpha, ld eps, Workspace& ws) {
	ws.new_value_round();
	int head = 0, tail = 0, qs = 0, pushes = 0;
	ws.touch_value(seed);
	ws.r[seed] = 1;
	if (G.deg[seed] > 0 && ws.r[seed] / (ld)G.deg[seed] > eps) {
		ws.que[tail++] = seed;
		qs++;
		ws.set_in_queue(seed);
	}
	while (qs) {
		int u = ws.que[head++];
		if (head == G.n) head = 0;
		qs--;
		ws.clear_in_queue(u);
		if (!G.deg[u]) continue;
		ld ru = ws.r[u];
		if (ru / (ld)G.deg[u] <= eps) continue;
		if (ws.p[u] <= FEPS) ws.support[ws.support_size++] = u;
		ws.p[u] += alpha * ru;
		ws.r[u] = 0;
		pushes++;
		ld add = (1.0L - alpha) * ru / (ld)G.deg[u];
		for (int j = 0; j < G.deg[u]; j++) {
			int v = G.adj[u][j];
			ws.touch_value(v);
			ws.r[v] += add;
			if (G.deg[v] > 0 && !ws.in_queue(v) && ws.r[v] / (ld)G.deg[v] > eps) {
				ws.que[tail++] = v;
				if (tail == G.n) tail = 0;
				qs++;
				ws.set_in_queue(v);
			}
		}
	}
	return pushes;
}

void conductance_sweep(Graph& G, Workspace& ws, int*& best_nodes, int& best_size, ld& best_phi, int prefix_expand = 1) {
	int ord_size = 0;
	for (int i = 0; i < ws.support_size; i++) {
		int u = ws.support[i];
		if (G.deg[u] > 0 && ws.p[u] > FEPS) ws.ord[ord_size++] = { u, ws.p[u] / (ld)G.deg[u] };
	}
	sort(ws.ord, ws.ord + ord_size, cmp_score);
	ws.new_in_round();
	ll vol = 0, cut = 0;
	best_phi = 1e100L;
	best_size = 0;
	if (!ord_size) {
		best_size = 1;
		best_nodes = (int*)malloc(sizeof(int));
		best_nodes[0] = 0;
		best_phi = 0;
		return;
	}
	int* tmp_best = (int*)malloc((size_t)ord_size * sizeof(int));
	for (int i = 0; i < ord_size; i++) {
		int u = ws.ord[i].u;
		ws.in_stamp[u] = ws.cur_in_stamp;
		vol += G.deg[u];
		for (int j = 0; j < G.deg[u]; j++) {
			int v = G.adj[u][j];
			if (ws.in_stamp[v] == ws.cur_in_stamp) cut--;
			else cut++;
		}
		if (vol > 0) {
			ll den = min(vol, G.total_vol - vol);
			if (den > 0) {
				ld phi = (ld)cut / (ld)den;
				if (phi + 1e-30L < best_phi) {
					best_phi = phi;
					best_size = i + 1;
					for (int k = 0; k <= i; k++) tmp_best[k] = ws.ord[k].u;
				}
			}
		}
	}
	if (!best_size) {
		best_size = 1;
		tmp_best[0] = ws.ord[0].u;
		best_phi = 0;
	}
	int final_size = min(ord_size, max(best_size, best_size * prefix_expand));
	best_nodes = (int*)malloc((size_t)final_size * sizeof(int));
	for (int i = 0; i < final_size; i++) best_nodes[i] = ws.ord[i].u;
	best_size = final_size;
	best_phi = conductance_of_set_local(G, ws, best_nodes, best_size);
	free(tmp_best);
}

void add_one_hop_neighbors(Graph& G, Workspace& ws, int*& nodes, int& node_size, ld& phi, ll& vol) {
	ws.new_mark_round();
	int expanded_size = 0;
	for (int i = 0; i < node_size; i++) {
		int u = nodes[i];
		if (ws.mark_stamp[u] != ws.cur_mark_stamp) ws.mark_stamp[u] = ws.cur_mark_stamp, ws.que[expanded_size++] = u;
	}
	for (int i = 0; i < node_size; i++) {
		int u = nodes[i];
		for (int j = 0; j < G.deg[u]; j++) {
			int v = G.adj[u][j];
			if (ws.mark_stamp[v] != ws.cur_mark_stamp) ws.mark_stamp[v] = ws.cur_mark_stamp, ws.que[expanded_size++] = v;
		}
	}
	int* expanded = (int*)malloc((size_t)expanded_size * sizeof(int));
	memcpy(expanded, ws.que, (size_t)expanded_size * sizeof(int));
	free(nodes); nodes = expanded; node_size = expanded_size;
	phi = conductance_of_set_local(G, ws, nodes, node_size, &vol);
}

void sample_random_set(vector<int>& cand, int set_size, mt19937_64& rng, int* out) {
	check((int)cand.size() >= set_size, "set_size is larger than the candidate pool");
	unordered_set<int> used;
	used.reserve((size_t)set_size * 2 + 1);
	uniform_int_distribution<int> pick(0, (int)cand.size() - 1);
	int out_size = 0;
	while (out_size < set_size) {
		int idx = pick(rng);
		if (used.insert(idx).second) out[out_size++] = cand[idx];
	}
}

int bfs_seed_set(Graph& G, Workspace& ws, int seed, int set_size, int* out) {
	ws.new_mark_round();
	int head = 0, tail = 0, out_size = 0;
	ws.mark_stamp[seed] = ws.cur_mark_stamp;
	ws.que[tail++] = seed;
	while (head < tail && out_size < set_size) {
		int u = ws.que[head++];
		out[out_size++] = u;
		for (int j = 0; j < G.deg[u]; j++) {
			int v = G.adj[u][j];
			if (ws.mark_stamp[v] != ws.cur_mark_stamp) {
				ws.mark_stamp[v] = ws.cur_mark_stamp;
				ws.que[tail++] = v;
			}
		}
	}
	return out_size;
}

int random_walk_seed_set(Graph& G, Workspace& ws, int seed, int set_size, mt19937_64& rng, int* out, int& walk_steps) {
	ws.new_mark_round();
	int out_size = 0, u = seed;
	walk_steps = 0;
	ws.mark_stamp[u] = ws.cur_mark_stamp;
	out[out_size++] = u;
	int max_steps = max(1000, set_size * 200);
	while (out_size < set_size && walk_steps < max_steps) {
		if (G.deg[u] <= 0) break;
		uniform_int_distribution<int> pick(0, G.deg[u] - 1);
		u = G.adj[u][pick(rng)];
		walk_steps++;
		if (ws.mark_stamp[u] != ws.cur_mark_stamp) {
			ws.mark_stamp[u] = ws.cur_mark_stamp;
			out[out_size++] = u;
		}
	}
	if (out_size < set_size) {
		int head = 0, tail = 0;
		ws.que[tail++] = seed;
		while (head < tail && out_size < set_size) {
			u = ws.que[head++];
			for (int j = 0; j < G.deg[u] && out_size < set_size; j++) {
				int v = G.adj[u][j];
				if (ws.mark_stamp[v] != ws.cur_mark_stamp) {
					ws.mark_stamp[v] = ws.cur_mark_stamp;
					out[out_size++] = v;
					ws.que[tail++] = v;
				}
			}
		}
	}
	return out_size;
}

void write_query_file(char* out_address, int** queries, int* qsize, int qn) {
	FILE* out = fopen(out_address, "w");
	check(out != NULL, "Can not open output file");
	fprintf(out, "%d\n", qn);
	for (int i = 0; i < qn; i++) {
		for (int j = 0; j < qsize[i]; j++) fprintf(out, "%s%d", j ? "," : "", queries[i][j]);
		fprintf(out, "\n");
	}
	fclose(out);
}

int main(int argc, char** argv) {
	if (argc < 5) {
		printf("Usage:\n");
		printf("./main <dataset> -ppr  <query_num> <out_file> [rng_seed] [--alpha=X]\n");
		printf("./main <dataset> -appr <query_num> <out_file> [rng_seed] [--alpha=X] [--eps=X]\n");
		printf("./main <dataset> -bfs  <query_num> <out_file> [rng_seed]\n");
		printf("./main <dataset> -bfs  <query_num> <set_size> <out_file> [rng_seed]\n");
		printf("./main <dataset> -rw   <query_num> <out_file> [rng_seed]\n");
		printf("./main <dataset> -rw   <query_num> <set_size> <out_file> [rng_seed]\n");
		printf("./main <dataset> -rand <query_num> <set_size> <out_file> [rng_seed]\n");
		return 0;
	}

	bool use_push = false, use_rand = false, use_bfs = false, use_rw = false;
	if (!strcmp(argv[2], "-ppr")) use_push = false;
	else if (!strcmp(argv[2], "-appr")) use_push = true;
	else if (!strcmp(argv[2], "-bfs")) use_bfs = true;
	else if (!strcmp(argv[2], "-rw")) use_rw = true;
	else if (!strcmp(argv[2], "-rand")) use_rand = true;
	else check(false, "Unknown method flag");

	char dataset_address[256], out_address[256];
	strcpy(dataset_address, argv[1]);
	int query_num = atoi(argv[3]);
	check(query_num > 0, "Bad query number");
	int set_size = 0;
	if (use_rand) strcpy(out_address, argv[5]), set_size = atoi(argv[4]), check(set_size > 0, "Bad set_size");
	else if ((use_bfs || use_rw) && argc >= 6 && is_positive_int_string(argv[4])) {
		set_size = atoi(argv[4]);
		strcpy(out_address, argv[5]);
	} else strcpy(out_address, argv[4]), set_size = DEFAULT_LOCAL_SET_SIZE;

	unsigned long long rng_seed = 0;
	if (use_rand) rng_seed = (argc == 7 ? strtoull(argv[6], NULL, 10) : (unsigned long long)chrono::high_resolution_clock::now().time_since_epoch().count());
	else if ((use_bfs || use_rw) && argc >= 6 && is_positive_int_string(argv[4])) rng_seed = (argc == 7 ? strtoull(argv[6], NULL, 10) : (unsigned long long)chrono::high_resolution_clock::now().time_since_epoch().count());
	else rng_seed = (argc >= 6 && is_positive_int_string(argv[5]) ? strtoull(argv[5], NULL, 10) : (unsigned long long)chrono::high_resolution_clock::now().time_since_epoch().count());

	Graph G;
	Timer timer;
	timer.start();
	G.read_graph_from_dataset(dataset_address);
	timer.end();

	vector<int> cand;
	cand.reserve(G.n);
	for (int u = 0; u < G.n; u++) if (G.deg[u] >= 10) cand.push_back(u);
	bool fallback_all_vertices = false;
	if ((int)cand.size() < query_num) {
		fallback_all_vertices = true;
		cand.clear();
		for (int u = 0; u < G.n; u++) cand.push_back(u);
	}
	check((int)cand.size() >= query_num, "query_num is larger than the candidate pool");
	if (use_rand) check((int)cand.size() >= set_size, "set_size is larger than the candidate pool");
	mt19937_64 rng(rng_seed);
	shuffle(cand.begin(), cand.end(), rng);

	ld alpha = 0.05L, appr_eps = 1e-6L; int prefix_expand = 1, add_neighbors = 0;
	for (int i = 3; i < argc; i++) {
		if (!strncmp(argv[i], "--alpha=", 8)) alpha = strtold(argv[i] + 8, NULL);
		else if (!strncmp(argv[i], "--eps=", 6)) appr_eps = strtold(argv[i] + 6, NULL);
		else if (!strncmp(argv[i], "--prefix_expand=", 16)) prefix_expand = atoi(argv[i] + 16);
		else if (!strncmp(argv[i], "--add_neighbors=", 16)) add_neighbors = atoi(argv[i] + 16);
	}
	check(alpha > 0 && alpha < 1, "alpha must be in (0,1)");
	check(appr_eps > 0, "eps must be positive");
	check(prefix_expand > 0, "prefix_expand must be positive");
	const ld power_tol = 1e-15L;
	const int power_max_iter = 200;
	Workspace ws(G);

	int** queries = (int**)malloc((size_t)query_num * sizeof(int*));
	int* qsize = (int*)malloc((size_t)query_num * sizeof(int));
	ld* phi = (ld*)malloc((size_t)query_num * sizeof(ld));
	int* seed = (int*)malloc((size_t)query_num * sizeof(int));
	int* work = (int*)malloc((size_t)query_num * sizeof(int));
	double* qtime = (double*)malloc((size_t)query_num * sizeof(double));
	ll* qvol = (ll*)malloc((size_t)query_num * sizeof(ll));
	ld* qtheta = (ld*)malloc((size_t)query_num * sizeof(ld));
	double total_ppr_time = 0;
	int attempt = 0;
	int accepted = 0;
	while (accepted < query_num) {
		Timer one;
		one.start();

		int cur_seed = -1;
		if (!use_rand) cur_seed = cand[attempt];
		attempt++;

		int* cur_query = NULL;
		int cur_size = 0, cur_work = 0;
		ld cur_phi = 0;
		ll cur_vol = 0;

		if (use_rand) {
			cur_size = set_size;
			cur_query = (int*)malloc((size_t)cur_size * sizeof(int));
			sample_random_set(cand, set_size, rng, cur_query);
			cur_phi = conductance_of_set_local(G, ws, cur_query, cur_size, &cur_vol);
		} else if (use_bfs) {
			cur_size = set_size;
			cur_query = (int*)malloc((size_t)cur_size * sizeof(int));
			cur_size = bfs_seed_set(G, ws, cur_seed, set_size, cur_query);
			cur_work = cur_size;
			cur_phi = conductance_of_set_local(G, ws, cur_query, cur_size, &cur_vol);
		} else if (use_rw) {
			cur_size = set_size;
			cur_query = (int*)malloc((size_t)cur_size * sizeof(int));
			cur_size = random_walk_seed_set(G, ws, cur_seed, set_size, rng, cur_query, cur_work);
			cur_phi = conductance_of_set_local(G, ws, cur_query, cur_size, &cur_vol);
		} else {
			if (!use_push) {
				cur_work = compute_ppr_power(G, cur_seed, alpha, power_max_iter, power_tol, ws);
				int ord_size = 0;
				for (int u = 0; u < G.n; u++) if (G.deg[u] > 0) ws.ord[ord_size++] = { u, ws.p[u] / (ld)G.deg[u] };
				sort(ws.ord, ws.ord + ord_size, cmp_score);
				ws.new_in_round();
				ll vol = 0, cut = 0;
				ld best_phi = 1e100L;
				int best_size = 0;
				int* tmp_best = (int*)malloc((size_t)max(1, ord_size) * sizeof(int));
				for (int i = 0; i < ord_size; i++) {
					int u = ws.ord[i].u;
					ws.in_stamp[u] = ws.cur_in_stamp;
					vol += G.deg[u];
					for (int j = 0; j < G.deg[u]; j++) {
						int v = G.adj[u][j];
						if (ws.in_stamp[v] == ws.cur_in_stamp) cut--;
						else cut++;
					}
					if (vol > 0) {
						ll den = min(vol, G.total_vol - vol);
						if (den > 0) {
							ld now_phi = (ld)cut / (ld)den;
							if (now_phi + 1e-30L < best_phi) {
								best_phi = now_phi;
								best_size = i + 1;
								for (int k = 0; k <= i; k++) tmp_best[k] = ws.ord[k].u;
							}
						}
					}
				}
				if (!best_size) {
					best_size = 1;
					tmp_best[0] = ord_size ? ws.ord[0].u : 0;
					best_phi = 0;
				}
				cur_size = best_size;
				cur_query = (int*)malloc((size_t)cur_size * sizeof(int));
				memcpy(cur_query, tmp_best, (size_t)cur_size * sizeof(int));
				free(tmp_best);
				cur_phi = best_phi;
				cur_vol = volume_of_set(G, cur_query, cur_size);
			} else {
				cur_work = compute_ppr_push(G, cur_seed, alpha, appr_eps, ws);
				conductance_sweep(G, ws, cur_query, cur_size, cur_phi, prefix_expand);
				cur_vol = volume_of_set(G, cur_query, cur_size);
			}
		}
		if (add_neighbors && !use_rand && !use_bfs && !use_rw) add_one_hop_neighbors(G, ws, cur_query, cur_size, cur_phi, cur_vol);

		ld cur_theta = (G.total_vol > cur_vol) ? (ld)cur_vol / (ld)(G.total_vol - cur_vol) : numeric_limits<ld>::infinity();
		one.end();

		queries[accepted] = cur_query;
		qsize[accepted] = cur_size;
		phi[accepted] = cur_phi;
		seed[accepted] = cur_seed;
		work[accepted] = cur_work;
		qtime[accepted] = one.time();
		qvol[accepted] = cur_vol;
		qtheta[accepted] = cur_theta;
		total_ppr_time += qtime[accepted];
		accepted++;
		fprintf(stderr, "accepted=%d seed=%d size=%d vol=%lld pushes=%d time=%.6lf\n", accepted, cur_seed, cur_size, cur_vol, cur_work, qtime[accepted - 1]);
		fflush(stderr);
	}

	write_query_file(out_address, queries, qsize, query_num);

	printf("- %-20s: %s\n", "Dataset address", dataset_address);
	printf("- %-20s: %d, %d\n", "|E|, |V|", G.m, G.n);
	printf("- %-20s: %d\n", "Header n", G.header_n);
	printf("- %-20s: %s\n", "Method", use_rand ? "uniform random set" : (use_bfs ? "BFS seed set" : (use_rw ? "random-walk seed set" : (use_push ? "local-push APPR" : "power-iteration PPR"))));
	printf("- %-20s: %d\n", "#queries", query_num);
	printf("- %-20s: %llu\n", "RNG seed", rng_seed);
	printf("- %-20s: %.6lf\n", "Read graph time", timer.time());
	printf("- %-20s: %s\n", "Seed degree filter", fallback_all_vertices ? "fallback to all vertices" : "deg(seed) >= 10");
	if (use_rand || use_bfs || use_rw) printf("- %-20s: %d\n", "Target set size", set_size);
	if (!use_rand && !use_bfs && !use_rw && !use_push) {
		printf("- %-20s: %.2Lf\n", "PPR alpha", alpha);
		printf("- %-20s: %d\n", "PPR max iter", power_max_iter);
		printf("- %-20s: %.2Le\n", "PPR tol", power_tol);
	} else if (!use_rand && !use_bfs && !use_rw) {
		printf("- %-20s: %.2Lf\n", "PPR alpha", alpha);
		printf("- %-20s: %.2Le\n", "APPR eps", appr_eps);
		printf("- %-20s: %d\n", "Prefix expand", prefix_expand);
		printf("- %-20s: %d\n", "Add neighbors", add_neighbors);
	}
	printf("- %-20s: %.6lf\n", "Total query time", total_ppr_time);
	for (int i = 0; i < query_num; i++) {
		if (!use_rand && !use_bfs && !use_rw) printf("- query %-12d seed=%d, |R|=%d, vol(R)=%lld, theta_R=%.12Lf, conductance=%.12Lf, pushes=%d, time=%.6lf\n", i + 1, seed[i], qsize[i], qvol[i], qtheta[i], phi[i], work[i], qtime[i]);
		else if (use_bfs) printf("- query %-12d seed=%d, |R|=%d, vol(R)=%lld, theta_R=%.12Lf, conductance=%.12Lf, reached=%d, time=%.6lf\n", i + 1, seed[i], qsize[i], qvol[i], qtheta[i], phi[i], work[i], qtime[i]);
		else if (use_rw) printf("- query %-12d seed=%d, |R|=%d, vol(R)=%lld, theta_R=%.12Lf, conductance=%.12Lf, walk_steps=%d, time=%.6lf\n", i + 1, seed[i], qsize[i], qvol[i], qtheta[i], phi[i], work[i], qtime[i]);
		else printf("- query %-12d seed=%d, |R|=%d, vol(R)=%lld, theta_R=%.12Lf, conductance=%.12Lf, time=%.6lf\n", i + 1, seed[i], qsize[i], qvol[i], qtheta[i], phi[i], qtime[i]);
	}
	printf("- %-20s: %s\n", "Output file", out_address);

	for (int i = 0; i < query_num; i++) free(queries[i]);
	free(queries);
	free(qsize);
	free(phi);
	free(seed);
	free(work);
	free(qtime);
	free(qvol);
	free(qtheta);
	return 0;
}
