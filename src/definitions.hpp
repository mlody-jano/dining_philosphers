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


void think(size_t);
void take_forks(size_t);
void eat(size_t);
void put_forks(size_t);
void test(size_t);
void signal_handler(int);
void draw_table_borders();
void display_thread();
size_t inline left(size_t id);
size_t inline right(size_t id);

mutex critical_region_mtx;

mutex output_mtx;