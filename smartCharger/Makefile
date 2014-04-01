TARGET = smchr
SOURCES =  CompressNUploadStateFile.cpp EVStateDependents.cpp EVStateNInputVInterface.cpp GPIOWrapper.cpp InputVoltDependents.cpp main.cpp VehicleState.cpp Thread.cpp
OBJS = $(SOURCES:.cpp=.o)

# What compiler to use
CC = g++

# Compiler flags, -g for debug, -c to make an object file
CFLAGS = -O2 -c

LDFLAGS = -L/usr/local/lib
LIBS = -lcurl -lcrypto -lsqlite3 -lpthread

all: $(TARGET)
debug: CFLAGS += -DDEBUG -DBBB-g
debug: $(TARGET)
# When LLC will never be turned on due to lack of rectifiers.
debug_noRect: CFLAGS += -DDEBUG -DBBB -DNO_RECT -g
debug_noRect: $(TARGET)
# When input current to the FPGA has to vary in order to show 
# controllable input current behavior on the battery load.
debug_simInCur: CFLAGS += -DDEBUG -DBBB -DSIM_IN_CUR -g
debug_simInCur: $(TARGET)
# Debugging on OSX platform
debug_osx: CFLAGS += -DDEBUG -DNO_RECT -g
debug_osx: $(TARGET)


# Link the target with all objects and libraries
$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(LIBS) -o $(TARGET)

# Compile the source files into object files
.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGET)
	rm -f $(OBJS)