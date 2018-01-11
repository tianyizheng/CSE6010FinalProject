/*
 * Heap.c
 *
 *  Created on: Nov 25, 2017
 *      Author: Yu
 */
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "sim.h"
int size=0;
int capacity=100;

/*typedef struct Event{
	//int payment;
	double ts;
	void* appData;
} Event;*/

//double Schedule(Event* );
Event* dequeue(void);
void heapify(int );
void heapify_up(int );
Event** arr;
double Now=0;

void initQueue(){
	arr=(Event**)malloc(capacity*sizeof(Event*));
}

double Schedule(double ts, void* data){
	if(size>=capacity){
        capacity = 2*capacity;
        arr = realloc(arr,capacity*sizeof(Event*));
//        printf("Exceeds Capacity.");
//        return 0;
	}
    
	Event* e;
	if ((e = malloc (sizeof (struct Event))) == NULL) exit(1);
	e->timestamp=ts;
	e->appData=data;

	arr[size]=e;
	int i=size;
	size++;

	heapify_up(i);
	/*while((i>0)&&(arr[i]->timestamp<arr[i/2]->timestamp)){
		Event* temp=arr[i];
		arr[i]=arr[i/2];
		arr[i/2]=temp;
		i=i/2;
	}*/

	return e->timestamp;
}

void heapify_up(int i){
	/*if(i==0)
		return;*/

	if(i>0&&arr[i]->timestamp<arr[(i-1)/2]->timestamp){
		Event* temp=arr[i];
		arr[i]=arr[(i-1)/2];
		arr[(i-1)/2]=temp;
	    heapify_up((i-1)/2);
	}
}

Event* dequeue(){
	if(size<1){
		return NULL;
	}

	Event* data=arr[0];
	arr[0]=arr[size-1];

	//arr[size-1]=NULL;

	size--;
	heapify(0);

	return data;
}

void heapify(int index){
	int l=index*2+1;
	int r=index*2+2;

	int lg=index;

	if(l<size&&arr[l]->timestamp<arr[index]->timestamp){
		lg=l;
	}

	if(r<size&&arr[r]->timestamp<arr[lg]->timestamp)
		lg=r;

	if(lg!=index){
		Event* data=arr[index];
		arr[index]=arr[lg];
		arr[lg]=data;

		heapify(lg);
	}
}

double CurrentTime(){
	return Now;
}

void RunSim (double EndTime)
{
	struct Event* e;

	//printf ("Initial event list:\n");
	//PrintList ();

	// Main scheduler loop
	while (size>=1) {
		e=dequeue();
		Now = e->timestamp;
		// printf("%f, %d", Now, size);
		// printf("\n");
        if (Now > EndTime) break;
		EventHandler(e->appData);
		free (e);
        //PrintList ();
	}
}

/*int main(){
	srand(time(NULL));
	int r;

	initQueue();

	for(int i=0; i<5; i++){
		r=rand();
		//Event newNode;
		//newNode.payment=i;
		//newNode.ts=r;
		//newNode.appData=NULL;
		printf("%f", Schedule(r, NULL));
		printf("\n");
		//enqueue(&newNode);
	}

	printf("\n");
	RunSim(1000000000);
	//printf("\n");

	for(int i=0; i<capacity; i++){

		printf("%d", dequeue());
		printf("\n");
	}

	//return 0;
}*/
