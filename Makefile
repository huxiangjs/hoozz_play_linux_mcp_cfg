# Compiler & Flags
CXX       := g++
CC        := gcc
CXXFLAGS  := -std=c++17 -Wall -Wextra -g
CFLAGS    := -Wall -g
LDFLAGS   := -lpthread
LIBS      :=
TARGET    := linux-pwd-sync
BUILDDIR  := build

# Initialize source lists
SRCS_C    := store.c
SRCS_CPP  := main.cpp

# Include subdirectory build fragments
include esp32-common.mk

# Combine sources and generate object/dependency paths
SRCS      := $(SRCS_C) $(SRCS_CPP)
OBJS      := $(patsubst %.c, $(BUILDDIR)/%.o, $(SRCS_C)) \
             $(patsubst %.cpp, $(BUILDDIR)/%.o, $(SRCS_CPP))
DEPS      := $(OBJS:.o=.d)

# Auto-generate include paths from source directories
INC_DIRS  := $(sort $(dir $(SRCS)))
INC_FLAGS := $(addprefix -I, $(INC_DIRS))

# Default target
.PHONY: all clean

all: $(TARGET)

# Link final executable
$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

# Compile C++ sources
$(BUILDDIR)/%.o: %.cpp | $(BUILDDIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INC_FLAGS) -MMD -MP -c $< -o $@

# Compile C sources
$(BUILDDIR)/%.o: %.c | $(BUILDDIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC_FLAGS) -MMD -MP -c $< -o $@

# Ensure build directory exists
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# Clean build artifacts
clean:
	rm -rf $(BUILDDIR) $(TARGET)

# Include auto-generated header dependencies
-include $(DEPS)
