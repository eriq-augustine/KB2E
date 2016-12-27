CC=g++
CFLAGS=-O2 -I.

ARCHIVE=ar
ARCHIVE_FLAGS=-rcs

BIN_DIR=bin

CPP_FILES = $(shell find . -name *.cpp)
OBJ_FILES = $(CPP_FILES:%.cpp=%.o)

COMMON_CPP_FILES = $(shell find common -name *.cpp)
COMMON_OBJ_FILES = $(COMMON_CPP_FILES:%.cpp=%.o)

all: objects common mains

# TEST
# MAIN_FILES = $(shell find -name *.cpp | xargs grep -l 'int main' | sed 's/^\.\///')
# $(info MAIN_FILES is $(MAIN_FILES))
# $(info CFLAGS is $(CFLAGS))

# All bject files
objects: $(OBJ_FILES)

# The common code that we will put in an archive.
common: objects binDir
	$(ARCHIVE) $(ARCHIVE_FLAGS) $(BIN_DIR)/common.a $(COMMON_OBJ_FILES)

# All the files that have mains.
mains: binDir trainTransE trainTransH testTransE

binDir:
	mkdir -p $(BIN_DIR)

trainTransE: TransE/Train_TransE.o common
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $< $(BIN_DIR)/common.a

trainTransH: TransH/Train_TransH.o common
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $< $(BIN_DIR)/common.a

testTransE: TransE/Test_TransE.o common
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $< $(BIN_DIR)/common.a

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -Rf $(BIN_DIR)
	rm -f $(OBJ_FILES)
