#include <iostream>
#include <math.h>
#include <fstream>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <chrono>
#include <ctime>
#include <stdio.h>
#include <string.h>
//#include <set>
#include <new>          // std::bad_alloc
//#include <vector>
//#include "utils.h"
#include <assert.h>     /* assert */
#include <string>
using namespace std;

// function declarations
//static inline void swap(uint64_t* a, uint64_t* b);
inline int read_config_file(uint64_t& level, uint64_t& nodes, uint64_t** mt);
//uint64_t translate_group_id(uint64_t groups, uint64_t pid, uint64_t group_id);
inline uint64_t my_random_generator(uint64_t from, uint64_t to, uint64_t counter);
inline int check_group(uint64_t la, uint64_t thread_id, uint64_t group_size_max, uint64_t groups_per_thread);
inline void leaf_range(const uint64_t level, uint64_t &from, uint64_t &to);
inline void add_log(ofstream& file, uint64_t failed_nodes_local, uint64_t total_writes_local);
const uint64_t max_threads = 4;
const uint64_t z = 4;
const uint64_t block_size = 64;
const uint64_t period = 100;
const uint64_t barrier_period = 10000;
const std::string dir = "log/";
int main(int argc, char** argv) {
	if (argc < 5) {
		std::cout << "ERROR, enter wmax, groups, process id, total number of threads\n";
		return 1;
	}


	// define configuration variables
	const uint64_t wmax             = (uint64_t)atoi(argv[1]);
	const uint64_t groups           = (uint64_t)atoi(argv[2]);
	const uint64_t my_id            = (uint64_t)atoi(argv[3]);
	const uint64_t total_threads	= (uint64_t)atoi(argv[4]);
	uint64_t* map_table;
	uint64_t level 	= 0;
	uint64_t nodes	= 0;
	uint64_t from 	= 0;
	uint64_t to 	= 0;
	 if (read_config_file(level, nodes, &map_table) == 1) {
                 return 1;
         }

	const uint64_t memory_size      = (nodes + groups) * z * block_size;
	const uint64_t thres            = nodes / 100;
	const uint64_t group_size_max  	= (uint64_t)ceil((double)nodes / (double) groups);
	const uint64_t groups_per_thread = (uint64_t)ceil((double)groups / (double)total_threads);
	const uint64_t groups_this_thread = (my_id == (total_threads - 1)) ? (groups - my_id * groups_per_thread) : groups_per_thread;
        // define output log file
	std::string log_file_name = dir + "log_" + to_string(my_id) + ".txt";
        ofstream logFile(log_file_name);
	if (logFile.fail()) {
		std::cout << "ERROR, failed to open " << log_file_name << std::endl;
		return 1;
	}
        logFile << "memory size:        " << (memory_size >> 20) << "MB" << std::endl;
	logFile << "wmax:               " << wmax << std::endl;
        logFile << "level:              " << level << std::endl;
        logFile << "nodes:              " << nodes << std::endl;
	logFile << "total threads: 	" << total_threads << std::endl;
	logFile << "total groups:       " << groups << std::endl;
	logFile << "start_gap period:   " << period << std::endl;
	logFile << "barrier period: 	" << barrier_period << std::endl;

	uint64_t la 			= 0;
	uint64_t failed_nodes_local 	= 0;
	uint64_t counter 		= 0;
	uint64_t total_writes_local	= 0;

	// define pointers
	uint64_t* start;
	uint64_t* gap;
	uint64_t* group_counter;
	uint64_t* group_size;
	uint64_t**pa;

	std::cout << "begin initialization ...\n";
	// allocate space for start-gap variables
	if (read_config_file(level, nodes, &map_table) == 1) {
		return 1;
	}
	leaf_range(level, from, to);
	try {
		pa              = new uint64_t*[groups_this_thread];
		start           = new uint64_t [groups_this_thread];
		gap             = new uint64_t [groups_this_thread];
		group_counter   = new uint64_t [groups_this_thread];
		group_size      = new uint64_t [groups_this_thread];
	} catch (std::bad_alloc& ba) {
		std::cerr << "bad allocation caught " << ba.what() << std::endl;
		return 1;
	}

	// initialize all variables
	for (uint64_t i = 0; i < groups_this_thread; i++) {
		try {
			pa[i] = new uint64_t [group_size_max + 1];
		} catch (std::bad_alloc& ba) {
			std::cerr << "bad allocation caught" << ba.what() << std::endl;
			return 1;
		}
		group_size[i]           = group_size_max;
		gap[i]                  = group_size_max;
		memset(pa[i], 0, sizeof(uint64_t) * (group_size_max + 1));
	}

	memset(group_counter, 0, sizeof(uint64_t) * groups_this_thread);
	memset(start,         0, sizeof(uint64_t) * groups_this_thread);

	if (my_id == total_threads - 1) {
		group_size[groups_this_thread - 1] = nodes - group_size_max * (groups - 1);
		gap[groups_this_thread - 1] = group_size[groups_this_thread - 1] + 1;
	}

	std::cout << "Finish initialization\n";

	auto start_time = std::chrono::system_clock::now();

	while (1) {
		// get a job from request queue
		uint64_t leaf_id = my_random_generator(from, to, counter);
		leaf_id = (uint64_t) rand() % (to-from+1) + from;
		for (uint64_t i = 0; i < level; i++) {
			counter++;
			la = map_table[leaf_id];
			leaf_id = (leaf_id - 1) >> 1;
			// then process the job
			if (check_group(la, my_id, group_size_max, groups_per_thread) == 1) {
				// increment_memory_line
				uint64_t group_id = (la / group_size_max) % groups_per_thread;
				// perform start-gap logic
				la = la % group_size_max;
				uint64_t tmp_pa = (la + start[group_id]) % group_size[group_id];
				tmp_pa = tmp_pa < gap[group_id] ? tmp_pa : tmp_pa + 1;
				(group_counter[group_id])++;
				(pa[group_id][tmp_pa])++;
				if (pa[group_id][tmp_pa] >= wmax) {
					failed_nodes_local++;
					pa[group_id][tmp_pa] = 0;
					if (failed_nodes_local >= thres) {
						add_log(logFile, failed_nodes_local, total_writes_local);
						goto L1;
					}
				}
				if (group_counter[group_id] % period == 0) {
					(gap[group_id])--;
					if (gap[group_id] == 0) {
						gap[group_id] = group_size[group_id];
						start[group_id] = (start[group_id] + 1) % (gap[group_id]);
					}
				}
				total_writes_local++;
			}
			if (counter % barrier_period == 0) {
				add_log(logFile, failed_nodes_local, total_writes_local);
				if (failed_nodes_local >= thres) {
					goto L1;
				}
			}
		}
	}

L1:
	auto end_time = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end_time - start_time;
	std::cout << "elapsed time is " << elapsed_seconds.count() << std::endl;

	delete[] map_table;
	delete[] start;
	delete[] gap;
	delete[] group_size;
	delete[] group_counter;
	for (uint64_t i = 0; i < groups_this_thread; i++) {
		delete[] pa[i];
	}
	delete[] pa;
	return 0;
	return 0;
}

inline void add_log(ofstream& file, uint64_t failed_nodes_local, uint64_t total_writes_local) {
	if (file.is_open() == false) {
		std::cerr << "ERROR: log file is not open\n";
		return;
	}
	std::cout << failed_nodes_local << " " << total_writes_local << std::endl;
	file << failed_nodes_local << " " << total_writes_local << std::endl;
	return;
}

inline int read_config_file(uint64_t& level, uint64_t& nodes, uint64_t** mt) {
	ifstream inputFile("config.txt");
	if (inputFile.fail()) {
		cout << "ERROR: cannot open config.txt\n";
		return 1;
	}
	inputFile >> level >> nodes;
	try {
		*mt = new uint64_t [nodes];
	} catch (std::bad_alloc & ba) {
		std::cerr << "bad_allocation caught: " << ba.what() << '\n';
		return 1;
	}
	assert(nodes == (((uint64_t)1 << level) - 1));
	for (uint64_t i = 0; i < nodes; i++) {
		inputFile >> (*mt)[i];
	}
	inputFile.close();
	return 0;
}

inline void leaf_range(const uint64_t level, uint64_t &from, uint64_t &to) {
	from = (uint64_t) ((uint64_t)1 << (level - 1)) - 1;
	to   = (uint64_t) ((uint64_t)1 << level) - 2;
}

inline int check_group(uint64_t la, uint64_t thread_id, uint64_t group_size_max, uint64_t groups_per_thread) {
	uint64_t group_id = la / group_size_max;
	return ((group_id / groups_per_thread) == thread_id);
}

inline uint64_t my_random_generator(uint64_t from, uint64_t to, uint64_t counter) {
	return (uint64_t) rand() % (to - from + 1) + from;
	return counter % (to - from + 1) + from;
}
