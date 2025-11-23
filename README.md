This is a simple simulation of an order book, I built this project to better understand C++ data structures and how they are used in high frequency trading.

Originally, I was using two deques which were filled with order objects.  This made a lot of sense to me at first, just two queues that I can add to the start and end of and keep sorted.  However, I soon realized that resorting each deque
after each and every limit order was entered had O(nlog(n)) time complexity, which isn't very fast.

I then resigned the engine to use two maps (buys and asks) which map prices to deques which are filled with Order objects, now limit order insertions had O(log(n)) time complexity.

This change alone knocked an entire decimal place off of the nanoseconds.

I decided that I would benchmark the speed of this engine by having a function add 1000 random orders to the book, with a 75% chance to add a limit order, and 25% chance to add a market order.
I used **std::chrono** and recorded the times in nanoseconds.  The benchmarks can be found in **stamps.txt**

I also experimented with -O2 and -O3 optimization flags, with -O2 yeilding better results.
