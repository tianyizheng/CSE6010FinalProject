/*
 * sim.h
 *
 *  Created on: Nov 25, 2017
 *      Author: Yu
 */

#ifndef SIM_H_
#define SIM_H_

#endif /* SIM_H_ */
typedef struct Event{
	//int payment;
	double timestamp;
	void* appData;
} Event;

void initQueue(void);

//return the timestamp of the event to be added
double Schedule(double ts, void* data);

//return the Event deleted
//Event dequeue();

//void heapify(int index);

void RunSim (double EndTime);
double CurrentTime(void);
//  Event handler function: called to process an event
void EventHandler (void *data);
