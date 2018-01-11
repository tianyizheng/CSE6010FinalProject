#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "sim.h"

/////////////////////////////////////////////////////////////////////////////////////////////
//A fast food restaurant discrete event simulation application
//
//Created On: Nov.26, 2017
//    Author: Tianyi Zheng
//Copyright © 2017 Tianyi Zheng. All rights reserved.
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
//
// State variables  and other global information
//
/////////////////////////////////////////////////////////////////////////////////////////////

///// variables that affect performance//////////////////////////////////////////////////
int cashier = 1;                 //number of available cashiers
int waiter = 3;                  //number of available waiters
int cooks = 2;                   //number of available cooks
double incoming_rate = 0.02;     //customers per seconds
double endTime = 1000*60;         //total simulation time
int foodsize = 50;               //how many customers does a food serving serves
int foodThreshold = 10;           //when new food needs to be cooked

///// variables that need to be measured//////////////////////////////////////////////////
double buzzcardwait = 25;          //paying time needed for buzzcard
double creditwait = 36;           //paying time for credit card
double cashwait = 60;             //paying time for cash
double foodPrepTime = 300;        //time needed for making food
double TapingoPercentage = 0.05;  //percent of people paying with tapingo
double servingLowEnd = 20;        //fastest serving time
double servingInterval = 20;      //variation interval; i.e. the longest serving time is lowend+interval
double tapingoServingLowEnd = 2;
double tapingoServingInterval = 10;

///// variables for statistical calculation ///////////////////////////////////////////////////////////////

double ServeWaitTime = 0;
int ServeWait = 0;
double PayWaitTime = 0;
int PayWait = 0;
double FoodWaitTime = 0;
int FoodWait = 0;

///// Do not change these //////////////////////////////////////////////////////////////////
int foodWaiting = 0;             //number of customers waiting for food
int serveWaiting = 0;             //number of customers waiting for serve
int payWaiting = 0;             //number of customers waiting for pay
int numFood = 6;                 //how many dishes are there
int food[6];                      //an array to hold the current amount of food 

//// Event types //////////////////////////////////////////////////////////////////////////////////////////////
// Representing the customers waiting for:
#define SERVE 1
#define PAY_CASHIER 21
#define PAY_WAITER 22
#define PAY 23
#define FOOD 31
#define COOK 32
#define TAPINGO 4
#define BUZZCARD 5
#define CASH 6
#define CREDITCARD 7
#define FINISH 81
#define FINISH_TAPINGO 82

/////////////////////////////////////////////////////////////////////////////////////////////
//
// Data structures for event data
//
/////////////////////////////////////////////////////////////////////////////////////////////

struct EventData
{
  int EventType;
  bool Tapingo;
  int PaymentType;
  int foodchoice;
};

//store queued customers/////////////////////////////////////////////////////////
struct QueuedNodes
{
  double Que_Time;
  struct EventData *data;
  struct QueuedNodes *Next;
};

//store front and end nodes/////////////////////////////////////////////////////////
struct QueuedNodes *front1 = NULL;
struct QueuedNodes *rear1 = NULL;
struct QueuedNodes *front2 = NULL;
struct QueuedNodes *rear2 = NULL;
struct QueuedNodes *front3 = NULL;
struct QueuedNodes *rear3 = NULL;
/////////////////////////////////////////////////////////////////////////////////////////////
//
// Function prototypes
//
/////////////////////////////////////////////////////////////////////////////////////////////
void EventHandler(void *data);        //general event handler
void Serve(struct EventData *e);      //when a customer needs to order
void Pay(struct EventData *e);        //when a customers needs to pay
void Food(struct EventData *e);       //when food runs out
void Cook(struct EventData *e);       //when more food needs to be made
void Finish(struct EventData *e);     //customer who leaves the restaurants
void Tapingo(struct EventData *e);    //customers who orders and pays with tapingo
double uRand(void);                   //random number generator
///// SUPPORTING FUNCTIONS //////////////////////////////////////////////////////////////////////
void Enqueue1(double x, struct EventData *e);
void Dequeue1(void);
struct QueuedNodes* Front1(void);
void Enqueue2(double x, struct EventData *e);
void Dequeue2(void);
struct QueuedNodes* Front2(void);
void Enqueue3(double x, struct EventData *e);
void Dequeue3(void);
struct QueuedNodes* Front3(void);
/////////////////////////////////////////////////////////////////////////////////////////////
//
// Event Handlers
// Parameter is a pointer to the data portion of the event
//
/////////////////////////////////////////////////////////////////////////////////////////////
// General Event Handler Procedure define in simulation engine interface
void EventHandler(void *data)
{
  struct EventData *d;
    // coerce type
  d = (struct EventData *)data;
   // printf("%.2f\t%d\t%d\t%d\t%d\n", CurrentTime(), d->EventType, serveWaiting, payWaiting,foodWaiting);
    // call an event handler based on the type of event
  if (d->EventType == SERVE)
    Serve(d);
  else if (d->EventType == PAY_CASHIER || d->EventType == PAY_WAITER || d->EventType == PAY)
    Pay(d);
  else if (d->EventType == FOOD)
    Food(d);
  else if (d->EventType == COOK)
    Cook(d);
  else if (d->EventType == TAPINGO)
    Tapingo(d);
  else if (d->EventType == FINISH || d->EventType == FINISH_TAPINGO)
    Finish(d);
  else
  {
    printf("%.2f\n", CurrentTime());
    fprintf(stderr, "Illegal event found:%d\n", d->EventType);
    exit(1);
  }
}

// event handler for SERVING events
void Serve(struct EventData *e)
{
  struct EventData *d;
  double ts;

  if (e->Tapingo){
    e->EventType = TAPINGO;
    Tapingo(e);
    return;
  }

  if (e->EventType != SERVE)
  {
    fprintf(stderr, "Unexpected event type\n");
    exit(1);
  }

  //schedules food event if ran out of food
  int foodchoice = uRand() * 5 + 1;
  if (food[foodchoice] < 1){
    if ((d = malloc(sizeof(struct EventData))) == NULL)
    {
      fprintf(stderr, "malloc error\n");
      exit(1);
    }
    d->EventType = FOOD;
    d->Tapingo = e->Tapingo;
    d->PaymentType = e->PaymentType;
    e->foodchoice = foodchoice;
    foodWaiting++;
    Schedule(CurrentTime() + foodPrepTime, d);
    Enqueue3(CurrentTime(), e);
    return;
  }

  // if waiter is free, serving will go on for some times; schedule pay event
  if (waiter > 0)
  {
    if ((d = malloc(sizeof(struct EventData))) == NULL)
    {
      fprintf(stderr, "malloc error\n");
      exit(1);
    }
    d->EventType = PAY_WAITER;
    d->Tapingo = e->Tapingo;
    d->PaymentType = e->PaymentType;
    ts = CurrentTime() + uRand() * servingInterval + servingLowEnd;
    waiter--;
    food[foodchoice]--;
    if (food[foodchoice] < foodThreshold && cooks > 0){
      struct EventData *f;
      if ((f = malloc(sizeof(struct EventData))) == NULL)
      {
        fprintf(stderr, "malloc error\n");
        exit(1);
      }
      f->EventType = COOK;
      f->Tapingo = e->Tapingo;
      f->PaymentType = e->PaymentType;
      f->foodchoice = foodchoice;
      Schedule(CurrentTime() + foodPrepTime, f);
    }
    Schedule(ts, d);
  }

  // else if a cashier is free, serving will go on for some time unts; schedule pay event

  else if (cashier > 1)
  {
    if ((d = malloc(sizeof(struct EventData))) == NULL)
    {
      fprintf(stderr, "malloc error\n");
      exit(1);
    }
    d->EventType = PAY_CASHIER;
    d->Tapingo = e->Tapingo;
    d->PaymentType = e->PaymentType;
    ts = CurrentTime() + uRand() * servingInterval + servingLowEnd;
    cashier--;
    food[foodchoice]--;
    if (food[foodchoice] < foodThreshold && cooks > 0){
      struct EventData *f;
      if ((f = malloc(sizeof(struct EventData))) == NULL)
      {
        fprintf(stderr, "malloc error\n");
        exit(1);
      }
      f->EventType = COOK;
      f->Tapingo = e->Tapingo;
      f->PaymentType = e->PaymentType;
      f->foodchoice = foodchoice;
      Schedule(CurrentTime() + foodPrepTime, f);
    }
    Schedule(ts, d);
  }

  else
  {
    if (uRand() < 0.90)
    {
      serveWaiting++;
      Enqueue1(CurrentTime(), e);
      return;
    }
    else
    {
      //customer leaves the restaurant
    }
  }
    free(e); // don't forget to free storage for event!
  }

//event handler for Tapingo customers
  void Tapingo(struct EventData *e)
  {
    struct EventData *d;
    double ts;
    if (e->EventType != TAPINGO)
    {
      fprintf(stderr, "Unexpected event type\n");
      exit(1);
    }

  //schedules food event if ran out of food
    int foodchoice = uRand() * 5 + 1;
    if (food[foodchoice] < 1 && cooks > 0){
      if ((d = malloc(sizeof(struct EventData))) == NULL)
      {
        fprintf(stderr, "malloc error\n");
        exit(1);
      }
      d->EventType = FOOD;
      d->Tapingo = e->Tapingo;
      d->PaymentType = e->PaymentType;
      e->foodchoice = foodchoice;
      foodWaiting++;
      cooks--;
      Schedule(CurrentTime() + foodPrepTime, d);
      Enqueue3(CurrentTime(), e);
      return;
    }

  // if waiter is free, serving will go on for some seconds; schedule tapingo_finish event
    if (waiter > 0)
    {
      if ((d = malloc(sizeof(struct EventData))) == NULL)
      {
        fprintf(stderr, "malloc error\n");
        exit(1);
      }
      d->EventType = PAY_WAITER;
      d->Tapingo = e->Tapingo;
      d->PaymentType = e->PaymentType;
      ts = CurrentTime() + uRand() * tapingoServingInterval + tapingoServingLowEnd;
      waiter--;
      food[foodchoice]--;
      if (food[foodchoice] < foodThreshold && cooks > 0){
        struct EventData *f;
        if ((f = malloc(sizeof(struct EventData))) == NULL)
        {
          fprintf(stderr, "malloc error\n");
          exit(1);
        }
        f->EventType = COOK;
        f->Tapingo = e->Tapingo;
        f->PaymentType = e->PaymentType;
        f->foodchoice = foodchoice;
        Schedule(CurrentTime() + foodPrepTime, f);
      }
      Schedule(ts, d);
    }

  // else if a cashier is free, serving will go on for some time unts; schedule tapingo_finish event

    else if (cashier > 1)
    {
      if ((d = malloc(sizeof(struct EventData))) == NULL)
      {
        fprintf(stderr, "malloc error\n");
        exit(1);
      }
      d->EventType = PAY_CASHIER;
      d->Tapingo = e->Tapingo;
      d->PaymentType = e->PaymentType;
      ts = CurrentTime() + uRand() * tapingoServingInterval + tapingoServingLowEnd;
      cashier--;
      food[foodchoice]--;
      if (food[foodchoice] < foodThreshold && cooks > 0){
        struct EventData *f;
        if ((f = malloc(sizeof(struct EventData))) == NULL)
        {
          fprintf(stderr, "malloc error\n");
          exit(1);
        }
        f->EventType = COOK;
        f->Tapingo = e->Tapingo;
        f->PaymentType = e->PaymentType;
        f->foodchoice = foodchoice;
        Schedule(CurrentTime() + foodPrepTime, f);
      }
      Schedule(ts, d);
    }

    else
    {
      if (uRand() < 0.90)
      {
        serveWaiting++;
        e->EventType = SERVE;
        Enqueue1(CurrentTime(), e);
        return;
      }
      else
      {
      //customer leaves the restaurant
      }
    }
    free(e); // don't forget to free storage for event!
  }


// event handler for PAY events
  void Pay(struct EventData *e)
  {
    struct EventData *d;
    double ts = CurrentTime();
    if (!(e->EventType == PAY_CASHIER || e->EventType == PAY_WAITER || e->EventType == PAY))
    {
      fprintf(stderr, "Unexpected event type\n");
      exit(1);
    }
    if (e->EventType == PAY_CASHIER)
    {
      cashier++;
    }
    else if (e->EventType == PAY_WAITER)
    {
      waiter++;
    }

    if(e->Tapingo){
      if ((d = malloc(sizeof(struct EventData))) == NULL)
      {
        fprintf(stderr, "malloc error\n");
        exit(1);
      }
      d->EventType = FINISH_TAPINGO;
      Schedule(CurrentTime(), d);
    // release memory for data part of event
      free(e);
    }

//schedule finishing event if cashier is available

    else if (cashier > 0){
      if ((d = malloc(sizeof(struct EventData))) == NULL)
      {
        fprintf(stderr, "malloc error\n");
        exit(1);
      }
      d->EventType = FINISH;

//determine timestamp of finishing event

      double temp = 0;
      if (e->PaymentType == BUZZCARD)
        temp = buzzcardwait;
      else if (e->PaymentType == CREDITCARD)
        temp = creditwait;
      else if (e->PaymentType == CASH)
        temp = cashwait;
      cashier--;
      Schedule(CurrentTime()+temp, d);
    // release memory for data part of event
      free(e);
    }

//else, enqueue a customer into the pay wait queue
    else {
      payWaiting++;
      Enqueue2(CurrentTime(), e);
    }

// schedule next queue event for serving
    if (serveWaiting > 0 && (waiter > 0 || cashier > 1))
    {
      struct EventData *d2;
      if ((d2 = malloc(sizeof(struct EventData))) == NULL)
      {
        fprintf(stderr, "malloc error\n");
        exit(1);
      }
      d2 = Front1()->data;
      ServeWaitTime = ServeWaitTime + CurrentTime() - Front1()->Que_Time ;
      ServeWait++;
      Dequeue1();
      serveWaiting--;
      Schedule(ts, d2);
    }

  }

//event handler for food events
  void Food(struct EventData *e)
  {
    struct EventData *d;
    if (e->EventType != FOOD)
    {
      fprintf(stderr, "Unexpected event type\n");
      exit(1);
    }
    if ((d = malloc(sizeof(struct EventData))) == NULL)
    {
      fprintf(stderr, "malloc error\n");
      exit(1);
    }
    double ts = CurrentTime();
    d = Front3()->data;
    d->EventType = SERVE;
    FoodWaitTime = FoodWaitTime + CurrentTime() - Front3()->Que_Time ;
    FoodWait++;
    Dequeue3();
    food[d->foodchoice] = foodsize;
    cooks++;
    Schedule(ts, d);
    free(e);
  }

  void Cook(struct EventData* e){
    struct EventData *d;
    if (e->EventType != COOK)
    {
      fprintf(stderr, "Unexpected event type\n");
      exit(1);
    }
    if ((d = malloc(sizeof(struct EventData))) == NULL)
    {
      fprintf(stderr, "malloc error\n");
      exit(1);
    }
    food[e->foodchoice] = foodsize;
    cooks++;
    free(e);
  }

//event handler for finishing events
  void Finish(struct EventData* e)
  {
    struct EventData *d;
    double ts = CurrentTime();
    if (!(e->EventType == FINISH || e->EventType == FINISH_TAPINGO))
    {
      fprintf(stderr, "Unexpected event type\n");
      exit(1);
    }
    if (e->EventType == FINISH_TAPINGO){
      free(e);
      return;
    }
    cashier++;
    if ((d = malloc(sizeof(struct EventData))) == NULL)
    {
      fprintf(stderr, "malloc error\n");
      exit(1);
    }
  //if there is a customer waiting for pay
    if (payWaiting > 0 && cashier > 0)
    {
      d = Front2()->data;
      PayWaitTime = PayWaitTime + CurrentTime() - Front2()->Que_Time;
      PayWait++;
      Dequeue2();
      payWaiting--;
      d->EventType = PAY;
      Schedule(ts, d);
    }

  free(e); // release memory for data part of event
}


////////////////////////////////////////////////////////////////////////////////
//
// SUPPORTING FUNCTIONS
//
////////////////////////////////////////////////////////////////////////////////

// To Enqueue a node
void Enqueue1(double x, struct EventData *e)
{
  struct QueuedNodes *temp;
  if ((temp = (struct QueuedNodes *)malloc(sizeof(struct QueuedNodes))) == NULL){
    fprintf(stderr, "Malloc QueuedNodes Error\n");
  };
  temp->Que_Time = x;
  temp->data = e;
  temp->Next = NULL;
  if (front1 == NULL && rear1 == NULL)
  {
    front1 = rear1 = temp;
    return;
  }
  rear1->Next = temp;
  rear1 = temp;
}

// To Dequeue a node.
void Dequeue1()
{
  struct QueuedNodes *temp = front1;
  if (front1 == NULL)
  {
    return;
  }
  if (front1 == rear1)
  {
    front1 = rear1 = NULL;
  }
  else
  {
    front1 = front1->Next;
  }
  if(temp == NULL){
    printf("Dequeue memory error\n");
  }
  free(temp);
}

struct QueuedNodes* Front1()
{
  if (front1 == NULL)
  {
    printf("Queue is empty. Check code\n");
    return NULL;
  }
  return front1;
}

// To Enqueue a node
void Enqueue2(double x, struct EventData *e)
{
  struct QueuedNodes *temp;
  if ((temp = (struct QueuedNodes *)malloc(sizeof(struct QueuedNodes))) == NULL){
    fprintf(stderr, "Malloc QueuedNodes Error\n");
  };
  temp->Que_Time = x;
  temp->data = e;
  temp->Next = NULL;
  if (front2 == NULL && rear2 == NULL)
  {
    front2 = rear2 = temp;
    return;
  }
  rear2->Next = temp;
  rear2 = temp;
}

// To Dequeue a node.
void Dequeue2()
{
  struct QueuedNodes *temp = front2;
  if (front2 == NULL)
  {
    return;
  }
  if (front2 == rear2)
  {
    front2 = rear2 = NULL;
  }
  else
  {
    front2 = front2->Next;
  }
  if(temp == NULL){
    printf("Dequeue memory error\n");
  }
  free(temp);
}

struct QueuedNodes* Front2()
{
  if (front2 == NULL)
  {
    printf("Queue is empty. Check code\n");
    return NULL;
  }
  return front2;
}

// To Enqueue a node
void Enqueue3(double x, struct EventData *e)
{
  struct QueuedNodes *temp;
  if ((temp = (struct QueuedNodes *)malloc(sizeof(struct QueuedNodes))) == NULL){
    fprintf(stderr, "Malloc QueuedNodes Error\n");
  };
  temp->Que_Time = x;
  temp->data = e;
  temp->Next = NULL;
  if (front3 == NULL && rear3 == NULL)
  {
    front3 = rear3 = temp;
    return;
  }
  rear3->Next = temp;
  rear3 = temp;
}

// To Dequeue a node.
void Dequeue3()
{
  struct QueuedNodes *temp = front3;
  if (front3 == NULL)
  {
    return;
  }
  if (front3 == rear3)
  {
    front3 = rear3 = NULL;
  }
  else
  {
    front3 = front3->Next;
  }
  if(temp == NULL){
    printf("Dequeue memory error\n");
  }
  free(temp);
}

struct QueuedNodes* Front3()
{
  if (front3 == NULL)
  {
    printf("Queue is empty. Check code\n");
    return NULL;
  }
  return front3;
}

//To calculate a random number
double uRand(void)
{
  return ((double)rand()) / (double)((unsigned)RAND_MAX + 1);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// MAIN PROGRAM
//
///////////////////////////////////////////////////////////////////////////////////////
int main (void){
  initQueue();
  struct EventData *d;
  double ts = -log(1.0 - uRand()) / incoming_rate;
  srand(time(NULL));
  int total = 0;
  //init food array
  for (int i = 0; i < numFood; i++){
    food[i] = foodsize;
  }
  //generate incoming calls
  while (ts < endTime)
  {
    ts += -log(1.0 - uRand()) / incoming_rate;
    if ((d = malloc(sizeof(struct EventData))) == NULL)
    {
      fprintf(stderr, "malloc error\n");
      exit(1);
    }
    if (uRand() < TapingoPercentage){
      d->EventType = TAPINGO;
      d->Tapingo = true;
    }
    else{
      d->EventType = SERVE;
      d->Tapingo = false;
      if (uRand() < 0.3){
        d->PaymentType = CREDITCARD;
      }
      else if (uRand() < 0.6){
        d->PaymentType = CASH;
      }
      else{
        d->PaymentType = BUZZCARD;
      }
    }
    total++;
    Schedule(ts, d);
  }
  // printf("Time\tEventType\tSW\tPW\tFW\n");
  RunSim(endTime);
  printf("Ran for:%.2f seconds\n\n", CurrentTime());
  printf("\t\tServeWait\tPayWait\t\tFoodWait\n");
  printf("TotalTime:\t%.2f\t\t%.2f\t\t%.2f\n", ServeWaitTime, PayWaitTime, FoodWaitTime);
  printf("TotalPeople:\t%d\t\t%d\t\t%d\n", ServeWait, PayWait, FoodWait);
}
