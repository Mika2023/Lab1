#include <errno.h>
#include "my_rwlock.h"

int rwlock_init(rwlock_t* lock){
	int err;

	err = pthread_mutex_init(&lock->mutex, NULL);
	if(err != 0)
		return err;
	err = pthread_cond_init(&lock->readers_cond, NULL);
	if(err != 0){
		pthread_mutex_destroy(&lock->mutex);
		return err;
	}
	err = pthread_cond_init(&lock->writers_cond, NULL);
	if(err != 0){
		pthread_cond_destroy(&lock->readers_cond);
		pthread_mutex_destroy(&lock->mutex);
		return err;
	}

	lock->readers = 0;
	lock->readers_waiting = 0;
	lock->writers_waiting = 0;
	lock->writer = 0;
	return 0;
}

int rwlock_destroy(rwlock_t* lock){
	int is_busy;

	pthread_mutex_lock(&lock->mutex);
	is_busy = lock->readers > 0 || lock->writer ||
	       lock->readers_waiting > 0 || lock->writers_waiting > 0;
	pthread_mutex_unlock(&lock->mutex);
	if(is_busy)
		return EBUSY;

	pthread_cond_destroy(&lock->writers_cond);
	pthread_cond_destroy(&lock->readers_cond);
	pthread_mutex_destroy(&lock->mutex);
	return 0;
}

int rwlock_rdlock(rwlock_t* lock){
	int err;

	err = pthread_mutex_lock(&lock->mutex);
	if(err != 0)
		return err;

	lock->readers_waiting++;
	while(lock->writer)
		pthread_cond_wait(&lock->readers_cond, &lock->mutex);
	lock->readers_waiting--;
	lock->readers++;

	pthread_mutex_unlock(&lock->mutex);
	return 0;
}

int rwlock_wrlock(rwlock_t* lock){
	int err;

	err = pthread_mutex_lock(&lock->mutex);
	if(err != 0)
		return err;

	lock->writers_waiting++;
	while(lock->writer || lock->readers > 0)
		pthread_cond_wait(&lock->writers_cond, &lock->mutex);
	lock->writers_waiting--;
	lock->writer = 1;

	pthread_mutex_unlock(&lock->mutex);
	return 0;
}

int rwlock_unlock(rwlock_t* lock){
	int err;

	err = pthread_mutex_lock(&lock->mutex);
	if(err != 0)
		return err;

	if(lock->writer)
		lock->writer = 0;
	else if(lock->readers > 0)
		lock->readers--;
	else{
		pthread_mutex_unlock(&lock->mutex);
		return EPERM;
	}
	
	if(lock->readers == 0){
		if(lock->readers_waiting > 0)
			pthread_cond_broadcast(&lock->readers_cond);
		else if(lock->writers_waiting > 0)
			pthread_cond_signal(&lock->writers_cond);
	}

	pthread_mutex_unlock(&lock->mutex);
	return 0;
}