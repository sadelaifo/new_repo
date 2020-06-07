#include "utils.h"

// compute the range of leaf ids
inline void leaf_range(const uint64_t level, uint64_t &from, uint64_t &to) {
	from = (uint64_t) (1 << (level - 1)) - 1;
	to   = (uint64_t) (1 << level) - 2;
}

// check whether the calling thread (specified by my_id) is responsible for logic address (la)
inline int check_group(const uint64_t la, const uint64_t nodes, const uint64_t groups, const uint64_t separate, const uint64_t total_threads, const uint64_t my_id) {
	uint64_t group_id = get_group_id(la, nodes, groups, separate);
	return (group_id / (groups / total_threads) == my_id);
}

// check whether 
inline uint64_t get_group_id(const uint64_t la, const uint64_t nodes, const uint64_t groups, const uint64_t separate) {
	uint64_t nodes_per_group = nodes / groups;
	return (la > separate) ? 1 + (separate / (nodes_per_group + 1) + (la - separate - 1) / nodes_per_group) : (la / (nodes_per_group + 1));
}

inline uint64_t compute_offset_la(const uint64_t la, const uint64_t nodes, const uint64_t groups, const uint64_t separate) {
	uint64_t nodes_per_group = nodes / groups;
	if (la > separate) {
		return (la - separate - 1) % nodes_per_group;
	}
	return la % (nodes_per_group + 1);
}

inline uint64_t compute_offset_group(const uint64_t group_id, const uint64_t groups, const uint64_t total_threads) {
	return group_id % (groups / total_threads);
}

inline uint64_t my_random_generator(
	std::default_random_engine& generator,
	std::uniform_int_distribution<uint64_t>& d
	) {
//	return counter % (to - from + 1) + from;
//	return (uint64_t) rand() % (to - from + 1) + from;

	return (uint64_t) d(generator);

/*
	sha256.add(&counter, sizeof(counter));
	std::string s = sha256.getHash();
	uint64_t num = 0;
	for (uint64_t i = 0; i < sizeof(counter); i++) {
		num += (uint64_t)(s[i] - '0') * (uint64_t)pow((double)100, (double)i);
//		num += (uint64_t)(s[i] - '0') << 7 * i;
	}
*/
//	return num % (to - from + 1) + from;
//	return counter % (to - from + 1) + from;
}

inline void cleanup(uint64_t** pa, uint64_t* start, uint64_t* gap, uint64_t* group_counter, uint64_t* group_size, const uint64_t groups_per_thread) {
//	delete[] start;
//	delete[] gap;
//	delete[] group_counter;
//	delete[] group_size;
//	for (uint64_t i = 0; i < groups_per_thread; i++) {
//		delete[] pa[i];
//	}
//	delete[] pa;
	return;
}

inline int initialize_consumer_thread(
	config_t* config, 
	uint64_t** &pa, 
	uint64_t* &map_table,
	uint64_t* &start, 
	uint64_t* &gap, 
	uint64_t* &group_counter, 
	uint64_t* &group_size, 
	uint64_t  groups_this_thread,
	uint64_t  total_threads,
	uint64_t  my_id) {

	uint64_t from 	= my_id * groups_this_thread;	
	uint64_t to	= (my_id + 1) * groups_this_thread - 1;

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
	map_table = config->map_table;
	memset(group_counter, 0, sizeof(uint64_t) * groups_this_thread);
	memset(start,         0, sizeof(uint64_t) * groups_this_thread);

	for (uint64_t group_id = from; group_id <= to ; group_id++) {
		uint64_t i = group_id % groups_this_thread;
		if (group_id < config->nodes % config->groups) {
			group_size[i] = 1 + config->nodes / config->groups;
		} else {
			group_size[i] = config->nodes / config->groups;
		}
		gap[i] = group_size[i];
		try {
                        pa[i] = new uint64_t [gap[i]];
                } catch (std::bad_alloc& ba) {
                        std::cerr << "bad allocation caught" << ba.what() << std::endl;
                        return 1;
                }

		memset(pa[i], 0, sizeof(uint64_t) * gap[i]);
	}
	return 0;
}

// synchronize the calling thread with all other threads,
// return 1 if the total number of failed nodes > 1%
// return 0 otherwise
inline int consumer_thread_wait(config_t* config, const uint64_t thres, uint64_t& counter, const uint64_t barrier_period, uint64_t &failed_nodes_local, uint64_t& total_writes_local, const uint64_t my_id, const uint64_t print_period) {
	if (counter % barrier_period == 0) {
		config->mutex.lock();
		if (counter % print_period == 0) {
			std::cout << "Thread id = " << my_id << "; Counter = " << counter << "; Total writes in this thread is " << total_writes_local << "; Failed nodes total is " << config->failed_nodes << std::endl;	
			(*config->outputFile) << "Thread id = " << my_id << "; Counter = " << counter << "; Total writes in this thread is " << total_writes_local << "; Failed nodes total is " << config->failed_nodes << std::endl;
		}
		config->failed_nodes += failed_nodes_local;
		config->mutex.unlock();
		config->thread_barrier->wait();

//		pthread_barrier_wait(config->barrier);

		if (config->failed_nodes >= thres) {
			config->total_writes += total_writes_local;
			// config->time[my_id] = std::chrono::system_clock::now() - start_time;
			return 1;
		}
		failed_nodes_local = 0;
	}
	return 0;
}

void consume_path_oram_requests(config_t* config, uint64_t my_id) {
	auto init_time = std::chrono::system_clock::now();
	
	// declare local const variables
	const uint64_t nodes		= config->nodes;
	const uint64_t wmax		= config->wmax;
	const uint64_t thres		= config->thres;
	const uint64_t period 		= config->period;
	const uint64_t groups		= config->groups;
        const uint64_t barrier_period 	= config->barrier_period;
	const uint64_t level 		= config->level;
	const uint64_t separate 	= config->separate;
	const uint64_t total_threads 	= config->total_threads;
	const uint64_t print_period	= config->print_period * barrier_period;
//	SHA256 sha256;

	// declare local variables
	uint64_t la 			= 0;
	uint64_t from 			= 0;
	uint64_t to 			= 0;
	uint64_t counter 		= 0;
	uint64_t failed_nodes_local 	= 0;
	uint64_t total_writes_local 	= 0;
        uint64_t groups_this_thread 	= groups / total_threads;
//	uint64_t flag			= config->flag;
	// declare local pointers
	uint64_t **pa;
	uint64_t *start;
	uint64_t *gap;
	uint64_t *group_counter;
	uint64_t *group_size;
	uint64_t *map_table;

	// init physical address array, 
	// start, gap, counter
	leaf_range(level, from, to);
	if (initialize_consumer_thread(
				config, 
				pa, 
				map_table,
				start, 
				gap, 
				group_counter, 
				group_size, 
				groups_this_thread, 
				total_threads, 
				my_id) == 1) {
		std::cerr << "ERROR, consumer thread init fails\n";
		return;
	}

        std::default_random_engine generator;
        std::uniform_int_distribution<uint64_t> d(from, to);

	std::chrono::duration<double> init_elapsed_time = std::chrono::system_clock::now() - init_time;

	// finish init
	std::cout << "Thread " << my_id << " has finished init within " << init_elapsed_time.count() << "seconds\n";


	// start simulation
	while (1) {
		// get a job from request queue
		uint64_t leaf_id = my_random_generator(generator, d);
//		counter++;
		for (uint64_t i = 0; i < level; i++) {
			counter++;
			//			la = transfer_matrix(leaf_id);
			la = map_table[leaf_id];
			leaf_id = (leaf_id - 1) >> 1;
			// then process the job
			// if this ORAM request lies in this thread
			if (check_group(la, nodes, groups, separate, total_threads, my_id) == 1) {
				uint64_t group_id = get_group_id(la, nodes, groups, separate);
				group_id = compute_offset_group(group_id, groups, total_threads);
				assert(group_id < groups_this_thread);
				// perform start-gap logic
				la = compute_offset_la(la, nodes, groups, separate);
				uint64_t tmp_pa = (la + start[group_id]) % group_size[group_id];
				tmp_pa = tmp_pa < gap[group_id] ? tmp_pa : tmp_pa + 1;

				assert(tmp_pa < group_size[group_id] + 1);

				(group_counter[group_id])++;
				(pa[group_id][tmp_pa])++;
//				if (flag == 1) {
//					cout << "group_id = " << group_id << "; offset = " << tmp_pa << endl;
//				}
				if (pa[group_id][tmp_pa] >= wmax) {
//					for (uint64_t j = 0; j < nodes
					failed_nodes_local++;
					pa[group_id][tmp_pa] = 0;
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

			if (consumer_thread_wait(config, thres, counter, barrier_period, failed_nodes_local, total_writes_local, my_id, print_period) == 1) {
				cleanup(pa, start, gap, group_counter, group_size, groups_this_thread);
				return;
			}
		}

	}
	//	config->cv.notify_all();
	std::cerr << "error, shouldn't come here \n";
	return;
}
