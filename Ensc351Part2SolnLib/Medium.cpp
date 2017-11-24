/*
 * Medium.cpp
 *
 *      Author: Craig Scratchley
 *      Copyright(c) 2017 Craig Scratchley
 */

#include <fcntl.h>
#include <unistd.h> // for write()
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/select.h>
#include "Medium.h"
#include "myIO.h"
#include "VNPE.h"
#include "AtomicCOUT.h"

#include "PeerX.h"

// Uncomment the line below to turn on debugging output from the medium
#define REPORT_INFO

//#define SEND_EXTRA_ACKS

//This is the kind medium.

using namespace std;

Medium::Medium(int d1, int d2, const char *fname)
:Term1D(d1), Term2D(d2), logFileName(fname)
{
	byteCount = 0;
	ACKforwarded = 0;
	ACKreceived = 0;
	sendExtraAck = false;

	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	logFileD = PE2(myCreat(logFileName, mode), logFileName);
	//logFileD = PE2(creat(logFileName, mode), logFileName);
}

Medium::~Medium() {}

bool Medium::MsgFromTerm2()
{
	blkT bytesReceived;
	int numOfByteReceived;
	int byteToCorrupt;

	// could read in bytes one-by-one if desired
//	if (!(numOfByteReceived = PE(myRead(Term2D, bytesReceived, 1))))
	if (!(numOfByteReceived = PE(myRead(Term2D, bytesReceived, (BLK_SZ_CRC+1) / 2)))) {
		COUT << "Medium thread: TERM2's socket closed, Medium terminating" << endl;
		return true;
	}

	byteCount += numOfByteReceived;
	if (byteCount >= 392) {
		byteCount = byteCount - 392;
		byteToCorrupt = numOfByteReceived - byteCount - 1;
		if (byteToCorrupt) {
			bytesReceived[byteToCorrupt] = (255 - bytesReceived[byteToCorrupt]);
#ifdef REPORT_INFO
			COUT << "<" << byteToCorrupt << "x>" << flush;
#endif
		}
	}

	if (sendExtraAck) {
#ifdef REPORT_INFO
			COUT << "{" << "+A" << "}" << flush;
#endif
		uint8_t buffer = ACK;
		PE_NOT(write(logFileD, &buffer, 1), 1);
		//Forward the buffer to term2,
		PE_NOT(myWrite(Term2D, &buffer, 1), 1);

		sendExtraAck = false;
	}

	PE_NOT(write(logFileD, bytesReceived, numOfByteReceived), numOfByteReceived);
	//Forward the bytes to RECEIVER,
	PE_NOT(myWrite(Term1D, bytesReceived, numOfByteReceived), numOfByteReceived);
	PE(myTcdrain(Term1D)); // wait for bytes to drain into receiverX
	// The medium should work with the above one line commented out too.

	return false;
}

bool Medium::MsgFromTerm1()
{
	uint8_t buffer[CAN_LEN];
	int numOfByte = PE(myRead(Term1D, buffer, CAN_LEN));
	if (numOfByte == 0) {
		COUT << "Medium thread: TERM1's socket closed, Medium terminating" << endl;
		return true;
	}

	/*note that we record the errors in the log file*/
	if(buffer[0]==ACK)
	{
		ACKreceived++;

		if((ACKreceived%10)==0)
		{
			ACKreceived = 0;
			buffer[0]=NAK;
#ifdef REPORT_INFO
			COUT << "{" << "AxN" << "}" << flush;
#endif
		}
#ifdef SEND_EXTRA_ACKS
		else/*actually forwarded ACKs*/
		{
			ACKforwarded++;

			if((ACKforwarded%7)==0)/*Note that this extra ACK is not an ACK forwarded from receiver to the sender, so we don't increment ACKforwarded*/
			{
				ACKforwarded = 0;
				sendExtraAck = true;
			}
		}
#endif
	}

	PE_NOT(write(logFileD, buffer, numOfByte), numOfByte);

	//Forward the buffer to term2,
	PE_NOT(myWrite(Term2D, buffer, numOfByte), numOfByte);
	return false;
}

void Medium::run()
{
	fd_set cset;
	FD_ZERO(&cset);

	bool finished=false;
	while(!finished) {

		//note that the file descriptor bitmap must be reset every time
		FD_SET(Term1D, &cset);
		FD_SET(Term2D, &cset);

		int rv = PE(select( max(Term2D,
							Term1D)+1, &cset, NULL, NULL, NULL ));
		if( rv == 0 ) {
			// timeout occurred
			CERR << "The medium should not timeout" << endl;
			exit (EXIT_FAILURE);
		} else {
			if( FD_ISSET( Term1D, &cset ) ) {
				finished = MsgFromTerm1(); //Term1D,Term2D);
			}
			if( FD_ISSET( Term2D, &cset ) ) {
				finished |= MsgFromTerm2(); //Term1D,Term2D);
			}
		}
	};
	PE(close(logFileD));
	PE(myClose(Term1D));
	PE(myClose(Term2D));
}
