/*
 * sort.c
 *
 *  Created on: Oct 17, 2014
 *      Author: Thomas Dubucq
 */

#include <stdlib.h>
#include <stdio.h>

void int_qsort(int* tab_p, int tab_size){

	int partition(int* tab_p, int i, int j){
		int current_position,tmp;
		int pivot_position  = i;

		for(current_position = i+1; current_position <= j; current_position ++ ){
			if(tab_p[current_position] < tab_p[pivot_position]){
				/* Array reorganization */
				tmp = tab_p[pivot_position];
				tab_p[pivot_position] = tab_p[current_position];
				tab_p[current_position] = tab_p[pivot_position+1];
				tab_p[pivot_position+1] = tmp;

				pivot_position++;
			}
		}
		return(pivot_position);
	}

	void qsort_it(int* tab_p, int i, int j){
		if(i < j){
			int k = partition(tab_p,i,j);
			qsort_it(tab_p, i, k-1);
			qsort_it(tab_p, k+1, j);
		}
	}

	qsort_it(tab_p,0, tab_size-1);
}

int* merge(int*a, int size_a, int*b, int size_b){
	int a_idx = 0;
	int b_idx = 0;
	int ab_idx = 0;

	int* merged_array = (int *)malloc((size_a+size_b)*sizeof(int));

	while(a_idx < size_a && b_idx < size_b){

		if(a[a_idx] < b[b_idx]){
			merged_array[ab_idx] = a[a_idx];
			a_idx++;
		}else{
			merged_array[ab_idx] = b[b_idx];
			b_idx++;
		}
		ab_idx++;
	}
	while(a_idx < size_a){
		merged_array[ab_idx] = a[a_idx];
		a_idx++;
		ab_idx++;
	}
	while(b_idx < size_b){
		merged_array[ab_idx] = b[b_idx];
		b_idx++;
		ab_idx++;
	}
	free(a);
	free(b);
	return(merged_array);
}
