#include "tdoku.h"

#include <chrono>
#include <iostream>


using namespace std;
using namespace std::chrono;

int main(int argc, const char **argv) {
    size_t limit = argc > 1 ? atoi(argv[1]) : 10000;

    char *puzzle = NULL;
    char solution[81];
    size_t size, guesses;
    auto data_start = high_resolution_clock::now();
    while (getline(&puzzle, &size, stdin) != -1) {
        solution[0] = '\0';
        auto start = high_resolution_clock::now();
        size_t count = SudokuSolve(puzzle, limit, 0, solution, &guesses);
        if (limit > 1 && count == 1) {
            SudokuSolve(puzzle, 1, 0, solution, &guesses);
        }
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop - start);
        cout << "Time taken by function: " << duration.count() << " microseconds" << endl;

        printf("%.81s:%ld:%.81s\n", puzzle, count, solution);
    }
    auto data_stop = high_resolution_clock::now();
    auto data_duration = duration_cast<seconds>(data_stop - data_start);
    cout << "Time taken by whole dataset: " << data_duration.count() << " seconds" << endl;
    return 0;
}
