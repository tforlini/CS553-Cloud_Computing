/*
 * int_list.c
 *
 *  Created on: Oct 17, 2014
 *      Author: Thomas Dubucq
 */


#include <stdio.h>
#include <stdlib.h>
#include "int_list.h"


int_list_t add(int a, int_list_t list){
	int_list_t tmp = (int_list_t)malloc(sizeof(struct s_list_el_t));
	tmp->head = a;
	tmp->tail = list;

	return(tmp);
}

int hd(int_list_t list){
	return(list->head);
}

int_list_t tl(int_list_t list){
	return(list->tail);
}

int_list_t free_list_el(int_list_t list){
	int_list_t tmp  = list->tail;
	free(list);
	return(tmp);
}

void delete(int el, int_list_t* list_p){
	int found = 0;
	int_list_t* current_p;
	int_list_t* previous_p;
	/* Test if first element */
	if((*list_p)->head == el){
		*list_p = free_list_el(*list_p);
	}else{
		current_p = &((*list_p)->tail);
		previous_p = list_p;
		while(found == 0 && *current_p != NULL ){
			if((*current_p)-> head == el){
				found = 1;
				*previous_p = (*current_p)->tail;
				free(*current_p);
			}
			previous_p = current_p;
			current_p = &((*current_p)->tail);
		}
	}
}
void free_list(int_list_t list){
	int_list_t previous = list;
	int_list_t current = list;
	while(current != NULL){
		current = current->tail;
		free(previous);
		previous = current;
	}
}
