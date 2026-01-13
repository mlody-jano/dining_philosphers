## Introduction
The following repository provides a simple solution to the problem known as **The Dining Philosophers Dilemma**, which is often used as an example of multithreading.
# Stack used
Libraries and dependencies:
- Program logic:
  - _thread_
  - _semaphore_
  - _mutex_
- Visualisation:
  - _ncurses.h_
# How to run
1. **Clone repository**
   
   ```bash
   git clone https://github.com/mlody-jano/dining_philosphers.git
   cd dining_philosophers
   ```
   
3. **Compile program**
   
   ```bash
   g++ -std=c++20 main.cpp -lncurses -pthread
   ```
   
5. **Run the script**
   
   ```bash
   ./a.out N # N -> desired number of philosophers (threads). Must be > 5!
   ```
   
# Contact information
- **Email** : [jasiu.hulboj@gmail.com](mailto:jasiu.hulboj@gmail.com)
