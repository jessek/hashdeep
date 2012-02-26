/****************************************************************
 *** THREADING SUPPORT
 ****************************************************************/

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stdio.h>
#include <pthread.h>
#include <algorithm>
#include <queue>
#include <vector>

class mutex_t {
public:
    mutable pthread_mutex_t	mutex;
    mutex_t(){
	if(pthread_mutex_init(&mutex,NULL)){
	    perror("pthread_mutex_init failed");
	    exit(1);
	}
    }
    ~mutex_t(){
	/*
	 * note that we do not destroy the mutex.
	 * On windows this seems to cause problems, so we just don't bother.
	 *
	 * if(pthread_mutex_destroy(&mutex)){
	 *    perror("pthread_mutex_destroy failed");
	 *    exit(1);
	 * }
	 */
    }
    void lock() const{
	if(pthread_mutex_lock(&mutex)){
	    perror("pthread_mutex_lock failed");
	    exit(1);
	}
    }
    void unlock() const{
	if(pthread_mutex_unlock(&mutex)){
	    perror("pthread_mutex_unlock failed");
	    exit(1);
	}
    }
};

class threadpool: public std::vector<class worker *> {
public:
    static void		win32_init(); // must be called under win32 to initialize posix threads
    mutex_t		M;			// protects the following variables
    volatile unsigned int numworkers;
    volatile unsigned int freethreads;
    pthread_cond_t	TOMAIN;
    pthread_cond_t	TOWORKER;
    std::queue<class file_data_hasher_t *> work_queue;
    unsigned int	get_free_count();
    void		schedule_work(class file_data_hasher_t *);
    bool		all_free() ;
    void		wait_till_all_free();
    void		kill_all_workers();
    static int		numCPU();
    threadpool(int numworkers);
    ~threadpool();
    unsigned int	num_workers();
};

class worker {
public:
    static void * start_worker(void *arg){return ((worker *)arg)->run();};
    worker(class threadpool *master_,int workerid_): master(master_),workerid(workerid_){}
    class threadpool *master;		// my master
    pthread_t thread;			// my thread; set when I am created
    int	workerid;			// my workerID, numbered 0 through numworkers-1
    void *run();
    void do_work(class file_data_hasher_t *); // must delete fdht when done
};
#endif
