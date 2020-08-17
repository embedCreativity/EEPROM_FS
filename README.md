# EEPROM_FS
Tiny File System for EEPROM in Microcontrollers

## Implementation Notes
This implementation supports testing in a Unix/Linux environment and is hard-coded to mimic a 2KB "EEPROM" often found in microcontrollers. There are compile-time directives in it that will switch between our Linux testing environment as well as Texas Instruments TIVA support. Additional architectures could easily be added to the system with minimal extension requirements.

The "File System" is really just a manager of strings. When the system is started, the existing state of the file system is validated with some sanity checking. This includes ensuring that there are no bytes in it that are not string safe. Adding a capability to store binary data would be trivial, but for the sake of being brief, this has been left out in this implementation.

The implementation was designed to simply support a maximum of 20 "files". This can easily be modified by changing that number in the header which defines it. By using an index as a file identifier rather than a file name, we don't have to store a string file name.

The idea is to allow tasks running on a microcontroller to have access to whichever files are of interest to that task and not have to worry about what other tasks are doing. The EEPROM_FS will manage the storing of all file data regardless of changes that may result in file data moving around in the storage medium.

This will allow tasks to manage their own non-volatile configuration files as well as any other file that may be used during execution. An example of this could be the storage and modification of configurable "scripts" when a static (i.e., compiled) interpreter is included in the microcontroller design.

This design is meant to be a service, so I suggest ensuring you only have one copy of the EEPROM_FS object in your system. The recommended method is to override the constructor and implement a Singleton design pattern for the service. When you're using it, make sure you make proper use of the getLock() and releaseLock() methods to ensure you don't have collisions from multiple threads/tasks accessing data concurrently.

Have a look at the testApp program to see variations of how the API can be exercised.

## API

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

    // Get map containing list of active fileIds along with their file size
    const std::map<uint8_t, uint16_t> getActiveFiles();
    
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
