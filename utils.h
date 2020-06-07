#include <iostream>
#include <cstring>
#include <stdlib.h>
#include <cmath>
#include <atomic>
#include <queue>
#include <mutex>
#include <thread>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <vector>
#include <string>
#include <condition_variable>
#include <array>
#include <assert.h>
#include <boost/thread/barrier.hpp>
#include <pthread.h>
#include <fstream>
#include <random>
#include "sha256.h"
using namespace std;

const uint64_t queue_uint64_thres = 1 << 11;

struct config_t {
/* read only variables */
	uint64_t  nodes;		// # of nodes
	uint64_t  level;	        // # of levels
	uint64_t  groups;	 	// # of groups
	uint64_t* map_table;	 	// # shuffled mapping table
//	uint64_t* thread_group_from;
//	uint64_t* thread_group_to;
//      uint64_t* groups_from;
//      uint64_t* groups_to;
	uint64_t  flag;
	uint64_t  total_threads;	// # of threads, defined by user
        uint64_t  wmax;
        uint64_t  thres;
	uint64_t  period;
	uint64_t barrier_period;
	uint64_t separate;
//	std::chrono::duration<double>* time;
/* shared variables */

//	uint64_t finished_threads;
//	atomic<uint64_t> failed_nodes;	// # of failed nodes
//	uint64_t* group_writes;
	boost::barrier* thread_barrier;
	atomic<uint64_t> total_writes;
	uint64_t failed_nodes;
//	uint64_t blocking_threads;	
//	std::condition_variable_any cv;
        std::mutex mutex;	// queue lock 
	pthread_barrier_t* barrier;
	std::ofstream* outputFile;
	
};
void consume_path_oram_requests(config_t* config, uint64_t id);
int increment_memory_line(uint64_t la, config_t* config);
inline void leaf_range(uint64_t level, uint64_t &from, uint64_t &to);
inline int check_group(const uint64_t la, const uint64_t nodes, const uint64_t groups, const uint64_t separate, const uint64_t total_threads, const uint64_t my_id);
inline uint64_t get_group_id(const uint64_t la, const uint64_t nodes, const uint64_t groups, const uint64_t separate);
inline uint64_t my_random_generator(std::default_random_engine& g, std::uniform_int_distribution<int>& d);
inline int consumer_thread_wait(config_t* config, const uint64_t thres, uint64_t& counter, const uint64_t barrier_period, uint64_t &failed_nodes_local, uint64_t& total_writes_local, uint64_t my_id);
inline uint64_t compute_offset_la(const uint64_t la, const uint64_t nodes,const uint64_t groups,const uint64_t separate);
inline int initialize_consumer_thread(
        config_t* config,
        uint64_t** &pa,
        uint64_t* &map_table,
        uint64_t* &start,
        uint64_t* &gap,
        uint64_t* &group_counter,
        uint64_t* &group_size,
        uint64_t  groups_this_thread,
        uint64_t  group_size_max,
        uint64_t  total_threads,
        uint64_t  my_id);


//inline int check_group(uint64_t la, uint64_t thread_id, uint64_t group_size_max, uint64_t total_threads);
//inline uint64_t my_random_generator(uint64_t from, uint64_t to, uint64_t counter);
