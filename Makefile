.RECIPEPREFIX = >
CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
tbf: src/main.cpp src/token_bucket.hpp src/policer.hpp src/shaper.hpp src/traffic.hpp
> $(CXX) $(CXXFLAGS) -o tbf src/main.cpp
clean:
> rm -f tbf out.csv
