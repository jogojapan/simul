# simul

Just a few tests with cairomm and graphics

## Compilation

```c++
g++ -O3 -W -Wall -Wno-parentheses -std=c++1y -o simul balls.cpp main.cpp `pkg-config gtkmm-3.0 --cflags --libs`
