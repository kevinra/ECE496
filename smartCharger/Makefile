TARGET = smchr
SOURCES =  main.cpp CompressNUploadStateFile.cpp EVStateDependents.cpp EVStateNInputVInterface.cpp GPIOWrapper.cpp VehicleState.cpp Thread.cpp
OBJS = $(SOURCES:.cpp=.o)

# What compiler to use
CC = g++

# Compiler flags, -g for debug, -c to make an object file
CFLAGS = -O2 -c

LDFLAGS = -L/usr/local/lib
LIBS = -lcurl -lcrypto -lsqlite3 -lpthread

all: $(TARGET)
debug: CFLAGS += -DDEBUG -g
debug: $(TARGET)
debug_nofpga: CFLAGS += -DDEBUG -DNOFPGA -g
debug_nofpga: $(TARGET)

# Link the target with all objects and libraries
$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(LIBS) -o $(TARGET)

# Compile the source files into object files
.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGET)
	rm -f $(OBJS)