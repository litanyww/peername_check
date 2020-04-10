# Copyright 2020 Sophos Limited. All rights reserved.

CFLAGS = -std=c++17 -Wall

.PHONEY : all

all : Safari notSafari


Safari : main.o
	$(CXX) -o $@ $^

notSafari : main.o
	$(CXX) -o $@ $^

main.o : main.cpp
	$(CXX) $(CFLAGS) -o $@ -c $<
