/*
 * EEPROMStatus.cpp
 *
 *  Created on: May 18, 2020
 *      Author: mark
 */

#include "EEPROMStatus.h"
#include <cstdio>
#include <cstring>

EEPROMStatus::EEPROMStatus () :
    status(EEPROM_ERROR_NOT_INITIALIZED)
{
    memset(szStatus, 0, EEPROMSTATUS_BUF_LEN); // init array with null
}

void EEPROMStatus::setStatus(eepromStatus_t input)
{
    status = input;
}

EEPROMStatus::eepromStatus_t EEPROMStatus::value()
{
    return status;
}

char* EEPROMStatus::c_str ()
{
    switch(status)
    {
    case EEPROM_OK:
        snprintf(szStatus, EEPROMSTATUS_BUF_LEN, "OK");
        break;
    case EEPROM_ERROR_BAD_PARAMS:
        snprintf(szStatus, EEPROMSTATUS_BUF_LEN, "BAD PARAMS");
        break;
    case EEPROM_ERROR_FILE_NOT_FOUND:
        snprintf(szStatus, EEPROMSTATUS_BUF_LEN, "NOT FOUND");
        break;
    case EEPROM_ERROR_INSUFFICIENT_STORAGE:
        snprintf(szStatus, EEPROMSTATUS_BUF_LEN, "OUT OF MEMORY");
        break;
    case EEPROM_ERROR_INSUFFICIENT_MEMORY:
        snprintf(szStatus, EEPROMSTATUS_BUF_LEN, "INSUFFICIENT RAM");
        break;
    case EEPROM_ERROR_WRITE_ERROR:
        snprintf(szStatus, EEPROMSTATUS_BUF_LEN, "WRITE ERROR");
        break;
    case EEPROM_ERROR_NOT_INITIALIZED:
        snprintf(szStatus, EEPROMSTATUS_BUF_LEN, "HW UNINITIALIZED");
        break;
    case EEPROM_ERROR_WRITE_PROTECTED:
        snprintf(szStatus, EEPROMSTATUS_BUF_LEN, "WRITE PREVENTED");
        break;
    case EEPROM_ERROR_INVALID_FILE_SYSTEM_TABLE:
        snprintf(szStatus, EEPROMSTATUS_BUF_LEN, "INVALID FS TABLE");
        break;
    case EEPROM_ERROR_NON_ASCII:
        snprintf(szStatus, EEPROMSTATUS_BUF_LEN, "BAD FILE DATA");
        break;
    case EEPROM_ERROR_UNEXPECTED_NULLS:
        snprintf(szStatus, EEPROMSTATUS_BUF_LEN, "UNEXPECTED NULLS");
        break;
    case EEPROM_ERROR_WORD_ALIGNMENT:
        snprintf(szStatus, EEPROMSTATUS_BUF_LEN, "WORD MISALIGNMENT");
        break;
    case EEPROM_ERROR_API:
        snprintf(szStatus, EEPROMSTATUS_BUF_LEN, "API ERROR");
        break;
    case EEPROM_ERROR_INTERNAL:
        snprintf(szStatus, EEPROMSTATUS_BUF_LEN, "INTERNAL ERROR");
        break;
    default:
        snprintf(szStatus, EEPROMSTATUS_BUF_LEN, "UNKNOWN");
    }
    return szStatus;
}



