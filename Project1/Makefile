## EECS 281 Project 1 Makefile

SHELL = /bin/bash

UNIQNAME   = jialanwu
IDENTIFIER = 40FB54C86566B9DDEAB902CC80E8CE85C1261AAD
EXECUTABLE = hunt
SOURCES    = project1.cpp treasure_hunt.cpp
HEADERS    = treasure_hunt.hpp
OBJECTS    = $(SOURCES:.cpp=.o)

CXX      = g++
CXXFLAGS = -std=c++17 -Wconversion -Wall -Werror -Wextra -pedantic
UNAME_S  = $(shell uname -s)
DEBUG_FLAGS = -g3 -DDEBUG -D_GLIBCXX_DEBUG

ifneq ($(UNAME_S),Darwin)
DEBUG_FLAGS += -fsanitize=address -fsanitize=undefined
endif

.DEFAULT_GOAL := release
.SUFFIXES:

release: CXXFLAGS += -O3 -DNDEBUG
release: $(EXECUTABLE)
.PHONY: release

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(EXECUTABLE)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJECTS): $(HEADERS)

debug: CXXFLAGS += $(DEBUG_FLAGS)
debug:
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(EXECUTABLE)_debug
.PHONY: debug

clean:
	rm -f $(OBJECTS) $(EXECUTABLE) $(EXECUTABLE)_debug fullsubmit.tar.gz partialsubmit.tar.gz
.PHONY: clean

fullsubmit: clean
	COPYFILE_DISABLE=true tar -czf fullsubmit.tar.gz Makefile $(SOURCES) $(HEADERS) $(wildcard test*.txt)
.PHONY: fullsubmit

partialsubmit: clean
	COPYFILE_DISABLE=true tar -czf partialsubmit.tar.gz Makefile $(SOURCES) $(HEADERS)
.PHONY: partialsubmit

help:
	@echo "make           Build optimized executable: hunt"
	@echo "make debug     Build hunt_debug with sanitizers"
	@echo "make clean     Remove generated files"
	@echo "make fullsubmit    Package source, Makefile, and tests"
	@echo "make partialsubmit Package source and Makefile"
.PHONY: help
