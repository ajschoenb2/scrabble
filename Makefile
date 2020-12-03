CCFLAGS = -std=c++17 -Wall -Werror -g $(CC_OPT)

TARGETS = scrabble

BUILD_DIR ?= build

.PHONY : all clean $(TARGETS)

all : $(TARGETS)

$(TARGETS) : % : Makefile
	mkdir -p $(BUILD_DIR)
	g++ $(CCFLAGS) -o $(BUILD_DIR)/$* $*.cc

clean :
	rm -rf $(BUILD_DIR)
