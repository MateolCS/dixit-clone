# Top-level Makefile for dixit project
# Builds: bin/dixit_client and bin/server

CXX ?= g++
# Build configuration: 'release' (default) or 'debug'
BUILD ?= release
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2

# Konfiguracja flag dla Debug/Release
ifeq ($(strip $(BUILD)),debug)
    CXXFLAGS += -g -O0 -DDEBUG
else
    CXXFLAGS += -O3 -DNDEBUG
endif

# Biblioteki
CLIENT_LIBS := -lsfml-graphics -lsfml-window -lsfml-system
SERVER_LIBS := -lpthread

# Ścieżki wyjściowe
BINROOT ?= bin
OBJROOT ?= bin_int
BINDIR ?= $(BINROOT)/$(BUILD)
OBJDIR ?= $(OBJROOT)/$(BUILD)

# Źródła
CLIENT_SRCS := $(wildcard client/src/*.cpp)
SERVER_SRCS := $(wildcard server/src/*.cpp)
COMMON_SRCS := $(wildcard common/src/*.cpp)

# Obiekty
CLIENT_OBJS := $(patsubst client/src/%.cpp,$(OBJDIR)/client/%.o,$(CLIENT_SRCS))
SERVER_OBJS := $(patsubst server/src/%.cpp,$(OBJDIR)/server/%.o,$(SERVER_SRCS))
COMMON_OBJS := $(patsubst common/src/%.cpp,$(OBJDIR)/common/%.o,$(COMMON_SRCS))

.PHONY: all client server clean distclean rebuild help

all: $(BINDIR)/client $(BINDIR)/server

client: $(BINDIR)/client

server: $(BINDIR)/server

# --- LINKOWANIE KLIENTA ---
$(BINDIR)/client: $(CLIENT_OBJS) $(COMMON_OBJS)
	@mkdir -p $(dir $@)
	@echo "[LINK] Linking Client..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(CLIENT_LIBS)
	@echo "[COPY] Copying assets..."
	
	@# Kopiowanie czcionki
	@if [ -f "CONSOLA.TTF" ]; then \
		cp CONSOLA.TTF $(BINDIR)/; \
		echo "       Copied CONSOLA.TTF to $(BINDIR)/"; \
	else \
		echo "       [WARNING] CONSOLA.TTF not found in root directory!"; \
	fi
	
	@# Kopiowanie folderu z kartami (używamy cp -r dla katalogów)
	@if [ -d "cards" ]; then \
		cp -r cards $(BINDIR)/; \
		echo "       Copied cards folder to $(BINDIR)/"; \
	else \
		echo "       [WARNING] cards folder not found in root directory!"; \
	fi

# --- LINKOWANIE SERWERA ---
$(BINDIR)/server: $(SERVER_OBJS) $(COMMON_OBJS)
	@mkdir -p $(dir $@)
	@echo "[LINK] Linking Server..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(SERVER_LIBS)

# --- KOMPILACJA OBIEKTÓW ---
$(OBJDIR)/client/%.o: client/src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -Iclient/include -Icommon/include -c $< -o $@

$(OBJDIR)/server/%.o: server/src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -Iserver/include -Icommon/include -c $< -o $@

$(OBJDIR)/common/%.o: common/src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -Icommon/include -c $< -o $@

clean:
	@echo "Cleaning build directories..."
	rm -rf $(BINROOT)
	rm -rf $(OBJROOT)

rebuild: clean all

help:
	@echo "Usage: make [target] [VARIABLE=value]"
	@echo "Targets: all, client, server, clean, rebuild"