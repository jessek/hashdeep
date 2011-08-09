/****************************************************************
 *** THREADING SUPPORT
 ****************************************************************/

#ifndef THREADING_H
#define THREADING_H

#include <pthread.h>
#include <algorithm>
#include <queue>
#include <vector>

class threadpool: public std::vector<class worker *> {
public:
    pthread_mutex_t	M;			// protects the following variables
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
