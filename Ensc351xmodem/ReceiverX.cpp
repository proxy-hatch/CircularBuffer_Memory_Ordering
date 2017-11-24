//============================================================================
//
//% Student Name 1: student1
//% Student 1 #: 123456781
//% Student 1 userid (email): stu1 (stu1@sfu.ca)
//
//% Student Name 2: student2
//% Student 2 #: 123456782
//% Student 2 userid (email): stu2 (stu2@sfu.ca)
//
//% Below, edit to list any people who helped you with the code in this file,
//%      or put 'None' if nobody helped (the two of) you.
//
// Helpers: _everybody helped us/me with the assignment (list names or put 'None')__
//
// Also, list any resources beyond the course textbooks and the course pages on Piazza
// that you used in making your submission.
//
// Resources:  ___________
//
//%% Instructions:
//% * Put your name(s), student number(s), userid(s) in the above section.
//% * Also enter the above information in other files to submit.
//% * Edit the "Helpers" line and, if necessary, the "Resources" line.
//% * Your group name should be "P2_<userid1>_<userid2>" (eg. P1_stu1_stu2)
//% * Form groups as described at:  https://courses.cs.sfu.ca/docs/students
//% * Submit files to courses.cs.sfu.ca
//
// File Name   : ReceiverX.cpp
// Version     : September 3rd, 2017
// Description : Starting point for ENSC 351 Project Part 2
// Original portions Copyright (c) 2017 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include <string.h> // for memset()
#include <fcntl.h>
#include <stdint.h>
#include "myIO.h"
#include "ReceiverX.h"
#include "ReceiverSS.h"
#include "VNPE.h"
#include "AtomicCOUT.h"

using namespace std;
using namespace Receiver_SS;

ReceiverX::
ReceiverX(int d, const char *fname, bool useCrc)
:PeerX(d, fname, useCrc), 
NCGbyte(useCrc ? 'C' : NAK),
goodBlk(false), 
goodBlk1st(false), 
syncLoss(true),
numLastGoodBlk(0),
firstBlock(true)
{
}

void ReceiverX::receiveFile()
{
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	// should we test for error and set result to "OpenError" if necessary?
	transferringFileD = PE2(myCreat(fileName, mode), fileName);
	//transferringFileD = PE2(creat(fileName, mode), fileName);

	ReceiverSS *myReceiverSM = new ReceiverSS(this);
	myReceiverSM->setDebugLog(NULL);

	char byteToReceive;
	while(myReceiverSM->isRunning()) {
		PE(myRead(mediumD, &byteToReceive, 1));
		myReceiverSM->postEvent(SER, byteToReceive);
	}

	COUT << "\n"; // insert new line.
	PE(close(transferringFileD));
}

// use an enumeration to handle result of block: ?
// goodBlk1st, syncLoss, goodBlkRpt, badblock

/* Only called after an SOH character has been received.
The function tries
to receive the remaining characters to form a complete
block.  The member
variable goodBlk1st will be made true if this is the first
time that the block was received in "good" condition.
*/
void ReceiverX::getRestBlk()
{
	//PE_NOT(myRead(mediumD, &rcvBlk[1], REST_BLK_SZ), REST_BLK_SZ);
	const int restBlkSz = Crcflg ? REST_BLK_SZ_CRC : REST_BLK_SZ_CS;
	PE_NOT(myReadcond(mediumD, &rcvBlk[1], restBlkSz, restBlkSz, 0, 0), restBlkSz);
	// consider only calculating local checksum if block and complement are okay.
	// consider receiving checksum after calculating local checksum

	//(blkNumsOk) = ( block # and its complement are matched );
	bool blkNumsOk = (rcvBlk[2] == (255 - rcvBlk[1]));
	if (!blkNumsOk) {
		goodBlk = goodBlk1st = syncLoss = false;
	}
	else {
		bool newExpectedBlk = (rcvBlk[1] == (uint8_t) (numLastGoodBlk + 1)); // ++numLastGoodBlk
		if (!newExpectedBlk) {
			goodBlk1st = false; // use in place of newExpectedBlk?
			// determine fatal loss of synchronization
			if (firstBlock || (rcvBlk[1] != numLastGoodBlk)) {
				syncLoss = true;
				goodBlk = false;
				COUT << "(s" << (unsigned) rcvBlk[1] << ")" << flush;
			}
			else {
				syncLoss = false;
				goodBlk = true; // deemed good block
				COUT << "(d" << (unsigned) rcvBlk[1] << ")" << flush;
			}
		}
		else {
			// detect data error in chunk
			// consider receiving checksum after calculating local checksum
			syncLoss = false;
			if (Crcflg) {
				uint16_t CRCbytes;
				crc16ns(&CRCbytes, &rcvBlk[DATA_POS]);
				goodBlk = goodBlk1st = (*((uint16_t*) &rcvBlk[PAST_CHUNK]) == CRCbytes);
			}
			else {
				uint8_t sum;
				checksum(&sum, rcvBlk);
				goodBlk = goodBlk1st = (rcvBlk[PAST_CHUNK] == sum);
			}
			if (goodBlk1st) {
				numLastGoodBlk = rcvBlk[1];
				firstBlock = false;
				COUT << "(f" << (unsigned) rcvBlk[1] << ")" << endl;
			}
			else
				COUT << "(b" << (unsigned) rcvBlk[1]<< ":" << (unsigned) numLastGoodBlk << ")" << flush;
		}
	}
}

//Write chunk (data) in a received block to disk
void ReceiverX::writeChunk()
{
	PE_NOT(write(transferringFileD, &rcvBlk[DATA_POS], CHUNK_SZ), CHUNK_SZ);
}

//Send 8 CAN characters in a row to the XMODEM sender, to inform it of
//	the cancelling of a file transfer
void ReceiverX::can8()
{
	// no need to space CAN chars coming from receiver
	const int canLen=8; // move to PeerX.h
    char buffer[canLen];
    memset( buffer, CAN, canLen);
    PE_NOT(myWrite(mediumD, buffer, canLen), canLen);
}
