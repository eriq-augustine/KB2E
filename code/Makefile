CC=g++
CFLAGS=-O3 -I.

ARCHIVE=ar
ARCHIVE_FLAGS=-rcs

BIN_DIR=bin

CPP_FILES = $(shell find . -name *.cpp)
HEADER_FILES = $(shell find . -name *.h)
OBJ_FILES = $(CPP_FILES:%.cpp=%.o)

# All files without a main.
# We will bundle these together in a single archive.
NO_MAIN_CPP_FILES = $(shell find -name *.cpp | xargs grep -L 'int main' | sed 's/^\.\///')
NO_MAIN_OBJ_FILES = $(NO_MAIN_CPP_FILES:%.cpp=%.o)

all: objects common mains

# All bject files
objects: $(OBJ_FILES) $(HEADER_FILES)

# The common code that we will put in an archive.
common: objects binDir
	$(ARCHIVE) $(ARCHIVE_FLAGS) $(BIN_DIR)/common.a $(NO_MAIN_OBJ_FILES)

# All the files that have mains.
mains: binDir trainTransE trainTransH trainTransR evalTransE evalTransH evalTransR

binDir:
	mkdir -p $(BIN_DIR)

trainTransE: transe/bin/trainTransE.o common
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $< $(BIN_DIR)/common.a

trainTransH: transh/bin/trainTransH.o common
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $< $(BIN_DIR)/common.a

trainTransR: transr/bin/trainTransR.o common
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $< $(BIN_DIR)/common.a

evalTransE: transe/bin/evalTransE.o common
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $< $(BIN_DIR)/common.a

evalTransH: transh/bin/evalTransH.o common
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $< $(BIN_DIR)/common.a

evalTransR: transr/bin/evalTransR.o common
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $< $(BIN_DIR)/common.a

# Just include the headers so we will rebuild if they change.
#  We could really scope it down more, but the project is small.
%.o: %.cpp $(HEADER_FILES)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -Rf $(BIN_DIR)
	rm -f $(OBJ_FILES)
