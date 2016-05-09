set -x
g++ monkey.cc -I/usr/local/include json11.cpp -o monkey -std=c++14 -Wall -O0 -g -L/usr/local/lib -lsfml-window -lsfml-system -lsfml-graphics
lldb ./monkey
