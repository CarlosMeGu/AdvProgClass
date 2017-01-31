/**
 * Copyright (c) 2015 Abelardo López Lagunas
 *
 * @file    ProdCons.c
 *
 * @author  Abelardo López Lagunas
 *
 * @date    Wed 30 Mar 2016 15:12 CST
 *
 * @brief   Shows how to implement the Producer-Consumer model of
 *          communication between two threads.
 *
 * References:
 *          Code was originally found on a tutorial from IBM but I have
 *          changed it so much that only the function names remain the same.
 *
 * Revision history:
 *          Wed 30 Mar 2016 15:12 CST -- File created
 *
 * @note    Use for the TC2025 class
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>                 // For thread management
#include <unistd.h>                  // For the sleep function
#include <time.h>                    // Used to get the current time

/*
 * Define the global constants for the buffer size between threads, the
 * maximum number of elements to produce and the end of the data stream
 * that tells the consumer that no more data will come in.
 */
#define   BUFFER_SIZE    4  // Use #define to avoid warnings
const int LIMIT       = -1;
const int COUNT       = 11;

/*
 * Encapsulate the buffer with the mutex and conditional variables
 * so its easier to extend to multiple consumers/producers. Note that
 * the use of a circular buffer and that there is ONE instance of the
 * data structure defined as a global variable.
 */
struct prodcons {
  int buffer[BUFFER_SIZE];   /* Data to store */
  pthread_mutex_t key;       /* Synchronization variable */
  int read, write;           /* Read/Write position */
  pthread_cond_t notEmpty;   /* Signal buffer NOT empty */
  pthread_cond_t notFull;    /* Signal buffer NOT full */
} buffer;

/* This function initializes the shared data structure */
void init(struct prodcons * b)
{
  pthread_mutex_init(&b->key, NULL);
  pthread_cond_init(&b->notEmpty, NULL);
  pthread_cond_init(&b->notFull, NULL);
  b->read = 0;
  b->write = 0;
}

/* This function stores a data item in the buffer */
void Store(struct prodcons * b, int data)
{
  pthread_mutex_lock(&b->key);
  /* Wait until the buffer is not full */
  while ((b->write + 1) % BUFFER_SIZE == b->read) {
    pthread_cond_wait(&b->notFull, &b->key);
  }
  /*
   * This function suspends the thread until notFull is true
   * note that it will acquire the key when the thread resumes
   */
  b->buffer[b->write] = data;
  b->write++;
  if (b->write >= BUFFER_SIZE) b->write = 0;
  /* Signal buffer not full */
  pthread_cond_signal(&b->notEmpty);
  pthread_mutex_unlock(&b->key);
}

/* This function reads and removes a data item from the buffer */
int Get (struct prodcons * b)
{
  int data;
  pthread_mutex_lock(&b->key);
  /* Wait until the buffer is not empty */
  while (b->write == b->read) {
    pthread_cond_wait(&b->notEmpty, &b->key);
  }
  /*
   * This function suspends the thread until notEmpty is true
   * it also acquires the key when the thread resumes. Read
   * the data item and advance the pointer.
   */
  data = b->buffer[b->read];
  b->read++;
  if (b->read >= BUFFER_SIZE) b->read = 0;
  /* Signal buffer not full */
  pthread_cond_signal(&b->notFull);
  pthread_mutex_unlock(&b->key);
  return (data);
}

/* This function produces COUNT elements and inserts them into the
 * shared data structure by calling Store()
 */
void * Producer(void * data)
{
  int n;
  for (n = 0; n < COUNT; n++) {
    printf("%d --->\n", n);
    Store(&buffer, n);          /* Insert an element */
	sleep(rand() % BUFFER_SIZE);
  }
  Store(&buffer, LIMIT);        /* Indicates the last element */
  return (NULL);
}

/* This function consumes elements and deletes them from the shared data
 * structure by calling Get() until the end of stream (LIMIT) value is read.
 */
void * Consumer(void * data)
{
  int d;
  while (1) {
    d = Get(&buffer);
    if (d == LIMIT) break;
    printf("---> %d\n", d);
	sleep( rand() % BUFFER_SIZE);
  }
  return (NULL);
}

/* The main entry point, this follows the boss-worker model so the main
 * thread only creates the worker threads and then waits fro their
 * termination.
 */
int main(void)
{
  pthread_t th_a, th_b;   /* Declare both threads */
  void * returnValue;     /* Returned value from the join function */

  init(&buffer);          // Initializes the shared structure

  srand (time(NULL));     // Use the current time as the seed

  /* Create both threads */
  pthread_create(&th_a, NULL, Consumer, NULL);
  pthread_create(&th_b, NULL, Producer, NULL);

  /* Wait until both threads are done */
  pthread_join(th_a, &returnValue);
  pthread_join(th_b, & returnValue);

  /* Not really needed but just show how conditionals and mutexes are
   * destroyed.
   */
  pthread_cond_destroy(&buffer.notEmpty);
  pthread_cond_destroy(&buffer.notFull);
  pthread_mutex_destroy(&buffer.key);

  return (0);             /* Tell the OS that there were no problems */
}
