#include <iostream>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <chrono>
#include <fstream>
#include <ctime>
#include <stdio.h>
#include <string.h>
#include <new>          // std::bad_alloc
#include <thread>
#include <vector>
#include <mutex>          // std::mutex
#include "utils.h"
#include <assert.h>     /* assert */
#include <boost/thread/barrier.hpp>
#include <pthread.h>

using namespace std;
static inline void swap(uint64_t* a, uint64_t* b);
void shuffle_map_table(uint64_t* mt, const uint64_t size);

const uint64_t z = 4;
const uint64_t block_size = 64;
const uint64_t period = 100;
std::vector<std::thread> consumer;
/*
void xxx(pthread_barrier_t* b, int id) {
	int c = 0;
	while (true) {
		while (c < 5) {
			c++;
		}
		pthread_barrier_wait(b);
		cout << "thread " << id << " finished " << c << " waits \n";
	}
	return;
}
*/
int main(int argc, char** argv) {
	//	srand((unsigned int) time(NULL));
	srand((unsigned int) time(NULL));
	//	for (int i = 0; i < 1000; i++) {
	//		cout << rand() % 1000 << endl;
	//	}
	//	return 0;

	pthread_barrier_t b;
	pthread_barrier_init(&b, NULL, 3);

//	for (int i = 0; i < 3; i++) {
//		consumer.push_back(std::thread(xxx, &b, i));
//	}
	
	
  //      for (int i = 0; i < 3; i++) {
    //            consumer[i].join();
      //  }
//	std::thread t1(xxx, &b, 0);
//	std::thread t2(xxx, &b, 1);
//	std::thread t3(xxx, &b, 2);

//	t1.join();
//	t2.join();
//	t3.join();

//	return 0;

	if (argc < 8) {
		std::cout << "ERROR, enter wmax, level, groups, threads, barrier period, print period, tree top levels\n" 
			<< "Wmax refers to the number of writes before a memory line dies\n"
			<< "Level refers to the size of ORAM tree\n"
			<< "Groups refers to the number of groups you want to simulate with start gap\n"
			<< "threads refers to the number of threads you want to use to speed up computation\n"
			<< "Barrier period refers to the number of ORAM requests synchronized. Increasing this number will speed up computation, but will also lose precision\n"
			<< "Print period refers to the period you want to print out status. The actual print period is computed by barrier period * print period\n"
			<< "Tree top levels refers to the number of levels that can be cached into a tree top cache. ";

		return 1;
	}	
	const uint64_t wmax 		= (uint64_t)atoi(argv[1]);
	const uint64_t level 		= (uint64_t)atoi(argv[2]);
	const uint64_t groups 		= (uint64_t)atoi(argv[3]);
	const uint64_t num_threads 	= (uint64_t)atoi(argv[4]);
	const uint64_t barrier_period   = (uint64_t)atoi(argv[5]);
	const uint64_t print_period	= (uint64_t)atoi(argv[6]);
	const uint64_t tree_top_level	= (uint64_t)atoi(argv[7]);
	const uint64_t nodes 		= ((uint64_t) 1 << level) - 1;
	const uint64_t size 		= nodes;
	const uint64_t memory_size 	= (nodes + groups) * z * block_size;
	const uint64_t thres 		= nodes / 100;
	const uint64_t separate		= (nodes % groups) * (nodes / groups + 1) - 1;


	//	const uint64_t group_size 	= (uint64_t) ceil((double)nodes / (double)groups);
	if (groups % num_threads != 0 || groups < num_threads || groups > nodes) {
		std::cerr << "Error, please ensure groups % threads == 0, and groups < nodes\n";
		return 1;
	}

	if (tree_top_level <= level) {
		std::cerr << "Error, please ensure tree top cache < memory size\n";
		return 1;
	}
	//	assert(barrier_period % level == 0);	
	uint64_t* 	map_table;
	//	uint64_t* 	thread_group_from;
	//	uint64_t*	thread_group_to;
	//	uint64_t* 	groups_from;
	//	uint64_t* 	groups_to;
	//	std::chrono::duration<double>* 	time;
	config_t 	config;

	std::cout << "Enter wmax, level, groups, threads, barrier period\n";
	std::cout << wmax << " " << level << " " << groups << " " << num_threads << " " << barrier_period << std::endl;

	// initialize configuration values
	std::cout << "Initializing ...\n";
	try {
		map_table		= new uint64_t  [nodes];
		//		time			= new std::chrono::duration<double> [num_threads];
		//		groups_from		= new uint64_t [groups];
		//		groups_to		= new uint64_t [groups];
		//		thread_group_from	= new uint64_t [num_threads];
		//		thread_group_to		= new uint64_t [num_threads];
	} catch (std::bad_alloc & ba) {
		std::cerr << "bad_allocation caught: " << ba.what() << '\n';
		return 1;
	}
	/*	thread_group_from[0] 	= 0;
		thread_group_to[0] 	= groups / num_threads + (groups % num_threads != 0) - 1;
		groups_from[0] 		= 0;
		groups_to[0]		= nodes / groups + (nodes % groups != 0) - 1;
		for (uint64_t i = 1; i < groups; i++) {
		groups_from[i] 	= groups_to[i - 1] + 1;
		groups_to[i] 	= groups_from[i] - 1 + nodes / groups + (i < (nodes % groups));
		}
		assert(groups_to[groups - 1] == nodes - 1);
		for (uint64_t i = 1; i < num_threads; i++) {
		thread_group_from[i] = thread_group_to[i - 1] + 1;
		thread_group_to[i] = thread_group_from[i] - 1 + groups / num_threads + (i < (groups % num_threads));
		}
	 */
	//	assert(thread_group_to[num_threads - 1] == groups - 1);
	// initialize & randomly permutate memory lines
	shuffle_map_table(map_table, size);
	boost::barrier thread_barrier((unsigned int) num_threads);
	//	config.thread_group_from= thread_group_from;
	//	config.thread_group_to	= thread_group_to;
	//	config.groups_from	= groups_from;
	//	config.groups_to	= groups_to;
	std::string file_name = "logs_level_" + level + "_group_" + group + "_treeTop_" + tree_top_level + ".txt";
	ofstream outputFile("log/logs.txt");
	config.thread_barrier 	= &thread_barrier;
	config.nodes 		= nodes;
	config.groups 		= groups;
	config.map_table	= map_table;
	config.level		= level;
	config.total_threads 	= num_threads;
	config.wmax 		= wmax;
	config.thres 		= thres;
	config.period 		= period;
	config.failed_nodes 	= 0;
	config.barrier_period   = barrier_period;
	config.total_writes 	= 0;
	config.outputFile	= &outputFile;
	config.print_period	= print_period;
	//	config.blocking_threads = 0;
	config.separate		= separate;
	config.barrier		= (pthread_barrier_t*)malloc(sizeof(pthread_barrier_t));
	config.tree_top_level_upper = tree_top_level;
	config.tree_top_level_lower = 0;
	pthread_barrier_init(config.barrier, NULL, (unsigned)num_threads);

	//	config.time		= time;
	// create new producer threa
	printf("Finish initialization\n");
	printf("Begin simulating start-gap\n");

	auto start_time = std::chrono::system_clock::now();	

	// create new consumer threads
	for (uint64_t i = 0; i < num_threads; i++) {
		consumer.push_back(std::thread(consume_path_oram_requests, &config, i));
	}

	// wait for return status
	for (uint64_t i = 0; i < consumer.size(); i++) {
		consumer[i].join();
	}

	//	producer.join();

	auto end_time = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end_time - start_time;

	// count the total # of writes
	uint64_t c = config.total_writes;

	double lifetime = (double) c / (double) wmax / (double) (nodes + groups);
	// finish up
	// print status & free memory
	std::cout << "elapsed time is " << elapsed_seconds.count() << std::endl;
	//	for (uint64_t i = 0; i < num_threads; i++) {
	//		std::cout << "elapsed time for thread " << i << " is: " << config.time[i].count() << std::endl;
	//	}
	std::cout << "*******************************************\n";
	std::cout << "total write is " << c << std::endl;
	std::cout << "memory size:        " << (memory_size >> 20) << "MB" << std::endl;
	std::cout << "level:              " << level << std::endl;
	std::cout << "nodes:              " << nodes << std::endl;
	std::cout << "ideal total write : " << wmax * nodes << std::endl;
	std::cout << "wmax=" << wmax << "; level=" << level << "; group=" << groups << " percent lifetime :  " << lifetime << std::endl;
	// this line is only used for do_sg.sh to easily grep the lifetime 
	std::cout << "pgs=" << lifetime << std::endl;
	std::cout << "*******************************************\n";
	if (outputFile.fail()) {
		cout << "ERROR, file not found\n";
	} else {
		outputFile << "elapsed time is " << elapsed_seconds.count() << std::endl;
		//		for (uint64_t i = 0; i < num_threads; i++) {
		//			outputFile << "elapsed time for thread " << i << " is: " << config.time[i].count() << std::endl;
		//		}
		outputFile << "total write is " << c << std::endl;
		outputFile << "memory size:        " << (memory_size >> 20) << "MB" << std::endl;
		outputFile << "level:              " << level << std::endl;
		outputFile << "nodes:              " << nodes << std::endl;
		outputFile << "ideal total write : " << wmax * nodes << std::endl;
		outputFile << "wmax=" << wmax << "; level=" << level << "; group=" << groups << " percent lifetime :  " << lifetime << std::endl;
	}

	//	delete []group_lock;
	//	delete []group_writes;
	//	delete []group_counter;
	//	delete []group_size_vector;
	delete []map_table;
	//	delete []start;
	//	delete []gap;
	//	for (uint64_t i = 0; i < groups; i++) {
	//		delete [] pa[i];
	//	}
	//	delete []pa;
}

static inline void swap(uint64_t* a, uint64_t* b) {
	uint64_t c = *a;
	*a = *b;
	*b = c;
	return;
}

void shuffle_map_table(uint64_t* mt, const uint64_t size) {
	for (uint64_t i = 0; i < size; i++) {
		mt[i] = i;
	}
	uint64_t tmp = 0;
	// randomly permutate array
	for (uint64_t i = 0; i < size; i++) {
		tmp = (uint64_t) rand() % size;
		swap(&mt[i], &mt[tmp]);
	}
}
