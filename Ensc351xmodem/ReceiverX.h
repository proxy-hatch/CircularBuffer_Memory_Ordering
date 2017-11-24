#ifndef RECEIVER_H
#define RECEIVER_H

#include "PeerX.h"

class ReceiverX : public PeerX
{
public:
	ReceiverX(int d, const char *fname, bool useCrc=true);

	void receiveFile();
	void getRestBlk();	// get the remaining bytes (131 or 132) of a block
	void writeChunk();
	void can8();		// send 8 CAN characters

	uint8_t NCGbyte;	// Either a 'C' or a NAK sent by receiver to initiate the file transfer

	/*A Boolean variable that indicates whether or not the
	 * block just received should be ACKed or NAKed.*/
	bool goodBlk;

	/*A Boolean variable that indicates that a good copy of a block
	 * being sent has been received for the first time.  It is an
	 * indication that the data in the block can be written to disk.
	 */
	bool goodBlk1st;

	/*A Boolean variable that indicates whether or not a fatal loss
of synchronization has been detected.*/
	bool syncLoss;

	/* A variable which counts the number of responses in a
row sent because of problems like communication
problems. An initial NAK (or 'C') does not add to the count. The reception
of a particular block in good condition for the first time resets the count. */
//	unsigned errCnt;	// moved to PeerX.h

private:
	// blkT rcvBlk;		// a received block
	uint8_t rcvBlk[BLK_SZ_CRC];		// a received block

	uint8_t numLastGoodBlk; // the number of the last good block
	bool firstBlock;  // are we trying to receive the very first block?
};

#endif
