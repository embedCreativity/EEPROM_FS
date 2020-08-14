CXX = g++
CXXFLAGS +=  -g -Wall -I. -std=c++11
LDFLAGS += -lpthread
LIBS +=

testApp: testApp.o EEPROM_FS.o EEPROMStatus.o
	$(CXX) $(LDFLAGS) testApp.o EEPROM_FS.o EEPROMStatus.o -o testApp $(LIBS)

testApp.o: testApp.cpp
	$(CXX) $(CXXFLAGS) -c testApp.cpp

EEPROM_FS.o: EEPROM_FS.cpp
	$(CXX) $(CXXFLAGS) -c EEPROM_FS.cpp

EEPROMStatus.o: EEPROMStatus.cpp
	$(CXX) $(CXXFLAGS) -c EEPROMStatus.cpp

all: testApp

# remove object files and executable when user executes "make clean"
clean:
	rm -f *.o testApp


