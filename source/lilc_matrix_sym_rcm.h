// -*- mode: c++ -*-
#ifndef _LILC_MATRIX_SYM_RCM_H_
#define _LILC_MATRIX_SYM_RCM_H_

template <class el_type>
struct by_degree {
	lilc_matrix<el_type>* A;
	by_degree(lilc_matrix<el_type>* mat) : A(mat) {}
	bool operator()(int const &a, int const &b) const { 
		int deg_a = A->list[a].size() + A->m_idx[a].size();
		int deg_b = A->list[b].size() + A->m_idx[b].size();
		if (deg_a == deg_b) return a < b;
		return deg_a < deg_b;
	}
};

template<class el_type> 
void lilc_matrix<el_type> :: sym_rcm(vector<int>& perm) {
	int i, s;
	vector<bool> visited(m_n_cols, false);
	vector<int> lvl_set;
	for (i = 0; i < m_n_cols; i++) {
		if (visited[i]) continue;
		
		lvl_set.clear();
		s = i;
		find_root(s);
		lvl_set.push_back(s);
		perm.push_back(s);
		
		by_degree<el_type> sorter(this);
		
		visited[s] = true;
		while (find_level_set(lvl_set, visited)) {
			sort(lvl_set.begin(), lvl_set.end(), sorter);
			for (auto it = lvl_set.begin(); it != lvl_set.end(); it++) {
				perm.push_back(*it);
			}
		}
	}
	
	reverse(perm.begin(), perm.end());
}

#endif