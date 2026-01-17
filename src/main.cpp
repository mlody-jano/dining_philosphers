#include "libraries.hpp"
#include "definitions.hpp"

void signal_handler(int signum) {
    running = false;
    cv.notify_all(); // Wake up any waiting threads
    // Clean up ncurses before exit
    if (win) {
        delwin(win);
    }
    endwin();
    exit(0);
}

void test(size_t id) {
    // A philosopher can eat if they are hungry and both neighbors are not eating
    if (philo_state[id] == State::HUNGRY &&
        philo_state[left(id)] != State::EATING &&
        philo_state[right(id)] != State::EATING) {
        philo_state[id] = State::EATING;
    }
}

void think(size_t id) {
    // Simulate thinking for a random duration between 2 and 7 seconds
    size_t duration = rand() % 5000 + 2000;
    {
        lock_guard<mutex> lock(stats_mtx); // Change stats under protection from lock_guard
        think_count[id]++;
        action_start_time[id] = steady_clock::now();
        action_duration[id] = duration;
    }
    this_thread::sleep_for(milliseconds(duration));
}

void take_forks(size_t id) {
    unique_lock<mutex> lock(philo_mtx); // Use unique_lock for condition_variable wait, rather than lock_guard
    philo_state[id] = State::HUNGRY;
    test(id);

    while(philo_state[id] != State::EATING && running) {
        cv.wait(lock); // Wait until notified
    }
}

void eat(size_t id) {
    size_t duration = rand() % 5000 + 2000;
    {
        lock_guard<mutex> lock(stats_mtx); // Change stats under protection from lock_guard
        eat_count[id]++;
        action_start_time[id] = steady_clock::now();
        action_duration[id] = duration;
    }
    this_thread::sleep_for(milliseconds(duration));
}

void put_forks(size_t id) {
    lock_guard<mutex> lock(philo_mtx); // Use lock_guard for simple locking, as no waiting is needed here
    philo_state[id] = State::THINKING;
    test(left(id));
    test(right(id));
    cv.notify_all();
}

void philosopher(size_t id) {
    // Philosopher routine loop
    while (running) {
        think(id);
        if (!running) break;
        take_forks(id);
        if (!running) break;
        eat(id);
        if (!running) break;
        put_forks(id);
    }
}

void draw_table_borders() {
    int table_width = col_occurs.start + col_occurs.width + 1;
    int table_height = N + 4;
    
    // Top border
    mvwaddch(win, 0, 0, '+');
    mvwhline(win, 0, 1, '-', table_width - 1);
    mvwaddch(win, 0, table_width, '+');
    
    // Header separator
    mvwaddch(win, 2, 0, '+');
    mvwhline(win, 2, 1, '-', table_width - 1);
    mvwaddch(win, 2, table_width, '+');
    
    // Bottom border
    mvwaddch(win, table_height - 1, 0, '+');
    mvwhline(win, table_height - 1, 1, '-', table_width - 1);
    mvwaddch(win, table_height - 1, table_width, '+');
    
    // Vertical borders
    for (int row = 1; row < table_height - 1; row++) {
        mvwaddch(win, row, 0, '|');
        mvwaddch(win, row, table_width, '|');
        
        if (row != 2) {
            // Column separators
            mvwaddch(win, row, col_state.start - 1, '|');
            mvwaddch(win, row, col_progress.start - 1, '|');
            mvwaddch(win, row, col_occurs.start - 1, '|');
        } else {
            // Intersections on header line
            mvwaddch(win, row, col_state.start - 1, '+');
            mvwaddch(win, row, col_progress.start - 1, '+');
            mvwaddch(win, row, col_occurs.start - 1, '+');
        }
    }
}

void display_thread() {
    const char* state_names[] = {"THINKING", "HUNGRY", "EATING"};
    
    // Define color pairs
    // 1 = Yellow for THINKING
    // 2 = Red for HUNGRY
    // 3 = Green for EATING
    
    int progress_bar_width = col_progress.width - 4; // Leave space for brackets and padding
    
    while (running) {
        {
            lock_guard<mutex> lock(output_mtx); // Protect ncurses output from interferences, only this thread can write to ncurses
            
            for (size_t i = 0; i < N; i++) {
                int row = i + 3;
                
                State current_state;
                {
                    lock_guard<mutex> lock(philo_mtx); // Read under protection from lock_guard
                    current_state = philo_state[i];
                }
                
                // Select color based on state
                int color_pair = 0;
                if (current_state == State::THINKING) {
                    color_pair = 1; // Yellow
                } else if (current_state == State::HUNGRY) {
                    color_pair = 2; // Red
                } else if (current_state == State::EATING) {
                    color_pair = 3; // Green
                }
                
                // Column 1: ID with color
                wattron(win, COLOR_PAIR(color_pair));
                mvwprintw(win, row, col_id.start, " %2zu", i);
                wattroff(win, COLOR_PAIR(color_pair));
                
                // Column 2: State with color
                wattron(win, COLOR_PAIR(color_pair));
                mvwprintw(win, row, col_state.start, " %-9s", state_names[static_cast<int>(current_state)]);
                wattroff(win, COLOR_PAIR(color_pair));
                
                // Column 3: Progress bar
                int progress_filled = 0;
                if (current_state == State::THINKING || current_state == State::EATING) {
                    auto now = steady_clock::now();
                    int elapsed, total;
                    {
                        lock_guard<mutex> lock(stats_mtx); // Read under protection from lock_guard
                        elapsed = duration_cast<milliseconds>(
                            now - action_start_time[i]).count();
                        total = action_duration[i];
                    }
                    
                    if (total > 0) {
                        float progress = (float)elapsed / total;
                        if (progress > 1.0f) progress = 1.0f;
                        progress_filled = (int)(progress * progress_bar_width);
                    }
                }
                
                // Build progress bar string with color
                string bar = " [";
                for (int j = 0; j < progress_bar_width; j++) {
                    bar += (j < progress_filled) ? '#' : ' ';
                }
                bar += "]";
                
                // Pad to full column width
                while (bar.length() < col_progress.width) bar += " ";
                
                wattron(win, COLOR_PAIR(color_pair));
                mvwprintw(win, row, col_progress.start, "%s", bar.substr(0, col_progress.width).c_str());
                wattroff(win, COLOR_PAIR(color_pair));
                
                // Column 4: Occurrences with color
                int eats, thinks;
                {
                    lock_guard<mutex> lock(stats_mtx); // Read under protection from lock_guard
                    eats = eat_count[i];
                    thinks = think_count[i];
                }
                wattron(win, COLOR_PAIR(color_pair));
                mvwprintw(win, row, col_occurs.start, " Eat:%3d Think:%3d", eats, thinks);
                wattroff(win, COLOR_PAIR(color_pair));
            }
            
            wrefresh(win);
        }
        
        this_thread::sleep_for(milliseconds(100)); // Update every 100 ms
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <number_of_philosophers>" << endl; // Check for argument
        return 1;
    }
    
    N = atoi(argv[1]);
    
    if (N < 5) {
        cerr << "Number of philosophers must be at least 5" << endl; // Check for N > 5
        return 1;
    }
    
    signal(SIGINT, signal_handler); // Register signal handler for graceful termination
    
    philo_state.resize(N, State::THINKING);
    eat_count.resize(N, 0);
    think_count.resize(N, 0);
    action_start_time.resize(N);
    action_duration.resize(N, 0);
    
    for (size_t i{0}; i < N; i++) {
        action_start_time[i] = steady_clock::now();
    }
    
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    
    // Initialize colors
    if (has_colors()) {
        start_color();
        // Define color pairs
        init_pair(1, COLOR_YELLOW, COLOR_BLACK);  // THINKING
        init_pair(2, COLOR_RED, COLOR_BLACK);     // HUNGRY
        init_pair(3, COLOR_GREEN, COLOR_BLACK);   // EATING
    }
    
    getmaxyx(stdscr, Y_max, X_max);
    
    // Define column structure

    // ID Column 
    col_id.start = 1;
    col_id.width = 4;
    col_id.header = "ID";
    
    // State Column
    col_state.start = col_id.start + col_id.width + 1;
    col_state.width = 11;
    col_state.header = "State";
    
    int remaining_width = X_max - col_state.start - col_state.width - 25; // 25 for occurrences
    int progress_width = remaining_width - 2;
    if (progress_width < 25) progress_width = 25;
    if (progress_width > 50) progress_width = 50;
    
    // Progress Column
    col_progress.start = col_state.start + col_state.width + 1;
    col_progress.width = progress_width;
    col_progress.header = "Progress";
    
    // Occurrences Column
    col_occurs.start = col_progress.start + col_progress.width + 1;
    col_occurs.width = 21;
    col_occurs.header = "Occurrences";
    
    int table_width = col_occurs.start + col_occurs.width + 1;
    win = newwin(N + 4, table_width + 1, 0, 0); // Define the pointer to the window performing the visualization
    
    // Draw table structure
    draw_table_borders();
    
    // Print headers
    mvwprintw(win, 1, col_id.start + 1, "%s", col_id.header.c_str());
    mvwprintw(win, 1, col_state.start + 1, "%s", col_state.header.c_str());
    mvwprintw(win, 1, col_progress.start + 1, "%s", col_progress.header.c_str());
    mvwprintw(win, 1, col_occurs.start + 1, "%s", col_occurs.header.c_str());
    
    wrefresh(win); // Refresh window to show the table
    
    threads.reserve(N + 1); // Reserve space for philosopher threads + display thread
    
    for (size_t i{0}; i < N; ++i) {
        threads.emplace_back(philosopher, i); // Create philosopher threads
    }
    
    threads.emplace_back(display_thread); // Create display thread

    for (auto& t : threads) {
        t.join(); // Join all thread to the main thread
    }
    
    // Clean up ncurses
    delwin(win);
    endwin();
    
    return 0;
}