//DSK lib
/* DSK functions and DSK data structures from Caprice32 - Amstrad CPC Emulator
   (c) Copyright 1997-2004 Ulrich Doewich*/
/*
Rest of things, by KaosOverride 2013
*/



#ifndef _MAX_PATH
 #ifdef _POSIX_PATH_MAX
 #define _MAX_PATH _POSIX_PATH_MAX
 #else
 #define _MAX_PATH 256
 #endif
#endif

typedef unsigned char byte; // 1byte
typedef unsigned short word; // 2bytes
typedef unsigned long dword; //4bytes



//ERROR CODES

#define ERR_FILE_NOT_FOUND       1
#define ERR_DSK_INVALID          2
#define ERR_DSK_SIDES            3
#define ERR_DSK_SECTORS          4
#define ERR_OUT_OF_MEMORY        5


// FDC constants

#define IBM_TRACKMAX    80   //
#define IBM_SIDEMAX     2
#define IBM_SECTORMAX   9    //



#define DSK_BPTMAX      8192
#define DSK_TRACKMAX    102   // max amount that fits in a DSK header
#define DSK_SIDEMAX     2
#define DSK_SECTORMAX   29    // max amount that fits in a track header

#define MAX_DISK_FORMAT 8
#define DEFAULT_DISK_FORMAT 0
#define FIRST_CUSTOM_DISK_FORMAT 2
/*t_disk_format disk_format[MAX_DISK_FORMAT] = {
   { "178K Data Format", 40, 1, 9, 2, 0x52, 0xe5, {{ 0xc1, 0xc6, 0xc2, 0xc7, 0xc3, 0xc8, 0xc4, 0xc9, 0xc5 }} },
   { "169K Vendor Format", 40, 1, 9, 2, 0x52, 0xe5, {{ 0x41, 0x46, 0x42, 0x47, 0x43, 0x48, 0x44, 0x49, 0x45 }} }
   { "154K IBM Format", 40, 1, 8, 2, 0x52, 0xe5, {{ 0x01, 0x06, 0x02, 0x07, 0x03, 0x08, 0x04, 0x09, 0x05 }} }
};
*/

typedef struct {
   unsigned char CHRN[4]; // the CHRN for this sector
   unsigned char flags[4]; // ST1 and ST2 - reflects any possible error conditions
   unsigned int size; // sector size in bytes
   unsigned char *data; // pointer to sector data
} t_sector;

typedef struct {
   unsigned int sectors; // sector count for this track
   unsigned int size; // track size in bytes
   unsigned char *data; // pointer to track data
   t_sector sector[DSK_SECTORMAX]; // array of sector information structures
} t_track;

typedef struct {
   unsigned int tracks; // total number of tracks
//   unsigned int current_track; // location of drive head
   unsigned int sides; // total number of sides
//   unsigned int current_side; // side being accessed
//   unsigned int current_sector; // sector being accessed
   unsigned int altered; // has the image been modified?
//   unsigned int write_protected; // is the image write protected?
   unsigned int random_DEs; // sectors with Data Errors return random data?
//   unsigned int flipped; // reverse the side to access?
   t_track track[DSK_TRACKMAX][DSK_SIDEMAX]; // array of track information structures
} t_drive;


typedef struct {
    unsigned char *data;
    int order;
 } ibm_sector;

typedef struct {
    ibm_sector sector[IBM_SECTORMAX];
 } ibm_track;

typedef struct {
    ibm_track track[IBM_TRACKMAX+1][IBM_SIDEMAX];
 } ibm_drive;
