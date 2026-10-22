#ifndef MY_RWLOCK_H
#define MY_RWLOCK_H

#include<pthread.h>

/* Own read-write lock, built from a mutex and two condition variables
   (seminar 2: mutexes, seminar 3: condition variables). */
typedef struct {
	pthread_mutex_t mutex;          /* protects all fields below          */
	pthread_cond_t  readers_cond;   /* readers sleep here                 */
	pthread_cond_t  writers_cond;   /* writers sleep here                 */
	int readers;                    /* threads currently reading          */
	int readers_waiting;            /* threads waiting for a read lock    */
	int writers_waiting;            /* threads waiting for a write lock   */
	int writer;                     /* 1 if a writer holds the lock       */
} rwlock_t;

int rwlock_init(rwlock_t* lock);
int rwlock_destroy(rwlock_t* lock);
int rwlock_rdlock(rwlock_t* lock);
int rwlock_wrlock(rwlock_t* lock);
int rwlock_unlock(rwlock_t* lock);

#endif
