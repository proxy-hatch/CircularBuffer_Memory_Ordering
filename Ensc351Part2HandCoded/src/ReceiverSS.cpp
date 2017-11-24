// Solution to ENSC 351 Project Part 2.  2017  Prepared by:
//      Copyright (C) 2017 Craig Scratchley, Simon Fraser University

#include <iostream>
#include <stdlib.h> // for EXIT_FAILURE

#include "ReceiverSS.h"
#include "ReceiverX.h"

namespace Receiver_SS
{
using namespace std;

#define FinalState 99

enum {InitialState,
	STATE_FIRSTBYTE,
	STATE_EOT,
	STATE_CAN,
};

//State Mgr
//--------------------------------------------------------------------
ReceiverSS::ReceiverSS(ReceiverX* receiverCtx, bool startMachine/*=true*/)
: StateMgr("ReceiverSS")
{
	myCtx = receiverCtx;	
	
	ReceiverX& ctx = *myCtx;
	
	//Entry code for TopLevel State
	ctx.NCGbyte = ctx.Crcflg ? 'C' : NAK;
//	if (ctx.Crcflg) ctx.NCGbyte='C';
//	else ctx.NCGbyte=NAK;
	ctx.sendByte(ctx.NCGbyte);
	ctx.errCnt=0;
	
	state = STATE_FIRSTBYTE;
//	state = InitialState;
//	postEvent(CONT);
}

/***************************************************************************/
bool ReceiverSS::isRunning() const
{
	return (state != FinalState);
}

void ReceiverSS::postEvent(unsigned int event, int /*wParam*/ c, int lParam) throw (std::string)
{
	ReceiverX& ctx = *myCtx;
	switch(state) {
//		case InitialState:
//			// event is CONT
//			state = STATE_FIRSTBYTE;
//			return;
		case STATE_FIRSTBYTE:
			// for Assignment 2, (event == SER) always 
			if (c == SOH){ // use case statement?
				ctx.getRestBlk();
				if (ctx.goodBlk1st)
					ctx.errCnt=0;
				else
					ctx.errCnt++;
				// state = STATE_CONDITIONAL_TRANSIENT;
				// conditional transient state "ConditionalTransient" appears here
				if (!ctx.syncLoss && ctx.errCnt < errB) {
					if (ctx.goodBlk)
						ctx.sendByte(ACK);
					else
						ctx.sendByte(NAK);
					if (ctx.goodBlk1st)
						ctx.writeChunk();
					// state = STATE_FIRSTBYTE;
				}
				else {
					ctx.can8();
					if (ctx.syncLoss)
						ctx.result = "LossOfSynchronization";
					else
						ctx.result = "ExcessiveErrors";

					state = FinalState;
				}
			}
			else if (c == EOT){
				ctx.sendByte(NAK);
				state = STATE_EOT;
			}
			else if (c == CAN)
				state = STATE_CAN;
			else
				break;
			return;

		case STATE_EOT:	
			// for Assignment 2, (event == SER) always 
			if (c == EOT) {
				ctx.sendByte(ACK);
				ctx.result="Done";
				state = FinalState;
				return;
			}
			break;

		case STATE_CAN:
			// for Assignment 2, (event == SER) always
			if (c == CAN) {
				ctx.result="SndCancelled";
				//ctx.clearCan();
				state = FinalState;
				return;
			}
			break;

		case FinalState:
			cerr << "Event should not be posted when receiver in final state" << endl;
			exit(EXIT_FAILURE);
		default:
			cerr << "Receiver in invalid state!" << endl;
			exit(EXIT_FAILURE);
	} // switch

	cerr << "In state " << state << " receiver received totally unexpected char #" << c << ": " << (char) c << endl;
	exit(EXIT_FAILURE);
}
} //namespace Receiver_SS
