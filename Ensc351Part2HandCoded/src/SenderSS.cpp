// Solution to ENSC 351 Project Part 2.  2017  Prepared by:
//      Copyright (C) 2017 Craig Scratchley, Simon Fraser University

#include <iostream>
#include <fstream>
#include <stdlib.h> // for EXIT_FAILURE

#include "SenderSS.h"
#include "SenderX.h"

#define c wParam

#define FinalState 99

namespace Sender_SS
{
using namespace std;

enum {InitialState,
	STATE_START,
	STATE_ACKNAK,
	STATE_EOT1,
	STATE_EOTEOT,
	STATE_CAN,
};

//State Mgr
//--------------------------------------------------------------------
SenderSS::SenderSS(SenderX* senderCtx, bool startMachine/*=true*/)
: StateMgr("SenderSS")
{
	myCtx = senderCtx;
	
	SenderX& ctx = *myCtx;
	
	//Entry code for TopLevel State
	ctx.Crcflg=true; ctx.prep1stBlk(); ctx.errCnt=0;
	ctx.firstCrcBlk=true;
	
	state = STATE_START;
//	state = InitialState;
//	postEvent(CONT);//,0,0);
}

/***************************************************************************/
bool SenderSS::isRunning() const
{
	return (state != FinalState);
}

void SenderSS::postEvent(unsigned int event, int /*wParam*/ c, int lParam) throw (std::string)
{
	SenderX& ctx = *myCtx;
	switch(state) {
	/*
		case InitialState:
			// event is CONT
			state = STATE_START;
			return;
		*/
		case STATE_START:
			// for Assignment 2, (event == SER) always 
			if ((c == NAK) || (c == 'C')) {
				if (!ctx.bytesRd) {
					if (c==NAK) {ctx.firstCrcBlk=false;}
					ctx.sendByte(EOT);
					state = STATE_EOT1;		// at end of file being transferred
				}
				else {
					if (c==NAK) {
						ctx.Crcflg=false;
						ctx.cs1stBlk();
						ctx.firstCrcBlk=false;
					}
					ctx.sendBlkPrepNext();
					state = STATE_ACKNAK;
				}
				return;
			}
			break;
			
		case STATE_ACKNAK:
			// for Assignment 2, (event == SER) always 
			if (c == ACK) {
				if (!ctx.bytesRd) {
					ctx.sendByte(EOT);
					ctx.errCnt=0;
					ctx.firstCrcBlk=false;
					state = STATE_EOT1;		// at end of file		
				}
				else {
					ctx.sendBlkPrepNext();
					ctx.errCnt = 0;
					ctx.firstCrcBlk=false;
				} 
			}
			else if (c == NAK) {
				if (ctx.errCnt < (errB)) { // (errB - 1) ??
					ctx.resendBlk();
					ctx.errCnt++;
				}
				else {
					ctx.can8();
					ctx.result="ExcessiveNAKs";
					state = FinalState;
				}
			}
			else if (c == CAN)
				// this is a mystery.  The other 7 CANs are going missing. **
				state = STATE_CAN;
			else
				break;
			return;
			
		case STATE_EOT1: 
			// for Assignment 2, (event == SER) always 
			if (c == NAK){
				ctx.sendByte(EOT);
				state = STATE_EOTEOT;
			}
			else if (c == ACK) {
				ctx.result="1st EOT ACK'd";
				state = FinalState;
			}
			else
				break;
			return;
			
		case STATE_EOTEOT: 
			// for Assignment 2, (event == SER) always 
			if (c == ACK) {
				ctx.result="Done";
				state = FinalState;
				return;
			};
			break;
		
		case STATE_CAN:
			// for Assignment 2, (event == SER) always 
			if (c == CAN) {
				ctx.result="RcvCancelled";
				//ctx.clearCan();
				state = FinalState;
				return;
			}
			break;

		case FinalState:
			cerr << "Event should not be posted when sender in final state" << endl;
			exit(EXIT_FAILURE);
							
		default:
			cerr << "Sender in invalid state!" << endl;
			exit(EXIT_FAILURE);
	}
	
	cerr << "In state " << state << " sender received totally unexpected char #" << c << ": " << (char) c << endl;
	exit(EXIT_FAILURE);
}
		
} // namespace Sender_SS
