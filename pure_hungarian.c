#include "pure_hungarian.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>

void set_all(int *arr, int arr_size, int value) {
	for (int i = 0; i < arr_size; i++) {
		arr[i] = value;
	}
}

int double_eq(double d1, double d2) {
	return fabs(d1 - d2) < 0.000001;
}

void alternate_pairs_in_path(int *curr_path, int last_elem_idx,
			     int *edge_r_to_c, int *edge_c_to_r) {

//	printf("Alternate path\n");
//	for (int i = 0; i < last_elem_idx + 1; i++) {
//		printf("%d ", curr_path[i]);
//	}
//	printf("\n");

	printf("Before alternate\n");
	printf("r_to_c\n");
	print_array(edge_r_to_c);
	printf("c_to_r\n");
	print_array(edge_c_to_r);

	assert(last_elem_idx == 1 || last_elem_idx / 2 == 1);

	bool include = true;
	for (int i = 0; i < last_elem_idx; i += 2) {
		if (include) {
			// do include for two elem
			int r = curr_path[i];
			int c = curr_path[i+1];

			edge_r_to_c[r] = c;

			edge_r_to_c[edge_c_to_r[c]] = -1;
			edge_c_to_r[c] = r;

			include = false;
		} else {
			// do exclude for two elems
			// actuall excluding was on the previous stage
			include = true;
		}
	}

	printf("After alternate\n");
	printf("r_to_c\n");
	print_array(edge_r_to_c);
	printf("c_to_r\n");
	print_array(edge_c_to_r);
}

// Tries to find non-saturated vertex from the second half.
// Returns zero if non-saturated vertex from second half was not found. In that
// case clean ups last element in curr_path and decreases it's curr_path_idx.

// Returns non-zero if non-saturated vertex from second half is found. In that
// case curr_path and curr_path_idx are not changed.
int DFS(int idx, char r_or_c, int *curr_path, int *curr_path_idx,
	const int *edge_r_to_c, const int *edge_c_to_r,
	int *is_r_visited, int *is_c_visited,
	int *r_visited_overall, int *c_visited_overall,
	const int n, double *u, double *v, double **m) {

	assert(r_or_c == 'c' || r_or_c == 'r');

//	printf("call to DFS\n");
//	printf("curr path idx %d\n", *curr_path_idx);

//	for (int i = 0; i < *curr_path_idx + 1; i++) {
//		printf("%d ", curr_path[i]);
//	}
//	printf("\n");

	// 'idx' is in second half
	if (r_or_c == 'c') {
		int c = idx;

		curr_path[*curr_path_idx] = c;
		*curr_path_idx += 1;
		is_c_visited[c] = true;
		c_visited_overall[c] = true;

		// reached non-saturated vertex from the second half
		if (edge_c_to_r[c] == -1) {
//			printf("reach non saturated\n");
			return 1;
		}

		// reached saturated vertex from the second half
		// run DFS for single one possible 'r'
		int r = edge_c_to_r[c];

		if (!is_r_visited[r] && double_eq(u[r] + v[c], m[r][c])) {
			if (DFS(r, 'r', curr_path, curr_path_idx,
				edge_r_to_c, edge_c_to_r,
				is_r_visited, is_c_visited,
				r_visited_overall, c_visited_overall,
				n, u, v, m)) {
				return 1;
			}
		}

		// clean up after unsuccessful DFS run
		*curr_path_idx -= 1;
		curr_path[*curr_path_idx] = -1;
		is_c_visited[c] = false;

		return 0;
	}

	// 'idx' is in first half
	if (r_or_c == 'r') {
		int r = idx;

		curr_path[*curr_path_idx] = r;
		*curr_path_idx += 1;
		is_r_visited[r] = true;
		r_visited_overall[r] = true;

		for (int c = 0; c < n; c++) {
			if (is_c_visited[c])
				continue;

			// if edge is solid
			if (!is_c_visited[c] && double_eq(u[r] + v[c], m[r][c])) {
				if (DFS(c, 'c', curr_path, curr_path_idx,
					edge_r_to_c, edge_c_to_r,
					is_r_visited, is_c_visited,
					r_visited_overall, c_visited_overall,
					n, u, v, m)) {
					return 1;
				}
			}
		}

		// clean up after unsuccessful DFS run
		*curr_path_idx -= 1;
		curr_path[*curr_path_idx] = -1;
		is_r_visited[r] = false;

		return 0;
	}
}

int ** hungarian_assignment(double **m, int n, int n_pairs_needed) {
	// init potential
	double *u = (double*) malloc(n * sizeof(double));
	double *v = (double*) malloc(n * sizeof(double));
	for (int i = 0; i < n; i++) {
		u[i] = 0.0;
		v[i] = 0.0;
	}

	// init result
	int n_pairs_found = 0;

	// dict (implemented as arrays)
	int *edge_r_to_c = (int*) calloc((size_t)n, sizeof(int));
	set_all(edge_r_to_c, n, -1);
	int *edge_c_to_r = (int*) calloc((size_t)n, sizeof(int));
	set_all(edge_c_to_r, n, -1);

	int *is_r_visited = (int*) calloc((size_t)n, sizeof(int));
	int *is_c_visited = (int*) calloc((size_t)n, sizeof(int));

	int *r_visited_overall = (int *) calloc((size_t)n, sizeof(int));
	int *c_visited_overall = (int *) calloc((size_t)n, sizeof(int));

	int *curr_path = (int*) calloc((size_t)(n * 2), sizeof(int));
	set_all(curr_path, n * 2, -1);

	int init = 0;
	int *curr_path_idx = &init;

	while (n_pairs_found < n_pairs_needed) {
		// try to find new pair
		for (int r = 0; r < n; r++) {
			// start DFS from each of non-saturated
			// vertexes of the first half
			if (edge_r_to_c[r] == -1) {
				if (DFS(r, 'r', curr_path, curr_path_idx,
					edge_r_to_c, edge_c_to_r,
					is_r_visited, is_c_visited,
					r_visited_overall, c_visited_overall,
					n, u, v, m)) {

					alternate_pairs_in_path(curr_path,
								*curr_path_idx - 1, edge_r_to_c,
								edge_c_to_r);
					n_pairs_found++;
					if (n_pairs_found == n_pairs_needed)
						goto finish;
				}
			}
			// clean up 'is visited' arrays
			set_all(curr_path, 2 * n, 0);
			*curr_path_idx = 0;
			set_all(is_r_visited, n, 0);
			set_all(is_c_visited, n, 0);
		}

		// update potential
		double delta = DBL_MAX;
		for (int r = 0; r < n; r++) {
			if (!r_visited_overall[r])
				continue;

			for (int c = 0; c < n; c++) {
				if (c_visited_overall[c])
					continue;

				double d = m[r][c] - u[r] - v[c];
				delta = (d < delta) ? d : delta;
			}
		}

		printf("new delta %f\n", delta);

		for (int i = 0; i < n; i++) {
			if (r_visited_overall[i])
				u[i] += delta;

			if (c_visited_overall[i])
				v[i] -= delta;
		}

		set_all(r_visited_overall, n, 0);
		set_all(c_visited_overall, n, 0);
	}

	// TODO make sure you clear up everything
	finish:
	for (int i = 0; i < n; i++) {
		free(m[i]);
	}
	free(u);
	free(v);


	int **optimal_pairs = (int **) malloc(n_pairs_needed * sizeof(int *));
	for (int i = 0; i < n_pairs_needed; i++) {
		optimal_pairs[i] = (int *) malloc(2 * sizeof(int));
	}

	int curr_pair = 0;
	for (int r = 0; r < n; r++) {
		int c = edge_r_to_c[r];
		if (c != -1) {
			optimal_pairs[curr_pair][0] = r;
			optimal_pairs[curr_pair][1] = c;
			curr_pair++;
		}
	}

	return optimal_pairs;

//	int **res = (int**) malloc(n * sizeof(int *));
//	for (int i = 0; i < n; i++) {
//		m[i] = (int*) malloc(n * sizeof(int));
//	}
//
//	// prepare result for print
//	for (int r = 0; r < n; r++) {
//		if (edge_r_to_c[r] != -1) {
//			int c = edge_r_to_c[r];
//			res[r][c] = 0;
//		}
//	}
//
//	for (int c = 0; c < n; c++) {
//		if (edge_c_to_r[c] != -1) {
//			int r = edge_c_to_r[c];
//			res[r][c] = 0;
//		}
//	}
//
//	for (int r = 0; r < n; r++) {
//		for (int c = 0; c < n; c++) {
//			if (res[r][c] != 0) {
//				res[r][c] = -1;
//			}
//		}
//	}

//	return res;
}