/*
 * EEPROMStatus.h
 *
 *  Created on: May 18, 2020
 *      Author: mark
 */

#ifndef BOARD_UTILS_EEPROM_EEPROMSTATUS_H_
#define BOARD_UTILS_EEPROM_EEPROMSTATUS_H_

// size of char array used for printing EEPROM status (sizeof(uint8_t) or less)
#define EEPROMSTATUS_BUF_LEN            20
class EEPROMStatus
{
public:
    typedef enum {
        EEPROM_OK = 0,
        // Invalid parameters supplied to EEPROM API
        EEPROM_ERROR_BAD_PARAMS,
        // Expected file not found
        EEPROM_ERROR_FILE_NOT_FOUND,
        // attempting to write out-of-bounds
        EEPROM_ERROR_INSUFFICIENT_STORAGE,
        // not enough system memory to support mirroring in RAM
        EEPROM_ERROR_INSUFFICIENT_MEMORY,
        // failure detected during write operation
        EEPROM_ERROR_WRITE_ERROR,
        // EEPROM hardware not initialized
        EEPROM_ERROR_NOT_INITIALIZED,
        // Caller did not enable write operations prior to write call
        EEPROM_ERROR_WRITE_PROTECTED,
        // corrupted or uninitialized file system table detected
        EEPROM_ERROR_INVALID_FILE_SYSTEM_TABLE,
        // non-printable characters found in file(s)
        EEPROM_ERROR_NON_ASCII,
        // NULL terminator found in middle of file (not only at the end of the file)
        EEPROM_ERROR_UNEXPECTED_NULLS,
        // Read Write API calls must be done with 32-bit word alignment in mind
        EEPROM_ERROR_WORD_ALIGNMENT,
        // Used to reflect an API-specific error was returned as a result of last operation
        EEPROM_ERROR_API,
        // Used to reflect an error internal to our implementation
        EEPROM_ERROR_INTERNAL
    } eepromStatus_t;

    // Constructor
    EEPROMStatus();
    // Destructor
    ~EEPROMStatus(){};

    // Getter/Setter
    void setStatus(eepromStatus_t status);
    eepromStatus_t value();

    // To C-string operator
    char* c_str (void);

private:
    eepromStatus_t status;
    char szStatus[EEPROMSTATUS_BUF_LEN];
};


#endif /* BOARD_UTILS_EEPROM_EEPROMSTATUS_H_ */
