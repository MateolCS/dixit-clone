# Top-level Makefile for dixit project
# Builds: bin/dixit_client and bin/server

CXX ?= g++
# Build configuration: 'release' (default) or 'debug'
BUILD ?= release
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
ifeq ($(strip $(BUILD)),debug)
CXXFLAGS += -g -O0 -DDEBUG
else
CXXFLAGS += -O3 -DNDEBUG
endif
# Binaries are placed in `bin/$(BUILD)` by default (e.g. bin/release)
BINROOT ?= bin
OBJROOT ?= bin_int
BINDIR ?= $(BINROOT)/$(BUILD)
# Intermediate object directory; set to empty (`OBJDIR=`) to place objects
# alongside sources.
OBJDIR ?= $(OBJROOT)/$(BUILD)

CLIENT_SRCS := $(wildcard client/src/*.cpp)
SERVER_SRCS := $(wildcard server/src/*.cpp)
COMMON_SRCS := $(wildcard common/src/*.cpp)

CLIENT_OBJS := $(patsubst client/src/%.cpp,$(OBJDIR)/client/%.o,$(CLIENT_SRCS))
SERVER_OBJS := $(patsubst server/src/%.cpp,$(OBJDIR)/server/%.o,$(SERVER_SRCS))
COMMON_OBJS := $(patsubst common/src/%.cpp,$(OBJDIR)/common/%.o,$(COMMON_SRCS))

.PHONY: all client server clean distclean rebuild help

all: $(BINDIR)/client $(BINDIR)/server

client: $(BINDIR)/client

server: $(BINDIR)/server

$(BINDIR)/client: $(CLIENT_OBJS) $(COMMON_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BINDIR)/server: $(SERVER_OBJS) $(COMMON_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJDIR)/dixit_client/%.o: dixit_client/src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -Iclient/include -Icommon/include -c $< -o $@

$(OBJDIR)/server/%.o: server/src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -Iserver/include -Icommon/include -c $< -o $@

$(OBJDIR)/common/%.o: common/src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -Icommon/include -c $< -o $@

clean:
	rm -rf $(BINROOT)
	rm -rf $(OBJROOT)

rebuild: clean all

help:
	@echo "Usage: make [target] [VARIABLE=value]"
	@echo "Targets: all, client, server, clean, distclean, rebuild, help"
	@echo "Variables:" 
	@echo "  BUILD     (release|debug) default: release"
	@echo "  CXX       (default g++)"
	@echo "  CXXFLAGS  (base flags; debug/release add optimization/debug flags)"
	@echo "  BINDIR    (default bin/\$(BUILD))"
	@echo "  OBJDIR    (default bin_int/\$(BUILD); set empty to avoid intermediate dir)"
	@echo "Examples:"
	@echo "  make                 # build release (default)"
	@echo "  make BUILD=debug     # build debug binaries"
	@echo "  make client BUILD=debug OBJDIR=  # debug client, objects next to sources"