#include <iostream>
#include <fstream>
using namespace std;

static inline void swap(uint64_t* a, uint64_t* b) {
        uint64_t c = *a;
        *a = *b;
        *b = c;
        return;
}

int main(int argc, char** argv) {
	if (argc < 2) {
                std::cout << "ERROR, Enter number of levels\n";
                return 1;
        }

	const uint64_t level = (uint64_t) atoi(argv[1]);
	const uint64_t nodes            = (1 << level) - 1;

	uint64_t* mt = new uint64_t[nodes];	// map table
	for (uint64_t i = 0; i < nodes; i++) {
                mt[i] = i;
        }
        uint64_t tmp = 0;
        // randomly permutate array
        for (uint64_t i = 0; i < nodes; i++) {
                tmp = (uint64_t) rand() % nodes;
                swap(&mt[i], &mt[tmp]);
        }

	ofstream outputFile;
	outputFile.open("config.txt");
	outputFile << level << endl;
	outputFile << nodes << endl;
//	outputFile << groups << endl;
	for (uint64_t i = 0; i < nodes; i++) {
		outputFile << mt[i] << endl;
	}
	outputFile.close();
	return 0;
}
