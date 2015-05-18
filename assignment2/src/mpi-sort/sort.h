/*
 * sort.h
 *
 *  Created on: Oct 17, 2014
 *      Author: Thomas Dubucq
 */

#ifndef SORT_H_
#define SORT_H_

/* Apply Quick Sort algorithm to tab_p */
void int_qsort(int* tab_p, int tab_size);

/* Merge 2 sorted arrays into 1 sorted array. a and b are freed during execution*/
int* merge(int*a, int size_a, int*b, int size_b);
#endif /* SORT_H_ */
