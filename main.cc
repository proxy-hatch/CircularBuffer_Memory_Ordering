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
// Version     : November, 2017 -- #define for MEDIUM
// Copyright   : Copyright 2017, Craig Scratchley
// Description : ENSC 351 Project Part 2 Solution -- #define for MEDIUM
//============================================================================

//#define MEDIUM

#include <stdlib.h> // EXIT_SUCCESS
#include <sys/socket.h> // AF_LOCAL, SOCK_STREAM
#include <pthread.h>
#include <thread>
#include <chrono>         
#include "myIO.h"
#include "SenderX.h"
#include "ReceiverX.h"
#include "VNPE.h"
#include "AtomicCOUT.h"

#ifdef MEDIUM
#include "Medium.h"
#endif

using namespace std;

enum  {Term1, Term2};
enum  {TermSkt, MediumSkt};

#ifndef MEDIUM
static int daSktPr[2];	  //Socket Pair between term1 and term2
#else
static int daSktPrT1M[2];	  //Socket Pair between term1 and medium
static int daSktPrMT2[2];	  //Socket Pair between medium and term2
#endif

void termFunc(int termNum)
{
	if (termNum == Term1) {
		const char *receiverFileName = "transferredFile";
		COUT << "Will try to receive to file:  " << receiverFileName << endl;

#ifndef MEDIUM
		ReceiverX xReceiver(daSktPr[Term1], receiverFileName, true);
#else
		ReceiverX xReceiver(daSktPrT1M[TermSkt], receiverFileName, true);
#endif
		xReceiver.receiveFile();
		COUT << "xReceiver result was: " << xReceiver.result << endl;

	    std::this_thread::sleep_for (std::chrono::milliseconds(1));
	    COUT << endl;
	    std::this_thread::sleep_for (std::chrono::milliseconds(1));

	    COUT << "Will try to receive to file with Checksum:  " << receiverFileName << endl;

#ifndef MEDIUM
		ReceiverX xReceiverCS(daSktPr[Term1], receiverFileName, false);
#else
		ReceiverX xReceiverCS(daSktPrT1M[TermSkt], receiverFileName, false);
#endif
		xReceiverCS.receiveFile();
		COUT << "xReceiver result was: " << xReceiverCS.result << endl;

#ifdef MEDIUM
	    std::this_thread::sleep_for (std::chrono::milliseconds(1));
		PE(myClose(daSktPrT1M[TermSkt]));
#endif
	}
	else {
		PE_0(pthread_setname_np(pthread_self(), "T2")); // give the thread (terminal 2) a name

		const char *senderFileName = "/etc/mailcap"; // for ubuntu target
		// const char *senderFileName = "/etc/printers/epijs.cfg"; // for QNX 6.5 target
		// const char *senderFileName = "/etc/system/sapphire/PasswordManager.tr"; // for BB Playbook target
		COUT << "Will try to send the file:  " << senderFileName << endl;

#ifndef MEDIUM
		SenderX xSender(senderFileName, daSktPr[Term2]);
#else
		SenderX xSender(senderFileName, daSktPrMT2[TermSkt]);
#endif

		xSender.sendFile();
		COUT << "xSender result was: " << xSender.result << endl;

	    std::this_thread::sleep_for (std::chrono::milliseconds(1));
	    COUT << endl;
	    std::this_thread::sleep_for (std::chrono::milliseconds(1));

		COUT << "Will try to send the file:  " << senderFileName << endl;
#ifndef MEDIUM
		SenderX xSender2(senderFileName, daSktPr[Term2]);
#else
		SenderX xSender2(senderFileName, daSktPrMT2[TermSkt]);
#endif
		xSender2.sendFile();
		COUT << "xSender result was: " << xSender2.result << endl;

#ifdef MEDIUM
	    std::this_thread::sleep_for (std::chrono::milliseconds(1));
		PE(myClose(daSktPrMT2[TermSkt]));
#endif
	}
#ifndef MEDIUM
    std::this_thread::sleep_for (std::chrono::milliseconds(1));
	PE(myClose(daSktPr[termNum]));
#endif
}

#ifdef MEDIUM
// ***** you will need this at some point *****
void mediumFunc(void)
{
	PE_0(pthread_setname_np(pthread_self(), "M")); // give the thread (medium) a name
	Medium medium(daSktPrT1M[MediumSkt],daSktPrMT2[MediumSkt], "xmodemData.dat");
	medium.run();
}
#endif

int main()
{
	PE_0(pthread_setname_np(pthread_self(), "P-T1")); // give the primary thread (terminal 1) a name


//daSktPr[Term1] =  PE(/*myO*/open("/dev/ser2", O_RDWR));
#ifndef MEDIUM
	PE(mySocketpair(AF_LOCAL, SOCK_STREAM, 0, daSktPr));
#else
	// ***** switch from having one socketpair for direct connection to having two socketpairs
	//			for connection through medium thread *****
	PE(mySocketpair(AF_LOCAL, SOCK_STREAM, 0, daSktPrT1M));
	PE(mySocketpair(AF_LOCAL, SOCK_STREAM, 0, daSktPrMT2));
	// ***** create thread for medium *****
	thread mediumThrd(mediumFunc);
#endif

	thread term2Thrd(termFunc, Term2);
	
	termFunc(Term1);

	term2Thrd.join();
	
#ifdef MEDIUM
	// ***** join with thread for medium *****
	mediumThrd.join();
#endif

	return EXIT_SUCCESS;
}
