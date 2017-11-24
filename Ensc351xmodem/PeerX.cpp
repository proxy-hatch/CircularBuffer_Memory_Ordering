/*
 * PeerX.cpp
 *
 *      Author: wcs
 */

#include <arpa/inet.h>  // for htons() -- not available with MinGW
#include "PeerX.h"
#include "VNPE.h"
#include "myIO.h"

uint16_t my_htons(uint16_t n)
{
    unsigned char *np = (unsigned char *)&n;

    return
        ((uint16_t)np[0] << 8) |
        (uint16_t)np[1];
}

/* update CRC */
/*
The following XMODEM crc routine is taken from "rbsb.c".  Please refer to
    the source code for these programs (contained in RZSZ.ZOO) for usage.
As found in Chapter 8 of the document "ymodem.txt".
    Original 1/13/85 by John Byrns
    */

/*
 * Programmers may incorporate any or all code into their programs,
 * giving proper credit within the source. Publication of the
 * source routines is permitted so long as proper credit is given
 * to Stephen Satchell, Satchell Evaluations and Chuck Forsberg,
 * Omen Technology.
 */

unsigned short
updcrc(register int c, register unsigned crc)
{
	register int count;

	for (count=8; --count>=0;) {
		if (crc & 0x8000) {
			crc <<= 1;
			crc += (((c<<=1) & 0400)  !=  0);
			crc ^= 0x1021;
		}
		else {
			crc <<= 1;
			crc += (((c<<=1) & 0400)  !=  0);
		}
	}
	return crc;
}

// returns a crc16 in 'network byte order'
// derived from code in "rbsb.c" (see above)
// line comments in function below show lines removed from original code
void
crc16ns (uint16_t* crc16nsP, uint8_t* buf)
{
	 register int wcj;
	 register uint8_t *cp;
	 unsigned oldcrc=0;
	 for (wcj=CHUNK_SZ,cp=buf; --wcj>=0; ) {
		 //sendline(*cp);

		 /* note the octal number in the line below */
		 oldcrc=updcrc((0377& *cp++), oldcrc);

		 //checksum += *cp++;
	 }
	 //if (Crcflg) {
		 oldcrc=updcrc(0,updcrc(0,oldcrc));

		 *crc16nsP = my_htons((uint16_t) oldcrc); 
		 /* *crc16nsP = htons((uint16_t) oldcrc); */

		 //sendline((int)oldcrc>>8);
		 //sendline((int)oldcrc);
	 //}
	 //else
		 //sendline(checksum);
}

void
checksum(uint8_t* sumP, blkT blkBuf)
{
	*sumP = blkBuf[DATA_POS];
	for( int ii=DATA_POS + 1; ii<PAST_CHUNK; ii++ )
		*sumP += blkBuf[ii];
}

PeerX::
PeerX(int d, const char *fname, bool useCrc)
:result("ResultNotSet"),
 errCnt(0),
 Crcflg(useCrc),
 mediumD(d),
 fileName(fname),
 transferringFileD(-1)
{
}

//Send a byte to the remote peer across the medium
void
PeerX::
sendByte(uint8_t byte)
{
	PE_NOT(myWrite(mediumD, &byte, sizeof(byte)), sizeof(byte));
}

