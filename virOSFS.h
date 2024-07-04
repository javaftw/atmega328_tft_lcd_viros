/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   virOSFS.h
 * Author: root
 *
 * Created on 16 April 2019, 4:45 PM
 */

#ifndef VIROSFS_H
#define VIROSFS_H


/********************************************************* EEPROM File System */

/************************************* Memory offsets 
 *                      EEPROM 'HARD DRIVE' MAP
 *           -------------------------------------------------
 *       000h|N|FDI1|FDI2|FDI3|...|.32.|f1....|f2..|f3........|...
 *         001h|    |    |        |    |
 *           : :005h|    |        |    |
 *           : :     009h|...     |    |
 *           : :              081h|    |
 *           : :                   085h|
 *           [1] Number of files (0-32) 000h         :
 *             [    File Lookup List   ] 001h-081h (32 files x 4 bytes per FDI)
 *                                     [ Files ... ] 081h-3FFh
 */
#define FILE_SYSTEM_VERSION_NUMBER_ADDR          0x000
#define FILE_SYSTEM_VERSION_NUMBER               0x01
#define FILE_DESCR_LIST_START_ADDR               0x001
#define FILE_DESCRIPTOR_INODE_SIZE               0x04
#define FILE_SECTOR_START_ADDR                   (FILE_DESCR_LIST_START_ADDR+ (FILE_DESCRIPTOR_INODE_SIZE * FILE_COUNT_MAX))
#define FILE_SECTOR_MAX_ADDR                     0x3FF//1024
#define FILE_COUNT_MAX                           0x1F//32 files max
#define FILE_BUFFER_SIZE_MAX                     0x03F//128

typedef struct FileDesciptor_INode {
    /* File descriptor 4 bytes */
    /* Maximum of 128 file names possible */
    /* 324 bytes total  */
    /* Address 0h to 17Fh (384 bytes) is reserved for the File Lookup Table */
    //===============================================BYTE 1
    //-------------------file exists
    // null file upon creation
    // allows for recovery? and efficient overwrite
    // of new file with same size
    //0 - doesn't exist/deleted
    //1 - active
    uint16_t file_exists : 1;
    //-------------------filename
    //The filename can be a7,h3,k2,f5...etc
    //128 possible filenames
    uint16_t name_number : 3; //0-7
    uint16_t name_letter : 4; //0-15 a,b,c,...,n,o,p
    //===============================================BYTE 2
    //-------------------write lock
    // Any write to a file must be accompanied by a write lock
    // The lock must be released by the caller after the write
    // is complete
    // A read of the file may also not be permitted while write locked
    //0 - unlocked
    //1 - locked
    uint16_t write_lock : 1;
    //-------------------defrag lock
    //0 - unlocked (may be repositioned)
    //1 - locked (may not be repositioned)
    uint16_t move_lock : 1;
    //-------------------name lock
    //0 - unlocked (may be renamed)
    //1 - locked (may not be renamed)
    uint16_t name_lock : 1;
    //-------------------type
    //00 - data
    //01 - executable
    //11 - text? filesystem?
    uint16_t type : 2;
    //-------------------size UPPER
    //the upper 3 bits of the size (max 9 bits = 512 bytes)
    uint16_t size_UPPER : 3;
    //===============================================BYTE 3
    //-------------------location of file UPPER 
    //Between FILE_SECTOR_START_ADDR and FILE_SECTOR_MAX_ADDR
    uint16_t file_address_UPPER : 2;
    //-------------------payload size LOWER
    //The lower 6 bits of the size (max 9 bits = 512 bytes)
    uint16_t size_LOWER : 6;
    //===============================================BYTE 4
    //-------------------location of file LOWER
    uint16_t file_address_LOWER : 8;
} FDI;

typedef struct FileDirectory {
    /*Note: the filename only occupies the lower 7 of the 8 bits
     so the 8th bit (MSB) is an "exists" flag*/
    uint8_t filenames[FILE_COUNT_MAX];
} FDir;

static FDir fDir;

enum FileErrorCodes {
    FILE_ERR_NOT_FOUND,
    FILE_ERR_TOO_MANY_FILES,
    FILE_ERR_TOO_LARGE,
    FILE_ERR_OUT_OF_MEMORY,
    FILE_ERR_WRITE_LOCKED,
    FILE_ERR_MOVE_LOCKED,
    FILE_ERR_NAME_LOCKED,
    FILE_ERR_IS_DELETED,
    FILE_ERR_ALREADY_EXISTS,
    FILE_ERR_CORRUPT,

};

enum FileSystemErrorCodes {
    FILE_SYSTEM_NONE_PRESENT,
    FILE_SYSTEM_ALREADY_PRESENT,
};

enum FileTypes {
    FILE_TYPE_DATA,
    FILE_TYPE_EXECUTABLE,
    FILE_TYPE_OTHER
};

typedef struct FileContent {
    uint16_t size;
    uint8_t data[FILE_BUFFER_SIZE_MAX];
} FC;


/********************************************************* EEPROM functions */
static void EEPROM_write_byte(ui16 address, ui8 data);
static ui8 EEPROM_read_byte(ui16 address);
static void EEPROM_write_bytes(ui16 address, ui8 *data, uint8_t size);
ui8 * EEPROM_read_bytes(ui16 address, uint8_t size);

static void EEPROM_write_byte(ui16 address, ui8 data)
{
    //ui8 data;//will be assigned value from stream

    address &= 0x03FF;
    while (EECR & _BV(EEPE)); //wait for any pending write jobs to finish
    EECR = (0 << EEPM1) | (0 << EEPM0); //set programming mode 
    EEAR = address; //load the address register
    EEDR = data; //load the data register
    char cSREG;
    cSREG = SREG; /* store SREG value */
    /* disable interrupts during timed sequence */
    cli();
    EECR |= (1 << EEMPE); /* enable EEPROM write */
    EECR |= (1 << EEPE); //start write
    SREG = cSREG; /* restore SREG value (I-bit) */
    return;
}

static void EEPROM_write_bytes(ui16 address, ui8 *data, uint8_t size)
{
    uint8_t index = 0;
    while (index < size) EEPROM_write_byte(address++, data[index++]);
    return;
}

static ui8 EEPROM_read_byte(ui16 address)
{
    //ui16 address;//will be assigned value from stream

    /* Wait for completion of previous write */
    while (EECR & (1 << EEPE));
    /* Set up address register */
    EEAR = address;
    /* Start eeprom read by writing EERE */
    EECR |= (1 << EERE);
    /* Return data from Data Register */
    return EEDR;
}

void EEPROM_read_bytes(ui16 address, uint8_t * buffer, uint8_t size)
{
    uint8_t index = 0;
    while (index < size) buffer[index++] = EEPROM_read_byte(address++);
    return;
}



/******************************************************* File System functions*/

void update_FDir(void);
uint8_t get_fdi_index(uint8_t filename);
FDI fetch_fdi(uint8_t);
void write_fdi(FDI fdi, uint8_t index);


FDI _ff(uint8_t name); //file find
uint8_t _fw(uint8_t filename, uint8_t * data, uint16_t size); //write
void _fs(void); //seek
FC _fr(uint8_t filename); //read
void _fx(void); //execute
void _fc(void); //copy
void _fd(void); //delete
void _fn(void); //rename
void _fm(void); //move

//================================== Data structure serialization/deserialization

void update_FDir(void)
{
    uint8_t idx;
    for (idx = 0; idx < FILE_COUNT_MAX; idx++) {
        FDI fdi;
        fdi = fetch_fdi(idx);
        /*Note: the filename only occupies the lower 7 of the 8 bits so the 8th bit (MSB) is an "exists" flag*/
        uint8_t exists_filename = ((fdi.name_letter << 3) | fdi.name_number) | (fdi.file_exists << 7);
        fDir.filenames[idx] = exists_filename;
    }
}

uint8_t get_fdi_index(uint8_t filename)
{
    update_FDir();
    uint8_t idx;
    for (idx = 0; idx < FILE_COUNT_MAX; idx++) {
        FDI fdi;
        fdi = fetch_fdi(idx);
        if (((fdi.name_letter << 3) | fdi.name_number) == filename) return idx;
    }
    return 0xFF;
}

FDI fetch_fdi(uint8_t index)
{
    FDI f;
    uint8_t arr[sizeof (f)] = {0x00};
    EEPROM_read_bytes(FILE_DESCR_LIST_START_ADDR + (index * sizeof (f)), arr, sizeof (f));
    memcpy(& f, & arr, sizeof (f));
    return f;
}

void write_fdi(FDI fdi, uint8_t index)
{
    uint8_t structbuf[sizeof (fdi)];
    memcpy(&structbuf, &fdi, sizeof (fdi));
    EEPROM_write_bytes(FILE_DESCR_LIST_START_ADDR, structbuf, FILE_DESCRIPTOR_INODE_SIZE);
}

FDI create_fdi(uint8_t filename, uint16_t size, uint8_t type)
{
    FDI fdi;
    fdi.name_lock = 0;
    fdi.move_lock = 0;
    fdi.file_address_LOWER = 0xFF & FILE_SECTOR_START_ADDR;
    fdi.file_address_UPPER = 0x03 & (FILE_SECTOR_START_ADDR >> 8);
    fdi.file_exists = 1;
    fdi.name_letter = (0x78 & filename) >> 3;
    fdi.name_number = (0x07 & filename) >> 0;
    fdi.size_LOWER = (0x003F & size) & 0xFF; //6 bits
    fdi.size_UPPER = ((0x1C0 & size) >> 6) & 0xFF; //3 bits
    fdi.type = 0;
    fdi.write_lock = 0;
    return fdi;
}

//================================== File Operations

FDI _ff(uint8_t filename)
{
    FDI fdi;
    uint8_t index;
    for (index = 0; index < FILE_COUNT_MAX; index++) {
        fdi = fetch_fdi(index);
        if (((fdi.name_letter << 3) | fdi.name_number) == filename) return fdi;
    }
    return fdi;

}

uint8_t _fw(uint8_t filename, uint8_t * data, uint16_t size)
{
    if (size >= FILE_BUFFER_SIZE_MAX) return FILE_ERR_TOO_LARGE;
    //---------------------create the fdi
    FDI fdi;
    fdi = _ff(filename);
    if ((fdi.size_LOWER | fdi.size_UPPER) > 0) return FILE_ERR_ALREADY_EXISTS;
    else {
        fdi = create_fdi(filename, size, FILE_TYPE_DATA);
        uint8_t file_directory_index = get_fdi_index(filename);
        if (file_directory_index == 0) file_directory_index = 1;
        write_fdi(fdi, file_directory_index);
        //----write data
        uint16_t file_address = fdi.file_address_LOWER | ((0x0FF & fdi.file_address_UPPER) << 8);
        EEPROM_write_bytes(file_address, data, 0xFF & size);
        return 0;
    }
}

FC _fr(uint8_t filename)
{
    FDI fdi;
    FC retfc;
    fdi = _ff(filename);
    if ((fdi.size_LOWER | fdi.size_UPPER) > 0) {
        uint16_t size = 0x1FF & (fdi.size_LOWER | (fdi.size_UPPER << 6));
        retfc.size = size;
        uint16_t addr = 0x3FF & (fdi.file_address_LOWER | (fdi.file_address_UPPER << 8));
        EEPROM_read_bytes(addr, retfc.data, size);
    }
    return retfc;
}


//=============================================== Defragmentation of filesystem



//=============================================== Installation
uint8_t install_virOSFS(void);

uint8_t install_virOSFS(void)
{
    //if the first byte of EEPROM memory is FFh then it is safe
    //to assume no FS exists, but if not, one possibly already does
    if (EEPROM_read_byte(FILE_SYSTEM_VERSION_NUMBER_ADDR) != 0xFF) return FILE_SYSTEM_ALREADY_PRESENT;
    //start installation by writing the FS version number to the first address
    EEPROM_write_byte(FILE_SYSTEM_VERSION_NUMBER_ADDR, FILE_SYSTEM_VERSION_NUMBER);
    //create empty FDIs
    uint16_t addr = FILE_DESCR_LIST_START_ADDR;
    uint8_t i;
    for (i = 0; i < FILE_COUNT_MAX; i++) {
        FDI fdi;
        fdi.name_lock = 0;
        fdi.move_lock = 0;
        uint16_t faddr = FILE_SECTOR_START_ADDR; //0x123;//0x1AB;//
        uint16_t faddrL = 0xFF & faddr;
        uint16_t faddrH = 0x03 & (faddr >> 8);
        fdi.file_address_LOWER = faddrL; //FILE_SECTOR_START_ADDR;//
        fdi.file_address_UPPER = faddrH; //FILE_SECTOR_START_ADDR;//
        fdi.file_exists = 0;
        fdi.name_letter = 0b0;
        fdi.name_number = 0b0;
        fdi.size_LOWER = 0x00;
        fdi.size_UPPER = 0x00;
        fdi.type = 0;
        fdi.write_lock = 0;

        uint8_t structbuf[sizeof (fdi)];
        memcpy(&structbuf, &fdi, sizeof (fdi));
        EEPROM_write_bytes(addr + (FILE_DESCRIPTOR_INODE_SIZE * i), structbuf, FILE_DESCRIPTOR_INODE_SIZE);

    }
    //update the file directory
    update_FDir();
    return 0;

}


#endif /* VIROSFS_H */

