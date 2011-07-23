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
    unsigned int	total_threads;
    unsigned int	freethreads;
    pthread_cond_t	TOMAIN;
    pthread_cond_t	TOWORKER;
    std::queue<class file_data_hasher_t *> work_queue;
    void		schedule_work(class file_data_hasher_t *);
    bool		all_free();
    unsigned int	get_free_count();
    static int		numCPU();
    threadpool(int numthreads);
    ~threadpool();
};

class worker {
public:
    static void * start_worker(void *arg){return ((worker *)arg)->run();};
    worker(class threadpool *master_): master(master_){}
    class threadpool *master;		// my master
    pthread_t thread;			// my thread; set when I am created
    void *run();
    void do_work(class file_data_hasher_t *); // must delete fdht when done
};
#endif
