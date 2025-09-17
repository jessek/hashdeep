/****************************************************************
 *** THREADING SUPPORT
 ****************************************************************/

// $Id$

#include "main.h"

#ifdef HAVE_PTHREAD
#include "threadpool.h"

/**
 * http://stackoverflow.com/questions/4264460/wait-for-one-of-several-threads-to-finish
 * Here is the algorithm to run the thread pool with a work queue:
 *
 * main:
 *     set freethreads to numthreads
 *     init mutex M, condvars TOMAIN and TOWORKER
 *     start N worker threads
 *     while true:
 *         wait for work item
 *         claim M
 *         while freethreads == 0:
 *             cond-wait TOMAIN, M
 *         put work item in queue
 *         decrement freethreads
 *         cond-signal TOWORKER
 *         release M
 * 
 * worker:
 *     init
 *     while true:
 *         claim M
 *         while no work in queue:
 *             cond-wait TOWORKER, M
 *         get work to local storage
 *         release M
 *         do work
 *         claim M
 *         increment freethreads
 *         cond-signal TOMAIN
 *         release M
 */

/* Return the number of CPUs we have on various architectures.
 * From http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine
 */

int threadpool::numCPU()
{
    int numCPU=1;			// default
#ifdef WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo( &sysinfo );
    numCPU = sysinfo.dwNumberOfProcessors;
#endif
#if defined(HW_AVAILCPU) && defined(HW_NCPU)
    int mib[4];
    size_t len=sizeof(numCPU);

    /* set the mib for hw.ncpu */
    memset(mib,0,sizeof(mib));
    mib[0] = CTL_HW;
    mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

    /* get the number of CPUs from the system */
    if(sysctl(mib, 2, &numCPU, &len, NULL, 0)){
	perror("sysctl");
    }

    if( numCPU <= 1 ) {
	mib[1] = HW_NCPU;
	sysctl( mib, 2, &numCPU, &len, NULL, 0 );
	if( numCPU < 1 ) {
	    numCPU = 1;
	}
    }
#endif
#ifdef _SC_NPROCESSORS_ONLN
    numCPU = sysconf(_SC_NPROCESSORS_ONLN);
#endif
    return numCPU;
}

/*
 * ERR_QUIT prints an error message, gets it out and then quits.
 * It used to be 'ERR', but this created a conflict on some SunOS
 * and SunOS derivatives. See 
 * http://dtrace.org/blogs/rm/2011/03/14/a-trip-down-into-sysregset-h/
 * for an example. On such systems 'ERR' is defined as '13'.
 */

void ERR_QUIT(int val,const char *msg) __attribute__ ((__noreturn__));
void ERR_QUIT(int val,const char *msg)
{
    std::cerr << msg << "\n";
    std::cerr.flush();
    exit(val);
}


/* Run non-portable pthread win32 startup */
void threadpool::win32_init()
{
#ifdef WIN32
    static bool initialized = false;
    if(initialized==false){
//	pthread_win32_process_attach_np();
//	pthread_win32_thread_attach_np();
	initialized=true;
    }
#endif
}


/**
 * Create the thread pool.
 * Each thread has its own feature_recorder_set.
 *
 * From the pthreads readme for mingw:
 * Define PTW32_STATIC_LIB when building your application. Also, your
 * application must call a two non-portable routines to initialise the
 * some state on startup and cleanup before exit. One other routine needs
 * to be called to cleanup after any Win32 threads have called POSIX API
 * routines. See README.NONPORTABLE or the html reference manual pages for
 * details on these routines:
 * 
 * BOOL pthread_win32_process_attach_np (void);
 * BOOL pthread_win32_process_detach_np (void);
 * BOOL pthread_win32_thread_attach_np (void); // Currently a no-op
 * BOOL pthread_win32_thread_detach_np (void);
 */

threadpool::threadpool(int numworkers_)
{
    numworkers		= numworkers_;
    freethreads		= numworkers;
    if(pthread_cond_init(&TOMAIN,NULL))   ERR_QUIT(1,"pthread_cond_init #1 failed");
    if(pthread_cond_init(&TOWORKER,NULL)) ERR_QUIT(1,"pthread_cond_init #2 failed");

    // lock while I create the threads
    M.lock();
    for(unsigned int i=0;i<numworkers;i++){
	class worker *w = new worker(this,i);
	push_back(w);
	pthread_create(&w->thread,NULL,worker::start_worker,(void *)w);
    }
    M.unlock();
}

threadpool::~threadpool()
{
    /* We previously sent the termination message to all of the sub-threads here.
     * However, their terminating caused wacky problems with the malloc library.
     * So we just leave them floating around now. Doesn't matter much, because
     * the main process will die soon enough.
     */
    kill_all_workers();
    /* Release our resources */
    pthread_cond_destroy(&TOMAIN);
    pthread_cond_destroy(&TOWORKER);

#ifdef WIN32
//    pthread_win32_process_detach_np();
//    pthread_win32_thread_detach_np();
#endif


}

/*
 * Send the message to kill the workers through
 */
void threadpool::kill_all_workers()
{
    M.lock();
    int worker_count = numworkers;
    M.unlock();
    while(worker_count>0){
	this->schedule_work(0);
	worker_count--;
    }
}

/** 
 * work is delivered in sbufs.
 * This blocks the caller if there are no free workers.
 */
void threadpool::schedule_work(file_data_hasher_t *fdht)
{
    M.lock();
    while(freethreads==0){
	// wait until a thread is free (doesn't matter which)
	if(pthread_cond_wait(&TOMAIN,&M.mutex)){
	    ERR_QUIT(1,"threadpool::schedule_work pthread_cond_wait failed");
	}
    }
    work_queue.push(fdht); 
    freethreads--;
    pthread_cond_signal(&TOWORKER);
    M.unlock();
}

unsigned int threadpool::get_free_count()
{
    M.lock();
    unsigned int ret = freethreads;
    M.unlock();
    return ret;
}

/* Run the worker.
 * Each worker runs run...
 */
void *worker::run()
{
    while(true){
	/* Get the lock, then wait for the queue to be empty.
	 * If it is not empty, wait for the lock again.
	 */
	master->M.lock();
	while(master->work_queue.empty()){
	    /* I didn't get any work; go back to sleep */
	    if(pthread_cond_wait(&master->TOWORKER,&master->M.mutex)){
		fprintf(stderr,"pthread_cond_wait error=%d\n",errno);
		exit(1);
	    }
	}
	file_data_hasher_t *fdht = master->work_queue.front(); // get the sbuf
	master->work_queue.pop();		   // pop from the list
	master->M.unlock();
	if(fdht==0) {
	    break;			// told to exit
	}
	do_work(fdht);
	master->M.lock();
	master->freethreads++;
	pthread_cond_signal(&master->TOMAIN); // tell the master that we are free!
	master->M.unlock();
    }
    master->M.lock();
    master->numworkers--;
    master->M.unlock();
    return 0;
}

bool threadpool::all_free() 
{
    return numworkers == get_free_count();
}

unsigned int threadpool::num_workers() 
{
    M.lock();
    unsigned int ret = numworkers;
    M.unlock();
    return ret;
}

void threadpool::wait_till_all_free()
{
    while(all_free()==false){
#ifdef HAVE_USLEEP
	usleep(50);
#else	    
	sleep(1);
#endif
    }
}


#endif
