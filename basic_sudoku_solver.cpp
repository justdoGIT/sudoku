#include "bitutil.h"

#include <iostream>
#include <string.h>
#include <cstdint>
#include <tuple>
#include <vector>
#include <chrono>

using namespace std;
using namespace std::chrono;
namespace {

typedef uint32_t Bits;

constexpr Bits kAll = 0x1ff;        // 9 Bits
typedef tuple<int, int, int> RowColBox;

struct BasicSolver {
    array<Bits, 9> rows{}, cols{}, boxes{};
    vector<RowColBox> cells_todo;
    size_t limit = 1;
    bool min_heuristic = false;
    size_t num_todo = 0, num_guesses = 0, num_solutions = 0;

    // pick from among the most constrained cells (i.e., those with the fewest remaining candidates) in order to reduce the effective branching factor of our search.
    int NumCandidates(const RowColBox &row_col_box) {
        int row = get<0>(row_col_box);
        int col = get<1>(row_col_box);
        int box = get<2>(row_col_box);
        auto candidates = rows[row] & cols[col] & boxes[box];
        return NumBitsSet(candidates);
    }

    // move a cell with the fewest candidates to the head of the sublist [todo_index:end]
    void MoveBestTodoToFront(size_t todo_index) {
        auto first = cells_todo.begin() + todo_index;
        auto best = first;
        int best_count = NumCandidates(*best);
        for (auto next = first + 1; best_count > 1 && next < cells_todo.end(); ++next) {
            int next_count = NumCandidates(*next);
            if (next_count < best_count) {
                best_count = next_count;
                best = next;
            }
        }
        swap(*first, *best);
    }

    // Returns true if a solution is found, updates *solution to reflect assignments
    // made on solution path. Also updates num_guesses_ to reflect the number of
    // guesses made during search.
    void SatisfyGivenPartialAssignment(size_t todo_index, char *solution) {
        if (min_heuristic) MoveBestTodoToFront(todo_index);

        //int [row, col, box] = cells_todo_[todo_index];
        int row = get<0>(cells_todo[todo_index]);
        int col = get<1>(cells_todo[todo_index]);
        int box = get<2>(cells_todo[todo_index]);

        auto candidates = rows[row] & cols[col] & boxes[box];
        while (candidates) {
            uint32_t candidate = GetLowBit(candidates);

            // only count assignment as a guess if there's more than one candidate.
            if (candidates ^ candidate) num_guesses++;

            // clear the candidate from available candidate sets for row, col, box
            rows[row] ^= candidate;
            cols[col] ^= candidate;
            boxes[box] ^= candidate;

            solution[row * 9 + col] = (char) ('1' + LowOrderBitIndex(candidate));

            // recursively solve remaining cells and back out with the last solution.
            if (todo_index < num_todo) {
                SatisfyGivenPartialAssignment(todo_index + 1, solution);
            } else {
                ++num_solutions;
            }

            if (num_solutions == limit) return;

            // restore the candidate to available candidate sets for row, col, box
            rows[row] ^= candidate;
            cols[col] ^= candidate;
            boxes[box] ^= candidate;

            candidates = ClearLowBit(candidates);
        }
    }

    const int boxen[81] = {0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2,
                           3, 3, 3, 4, 4, 4, 5, 5, 5, 3, 3, 3, 4, 4, 4, 5, 5, 5, 3, 3, 3, 4, 4, 4, 5, 5, 5,
                           6, 6, 6, 7, 7, 7, 8, 8, 8, 6, 6, 6, 7, 7, 7, 8, 8, 8, 6, 6, 6, 7, 7, 7, 8, 8, 8};
  
    bool Initialize(const char *input, size_t limit_, uint32_t configuration, char *solution) {
        rows.fill(kAll);
        cols.fill(kAll);
        boxes.fill(kAll);
        limit = limit_;
        min_heuristic = configuration > 0;
        num_guesses = 0;
        num_solutions = 0;

        // copy initial clues to solution since our todo list won't include these cells
        memcpy(solution, input, 81);
        cells_todo.clear();

        for (int row = 0; row < 9; ++row) {
            for (int col = 0; col < 9; ++col) {
                int cell = row * 9 + col;
                int box = boxen[cell];
                if (input[row * 9 + col] == '.') {
                    // blank cell: add to the todo list
                    cells_todo.emplace_back(make_tuple(row, col, box));
                } else {
                    // a given clue: clear availability bits for row, col, and box
                    uint32_t value = 1u << (uint32_t) (input[cell] - '1');
                    if (rows[row] & value && cols[col] & value && boxes[box] & value) {
                        rows[row] ^= value;
                        cols[col] ^= value;
                        boxes[box] ^= value;
                    } else {
                        return false;
                    }
                }
            }
        }
        num_todo = cells_todo.size() - 1;
        return true;
    }
};
} // namespace

extern "C"
size_t TdokuSolverBasic(const char *input, size_t limit, uint32_t configuration,
                        char *solution, size_t *num_of_guesses) {
    static BasicSolver solver;
    if (solver.Initialize(input, limit, configuration, solution)) {
        solver.SatisfyGivenPartialAssignment(0, solution);
        *num_of_guesses = solver.num_guesses;
        return solver.num_solutions;
    } else {
        *num_of_guesses = 0;
        return 0;
    }
}

// main function
int main(int argc, const char **argv) {
    size_t limit = argc > 1 ? atoi(argv[1]) : 10000;

    char *puzzle = NULL;
    char solution[81];
    size_t size, guesses;
    auto data_start = high_resolution_clock::now();
    while (getline(&puzzle, &size, stdin) != -1) {
        solution[0] = '\0';
        auto start = high_resolution_clock::now();
        size_t count = TdokuSolverBasic(puzzle, limit, 0, solution, &guesses);
        if (limit > 1 && count == 1) {
            TdokuSolverBasic(puzzle, 1, 0, solution, &guesses);
        }
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop - start);
        cout << "Time taken by function: "
         << duration.count() << " microseconds" << endl;

        printf("%.81s:%ld:%.81s\n", puzzle, count, solution);
    }
    auto data_stop = high_resolution_clock::now();
    auto data_duration = duration_cast<seconds>(data_stop - data_start);
    cout << "Time taken by whole dataset: "
         << data_duration.count() << " seconds" << endl;
}