#ifndef _LILC_MATRIX_ILDL_H_
#define _LILC_MATRIX_ILDL_H_

using std::cout;
using std::endl;

template <class el_type>
void lilc_matrix<el_type> :: ildl(lilc_matrix<el_type>& L, block_diag_matrix<el_type>& D, idx_vector_type& perm, int lfil, double tol)
{	
	const double alpha = (1+sqrt(17))/8;  //for use in pivoting.
	el_type w1, wr, d1(0), dr(0), det_D, D_inv11, D_inv22, D_inv12, l_11, l_12;
	
	//L.list is a deque of linked lists that gives the non-zero elements in each row of L. 
	//since at any time we may swap between two rows, we require a linked lists for each row of L. 
	//A deque is used as it might be desirable to deallocate all linked lists for rows i < k on step k 
	//(this is currently not done, as the memory used in maintaining linked lists for all rows is not much).
	
	const int ncols = n_cols();
	//work is a work vector for the current column. L.first is a linked list that gives the first nonzero element in column k with row index i > k. (i.e. the first nonzero in L(k+1:n, k).
	elt_vector_type work(ncols, 0), temp(ncols, 0), col_k, col_r;
	idx_vector_type curr_nnzs, temp_nnzs, col_k_nnzs, col_r_nnzs, all_swaps;  //non-zeros on current col.
	vector<bool> in_set(ncols, 0);
	std::pair<idx_it, elt_it> its_k, its_r;
	vector<idx_it> swapk, swapr;
	vector<list_it> swapk_, swapr_;
	
	int count = 0; //the current non-zero in L.
	int i, j, k, r, offset, col_size, col_size2(1);
	bool size_two_piv = false;

	L.resize(ncols, ncols);
	curr_nnzs.reserve(ncols); //makes sure that there is enough space if every element in the column is nonzero
	L.list.resize(ncols ); //allocate a vector of size n for Llist.
	D.resize(ncols ); 
	
	for (i = 0; i < ncols; i++) {
		L.m_idx[i].resize(lfil+1);
		L.m_x[i].resize(lfil+1);
	}
	
	for (k = 0; k < ncols; k++) {
		// if (k == 6) cout << endl << endl;
		// if (k == 8) {cout << L; return;}
		
		size_two_piv = false;
		
		//zero out work vector
		std::fill (work.begin() + k, work.end(), 0);
		curr_nnzs.clear();
		
		col_size = 1;
		
		//future self: remember you need m_idx[k]+1 only if there is a diag elem in A.col(k)
		if (m_idx[k].size() > 0) {
			offset = 0;
			//offset = (m_idx[k][0] == k ? 1 : 0);
			curr_nnzs.assign (m_idx[k].begin()+offset, m_idx[k].end());
			
			//assigns the non zeros in A(k,:) to the work vector. since only the lower diagonal of A is stored, this is essentially A(k,k+1:n).
			for (j = 0; j < (int) curr_nnzs.size(); j++) {
				work[curr_nnzs[j]] = m_x[k][j+offset];
			}
		}
		
		// d1 = coeff(k,k);
		// for (auto it = L.list[k].begin(); it != L.list[k].end(); it++) { 
			// offset = L.first[*it];
			// d1 -= L.m_x[*it][offset] * D[*it] * L.m_x[*it][offset];	//update diagonal
		// }

		//cout << "bisect 1" << endl;

			//--------------begin pivoting--------------//
			//cout << work << endl;
			//cout << curr_nnzs << endl;
			update(k, work, curr_nnzs, L, D, in_set, true);
			
			//cout << "after updates for work" << endl;
			//cout << work << endl;
			//cout << curr_nnzs << endl;
			
			d1 = work[k];
			work[k] = 0;
			
			w1 = max(work, curr_nnzs, r);
			

			//cout << "bisect 1.1" << endl;
			if (w1 == 0) {
				//case 0: do nothing. pivot is k.
			} else if (std::abs(d1) >= alpha * w1 ) {
				//case 1: do nothing. pivot is k.
			} else {
				std::fill (temp.begin() + k, temp.end(), 0);
				temp_nnzs.clear();
				
           //TODO: should it be coeff(r,k)?
				// temp[k] = coeff(r,k);
				// for (auto it = L.list[k].begin(); it != L.list[k].end(); it++) { 
					// offset = L.first[*it];
					// temp[k] -= L.coeff(r, *it) * D[*it] * L.m_x[*it][offset];
				// }
				
				offset = 0;
				//offset = (list[r][0] == k ? 1 : 0);
				for (j = offset; j < (int) list[r].size(); j++) {
					temp_nnzs.push_back(list[r][j]);
					temp[list[r][j]] = coeff(r, list[r][j]);
				}
				
				unordered_inplace_union(temp_nnzs, m_idx[r].begin(), m_idx[r].end(), in_set);
				
				for (j = 0; j < (int) m_idx[r].size(); j++) {
					temp[m_idx[r][j]] = m_x[r][j];
				}
				
				//cout << temp << endl;
				//cout << temp_nnzs << endl;
				update(r, temp, temp_nnzs, L, D, in_set, true);
				//cout << "after updates for temp" << endl;
				//cout << temp << endl;
				//cout << temp_nnzs << endl;
				
				dr = temp[r];
				temp[r] = 0;
				
				wr = max(temp, temp_nnzs, j);
				


				if (std::abs(d1 * wr)>= alpha*w1*w1) {
					//case 2: do nothing. pivot is k.
					
				} else if (std::abs(dr) >= std::abs(alpha * wr)) {
					//case 3: pivot is k with r: 1x1 pivot case.
					//cout << "case 3! " << k << " " << r << endl;
					temp[r] = dr;
					work[k] = d1;
					
					//TODO: needed?
					//advance_list(k);
					
					//--------pivot A and L ---------//
					pivot(swapk, swapr, swapk_, swapr_, all_swaps, in_set, col_k, col_k_nnzs, col_r, col_r_nnzs, L, k, r);
					
					//cout << "bisect pre-piv" << endl;
					//----------pivot rest ----------//
					std::swap(d1, dr);
					
					//permute perm
					std::swap(perm[k], perm[r]);
					
					work.swap(temp);	//swap work with temp.
					std::swap(work[k], work[r]);
					
					curr_nnzs.swap(temp_nnzs);	//swap curr_nnzs with temp_nnzs
					
					//cout << "bisect post-piv" << endl;
					//check if this is working.
					safe_swap(curr_nnzs, k, r);
					//cout << "bisect post-piv" << endl;
					//--------end pivot rest---------//
					



				} else {
					//case 4: pivot is k+1 with r: 2x2 pivot case.
					//cout << "case 4!" << k+1 << " " << r << endl;

					//TODO: needed?
					//advance_list(k+1);
					
					temp[r] = dr;
					work[k] = d1;

					size_two_piv = true;
					
					if (k+1 != r) {
						pivot(swapk, swapr, swapk_, swapr_, all_swaps, in_set, col_k, col_k_nnzs, col_r, col_r_nnzs, L, k+1, r);
						

						//----------pivot rest ----------//
						
						//permute perm
						std::swap(perm[k+1], perm[r]);
						
						//swap two cols of L
						std::swap(work[k+1], work[r]);
						std::swap(temp[k+1], temp[r]);
						
						safe_swap(curr_nnzs, k+1, r);
						safe_swap(temp_nnzs, k+1, r);
					}
				}
			}
			//--------------end pivoting--------------//
			//update lfirst
			L.advance_first(k);
			
			//! danger below
			advance_list(k);
			
			if (size_two_piv) {
				advance_list(k+1);
			}
			//!
			
			curr_nnzs.erase(std::remove(curr_nnzs.begin(), curr_nnzs.end(), k), curr_nnzs.end());
				
			//performs the dual dropping procedure.
			drop_tol(work, curr_nnzs, lfil, tol);
			col_size += std::min(lfil, (int) curr_nnzs.size());
			
			if (size_two_piv) {
				temp_nnzs.erase(std::remove(temp_nnzs.begin(), temp_nnzs.end(), k), temp_nnzs.end());

				curr_nnzs.erase(std::remove(curr_nnzs.begin(), curr_nnzs.end(), k+1), curr_nnzs.end());
				temp_nnzs.erase(std::remove(temp_nnzs.begin(), temp_nnzs.end(), k+1), temp_nnzs.end());
				
				
				drop_tol(temp, temp_nnzs, lfil, tol);
				
				unordered_inplace_union(curr_nnzs, temp_nnzs.begin(), temp_nnzs.end(), in_set);
				
				//col_size2 = 1+std::min(lfil, (int) temp_nnzs.size());
				col_size = std::min(lfil, (int) curr_nnzs.size());
			}
			
			
		
		
		//cout << "bisect 2" << endl;
		
		D[k] = d1;
		
		L.m_x[k][0] = 1;
		L.m_idx[k][0] = k;
		count++;
		if (!size_two_piv) {

			if (k < ncols - 1)
			for (i = 0; i < col_size-1; i++) //need -1 on col_size to remove offset from initializing col_size to 1
			{
				L.m_idx[k][i+1] = curr_nnzs[i]; //row_idx of L is updated
				L.m_x[k][i+1] = work[curr_nnzs[i]]/D[k]; //work vector is scaled by D[k]
				
				L.list[curr_nnzs[i]].push_back(k); //update Llist
				count++;
			}
		} else {
			//cout << "all non zeros: " << endl;
			//cout << curr_nnzs << endl;
			D.off_diagonal(k) = work[k+1];
			D[k+1] = dr;
			
			L.m_x[k+1][0] = 1;
			L.m_idx[k+1][0] = k+1;
			count++;
			
			det_D = d1*dr - work[k+1]*work[k+1];
			
			if (det_D != 0) { //replace with EPS later
				D_inv11 = dr/det_D;
				D_inv22 = d1/det_D;
				D_inv12 = -work[k+1]/det_D;


				for (r = 0, i = 0, j = 0; r < col_size; r++) //need -1 on col_size to remove offset from initializing col_size to 1
				{
					l_11 = work[curr_nnzs[r]]*D_inv11 + temp[curr_nnzs[r]]*D_inv12; 
					l_12 = work[curr_nnzs[r]]*D_inv12 + temp[curr_nnzs[r]]*D_inv22; 
					
					if (l_11 != 0) {
						L.m_x[k][i+1] = l_11; //row_idx of L is updated
						L.m_idx[k][i+1] = curr_nnzs[r]; //work vector is scaled by D[k]
						L.list[curr_nnzs[r]].push_back(k); //update Llist
						count++;
						i++;
					}

					if (l_12 != 0) {
						L.m_x[k+1][j+1] = l_12; //row_idx of L is updated
						L.m_idx[k+1][j+1] = curr_nnzs[r]; //work vector is scaled by D[k]
						L.list[curr_nnzs[r]].push_back(k+1); //update Llist
						count++;
						j++;
					}
					
				}
				
				col_size = 1 + i;
				col_size2 = 1 + j;
				
				//update lfirst
				L.advance_first(k+1);
			}
			

		}
		
		L.m_x[k].resize(col_size);
		L.m_idx[k].resize(col_size);
		
		if (size_two_piv) {
			L.m_x[k+1].resize(col_size2);
			L.m_idx[k+1].resize(col_size2);
			k++;
		}
		
		// std::string s;
		// std::cin >> s;
		
		// switch (s.c_str()[0]) {
			// case 'a':
				// cout << to_string() << endl;
			// break;
			// case 'l':
				// cout << L << endl;
			// break;
			// default:
				// break;
		// }
		
		//cout << k << endl << endl;
	}
	
	L.nnz_count = count;
	
}

#endif 
