
/* DSK functions and DSK data structures from Caprice32 - Amstrad CPC Emulator
   (c) Copyright 1997-2004 Ulrich Doewich*/
/*
Rest of things, by KaosOverride 2013
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DSKlib.h"

FILE *pfileObject;
FILE *pfileOUT;

int IBM_currenttrack;
int IBM_currentsector;
int IBM_currenthead;

void IBM_nextsector()
{
    IBM_currentsector++;
if (IBM_currentsector>8)
    {
        IBM_currentsector=0;
        IBM_currenthead++;
        if (IBM_currenthead>8)
        {
            IBM_currenthead=0;        
            IBM_currenttrack++;
        }

    }

}

byte IBM_free_sector [512];

void IBM_clear (ibm_drive *drive)
{
    int track,sector,side;
    for (track=0;track<=IBM_TRACKMAX;track++)
        {
        for (sector=0;sector<=IBM_SECTORMAX;sector++)
            {
            for (side=0;side<IBM_SIDEMAX;side++)
                drive->track[track][side].sector[sector].data=IBM_free_sector;//DUMP SECTOR

            }
        }
}



//DSK lib


byte pbGPBuffer[128*8192]; // attempt to allocate the general purpose buffer

void dsk_clear (t_drive *drive)
{
   dword track, side;
   
   for (track = 0; track < DSK_TRACKMAX; track++) { // loop for all tracks
      for (side = 0; side < DSK_SIDEMAX; side++) { // loop for all sides
         if (drive->track[track][side].data) { // track is formatted?
            free(drive->track[track][side].data); // release memory allocated for this track
         }
      }
   }
   memset(drive, 0, sizeof(t_drive)); // clear drive info structure

}


byte dsk_whatformat(t_drive *drive)
{
    byte secID;
    secID=drive->track[0][0].sector[0].CHRN[2];
    secID = secID & 0xf0;
    return secID;
}


int dsk_load (char *pchFileName, t_drive *drive)
{
   int iRetCode;
   dword dwTrackSize, track, side, sector, dwSectorSize, dwSectors;
   byte *pbPtr, *pbDataPtr, *pbTempPtr, *pbTrackSizeTable;

   iRetCode = 0;
    dsk_clear(drive);

   if ((pfileObject = fopen(pchFileName, "rb")) != NULL) {
      fread(pbGPBuffer, 0x100, 1, pfileObject); // read DSK header
      pbPtr = pbGPBuffer;

      if (memcmp(pbPtr, "MV - CPC", 8) == 0) { // normal DSK image?
         drive->tracks = *(pbPtr + 0x30); // grab number of tracks
         if (drive->tracks > DSK_TRACKMAX) { // compare against upper limit
            drive->tracks = DSK_TRACKMAX; // limit to maximum
         }
         drive->sides = *(pbPtr + 0x31); // grab number of sides
         if (drive->sides > DSK_SIDEMAX) { // abort if more than maximum
            iRetCode = ERR_DSK_SIDES;
            goto exit;
         }
         dwTrackSize = (*(pbPtr + 0x32) + (*(pbPtr + 0x33) << 8)) - 0x100; // determine track size in bytes, minus track header
         drive->sides--; // zero base number of sides
         for (track = 0; track < drive->tracks; track++) { // loop for all tracks
            for (side = 0; side <= drive->sides; side++) { // loop for all sides
               fread(pbGPBuffer+0x100, 0x100, 1, pfileObject); // read track header
               pbPtr = pbGPBuffer + 0x100;
               if (memcmp(pbPtr, "Track-Info", 10) != 0) { // abort if ID does not match
                  iRetCode = ERR_DSK_INVALID;
                  goto exit;
               }
               dwSectorSize = 0x80 << *(pbPtr + 0x14); // determine sector size in bytes
               dwSectors = *(pbPtr + 0x15); // grab number of sectors
               if (dwSectors > DSK_SECTORMAX) { // abort if sector count greater than maximum
                  iRetCode = ERR_DSK_SECTORS;
                  goto exit;
               }
               drive->track[track][side].sectors = dwSectors; // store sector count
               drive->track[track][side].size = dwTrackSize; // store track size
               drive->track[track][side].data = (byte *)malloc(dwTrackSize); // attempt to allocate the required memory
               if (drive->track[track][side].data == NULL) { // abort if not enough
                  iRetCode = ERR_OUT_OF_MEMORY;
                  goto exit;
               }
               pbDataPtr = drive->track[track][side].data; // pointer to start of memory buffer
               pbTempPtr = pbDataPtr; // keep a pointer to the beginning of the buffer for the current track
               for (sector = 0; sector < dwSectors; sector++) { // loop for all sectors
                  memcpy(drive->track[track][side].sector[sector].CHRN, (pbPtr + 0x18), 4); // copy CHRN
                  memcpy(drive->track[track][side].sector[sector].flags, (pbPtr + 0x1c), 2); // copy ST1 & ST2
                  drive->track[track][side].sector[sector].size = dwSectorSize;
                  drive->track[track][side].sector[sector].data = pbDataPtr; // store pointer to sector data
                  pbDataPtr += dwSectorSize;
                  pbPtr += 8;
               }
               if (!fread(pbTempPtr, dwTrackSize, 1, pfileObject)) { // read entire track data in one go
                  iRetCode = ERR_DSK_INVALID;
                  goto exit;
               }
            }
         }
         drive->altered = 0; // disk is as yet unmodified
      } else {
         if (memcmp(pbPtr, "EXTENDED", 8) == 0) { // extended DSK image?
            drive->tracks = *(pbPtr + 0x30); // number of tracks
            if (drive->tracks > DSK_TRACKMAX) {  // limit to maximum possible
               drive->tracks = DSK_TRACKMAX;
            }
            drive->random_DEs = *(pbPtr + 0x31) & 0x80; // simulate random Data Errors?
            drive->sides = *(pbPtr + 0x31) & 3; // number of sides
            if (drive->sides > DSK_SIDEMAX) { // abort if more than maximum
               iRetCode = ERR_DSK_SIDES;
               goto exit;
            }
            pbTrackSizeTable = pbPtr + 0x34; // pointer to track size table in DSK header
            drive->sides--; // zero base number of sides
            for (track = 0; track < drive->tracks; track++) { // loop for all tracks
               for (side = 0; side <= drive->sides; side++) { // loop for all sides
                  dwTrackSize = (*pbTrackSizeTable++ << 8); // track size in bytes
                  if (dwTrackSize != 0) { // only process if track contains data
                     dwTrackSize -= 0x100; // compensate for track header
                     fread(pbGPBuffer+0x100, 0x100, 1, pfileObject); // read track header
                     pbPtr = pbGPBuffer + 0x100;
                     if (memcmp(pbPtr, "Track-Info", 10) != 0) { // valid track header?
                        iRetCode = ERR_DSK_INVALID;
                        goto exit;
                     }
                     dwSectors = *(pbPtr + 0x15); // number of sectors for this track
                     if (dwSectors > DSK_SECTORMAX) { // abort if sector count greater than maximum
                        iRetCode = ERR_DSK_SECTORS;
                        goto exit;
                     }
                     drive->track[track][side].sectors = dwSectors; // store sector count
                     drive->track[track][side].size = dwTrackSize; // store track size
                     drive->track[track][side].data = (byte *)malloc(dwTrackSize); // attempt to allocate the required memory
                     if (drive->track[track][side].data == NULL) { // abort if not enough
                        iRetCode = ERR_OUT_OF_MEMORY;
                        goto exit;
                     }
                     pbDataPtr = drive->track[track][side].data; // pointer to start of memory buffer
                     pbTempPtr = pbDataPtr; // keep a pointer to the beginning of the buffer for the current track
                     for (sector = 0; sector < dwSectors; sector++) { // loop for all sectors
                        memcpy(drive->track[track][side].sector[sector].CHRN, (pbPtr + 0x18), 4); // copy CHRN
                        memcpy(drive->track[track][side].sector[sector].flags, (pbPtr + 0x1c), 2); // copy ST1 & ST2
                        dwSectorSize = *(pbPtr + 0x1e) + (*(pbPtr + 0x1f) << 8); // sector size in bytes
                        drive->track[track][side].sector[sector].size = dwSectorSize;
                        drive->track[track][side].sector[sector].data = pbDataPtr; // store pointer to sector data
                        pbDataPtr += dwSectorSize;
                        pbPtr += 8;
                     }
                     if (!fread(pbTempPtr, dwTrackSize, 1, pfileObject)) { // read entire track data in one go
                        iRetCode = ERR_DSK_INVALID;
                        goto exit;
                     }
                  } else {
                     memset(&drive->track[track][side], 0, sizeof(t_track)); // track not formatted
                  }
               }
            }
            drive->altered = 0; // disk is as yet unmodified
         } else {
            iRetCode = ERR_DSK_INVALID; // file could not be identified as a valid DSK
         }
      }

exit:
      fclose(pfileObject);
   } else {
      iRetCode = ERR_FILE_NOT_FOUND;
   }

   if (iRetCode != 0) { // on error, 'eject' disk from drive
      dsk_clear(drive);
   }
   return iRetCode;
}



//
/*
unsigned char DSK_TRK_buffer [9192] //MAX 8ks + 0x1000 sector headers
unsigned char IMA_TRK_buffer [16384] //MAX 16ks
*/



char *DSK_filename;
char filename[_MAX_PATH + 1];
char *IMA_filename=filename;

char *rename_to_IMA(char *dst, const char *filename) {
    size_t len = strlen(filename);
    memcpy(dst, filename, len);
    len = strlen(dst);

    dst[len - 3] = 'I';
    dst[len - 2] = 'M';
    dst[len - 1] = 'G';

    return dst;
}



int main(int argc,char *argv[])
{

int DSK_side=0;
int seektrack,seeksect,side,track, sector,dsk_initialtrack,DSKtracks;
byte secID;
int errorcode;
char *errortext;
t_drive DSK_image;
ibm_drive IMA_image;

 printf("DSKIMG V0.0001PC - generic DSK to 720ks IMG compatible conversor\n");
 printf("2018 by KaosOverride\n");


    if ( argc != 2 )

        printf( "usage: %s DSK_image.DSK", argv[0] );

    else
    {

    memset(&DSK_image, 0, sizeof(t_drive));
    DSK_filename=argv[1];

//IMA_image->[IBM_currenttrack][side].sector[IBM_currentsector]

    IBM_currentsector=0;
    IBM_currenttrack=0; //TYrack 0 no used in CPC
    IBM_currenthead=0;

    IBM_clear(&IMA_image);

    errorcode=dsk_load (DSK_filename, &DSK_image);

    switch(errorcode)
        {
            case 0:
                errortext="OK";
                break;
            case 1:
                errortext="FILE NOT FOUND";
                break;

            case 2:
                errortext="DSK INVALID";
                break;

            case 3:
                errortext="DSK INVALID SIDES";
                break;

            case 4:
                errortext="DSK INVALID SECTORS";
                break;
            default:
                errortext="UNKNOWN";

        }
        printf( "Opening %s DSK file: %s %d\n", IMA_filename, errortext, errorcode );


// TO-DO - Two side detected, ask for SIDE 1 or 2

// TO-DO - Check format are STANDARD. No tricks, illegal or protected tracks allowed
// TO-DO - Depending of format, starting seektrack is variable....

            printf( "DSK Geometry Tracks %d, Sectors %d\n", DSK_image.tracks,DSK_image.track[0][0].sectors);

            errorcode=0;
            secID=dsk_whatformat(&DSK_image);
            switch(secID)
            {
 /*               case 0xC0:
                        dsk_initialtrack=0;
                        break;
                case 0x40:
                        dsk_initialtrack=2;

                        break;
*/
                case 0:
                        dsk_initialtrack=0;
//                        dsk_initialtrack=1;
                        break;
                default:
                        errorcode=1;
            }
            if (errorcode)
            {
            printf( "\nERROR: Unidentified format %d ", errorcode);
             return 0;
            }
      
            DSKtracks=DSK_image.tracks;

    for (seektrack=dsk_initialtrack;seektrack<DSKtracks;seektrack++)
    {
      for (DSK_side=0;DSK_side<2;DSK_side++)
      {
            unsigned char CHRN;
            int orden[10];

            for (seeksect=0;seeksect<DSK_image.track[seektrack][DSK_side].sectors;seeksect++)
            {
                CHRN=DSK_image.track[seektrack][DSK_side].sector[seeksect].CHRN[2];
                CHRN = CHRN & 0x0f;
                orden[CHRN-1]=seeksect;
            }
            for (seeksect=0;seeksect<DSK_image.track[seektrack][DSK_side].sectors;seeksect++)
            {
                unsigned char  *tmppoint;
                tmppoint=DSK_image.track[seektrack][DSK_side].sector[orden[seeksect]].data;
                IMA_image.track[IBM_currenttrack][IBM_currenthead].sector[IBM_currentsector].data=tmppoint;
                IBM_nextsector();
            }
        }
    }

    rename_to_IMA(IMA_filename,DSK_filename);
        printf( "\nWriting %s file from %s\n", IMA_filename, DSK_filename );

    if ((pfileOUT = fopen(IMA_filename, "wb")) != NULL)
    {
        int sizefile=0;
    for (track=0;track<IBM_TRACKMAX;track++)
        {
         for (side=0;side<IBM_SIDEMAX;side++)
            {
                for (sector=0;sector<IBM_SECTORMAX;sector++)
              {
               fwrite (IMA_image.track[track][side].sector[sector].data,512,1,pfileOUT);//DUMP SECTOR
               sizefile=sizefile+512;
              }
             
            }
        }

            printf( "\nSize: %d:\n",sizefile);

    fclose(pfileOUT);
    }else
    {
        printf( "\nError creating %s\n", IMA_filename);

    }

}
 return 0;
}
