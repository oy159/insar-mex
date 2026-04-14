#include "mcf.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <limits>
#include <iostream>
#include <queue>

namespace mcf {

namespace {

	const double PI = utils::PI;

	double wrap_to_pi(double x) {
		return std::atan2(std::sin(x), std::cos(x));
	}

	double wrap_diff(double a, double b) {
		double d = std::fmod(a - b + PI, 2 * PI);
		if (d < 0) d += 2 * PI;
		return d - PI;
	}

	double second_diff(double a, double b, double c) {
		return wrap_diff(wrap_diff(c, b), wrap_diff(b, a));
	}

	int select_node(int r, int c, int max_r, int max_c, int bnd, int M_minus_1) {
		if (r >= 0 && r < max_r && c >= 0 && c < max_c) {
			return c * max_r + r; // D(r,c) column-major, 0-based
		}
		return bnd;
	}

	void dijkstra_sp(
		const std::vector<std::vector<int>>& adj,
		const std::vector<std::pair<int, int>>& edges,
		const std::vector<int>& costs,
		const std::vector<int>& flow,
		const std::vector<int>& potential,
		int num_nodes,
		int src,
		std::vector<long long>& dist,
		std::vector<int>& pred_edge)
	{
		dist.assign(num_nodes, std::numeric_limits<long long>::max());
		pred_edge.assign(num_nodes, -1);
		std::vector<bool> visited(num_nodes, false);

		dist[src] = 0;

		// 采用优先队列进行优化的 Dijkstra
		using P = std::pair<long long, int>;
		std::priority_queue<P, std::vector<P>, std::greater<P>> pq;
		pq.push({0, src});

		while (!pq.empty()) {
			auto [d, u] = pq.top();
			pq.pop();

			if (visited[u]) continue;
			visited[u] = true;

			for (int e : adj[u]) {
				int v = edges[e].second;
				if (visited[v]) continue;

				long long rc = (long long)costs[e] - potential[u] + potential[v];
				if (rc < 0) rc = 0;

				if (dist[u] + rc < dist[v]) {
					dist[v] = dist[u] + rc;
					pred_edge[v] = e;
					pq.push({dist[v], v});
				}
			}
		}
	}

	void augment_path(std::vector<int>& flow,
		const std::vector<std::pair<int, int>>& edges,
		const std::vector<int>& rev_edge,
		const std::vector<int>& pred_edge,
		int s, int t, int amount) {
		int v = t;
		while (v != s) {
			int e = pred_edge[v];
			if (e == -1) break;
			flow[e] += amount;
			flow[rev_edge[e]] -= amount;
			v = edges[e].first;
		}
	}

} // namespace

std::unique_ptr<utils::MatrixD> Method::unwrap(const utils::MatrixD& wrappedMap) {
	int M = wrappedMap.rows();
	int N = wrappedMap.cols();

	auto unwrappedMap = std::make_unique<utils::MatrixD>(M, N, 0.0);
	if (M <= 1 || N <= 1) return unwrappedMap;

	const double* wrapped_phase = wrappedMap.data().data();
	double* unwrapped_phase = unwrappedMap->data().data();

	// 1. Phase Gradients
	std::vector<double> dh(M * (N - 1));
	std::vector<double> dv((M - 1) * N);

	for (int r = 0; r < M; ++r) {
		for (int c = 0; c < N - 1; ++c) {
			dh[r * (N - 1) + c] = wrap_to_pi(wrapped_phase[r * N + c + 1] - wrapped_phase[r * N + c]);
		}
	}
	for (int r = 0; r < M - 1; ++r) {
		for (int c = 0; c < N; ++c) {
			dv[r * N + c] = wrap_to_pi(wrapped_phase[(r + 1) * N + c] - wrapped_phase[r * N + c]);
		}
	}

	// 2. Residues
	std::vector<int> R((M - 1) * (N - 1));
	int total_abs_R = 0;
	int sum_R = 0;

    if (debugMode) {
        debugData.residues = std::make_unique<utils::MatrixD>(M, N, 0.0);
    }

	for (int r = 0; r < M - 1; ++r) {
		for (int c = 0; c < N - 1; ++c) {
			double curl = dh[r * (N - 1) + c] - dh[(r + 1) * (N - 1) + c]
				+ dv[r * N + c + 1] - dv[r * N + c];
			R[r * (N - 1) + c] = std::round(curl / (2 * PI));
			total_abs_R += std::abs(R[r * (N - 1) + c]);
			sum_R += R[r * (N - 1) + c];

            if (debugMode) {
                (*debugData.residues)(r, c) = R[r * (N - 1) + c];
            }
		}
	}

	// 3. Build Dual-Grid Cost Network
	int num_cells = (M - 1) * (N - 1);
	int num_nodes = num_cells + 1;
	int bnd = num_cells;

	std::vector<int> supply(num_nodes, 0);
	for (int c = 0; c < N - 1; ++c) {
		for (int r = 0; r < M - 1; ++r) {
			int D_rc = c * (M - 1) + r;
			supply[D_rc] = -R[r * (N - 1) + c];
		}
	}
	supply[bnd] = sum_R;

	// Reliability Map
	std::vector<double> rel(M * N, 0.0);
	double max_D = 0.0;
	std::vector<double> D_val(M * N, 0.0);

	for (int i = 1; i < M - 1; ++i) {
		for (int j = 1; j < N - 1; ++j) {
			double d1 = second_diff(wrapped_phase[i * N + j - 1], wrapped_phase[i * N + j], wrapped_phase[i * N + j + 1]);
			double d2 = second_diff(wrapped_phase[(i - 1) * N + j], wrapped_phase[i * N + j], wrapped_phase[(i + 1) * N + j]);
			double d3 = second_diff(wrapped_phase[(i - 1) * N + j - 1], wrapped_phase[i * N + j], wrapped_phase[(i + 1) * N + j + 1]);
			double d4 = second_diff(wrapped_phase[(i - 1) * N + j + 1], wrapped_phase[i * N + j], wrapped_phase[(i + 1) * N + j - 1]);

			D_val[i * N + j] = d1 * d1 + d2 * d2 + d3 * d3 + d4 * d4;
			max_D = std::max(max_D, D_val[i * N + j]);
		}
	}
	max_D += 1e-12; // eps

	for (int i = 1; i < M - 1; ++i) {
		for (int j = 1; j < N - 1; ++j) {
			rel[i * N + j] = 1.0 - D_val[i * N + j] / max_D;
		}
	}

	// Border pixels median handling simplified for brevity
	double brd_rel = 0.5; // Simplified median
	for(int i = 0; i < M; ++i) { rel[i*N] = brd_rel; rel[i*N + N - 1] = brd_rel; }
	for(int j = 0; j < N; ++j) { rel[j] = brd_rel; rel[(M-1)*N + j] = brd_rel; }

    if (debugMode) {
        debugData.reliability = std::make_unique<utils::MatrixD>(M, N, 0.0);
        for(int i = 0; i < M; ++i) {
            for(int j = 0; j < N; ++j) {
                (*debugData.reliability)(i, j) = rel[i * N + j];
            }
        }
    }

	int max_cost = 100;
	int E_h = M * (N - 1);
	int E_v = (M - 1) * N;
	int E = E_h + E_v;

	std::vector<std::pair<int, int>> edges(2 * E);
	std::vector<int> costs(2 * E);

	struct EdgeInfo { int r, c, type; };
	std::vector<EdgeInfo> edge_info(E);

	int idx = 0;
	for (int r = 0; r < M; ++r) {
		for (int c = 0; c < N - 1; ++c) {
			int node_below = select_node(r, c, M - 1, N - 1, bnd, M - 1);
			int node_above = select_node(r - 1, c, M - 1, N - 1, bnd, M - 1);
			edges[idx] = {node_below, node_above};
			costs[idx] = std::max(1, (int)std::round(max_cost * std::min(rel[r * N + c], rel[r * N + c + 1])));
			edge_info[idx] = {r, c, 0};
			idx++;
		}
	}
	for (int r = 0; r < M - 1; ++r) {
		for (int c = 0; c < N; ++c) {
			int node_left = select_node(r, c - 1, M - 1, N - 1, bnd, M - 1);
			int node_right = select_node(r, c, M - 1, N - 1, bnd, M - 1);
			edges[idx] = {node_left, node_right};
			costs[idx] = std::max(1, (int)std::round(max_cost * std::min(rel[r * N + c], rel[(r + 1) * N + c])));
			edge_info[idx] = {r, c, 1};
			idx++;
		}
	}

	for (int i = 0; i < E; ++i) {
		edges[E + i] = {edges[i].second, edges[i].first};
		costs[E + i] = costs[i];
	}

	// 4. Solve MCF
	int num_edges = 2 * E;
	std::vector<int> flow(num_edges, 0);

	if (total_abs_R > 0) {
		std::vector<int> rev_edge(num_edges);
		for (int i = 0; i < E; ++i) {
			rev_edge[i] = E + i;
			rev_edge[E + i] = i;
		}

		std::vector<int> rem_supply = supply;
		std::vector<int> sinks;
		for (int i = 0; i < num_nodes; ++i) {
			if (rem_supply[i] < 0) sinks.push_back(i);
		}

		std::vector<std::vector<int>> adj(num_nodes);
		for (int e = 0; e < num_edges; ++e) {
			adj[edges[e].first].push_back(e);
		}

		std::vector<int> potential(num_nodes, 0);
		int max_iter = sum_R + total_abs_R + 1; // upper bound

		for (int iter = 0; iter < max_iter; ++iter) {
			int s = -1;
			for (int i = 0; i < num_nodes; ++i) {
				if (rem_supply[i] > 0) { s = i; break; }
			}
			if (s == -1) break;

			std::vector<long long> dist;
			std::vector<int> pred_edge;
			dijkstra_sp(adj, edges, costs, flow, potential, num_nodes, s, dist, pred_edge);

			int t = -1;
			long long best_dist = std::numeric_limits<long long>::max();
			for (int sink : sinks) {
				if (rem_supply[sink] < 0 && dist[sink] < best_dist) {
					best_dist = dist[sink];
					t = sink;
				}
			}
			if (t == -1) break;

			int send = std::min(rem_supply[s], -rem_supply[t]);
			augment_path(flow, edges, rev_edge, pred_edge, s, t, send);

			rem_supply[s] -= send;
			rem_supply[t] += send;

			for (int i = 0; i < num_nodes; ++i) {
				if (dist[i] != std::numeric_limits<long long>::max()) {
					potential[i] += dist[i];
				}
			}
		}
	}

	// 5. Apply Corrections
	for (int e = 0; e < E; ++e) {
		int f = flow[e];
		if (f != 0) {
			int r = edge_info[e].r;
			int c = edge_info[e].c;
			if (edge_info[e].type == 0) {
				dh[r * (N - 1) + c] += f * 2 * PI;
			} else {
				dv[r * N + c] += f * 2 * PI;
			}
		}
	}

	// 6. Integrate Gradients
	for(int i = 0; i < M * N; ++i) unwrapped_phase[i] = 0;

	int r0 = 0, c0 = 0;
	unwrapped_phase[r0 * N + c0] = wrapped_phase[r0 * N + c0];

	for (int c = c0 + 1; c < N; ++c) {
		unwrapped_phase[r0 * N + c] = unwrapped_phase[r0 * N + c - 1] + dh[r0 * (N - 1) + c - 1];
	}

	for (int r = r0 + 1; r < M; ++r) {
		unwrapped_phase[r * N + c0] = unwrapped_phase[(r - 1) * N + c0] + dv[(r - 1) * N + c0];
		for (int c = c0 + 1; c < N; ++c) {
			unwrapped_phase[r * N + c] = unwrapped_phase[r * N + c - 1] + dh[r * (N - 1) + c - 1];
		}
	}

	return unwrappedMap;
}

} // namespace mcf
