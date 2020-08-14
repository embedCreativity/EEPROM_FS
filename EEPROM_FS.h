/*
 * EEPROM_FS.h
 *
 *  Created on: May 15, 2020
 *      Author: mark
 */

#ifndef EEPROM_FS_H_
#define EEPROM_FS_H_

#include <cstdint>
#include <vector>
#include <map>
#include <set>

// OS-dependent semaphore/mutex mechanism
#if defined(__linux__)
    #include <pthread.h>
    // Define the type we're using for a lock
    #define Lock_t pthread_mutex_t
#elif defined(TIVAWARE)  // Texas Instruments TI-RTOS
    #include <ti/sysbios/BIOS.h>
    #include <ti/sysbios/knl/Semaphore.h>
    #include <xdc/runtime/Error.h>
    // Define the type we're using for a lock
    #define Lock_t Semaphore_Handle
#else
    #error Environment must either be defined as __linux__ or TIVAWARE. Add additional support for new environments as needed.
#endif

#include "EEPROMStatus.h"


// Handle to file provided to tasks that require file access
// The EEPROMFS class stores each of these handles in a vector
// and may update the size and data properties as the file system changes
// to maintain a task's ability to access their file as they change in memory
typedef struct _handle_t {
    uint8_t* data;
    uint8_t size;
} handle_t;

// File system table has a fixed size to make it a little simpler
#define EEPROM_MAX_NUM_FILES           20

// Structure containing single file entry in file system table.
//   There will be EEPROM_MAX_NUM_FILES of this sequentially to create the full table.
typedef struct _fileEntry_t
{
    uint16_t startAddress;
    uint16_t size;
} __attribute__ ((__packed__)) fileEntry_t;

// Internal structure for managing file handles and reference counts to files
typedef struct _manager_t
{
    int handleCount;
    handle_t* handle;
} manager_t;


class EEPROMFS
{
public:

    // Constructor
    EEPROMFS();

    // Destructor
    ~EEPROMFS();

    // all write operations must be enabled immediately prior to each call
    void enableWrite();

    // Return size of EEPROM space on device
    uint32_t getTotalCapacity();

    // Return the used capacity
    uint32_t getUsedCapacity();

    // Return size of activeFiles
    uint32_t getActiveFileCount();

    // Get EEPROM status
    EEPROMStatus getStatus();

    // Tasks requiring access should call this to get a file handle
    //   Internally, this calls 'new'
    //   It is fine for a task to have a handle open over its entire lifetime,
    //   but tasks should call close() prior to exit
    handle_t* open(int index);

    // Tasks need to call this prior to exiting.
    void close(int index);

    // Tasks must call this prior to any reading from their file handle to avoid collisions
    //  Tasks should never write to their file handle's data pointer.
    //  Do not call this prior to writing - writeFile() calls it internally
    //  The pointer is not protected as a way of preventing memory
    //  duplication in constrained environments.
    void getLock(void);

    // Tasks then call this when they're done reading
    void releaseLock(void);

    // Tasks call this method to write to new or replace existing files
    // This task will internally call getLock() to ensure no conflicts happen during write operations
    //   Caller must call enableWrite() immediately prior to calling this method
    bool writeFile(uint8_t fileId, uint8_t* writeBuf, uint16_t bufLen);

    // Tasks can call this method to delete files from the system and reclaim the space associated
    //   Caller must call enableWrite() immediately prior to calling this method
    bool deleteFile(uint8_t fileId);

    // Public accessor for formatEEPROM(). This will erase entire contents of EEPROM and
    //   initialize the file system table
    bool format();

private:

    // Generic reader, given return buffer and desired length
    // Will read starting from current address position
    // Returns true if successful, false if there was an error
    uint32_t read( uint8_t* buf, uint32_t startAddress, uint32_t len );

    // Generic writer, given input buffer and length of bytes to write
    // Will start writing to EEPROM starting at the current address position
    // Returns true if successful, false if there was an error
    // NOTE: You must call enableWrite() prior to each call of this function
    bool write( uint8_t* buf, uint32_t startAddress, uint32_t len );

    // Initialize the EEPROM hardware interface on the TM4C123
    bool init();

    // Verify file system table is reasonable
    bool validateFileSystem();

    // Erase the contents of the file system table
    bool formatEEPROM();

    // Fills handle with info about file residing at index
    // Called upon initial handle creation and after any subsequent update of the file table
    // Returns boolean pass/fail success
    bool updateHandle(uint8_t index);

    // Shift data in the "disk" buffer over to the right to make room to insert new data
    // Data from headPtr to tailPtr will be shifted "to the right" by the number of
    // bytes listed in "distance".  It is important to note that you need to have room in
    // the buffer *after* the tailPtr to accommodate shifting by that number of bytes.
    bool shiftFileData(uint8_t* headPtr, uint16_t size, int32_t distance);

#if defined(__linux__)
    uint32_t FauxEEPROMMassErase();
#endif

    // Lock access to the EEPROM_FS read/write functions
    Lock_t lock;

    // pointer to array of file system table entries
    fileEntry_t *fileTable;

    // keep file system data in memory - only access EEPROM when making changes
    uint32_t* wordAlignedDisk;
    uint8_t* disk;

    // index into file for read/write
    uint16_t readWriteIndex;

    // flag indicating hw has been initialized and ready for access APIs
    bool hwInitialized;

    // flag to indicate EEPROM has been successfully initialized and ready
    bool ready;

    // write enable/disable
    bool writeEnabled;

    // size of EEPROM (in bytes)
    uint32_t eepromSize;

    // number of bytes used by files
    uint32_t bytesUsed;

    // array of indexes indicating active files in file system table
    std::set<uint8_t, std::less<uint8_t> > activeFiles;

    // corrupted file system table flag
    //   true: valid file system table
    //   false: corrupted file system table
    bool validFileSystemTable;

    // status object (contains helper print method)
    EEPROMStatus status;

    // Map to track file handles provided to tasks
    std::map<int, manager_t*> handleManager;
};

#endif /* EEPROM_FS_H_ */
