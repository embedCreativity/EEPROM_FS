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

#include <iostream>
#include <string>
#include <cstring>
#include "EEPROM_FS.h"
#include "EEPROMStatus.h"

int main ( void )
{
    EEPROMFS hEeprom;
    std::string userInput;

    std::cout << "EEPROM status: " << hEeprom.getStatus().c_str() << std::endl;

    // Check if we need to initialize the file system
    if ( hEeprom.getStatus().value() == EEPROMStatus::EEPROM_ERROR_INVALID_FILE_SYSTEM_TABLE )
    {
        std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
        std::cout << "Formatting filesystem now..." << std::endl;
        hEeprom.enableWrite();
        hEeprom.format();
        std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
    }

    if ( hEeprom.getStatus().value() != EEPROMStatus::EEPROM_OK )
    {
        std::cout << "ERROR: EEPROM is in a failed state (" << hEeprom.getStatus().c_str() << ") and unit test cannot continue" << std::endl;
        return -1;
    }

    std::cout << "File system is valid and ready to use. Current usage: " << hEeprom.getUsedCapacity() << " out of " << hEeprom.getTotalCapacity() << " bytes" << std::endl;
    std::cout << "Current active file count: " << hEeprom.getActiveFileCount() << std::endl;

    /*********
    std::cout << "Hit enter to continue" << std::endl;
    std::cin >> userInput;
    *********/

    // Now we'll try and write a file to the first file index
    std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "--> First File Insertion Test at Index=0 <--" << std::endl;
    char msg[] = "Hello, World!";
    std::cout << "Writing a file with " << strlen(msg) + 1 << " bytes into index 0" << std::endl;
    hEeprom.enableWrite();
    if ( !hEeprom.writeFile(0, (uint8_t*)msg, static_cast<uint16_t>(strlen(msg)) + 1) ) // capture that NULL character
    {
        std::cout << "ERROR: writeFile returned an error for our initial write attempt" << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        return -1;
    }
    std::cout << "New file system usage: " << hEeprom.getUsedCapacity() << " out of " << hEeprom.getTotalCapacity() << " bytes" << std::endl;
    std::cout << "New active file count: " << hEeprom.getActiveFileCount() << std::endl;

    /*********
    std::cout << "Hit enter to continue" << std::endl;
    std::cin >> userInput;
    *********/

    // Now we'll try and write another file, but this time in the third file index
    std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "--> Second File Insertion Test at Index=2 <--" << std::endl;
    char msg2[] = "I like big butts and I cannot lie.\nIt's what you other brothers can't deny...";
    std::cout << "Writing a file with " << strlen(msg2) + 1 << " bytes into index 2" << std::endl;
    hEeprom.enableWrite();
    if ( !hEeprom.writeFile(2, (uint8_t*)msg2, static_cast<uint16_t>(strlen(msg2)) + 1) ) // capture that NULL character
    {
        std::cout << "ERROR: writeFile returned an error for our second write attempt" << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        return -1;
    }
    std::cout << "New file system usage: " << hEeprom.getUsedCapacity() << " out of " << hEeprom.getTotalCapacity() << " bytes" << std::endl;
    std::cout << "New active file count: " << hEeprom.getActiveFileCount() << std::endl;

    /*********
    std::cout << "Hit enter to continue" << std::endl;
    std::cin >> userInput;
    *********/

    // Now we'll try and write a third file, but this time between the first two files at index 1
    std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "--> Third File Insertion Test at Index=1 <--" << std::endl;
    char msg3[] = "My mother always said,\nLife is like a box of chocolates";
    std::cout << "Writing a file with " << strlen(msg3) + 1 << " bytes into index 1" << std::endl;
    hEeprom.enableWrite();
    if ( !hEeprom.writeFile(1, (uint8_t*)msg3, static_cast<uint16_t>(strlen(msg3)) + 1) ) // capture that NULL character
    {
        std::cout << "ERROR: writeFile returned an error for our third write attempt" << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        return -1;
    }
    std::cout << "New file system usage: " << hEeprom.getUsedCapacity() << " out of " << hEeprom.getTotalCapacity() << " bytes" << std::endl;
    std::cout << "New active file count: " << hEeprom.getActiveFileCount() << std::endl;

    /*********
    std::cout << "Hit enter to continue" << std::endl;
    std::cin >> userInput;
    *********/

    // Now we'll try and write a fourth file, but this time we'll overwrite the first file with a new file of the same length
    std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "--> Fourth File Insertion Test - Overwriting file at Index 0 with same length <--" << std::endl;
    char msg4[] = "Puppy kibble!";
    std::cout << "Writing a file with " << strlen(msg4) + 1 << " bytes into index 0" << std::endl;
    hEeprom.enableWrite();
    if ( !hEeprom.writeFile(0, (uint8_t*)msg4, static_cast<uint16_t>(strlen(msg4)) + 1) ) // capture that NULL character
    {
        std::cout << "ERROR: writeFile returned an error during our write attempt" << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        return -1;
    }
    std::cout << "New file system usage: " << hEeprom.getUsedCapacity() << " out of " << hEeprom.getTotalCapacity() << " bytes" << std::endl;
    std::cout << "New active file count: " << hEeprom.getActiveFileCount() << std::endl;

    /*********
    std::cout << "Hit enter to continue" << std::endl;
    std::cin >> userInput;
    *********/

    // Now we'll try and write a fifth file, but this time we'll overwrite the second file with a new file of longer length
    std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "--> Fifth File Insertion Test - Overwriting file at Index 1 with longer length <--" << std::endl;
    char msg5[] = "A pumpkin gets turned into a chariot that brought some lady to a ball. There were mice";
    std::cout << "Writing a file with " << strlen(msg5) + 1 << " bytes into index 1" << std::endl;
    hEeprom.enableWrite();
    if ( !hEeprom.writeFile(1, (uint8_t*)msg5, static_cast<uint16_t>(strlen(msg5)) + 1) ) // capture that NULL character
    {
        std::cout << "ERROR: writeFile returned an error during our write attempt" << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        return -1;
    }
    std::cout << "New file system usage: " << hEeprom.getUsedCapacity() << " out of " << hEeprom.getTotalCapacity() << " bytes" << std::endl;
    std::cout << "New active file count: " << hEeprom.getActiveFileCount() << std::endl;

    /*********
    std::cout << "Hit enter to continue" << std::endl;
    std::cin >> userInput;
    *********/

    // Now we'll try and write a sixth file, but this time we'll overwrite the second file with a new file of smaller length
    std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "--> Sixth File Insertion Test - Overwriting file at Index 1 with shorter length <--" << std::endl;
    char msg6[] = "Leif is a cat";
    std::cout << "Writing a file with " << strlen(msg6) + 1 << " bytes into index 1" << std::endl;
    hEeprom.enableWrite();
    if ( !hEeprom.writeFile(1, (uint8_t*)msg6, static_cast<uint16_t>(strlen(msg6)) + 1) ) // capture that NULL character
    {
        std::cout << "ERROR: writeFile returned an error during our write attempt" << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        return -1;
    }
    std::cout << "New file system usage: " << hEeprom.getUsedCapacity() << " out of " << hEeprom.getTotalCapacity() << " bytes" << std::endl;
    std::cout << "New active file count: " << hEeprom.getActiveFileCount() << std::endl;

    /*********
    std::cout << "Hit enter to continue" << std::endl;
    std::cin >> userInput;
    *********/

    // Now we'll try and write a seventh file, but this time we'll overwrite the first file with a new file of smaller length
    std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "--> Seventh File Insertion Test - Overwriting file at Index 0 with shorter length <--" << std::endl;
    char msg7[] = "Foobar";
    std::cout << "Writing a file with " << strlen(msg7) + 1 << " bytes into index 0" << std::endl;
    hEeprom.enableWrite();
    if ( !hEeprom.writeFile(0, (uint8_t*)msg7, static_cast<uint16_t>(strlen(msg7)) + 1) ) // capture that NULL character
    {
        std::cout << "ERROR: writeFile returned an error during our write attempt" << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        return -1;
    }
    std::cout << "New file system usage: " << hEeprom.getUsedCapacity() << " out of " << hEeprom.getTotalCapacity() << " bytes" << std::endl;
    std::cout << "New active file count: " << hEeprom.getActiveFileCount() << std::endl;

    /*********
    std::cout << "Hit enter to continue" << std::endl;
    std::cin >> userInput;
    *********/

    // Now we'll try and write a eigth file, but this time we'll overwrite the last file with a new file of smaller length
    std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "--> Eigth File Insertion Test - Overwriting file at Index 2 with shorter length <--" << std::endl;
    char msg8[] = "Second!";
    std::cout << "Writing a file with " << strlen(msg8) + 1 << " bytes into index 2" << std::endl;
    hEeprom.enableWrite();
    if ( !hEeprom.writeFile(2, (uint8_t*)msg8, static_cast<uint16_t>(strlen(msg8)) + 1) ) // capture that NULL character
    {
        std::cout << "ERROR: writeFile returned an error during our write attempt" << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        return -1;
    }
    std::cout << "New file system usage: " << hEeprom.getUsedCapacity() << " out of " << hEeprom.getTotalCapacity() << " bytes" << std::endl;
    std::cout << "New active file count: " << hEeprom.getActiveFileCount() << std::endl;

    /*********
    std::cout << "Hit enter to continue" << std::endl;
    std::cin >> userInput;
    *********/

    // Now we'll try and write a nineth file, but this time we'll make a new file at the last file location
    std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "--> Nineth File Insertion Test - New file at Index 19 <--" << std::endl;
    char msg9[] = "19th file at end of space!";
    std::cout << "Writing a file with " << strlen(msg9) + 1 << " bytes into index 19" << std::endl;
    hEeprom.enableWrite();
    if ( !hEeprom.writeFile(19, (uint8_t*)msg9, static_cast<uint16_t>(strlen(msg9)) + 1) ) // capture that NULL character
    {
        std::cout << "ERROR: writeFile returned an error during our write attempt" << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        return -1;
    }
    std::cout << "New file system usage: " << hEeprom.getUsedCapacity() << " out of " << hEeprom.getTotalCapacity() << " bytes" << std::endl;
    std::cout << "New active file count: " << hEeprom.getActiveFileCount() << std::endl;

    /*********
    std::cout << "Hit enter to continue" << std::endl;
    std::cin >> userInput;
    *********/

    // Now we'll try and write a tenth file, but we'll attempt to put it off the end of the file table
    std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "--> Tenth File Insertion Test - BAD New file at Index 20 (out of bounds of file table) <--" << std::endl;
    char msg10[] = "This shouldn't work";
    std::cout << "Writing a file with " << strlen(msg10) + 1 << " bytes into index 20" << std::endl;
    hEeprom.enableWrite();
    if ( hEeprom.writeFile(20, (uint8_t*)msg10, static_cast<uint16_t>(strlen(msg10)) + 1) ) // capture that NULL character
    {
        std::cout << "ERROR: writeFile Did NOT return an error like it should have upon a bad file number attempt" << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        return -1;
    }
    else
    {
        std::cout << "INFO: Good failure returned: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
    }
    std::cout << "New file system usage: " << hEeprom.getUsedCapacity() << " out of " << hEeprom.getTotalCapacity() << " bytes" << std::endl;
    std::cout << "New active file count: " << hEeprom.getActiveFileCount() << std::endl;

    /*********
    std::cout << "Hit enter to continue" << std::endl;
    std::cin >> userInput;
    *********/

    // Now we'll try and write a eleventh file, this time it will be huge and not at the end AND it will be ONE byte too long (NULL terminator pushes it over)
    std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "--> Eleventh File Insertion Test - BAD New file at Index 10 (ONE byte too long) <--" << std::endl;
    char msg11[] = "Contrary to popular belief, Lorem Ipsum is not simply random text. It has roots in a piece of classical Latin literature from 45 BC,"
                   " making it over 2000 years old. Richard McClintock, a Latin professor at Hampden-Sydney College in Virginia, looked up one of the more"
                   " obscure Latin words, consectetur, from a Lorem Ipsum passage, and going through the cites of the word in classical literature, discovered"
                   " the undoubtable source. Lorem Ipsum comes from sections 1.10.32 and 1.10.33 of \"de Finibus Bonorum et Malorum\" (The Extremes of Good and"
                   " Evil) by Cicero, written in 45 BC. This book is a treatise on the theory of ethics, very popular during the Renaissance. The first line of"
                   " Lorem Ipsum \"Lorem ipsum dolor sit\".\n"
                   "Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since"
                   " the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five"
                   " centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release"
                   " of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions"
                   " of Lorem Ipsum.\n"
                   "It is a long established fact that a reader will be distracted by the readable content of a page when looking at its layout. The point of using"
                   " Lorem Ipsum is that it has a more-or-less normal distribution of letters, as opposed to using 'Content here, content here', making it look like"
                   " readable English. Many desktop publishing packages and web page editors now use Lorem Ipsum as their default model text, and a search for 'lorem"
                   " ipsum' will uncover many web sites still in their infancy. Various versions have evolved over the years, sometimes by accident, sometimes on"
                   " purpose (injected humour and the like).FEDCBA";

    std::cout << "Writing a file with " << strlen(msg11) + 1 << " bytes into index 10" << std::endl;
    hEeprom.enableWrite();
    if ( hEeprom.writeFile(10, (uint8_t*)msg11, static_cast<uint16_t>(strlen(msg11)) + 1) ) // capture that NULL character
    {
        std::cout << "ERROR: writeFile Did NOT return an error like it should have upon a bad file length attempt" << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        return -1;
    }
    else
    {
        std::cout << "INFO: Good failure returned: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
    }
    std::cout << "New file system usage: " << hEeprom.getUsedCapacity() << " out of " << hEeprom.getTotalCapacity() << " bytes" << std::endl;
    std::cout << "New active file count: " << hEeprom.getActiveFileCount() << std::endl;

    /*********
    std::cout << "Hit enter to continue" << std::endl;
    std::cin >> userInput;
    *********/

    // Now we'll try and write a twelfth file, this time it will be huge and not at the end but JUST fit in, maxing out file system
    std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "--> Twelfth File Insertion Test - New file at Index 10 <--" << std::endl;
    char msg12[] = "Contrary to popular belief, Lorem Ipsum is not simply random text. It has roots in a piece of classical Latin literature from 45 BC,"
                   " making it over 2000 years old. Richard McClintock, a Latin professor at Hampden-Sydney College in Virginia, looked up one of the more"
                   " obscure Latin words, consectetur, from a Lorem Ipsum passage, and going through the cites of the word in classical literature, discovered"
                   " the undoubtable source. Lorem Ipsum comes from sections 1.10.32 and 1.10.33 of \"de Finibus Bonorum et Malorum\" (The Extremes of Good and"
                   " Evil) by Cicero, written in 45 BC. This book is a treatise on the theory of ethics, very popular during the Renaissance. The first line of"
                   " Lorem Ipsum \"Lorem ipsum dolor sit\".\n"
                   "Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since"
                   " the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five"
                   " centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release"
                   " of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions"
                   " of Lorem Ipsum.\n"
                   "It is a long established fact that a reader will be distracted by the readable content of a page when looking at its layout. The point of using"
                   " Lorem Ipsum is that it has a more-or-less normal distribution of letters, as opposed to using 'Content here, content here', making it look like"
                   " readable English. Many desktop publishing packages and web page editors now use Lorem Ipsum as their default model text, and a search for 'lorem"
                   " ipsum' will uncover many web sites still in their infancy. Various versions have evolved over the years, sometimes by accident, sometimes on"
                   " purpose (injected humour and the like).EDCBA";

    std::cout << "Writing a file with " << strlen(msg12) + 1 << " bytes into index 10" << std::endl;
    hEeprom.enableWrite();
    if ( !hEeprom.writeFile(10, (uint8_t*)msg12, static_cast<uint16_t>(strlen(msg12)) + 1) ) // capture that NULL character
    {
        std::cout << "ERROR: writeFile returned an error during our write attempt" << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        return -1;
    }
    std::cout << "New file system usage: " << hEeprom.getUsedCapacity() << " out of " << hEeprom.getTotalCapacity() << " bytes" << std::endl;
    std::cout << "New active file count: " << hEeprom.getActiveFileCount() << std::endl;

    /*********
    std::cout << "Hit enter to continue" << std::endl;
    std::cin >> userInput;
    *********/

    // Now we'll try and write a thirteenth file, replacing the big file, testing if it realizes that it WILL have room if ignoring its present size
    std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "--> Thirteenth File Insertion Test - Small overwrite of big file at Index 10 <--" << std::endl;
    char msg13[] = "Tenth";

    std::cout << "Writing a file with " << strlen(msg13) + 1 << " bytes into index 10" << std::endl;
    hEeprom.enableWrite();
    if ( !hEeprom.writeFile(10, (uint8_t*)msg13, static_cast<uint16_t>(strlen(msg13)) + 1) ) // capture that NULL character
    {
        std::cout << "ERROR: writeFile returned an error during our write attempt" << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        return -1;
    }
    std::cout << "New file system usage: " << hEeprom.getUsedCapacity() << " out of " << hEeprom.getTotalCapacity() << " bytes" << std::endl;
    std::cout << "New active file count: " << hEeprom.getActiveFileCount() << std::endl;

    /*********
    std::cout << "Hit enter to continue" << std::endl;
    std::cin >> userInput;
    *********/

    // Now we'll try to delete a file
    std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "--> First File Deletion Test - Delete file at index 0 <--" << std::endl;

    hEeprom.enableWrite();
    if ( !hEeprom.deleteFile(0) )
    {
        std::cout << "ERROR: deleteFile returned an error" << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        return -1;
    }
    std::cout << "New file system usage: " << hEeprom.getUsedCapacity() << " out of " << hEeprom.getTotalCapacity() << " bytes" << std::endl;
    std::cout << "New active file count: " << hEeprom.getActiveFileCount() << std::endl;

    /*********
    std::cout << "Hit enter to continue" << std::endl;
    std::cin >> userInput;
    *********/

    // Now we'll try and write a fourteenth file, we'll put the file back at index 0
    std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "--> Fourteenth File Insertion Test - we'll put the file back at index 0 <--" << std::endl;
    char msg14[] = "Shazaam";
    std::cout << "Writing a file with " << strlen(msg14) + 1 << " bytes into index 0" << std::endl;
    hEeprom.enableWrite();
    if ( !hEeprom.writeFile(0, (uint8_t*)msg14, static_cast<uint16_t>(strlen(msg14)) + 1) ) // capture that NULL character
    {
        std::cout << "ERROR: writeFile returned an error during our write attempt" << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        return -1;
    }
    std::cout << "New file system usage: " << hEeprom.getUsedCapacity() << " out of " << hEeprom.getTotalCapacity() << " bytes" << std::endl;
    std::cout << "New active file count: " << hEeprom.getActiveFileCount() << std::endl;

    /*********
    std::cout << "Hit enter to continue" << std::endl;
    std::cin >> userInput;
    *********/


    /***************************************************************************************************************************/
    std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "--> First Read Test <--" << std::endl;
    // Open file at index 1 - "Leif is a cat"
    handle_t* hFile = hEeprom.open(1);

    if ( NULL == hFile )
    {
        std::cout << "ERROR: open returned an error" << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        return -1;
    }

    // Be a good person and lock the system whilst accessing data
    hEeprom.getLock();

    // Verify size
    if ( hFile->size != (strlen("Leif is a cat") + 1) )
    {
        std::cout << "ERROR: file handle returned unexpected length: " << hFile->size << ". Expecting " << (strlen("Leif is a cat") + 1) << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        hEeprom.releaseLock();
        hEeprom.close(1);
        return -1;
    }

    // Verify file pointer (just not NULL)
    if ( NULL == hFile->data )
    {
        std::cout << "ERROR: file handle returned NULL data pointer" << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        hEeprom.releaseLock();
        hEeprom.close(1);
        return -1;
    }

    // Print data
    std::cout << "INFO: open file test for index 1 (Leif is a cat): data-> \"" << hFile->data << "\"" << std::endl;

    // Be a good person and release assets
    hEeprom.releaseLock();

    /***************************************************************************************************************************/
    std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "--> Second Read After Relocating File Data <--" << std::endl;

    char msg15[] = "This is a longer 1st file than what we had";
    std::cout << "Writing a file with " << strlen(msg15) + 1 << " bytes into index 0" << std::endl;
    hEeprom.enableWrite();
    if ( !hEeprom.writeFile(0, (uint8_t*)msg15, static_cast<uint16_t>(strlen(msg15)) + 1) ) // capture that NULL character
    {
        std::cout << "ERROR: writeFile returned an error during our write attempt" << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        return -1;
    }
    std::cout << "New file system usage: " << hEeprom.getUsedCapacity() << " out of " << hEeprom.getTotalCapacity() << " bytes" << std::endl;
    std::cout << "New active file count: " << hEeprom.getActiveFileCount() << std::endl;

    /*********
    std::cout << "Hit enter to continue" << std::endl;
    std::cin >> userInput;
    *********/

    if ( NULL == hFile )
    {
        std::cout << "ERROR: handle is NULL" << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        return -1;
    }

    // Be a good person and lock the system whilst accessing data
    hEeprom.getLock();

    // Verify size
    if ( hFile->size != (strlen("Leif is a cat") + 1) )
    {
        std::cout << "ERROR: file handle returned unexpected length: " << hFile->size << ". Expecting " << (strlen("Leif is a cat") + 1) << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        hEeprom.releaseLock();
        hEeprom.close(1);
        return -1;
    }

    // Verify file pointer (just not NULL)
    if ( NULL == hFile->data )
    {
        std::cout << "ERROR: file handle returned NULL data pointer" << std::endl;
        std::cout << "INFO: EEPROM state: " << hEeprom.getStatus().c_str() << std::endl;
        hEeprom.releaseLock();
        hEeprom.close(1);
        return -1;
    }

    // Print data
    std::cout << "INFO: open file test for index 1 (Leif is a cat): data-> \"" << hFile->data << "\"" << std::endl;

    // Be a good person and release assets
    hEeprom.releaseLock();
    hEeprom.close(1);

    /***************************************************************************************************************************/
    std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "--> Verify getActiveFiles() <--" << std::endl;
    std::map<uint8_t, uint16_t> fileMap = hEeprom.getActiveFiles();

    // Create a map iterator and point to beginning of map
    std::map<uint8_t, uint16_t>::iterator mit = fileMap.begin();
    // Iterate over the map using Iterator till end.
    for (std::map<uint8_t, uint16_t>::iterator mit = fileMap.begin(); mit != fileMap.end(); mit++)
    {
        // Accessing KEY from element pointed by it.
        uint8_t fileId = mit->first;
        // Accessing VALUE from element pointed by it.
        uint16_t size = mit->second;
        std::cout << "FileId: " << unsigned(fileId) << ", size: " << size << std::endl;
    }

    return 0;
}
