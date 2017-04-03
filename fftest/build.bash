#!/bin/bash
set -x
g++ -Wall -O3 -std=c++11 main.cpp -lpulse-simple -lpulse -lrt 2>&1
