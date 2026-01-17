#include "libraries.hpp"

enum class State {
    THINKING = 0,
    HUNGRY = 1,
    EATING = 2,
};

struct Column {
    int start;
    int width;
    string header;
};

unsigned int N{0};
int Y_max{0}, X_max{0};

WINDOW * win = nullptr;
Column col_id, col_state, col_progress, col_occurs;

vector<State> philo_state;
vector<int> eat_count;
vector<int> think_count;
vector<thread> threads;
vector<chrono::steady_clock::time_point> action_start_time;
vector<int> action_duration;

mutex stats_mtx;
mutex philo_mtx;
mutex output_mtx;

condition_variable cv;

atomic<bool> running{true};

void think(size_t);
void take_forks(size_t);
void eat(size_t);
void put_forks(size_t);
void test(size_t);
void signal_handler(int);
void draw_table_borders();
void display_thread();
size_t inline left(size_t id) { return (id + N - 1) % N; }
size_t inline right(size_t id) { return (id + 1) % N; }