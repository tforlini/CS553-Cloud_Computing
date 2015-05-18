/*
 * int_list.h
 *
 *  Created on: Oct 17, 2014
 *      Author: Thomas Dubucq
 */

#ifndef INT_LIST_H_
#define INT_LIST_H_


typedef struct s_list_el_t* int_list_t;
struct s_list_el_t {int head; int_list_t tail;} ;

/* Add an integer to a list */
int_list_t add(int a, int_list_t list);

/* Retrieve the head of a list */
int hd(int_list_t list);

/* Retrieve the tail of a list */
int_list_t tl(int_list_t list);

/* Free the first element of a list and returns its tail */
int_list_t free_list_el(int_list_t list);

/* Delete one element of the list by giving its value */
void delete(int el, int_list_t* list);

/* Free the whole list */
void free_list(int_list_t list);

#endif /* INT_LIST_H_ */
