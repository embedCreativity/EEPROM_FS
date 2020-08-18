/*
  Copyright (C) 2020 Embed Creativity LLC

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <EEPROM_FS.h>

// OS-dependent adapter declarations
#if defined(__linux__)
    #include <iostream>
    #include <fstream>
    #include <assert.h>
    // lock initialization
    #define initLock()    {                             \
        assert (pthread_mutex_init(&lock, NULL) == 0);  \
    }
    // non-volatile file location
    #define UNIX_NONVOLATILE_FILE    "nonvolatile.bin"
    #define UNIX_FILE_SIZE            2048
    // Fake out the TI-RTOS EEPROM library API
    #define EEPROMSizeGet()            UNIX_FILE_SIZE
    #define EEPROM_INIT_OK            0
    #define EEPROMInit()            EEPROM_INIT_OK

    #define EEPROMMassErase()        FauxEEPROMMassErase()

#elif defined(TIVAWARE)  // Texas Instruments TI-RTOS
    #include <xdc/runtime/System.h>
    #include <driverlib/eeprom.h>
    // lock initialization
    #define initLock()    {                                             \
        Error_Block ebLock;                                             \
        Error_init(&ebLock);                                            \
        lock = Semaphore_create(1, NULL, &ebLock);                      \
        if ( lock == NULL ) {                                           \
            System_abort("ERROR: EEPROMFS: Semaphore creation failed"); \
        }                                                               \
    }

#else // Assert failure
    #error Environment must either be defined as __linux__ or TIVAWARE. Add additional support for new environments as needed.
#endif

#include <algorithm>    // std::find
#include <cstdio>
#include <cstring>

/************************************/
/*   Debug Print Support            */
/************************************/
//#define DEBUG_PRINT // Comment out to disable debug print statements

#ifdef DEBUG_PRINT
#define debugPrint(format, ...) {\
                            System_printf(format, ## __VA_ARGS__);\
                            System_flush();\
                        }
#else
#define debugPrint(format, ...) {\
                            do {} while (0);\
                        }
#endif


// Address in EEPROM of the file system table
#define EEPROM_FTABLE_ADDR      0
// Address in EEPROM of the start of the first file
#define EEPROM_FIRST_FILE_ADDR  (EEPROM_MAX_NUM_FILES * sizeof(fileEntry_t))


EEPROMFS::EEPROMFS () :
    disk(NULL),
    readWriteIndex(0),
    hwInitialized(false),
    ready(false),
    writeEnabled(false),
    eepromSize(0),
    bytesUsed(0),
    validFileSystemTable(false)
{
    initLock();
    getLock();
    ready = init();
    releaseLock();
}

EEPROMFS::~EEPROMFS()
{
    getLock();
    if ( wordAlignedDisk != NULL )
    {
        delete[] wordAlignedDisk;
        wordAlignedDisk = NULL;
        disk = NULL;
    }
    // Clean up all handles and managers from map: handleManager
    for ( std::map<int, manager_t*>::iterator it = handleManager.begin(); it != handleManager.end(); it++ )
    {
        // clean up the handle that the manager is holding
        delete it->second->handle;
        // clean up the manager itself
        delete it->second;
    }
    releaseLock();
}

void EEPROMFS::enableWrite()
{
    getLock();
    writeEnabled = true;
    releaseLock();
}

uint32_t EEPROMFS::getTotalCapacity()
{
    uint32_t size;

    getLock();
    writeEnabled = false;
    size = eepromSize;
    releaseLock();

    return size;
}

uint32_t EEPROMFS::getUsedCapacity()
{
    uint32_t usage;
    getLock();
    if ( !validFileSystemTable )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_INVALID_FILE_SYSTEM_TABLE);
        bytesUsed = 0;
    }

    usage = bytesUsed;
    releaseLock();
    return usage;
}

uint32_t EEPROMFS::getActiveFileCount()
{
    if ( !validFileSystemTable )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_INVALID_FILE_SYSTEM_TABLE);
        activeFiles.clear();
    }
    return activeFiles.size();
}

EEPROMStatus EEPROMFS::getStatus() // TODO: Am I making a copy of the entire class here? Pass by reference?
{
    return status;
}

const std::map<uint8_t, uint16_t> EEPROMFS::getActiveFiles()
{
    std::map<uint8_t, uint16_t> retSet;

    for (std::set<uint8_t, std::less<uint8_t> >::iterator it = activeFiles.begin();
        it != activeFiles.end(); ++it)
    {
        uint8_t id = *it;
        uint16_t size = fileTable[*it].size;
        retSet.insert(std::pair<uint8_t, uint16_t>(id, size));
    }

    return retSet;
}

handle_t* EEPROMFS::open(int index)
{
    handle_t* tempHandle;
    manager_t* tempManager;
    std::map<int, manager_t*>::iterator it;

    getLock();

    if ( !validFileSystemTable )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_INVALID_FILE_SYSTEM_TABLE);
        releaseLock();
        return NULL;
    }

    // Bounds check user input
    if (0 > index || index >= EEPROM_MAX_NUM_FILES)
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_BAD_PARAMS);
        releaseLock();
        return NULL;
    }

    // Verify that the file has exists
    std::set<uint8_t, std::less<uint8_t> >::iterator fit;
    fit = std::find(activeFiles.begin(), activeFiles.end(), index);
    if (fit == activeFiles.end())
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_FILE_NOT_FOUND);
        releaseLock();
        return NULL;
    }

    // Look for object managing file at given index
    it = handleManager.find(index);

    // If we don't have it in our map, we have to create a manager for it
    if ( it == handleManager.end() )
    {
        tempHandle = new (std::nothrow) handle_t;
        if ( NULL == tempHandle )
        {
            status.setStatus(EEPROMStatus::EEPROM_ERROR_INSUFFICIENT_MEMORY);
            releaseLock();
            return NULL;
        }

        // Create a manager for this file handle
        tempManager = new (std::nothrow) manager_t;
        if ( NULL == tempManager )
        {
            status.setStatus(EEPROMStatus::EEPROM_ERROR_INSUFFICIENT_MEMORY);
            releaseLock();
            return NULL;
        }

        tempManager->handle = tempHandle;
        tempManager->handleCount = 1; // first customer!

        // insert manager into the map for this index
        handleManager[index] = tempManager;

        // Populate handle with file info
        if ( ! updateHandle(index) )
        {
            status.setStatus(EEPROMStatus::EEPROM_ERROR_INTERNAL);
            releaseLock();
            return NULL;
        }
        releaseLock();

        // return handle to calling task
        return tempHandle;
    }
    // If we already have a manager for the file, increment reference count and return existing handle found
    else
    {
        // increment the reference count to the handle
        it->second->handleCount += 1;
        // return the handle to the calling task
        releaseLock();
        return it->second->handle;
    }
}

void EEPROMFS::close(int index)
{
    manager_t* tempManager;
    std::map<int, manager_t*>::iterator it;

    // Look for object managing file at given index
    it = handleManager.find(index);

    // If we have it in our map, we need to decrement access count and possibly garbage collect
    if ( it != handleManager.end() )
    {
        tempManager = it->second;

        // There needs to be at least one reference to this handle
        if ( 0 < tempManager->handleCount )
        {
            // decrement the reference count to the handle
            tempManager->handleCount -= 1;
        }
        // No more references to the handle, it can be cleaned up
        if ( 0 == tempManager->handleCount )
        {
            delete tempManager->handle; // garbage collect handle
            tempManager->handle = NULL; // reset to NULL
            handleManager.erase(it); // remove file manager from map by iterator
            delete tempManager; // garbage collect manager
            tempManager = NULL;
        }
    }
}

void  EEPROMFS::getLock(void)
{
#if defined(__linux__)
    pthread_mutex_lock(&lock);
#elif defined(TIVAWARE)  // Texas Instruments TI-RTOS
    Semaphore_pend(lock, BIOS_WAIT_FOREVER);
#else // Assert failure
    #error Environment must either be defined as __linux__ or TIVAWARE. Add additional support for new environments as needed.
#endif
}

void  EEPROMFS::releaseLock(void)
{
#if defined(__linux__)
    pthread_mutex_unlock(&lock);
#elif defined(TIVAWARE)  // Texas Instruments TI-RTOS
    Semaphore_post(lock);
#else // Assert failure
    #error Environment must either be defined as __linux__ or TIVAWARE. Add additional support for new environments as needed.
#endif
}

bool EEPROMFS::writeFile(uint8_t fileId, uint8_t* writeBuf, uint16_t bufLen)
{
    std::set<uint8_t, std::less<uint8_t> >::iterator it;

    getLock();

    if ( !validFileSystemTable )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_INVALID_FILE_SYSTEM_TABLE);
        releaseLock();
        return false;
    }
    if ( !ready )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_NOT_INITIALIZED);
        releaseLock();
        return false;
    }
    if ( !writeEnabled )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_WRITE_PROTECTED);
        releaseLock();
        return false;
    }
    if ( EEPROM_MAX_NUM_FILES <= fileId )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_BAD_PARAMS);
        releaseLock();
        return false;
    }

    // disable to protect against follow up write call
    writeEnabled = false;

    // check to see if file is in the activeFiles set
    it = std::find(activeFiles.begin(), activeFiles.end(), fileId);

    // File does not exist in set yet
    if ( it == activeFiles.end() )
    {
        // if file is not, then check if eepromSize and bytesUsed can accommodate this new file
        if ( bufLen + bytesUsed > eepromSize )
        {
            status.setStatus(EEPROMStatus::EEPROM_ERROR_INSUFFICIENT_STORAGE);
            releaseLock();
            return false;
        }
        // Iterate through the files that ARE there, and find where we need to start moving files out to make room
        // We do this by iterating through and finding the start of what comes AFTER our target file
        std::set<uint8_t, std::less<uint8_t> >::iterator itrCopy = activeFiles.begin();
        for ( it = activeFiles.begin(); it != activeFiles.end() && *it < fileId; it++ )
        {
            itrCopy = it;
        }
        // See if our new file is going to be the FIRST file
        if ( it == activeFiles.begin() )
        {
            // Check if our new file is the ONLY file in the system.
            //   If the new file is NOT the only file in the system,
            //   we're going to have to move data to make room
            if ( 0 != getActiveFileCount() )
            {
                // Starting from the last file and moving towards the front, move each file to the "right"
                for ( std::set<uint8_t, std::less<uint8_t> >::reverse_iterator rit = activeFiles.rbegin();
                      rit != activeFiles.rend(); ++rit )
                {
                    uint8_t* headPtr = disk + fileTable[*rit].startAddress;
                    if ( ! shiftFileData(headPtr, fileTable[*rit].size, bufLen) )
                    {
                        releaseLock();
                        return false;
                    }
                    fileTable[*rit].startAddress += bufLen; // advance starting position
                    updateHandle(*rit); // update any handles that have this affected file
                }
            }
            // Write out file data to file table and disk
            fileTable[fileId].size = bufLen;
            fileTable[fileId].startAddress = EEPROM_FIRST_FILE_ADDR;
            std::memcpy(&disk[EEPROM_FIRST_FILE_ADDR], writeBuf, bufLen);
            activeFiles.insert(fileId);
            write(disk, 0, eepromSize); // write out entire disk image
            updateHandle(fileId);
            bytesUsed += bufLen; // add to the total bytesUsed tracker
        }
        //   File will NOT be the first or last file - move the trailing files to make room
        else if ( it != activeFiles.end() )
        {
            // itrCopy points at what precedes our new file
            // it points to what comes after

            // Starting from the last file and moving towards the front and stopping at file would come
            //   before our new file, move each file to the "right"
            std::set<uint8_t, std::less<uint8_t> >::reverse_iterator ritHead = std::find(activeFiles.rbegin(),
                                                                                        activeFiles.rend(), *itrCopy);

            for ( std::set<uint8_t, std::less<uint8_t> >::reverse_iterator rit = activeFiles.rbegin();
                  rit != ritHead; ++rit )
            {
                uint8_t* headPtr = disk + fileTable[*rit].startAddress;
                if ( ! shiftFileData(headPtr, fileTable[*rit].size, bufLen) )
                {
                    releaseLock();
                    return false;
                }
                fileTable[*rit].startAddress += bufLen; // advance starting position
                updateHandle(*rit); // update any handles that have this affected file
            }

            // Write out file data to file table and disk
            fileTable[fileId].size = bufLen;
            // starting address comes directly after the file that precedes it
            fileTable[fileId].startAddress = fileTable[*itrCopy].startAddress + fileTable[*itrCopy].size;
            std::memcpy(&disk[fileTable[fileId].startAddress], writeBuf, bufLen);
            activeFiles.insert(fileId);
            write(disk, 0, eepromSize); // write out entire disk image
            updateHandle(fileId);
            bytesUsed += bufLen; // add to the total bytesUsed tracker
        }
        // New file will be at the end. Copy writeBuf data directly after the last file present
        else
        {
            // assign iterator to the end of the set
            std::set<uint8_t, std::less<uint8_t> >::reverse_iterator rit = activeFiles.rbegin();
            // Write out file data to file table and disk
            fileTable[fileId].size = bufLen;
            // starting address comes directly after the file that precedes it
            fileTable[fileId].startAddress = fileTable[*rit].startAddress + fileTable[*rit].size;
            std::memcpy(&disk[fileTable[fileId].startAddress], writeBuf, bufLen);
            activeFiles.insert(fileId);
            write(disk, 0, eepromSize); // write out entire disk image
            updateHandle(fileId);
            bytesUsed += bufLen; // add to the total bytesUsed tracker
        }
    }
    // else, file already exists
    else
    {
        int32_t distance;

        // Ignore current size of file, as we're replacing it
        if ( bytesUsed - fileTable[fileId].size + bufLen > eepromSize )
        {
            status.setStatus(EEPROMStatus::EEPROM_ERROR_INSUFFICIENT_STORAGE);
            releaseLock();
            return false;
        }

        // Nuke the original to prevent trailing characters
        std::memset(&disk[fileTable[fileId].startAddress], 0xFF, fileTable[fileId].size);

        // find the change in file size
        distance = bufLen - fileTable[fileId].size;

        // If our file was the "last" file or the size is not changing, simply stick it in place
        if ( (distance == 0) || (it == activeFiles.end()) )
        {
            // Write new file data
            std::memcpy(&disk[fileTable[fileId].startAddress], writeBuf, bufLen);

            // Update file table info
            fileTable[fileId].size = bufLen;
            updateHandle(fileId);
            bytesUsed += distance; // adjust based on change

            write(disk, 0, eepromSize); // write out entire disk image
            updateHandle(fileId);
        }
        // file was not the "last" in the system and/or the size has changed with this update
        else
        {
            if ( 0 > distance ) // new file is smaller
            {
                // advance "it" iterator to what comes 'after' our file (all the ones we'd need to move)
                it++;

                // Starting from the file after our target file and moving towards the end, adjust the position of each file
                for ( std::set<uint8_t, std::less<uint8_t> >::iterator itMover = it;
                    itMover != activeFiles.end(); ++itMover )
                {
                    uint8_t* headPtr = disk + fileTable[*itMover].startAddress;
                    if ( ! shiftFileData(headPtr, fileTable[*itMover].size, distance) )
                    {
                        status.setStatus(EEPROMStatus::EEPROM_ERROR_INTERNAL);
                        releaseLock();
                        return false;
                    }
                    fileTable[*itMover].startAddress += distance; // adjust starting position
                    updateHandle(*itMover); // update any handles that have this affected file
                }
            }
            else // new file is larger
            {
                // Starting from the last file and moving towards the front, move each file to the "right"
                for ( std::set<uint8_t, std::less<uint8_t> >::reverse_iterator rit = activeFiles.rbegin();
                      *rit != *it; ++rit )
                {
                    uint8_t* headPtr = disk + fileTable[*rit].startAddress;
                    if ( ! shiftFileData(headPtr, fileTable[*rit].size, distance) )
                    {
                        status.setStatus(EEPROMStatus::EEPROM_ERROR_INTERNAL);
                        releaseLock();
                        return false;
                    }
                    fileTable[*rit].startAddress += distance; // adjust starting position
                    updateHandle(*rit); // update any handles that have this affected file
                }
            }

            // Write out updated file data to file table and disk (starting address does not change)
            fileTable[fileId].size = bufLen;
            std::memcpy(&disk[fileTable[fileId].startAddress], writeBuf, bufLen);
            write(disk, 0, eepromSize); // write out entire disk image
            updateHandle(fileId);
            bytesUsed += distance; // adjust based on change
        }
    }

    releaseLock();
    return true;
}

bool EEPROMFS::deleteFile(uint8_t fileId)
{
   std::set<uint8_t, std::less<uint8_t> >::iterator it;
   int32_t distance;

    getLock();

    if ( !validFileSystemTable )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_INVALID_FILE_SYSTEM_TABLE);
        releaseLock();
        return false;
    }
    if ( !ready )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_NOT_INITIALIZED);
        releaseLock();
        return false;
    }
    if ( !writeEnabled )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_WRITE_PROTECTED);
        releaseLock();
        return false;
    }
    if ( EEPROM_MAX_NUM_FILES <= fileId )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_BAD_PARAMS);
        releaseLock();
        return false;
    }

    // disable to protect against follow up write call
    writeEnabled = false;

    // check to see if file is in the activeFiles set
    it = std::find(activeFiles.begin(), activeFiles.end(), fileId);

    // File does not exist in set
    if ( it == activeFiles.end() )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_FILE_NOT_FOUND);
        releaseLock();
        return false;
    }

    // create distance to move files
    distance = (-1 * fileTable[fileId].size);

    // Verify the file was not disabled (zero length size)
    if ( fileTable[fileId].size == 0 )
    {
        fileTable[fileId].startAddress = 0;
        activeFiles.erase(fileId);
        updateHandle(fileId);
        releaseLock();
        return true;
    }

    // Nuke the file
    std::memset(&disk[fileTable[fileId].startAddress], 0xFF, fileTable[fileId].size);
    // Reclaim size
    bytesUsed -= fileTable[fileId].size;

    // invalidate file table entry
    fileTable[fileId].startAddress = 0;
    fileTable[fileId].size = 0;
    updateHandle(fileId);

    // iterator "it" points to index in activeFiles array
    // advance "it" iterator to what comes 'after' our file (all the ones we'd need to move)
    // Starting from the file after our target file and moving towards the end, adjust the position of each file
    for ( it++ ; it != activeFiles.end(); ++it )
    {
        uint8_t* headPtr = disk + fileTable[*it].startAddress;
        if ( ! shiftFileData(headPtr, fileTable[*it].size, distance) )
        {
            status.setStatus(EEPROMStatus::EEPROM_ERROR_INTERNAL);
            releaseLock();
            return false;
        }
        fileTable[*it].startAddress += distance; // adjust starting position
        updateHandle(*it); // update any handles that have this affected file
    }

    activeFiles.erase(fileId);
    write(disk, 0, eepromSize); // write out entire disk image
    releaseLock();
    return true;
}

bool EEPROMFS::format()
{
    bool success = false;

    getLock();

    if ( !hwInitialized )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_NOT_INITIALIZED);
    }
    else if ( !writeEnabled )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_WRITE_PROTECTED);
    }
    else
    {
        writeEnabled = false;
        if ( formatEEPROM() )
        {
            // re-verify the filesystem table
            validFileSystemTable = validateFileSystem();
            success = validFileSystemTable;
        }
    }

    releaseLock();

    return success;
}

/*****************************************************************************************************/
/* Private                                                                                           */
/*****************************************************************************************************/

uint32_t EEPROMFS::read( uint8_t* buf, uint32_t startAddress, uint32_t len )
{
    uint32_t readLen;

    if ( !hwInitialized )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_NOT_INITIALIZED);
        return 0;
    }

    // Check for correct word alignment prior to making API calls
    // Must be a multiple of 4
    if ( 0 != (startAddress & 0x03) || 0 != (len & 0x03) )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_WORD_ALIGNMENT);
        return 0;
    }

    if ( startAddress + len > eepromSize )
    {
        readLen = eepromSize - startAddress;
    }
    else
    {
        readLen = len;
    }

#if defined(__linux__)
    std::fstream fs;
    int32_t size;

    // Open the file without std::ios::in first to ensure it's created if it doesn't exist
    fs.open(UNIX_NONVOLATILE_FILE, std::ios::out | std::ios::binary | std::ios::app);
    if ( !fs.is_open() )
    {
        fs.close();
        status.setStatus(EEPROMStatus::EEPROM_ERROR_INTERNAL);
    }
    fs.close();
    // Reopen it with std::ios::in and with std::ios::ate to open it with the cursor at the end
    fs.open(UNIX_NONVOLATILE_FILE, std::ios::in | std::ios::binary | std::ios::ate);

    if (fs.is_open())
    {
        size = fs.tellg();
        // check for epic failure
        if ( -1 == size )
        {
            fs.close();
            status.setStatus(EEPROMStatus::EEPROM_ERROR_INTERNAL);
            return 0;
        }

        // Ensure the size is as expected
        if ( static_cast<uint32_t>(size) != eepromSize )
        {
            fs.close();
            // nuke entire EEPROM - set to FF's
            if ( 0 != EEPROMMassErase() )
            {
                status.setStatus(EEPROMStatus::EEPROM_ERROR_INTERNAL);
                return 0;
            }
            status.setStatus(EEPROMStatus::EEPROM_ERROR_INVALID_FILE_SYSTEM_TABLE);
            return 0;
        }

        // Move cursor to where the caller wanted it
        fs.seekg(startAddress, std::ios::beg);
        // Read out to the memory location passed in
        fs.read ((char*)buf, readLen);
        fs.close();
    }
    else
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_INTERNAL);
        return 0;
    }

#elif defined(TIVAWARE)  // Texas Instruments TI-RTOS
    EEPROMRead((uint32_t*)buf, startAddress, readLen);
#else // Assert failure
    #error Environment must either be defined as __linux__ or TIVAWARE. Add additional support for new environments as needed.
#endif

    status.setStatus(EEPROMStatus::EEPROM_OK);
    return readLen;
}

bool EEPROMFS::write( uint8_t* buf, uint32_t startAddress, uint32_t len )
{

    if ( !hwInitialized )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_NOT_INITIALIZED);
        return false;
    }

    // Check for correct word alignment prior to making API calls
    // Must be a multiple of 4
    if ( 0 != (startAddress & 0x03) || 0 != (len & 0x03) )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_WORD_ALIGNMENT);
        return 0;
    }

    if ( startAddress + len > eepromSize )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_INSUFFICIENT_STORAGE);
        return false;
    }

#if defined(__linux__)
    int32_t size;
    std::fstream fs;

    fs.open(UNIX_NONVOLATILE_FILE, std::ios::in | std::ios::binary | std::ios::ate);
    if ( !fs.is_open() )
    {
        fs.close();
        status.setStatus(EEPROMStatus::EEPROM_ERROR_INTERNAL);
        return false;
    }

    size = fs.tellg();
    // check for epic failure
    if ( -1 == size )
    {
        fs.close();
        status.setStatus(EEPROMStatus::EEPROM_ERROR_INTERNAL);
        return false;
    }
    // Ensure the size is as expected
    if ( static_cast<uint32_t>(size) != eepromSize )
    {
        fs.close();
        // nuke entire EEPROM - set to FF's
        if ( 0 != EEPROMMassErase() )
        {
            status.setStatus(EEPROMStatus::EEPROM_ERROR_INTERNAL);
            return false;
        }
        status.setStatus(EEPROMStatus::EEPROM_ERROR_INVALID_FILE_SYSTEM_TABLE);
        return false;
    }

    // Reset cursor to beginning of file
    fs.seekg(0, std::ios::beg);
    // allocate memory:
    char* buffer = new char [size];
    // read data as a block:
    fs.read(buffer, size);
    fs.close();

    // Modify memory starting at desired startAddress
    std::memcpy(&buffer[startAddress], buf, len);

    // Write contents back out to file
    fs.open(UNIX_NONVOLATILE_FILE, std::ios::out | std::ios::binary);
    if ( !fs.is_open() )
    {
        fs.close();
        delete[] buffer;
        return false;
    }
    fs.write(buffer, size);
    fs.close();
    delete[] buffer;
#elif defined(TIVAWARE)  // Texas Instruments TI-RTOS
    // API returns 0 on success, otherwise API-specific error code we don't want to get into
    if ( 0 != EEPROMProgram((uint32_t*)buf, startAddress, len) )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_WRITE_ERROR);
        return false;
    }
#else // Assert failure
    #error Environment must either be defined as __linux__ or TIVAWARE. Add additional support for new environments as needed.
#endif

    status.setStatus(EEPROMStatus::EEPROM_OK);
    return true;
}

bool EEPROMFS::init()
{
    bool success;

    if ( EEPROM_INIT_OK == EEPROMInit() )
    {
        hwInitialized = true;
        eepromSize = EEPROMSizeGet();
        if ( eepromSize <= EEPROM_FIRST_FILE_ADDR )
        {
            status.setStatus(EEPROMStatus::EEPROM_ERROR_INSUFFICIENT_STORAGE);
            success = false;
        }
        else
        {
            // Allocate a word-aligned block of memory to use as a disk image
            // Since we're using uint32_t to accomplish this, we only need 1/4 as many units of memory
            wordAlignedDisk = new (std::nothrow) uint32_t[(eepromSize>>2)];
            // copy the address of the allocated space to our uint8_t* pointer
            disk = (uint8_t*)wordAlignedDisk;
            if ( NULL == disk )
            {
                status.setStatus(EEPROMStatus::EEPROM_ERROR_INSUFFICIENT_MEMORY);
                success = false;
            }
            else
            {
                fileTable = (fileEntry_t *)disk; // set pointer to fileTable at start of disk
                validFileSystemTable = validateFileSystem(); // this also sets the status
                success = true; // we return true even if the filesystem has no valid table - that's reflected in the status message
            }
        }
    }
    else
    {
        status.setStatus(EEPROMStatus:: EEPROM_ERROR_API);
        success = false;
    }

    return success;
}

bool EEPROMFS::validateFileSystem()
{
    uint32_t i;

    // initially, assume we're going to fail, update with specific failure types if we can
    status.setStatus(EEPROMStatus::EEPROM_ERROR_INVALID_FILE_SYSTEM_TABLE);
    validFileSystemTable = false;

    // reset properties
    activeFiles.clear();
    bytesUsed = EEPROM_FIRST_FILE_ADDR; // at a minimum, we're using a portion for the file system table

    // read our entire "disk" into memory
    if ( eepromSize != read(disk, EEPROM_FTABLE_ADDR, eepromSize) )
    {
        bytesUsed = 0; // we've failed
        return false;
    }

    status.setStatus(EEPROMStatus::EEPROM_ERROR_INVALID_FILE_SYSTEM_TABLE);

    uint32_t lastEndPoint = EEPROM_FIRST_FILE_ADDR; // the very first occurs at start of file data section

    // Verify the table is reasonable
    for ( i = 0; i < EEPROM_MAX_NUM_FILES; i++ )
    {
        // Check for a disabled entry (zeroed out startAddress), verify size is also disabled
        if ( fileTable[i].startAddress == 0 && fileTable[i].size != 0 )
        {
            bytesUsed = 0; // we've failed
            return false;
        }
        // If the entry is purely disabled, skip and go to next entry
        else if ( fileTable[i].startAddress == 0 && fileTable[i].size == 0 )
        {
            continue;
        }
        // If it's enabled, verify it comes after end of last file
        else if ( fileTable[i].startAddress < lastEndPoint )
        {
            bytesUsed = 0; // we've failed
            return false;
        }
        // If the file's address is good, verify length is reasonable given size and place in EEPROM
        else if ( (fileTable[i].startAddress + fileTable[i].size) > eepromSize )
        {
            bytesUsed = 0; // we've failed
            return false;
        }

        activeFiles.insert(i);
        lastEndPoint = fileTable[i].startAddress + fileTable[i].size; // update pointer to end of this file
        bytesUsed += fileTable[i].size; // add file usage to total amount tracked
    }

    // For each active file, verify that they are ASCII string operation safe (printable text and NULL terminated)
    for ( uint8_t file : activeFiles )
    {
        uint32_t nullCount;
        uint32_t j = 0;

        for ( nullCount = 0, j = 0; j < fileTable[file].size; j++ )
        {
            // Look for NULL, they (maybe more than one) should only appear at the end (not in the middle) of a file
            if (0 == disk[fileTable[file].startAddress + j])
            {
                nullCount += 1;
            }
            // Look for non-printable characters
            else if ( (' ' > disk[fileTable[file].startAddress + j]) && \
                 (disk[fileTable[file].startAddress + j] > '~') )
            {
                status.setStatus(EEPROMStatus::EEPROM_ERROR_NON_ASCII);
                // TODO: This sucks a little, because it will require the user
                //   to nuke the entire EEPROM, where one file may be at fault
                bytesUsed = 0; // we've failed
                return false;
            }
            // This is a printable character. It should not come after any NULL character
            else if ( 0 != nullCount )
            {
                status.setStatus(EEPROMStatus::EEPROM_ERROR_UNEXPECTED_NULLS);
                // TODO: This sucks a little, because it will require the user
                //   to nuke the entire EEPROM, where one file may be at fault
                bytesUsed = 0; // we've failed
                return false;
            }
        }
    }

    // set the status and validFileSystemTable flags indicating everything is good!
    status.setStatus(EEPROMStatus::EEPROM_OK);
    validFileSystemTable = true;

    return validFileSystemTable;
}

bool EEPROMFS::formatEEPROM()
{
    uint32_t i;
    bool writeStatus;

    // nuke entire EEPROM - set to FF's
    if ( 0 != EEPROMMassErase() )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_API);
        return false;
    }

    bytesUsed = EEPROM_FIRST_FILE_ADDR;
    // init file system table to zeros (all disabled)
    for ( i = 0; i < EEPROM_MAX_NUM_FILES; i++ ) {
        fileTable[i].startAddress = 0;
        fileTable[i].size = 0;
    }

    // Call to write() will set the EEPROM status property
    writeStatus = write((uint8_t*)fileTable, EEPROM_FTABLE_ADDR, (EEPROM_MAX_NUM_FILES * sizeof(fileEntry_t)));

    if ( !writeStatus )
    {
        bytesUsed = 0;
    }

    return writeStatus;
}

bool EEPROMFS::updateHandle(uint8_t index)
{
    std::map<int, manager_t*>::iterator it;

    // Look for object managing file at given index
    it = handleManager.find(index);

    // If we don't have it in our map we've made an error somewhere
    if ( it == handleManager.end() )
    {
        return false;
    }
    else
    {
        it->second->handle->size = fileTable[index].size;
        it->second->handle->data = disk + fileTable[index].startAddress;
    }

    return true;
}

bool EEPROMFS::shiftFileData(uint8_t* headPtr, uint16_t size, int32_t distance)
{
    uint16_t i;
    uint8_t *copyPtr;
    int8_t dir;

    // Sanity checks
    // Are both headPtr and "tail pointer" within the range covered by "disk" buffer?
    if ( (disk > headPtr) ||
        ((headPtr - disk) > eepromSize) ||
        ((headPtr + size) > (disk + eepromSize)) )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_INTERNAL);
        return false;
    }

    // Move to the "right"
    if ( (distance > 0) && (distance < static_cast<int32_t>(eepromSize)) )
    {
        copyPtr = (headPtr + size - 1); // tail pointer

        // make sure the move will not fall off the end of the buffer
        if ( (disk + eepromSize) < (copyPtr + distance) )
        {
            status.setStatus(EEPROMStatus::EEPROM_ERROR_INSUFFICIENT_STORAGE);
            return false;
        }

        dir = -1; // moving the pointer to the "left" each iteration
    }
    // Move to the "left"
    else if ( (distance < 0) && (distance > (-1 * static_cast<int32_t>(eepromSize))) )
    {
        copyPtr = headPtr;

        // make sure the move will not moves past the beginning of the buffer
        if ( disk > (copyPtr + distance) ) // distance is negative
        {
            status.setStatus(EEPROMStatus::EEPROM_ERROR_INTERNAL);
            return false;
        }

        dir = 1; // moving the pointer to the "right" each iteration
    }
    else
    {
        return false;
    }

    // Shift buffer section over by 'distance' bytes repetitively until entire section has been moved
    for ( i = 0; i < size; i++ )
    {
        *(copyPtr + distance) = *copyPtr;
        // Invalidate where our old data used to be - don't want remnanats of data hanging around
        *copyPtr = 0xFF;
        // move pointer for next iteration
        copyPtr += dir;
    }

    return true;
}

#if defined(__linux__)
uint32_t EEPROMFS::FauxEEPROMMassErase()
{
    std::fstream fs;

    fs.open(UNIX_NONVOLATILE_FILE, std::ios::out);
    if ( !fs.is_open() )
    {
        status.setStatus(EEPROMStatus::EEPROM_ERROR_INTERNAL);
        return 1; // anything other than zero is a failure flag
    }
    // fill the space with invalid data
    const char filler[2] = { '\xff', 0 };
    for ( uint32_t i = 0; i < eepromSize; i++ )
    {
        fs.write(filler, 1);
    }
    fs.close();

    return 0;
}
#endif

/******************************* EOF *******************************************/



