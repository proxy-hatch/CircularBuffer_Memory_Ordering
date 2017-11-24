//============================================================================
// Name        : myIO.cpp
// Author(s)   : Craig Scratchley
//			   : E. Kan
//			   :
// Version     : November, 2017 -- socketpairs by circular buffer (including tcDrain support)
// Copyright   : Copyright 2017, Engineering Science, SFU
// Description : An implementation of tcdrain-like behaviour for socketpairs.
//============================================================================

#include <sys/socket.h>
#include <unistd.h>				// for posix i/o functions
#include <termios.h>			// for tcdrain()
#include <fcntl.h>				// for open/creat
#include <mutex>				
#include <condition_variable>	
#include <vector>				
#include "AtomicCOUT.h"
#include "SocketReadcond.h"
#include "VNPE.h"
#include "myIO.h"
#include "RageUtil_CircularBuffer.h"
#include <iostream>

using namespace std;

//Unnamed namespace
namespace{

class socketDrainClass;

typedef vector<socketDrainClass*> sdcVectType;
sdcVectType desInfoVect(3); // start with size of 3 due to stdin, stdout, stderr

//	A mutex used to protect desInfoVect so only a single thread can modify the vector at a time.
//  This also means that only one socketpair() or close() function can be called at a time from this file.
//  This mutex is also used to prevent a socket from being closed at the beginning of a myWrite or myTcdrain function.
mutex vectMutex;

class socketDrainClass {
	int buffered = 0;
	CircBuf<char> buffer;
	condition_variable cvDrain;
	condition_variable cvRead;
	mutex socketDrainMutex;
public:
	int pair; // Cannot be private because myWrite and myTcdrain using it

	socketDrainClass(unsigned pairInit)
	:buffered(0), pair(pairInit) {
		buffer.reserve(300); // note constant of 300
	}

	/*
	 * Function:  make the calling thread wait for a reading thread to drain the data
	 */
	int waitForDraining(unique_lock<mutex>& passedVectlk)
	{
		unique_lock<mutex> condlk(socketDrainMutex);
		passedVectlk.unlock();
		if (buffered > 0) { // this if statement is optional
			//Waiting for a reading thread to drain out the data
			cvDrain.wait(condlk, [this] {return buffered <= 0;});
		}
		return 0;
	}

	/*
	 * Function:	Writing on the descriptor and update the reading buffered
	 */
	int writing(int des, const void* buf, size_t nbyte)
	{
		lock_guard<mutex> condlk(socketDrainMutex);
		// int written = write(des, buf, nbyte);
		// never returns -1 below.
		int written = buffer.write((const char*) buf, nbyte);
		buffered += written;
		if (buffered >= 0)
			cvRead.notify_one();
		return written;
	}

	/*
	 * Function:  Checking buffered and waking up the wait-for-draining threads if needed.
	 */
	int reading(int des, void * buf, int n, int min, int time, int timeout)
	{
		int bytesRead;
		unique_lock<mutex> condlk(socketDrainMutex);
		if (buffered >= min || pair == -1) {
			// wcsReadcond should not wait in this situation.
			bytesRead = buffer.read((char *) buf, n);
			// bytesRead = read(des, buf, n); // at least min will be waiting.
			//bytesRead = wcsReadcond(des, buf, n, min, time, timeout);
			if (bytesRead > 0) {
				buffered -= bytesRead;
				if (!buffered)
					cvDrain.notify_all();
			}
		}
		else {
			if (buffered < 0) {
				COUT << "Currently only support one reading call at a time" << endl;
				exit(EXIT_FAILURE);
			}
			if (time != 0 || timeout != 0) {
				COUT << "Currently only supporting no timeouts or immediate timeout" << endl;
				exit(EXIT_FAILURE);
			}

			buffered -= min;
			cvDrain.notify_all(); // buffered must be <= 0

			cvRead.wait(condlk, [this] {return (buffered) >= 0 || pair == -1;});
			bytesRead = buffer.read((char *) buf, n);
			buffered -= (bytesRead - min);
			/*
			// bytesRead = wcsReadcond(des, buf, n, min, time, timeout);
			bytesRead = read(des, buf, n);
			if (bytesRead == -1) {
				// an error occurred.
				// but be aware that a cvDrain notification may already have occurred above.
				buffered += min;
			}
			else {
				buffered -= (bytesRead - min);
			}*/
		}
		return bytesRead;
	}

	int finishClosing(int des, socketDrainClass* des_pair)
	{
		int retVal = close(des);

		if (retVal != -1) {
			if(des_pair)
			{
				des_pair->pair = -1;
				if (des_pair->buffered < 0) {
					// no more data will be written from des
					// notify thread waiting on reading on paired descriptor
					des_pair->cvRead.notify_one();
				}
				else if (des_pair->buffered > 0) {
					// there shouldn't be any threads waiting in myTcdrain on des, but just in case.
					des_pair->cvDrain.notify_all();
				}
				if (buffered > 0) {
					// by closing the socket we are throwing away any buffered data.
					// notification will be sent immediately below to myTcdrain waiters on paired descriptor.
					buffered = 0;
					cvDrain.notify_all(); // is this needed?  Will notification be sent anyhow at cv destruction?
				}
				else if (buffered < 0) {
					// there shouldn't be any threads waiting in myReadond() on des, but just in case.
					buffered = 0;
					cvRead.notify_one(); // when will the actual notification take place? too late?
				}
			}
			delete desInfoVect[des];
			desInfoVect[des] = nullptr;
		}
		return retVal;
	}

	/*
	 * Function:  Closing des. Should be done only after all other operations on des have returned.
	 */
	int closing(int des)
	{
		// vectMutex already locked at this point.
		socketDrainClass* des_pair = nullptr;
		unique_lock<mutex> condlk(socketDrainMutex, defer_lock);
		if(pair == -1) { //  It is safe to reference pair here and below because vectMutex is locked.
			// the paired descriptor has already been closed -- no need to lock its mutex
			condlk.lock();
			return finishClosing(des, des_pair);
		}
		else {
			des_pair = desInfoVect[pair];
			unique_lock<mutex> condPairlk(des_pair->socketDrainMutex, defer_lock);
			lock(condPairlk, condlk);
			return finishClosing(des, des_pair);
		}
	}
};
} // unnamed namespace

/*
 * Function:	Open a file and get its file descriptor. If needed, expand the vector
 * 				to fit the new descriptor number
 * Return:		return value of open
 */
int myOpen(const char *pathname, int flags, mode_t mode)
{
	lock_guard<mutex> vectlk(vectMutex);
	int des = open(pathname, flags, mode);
	if((sdcVectType::size_type) des >= desInfoVect.size())
		desInfoVect.resize(des+1);
//	else
//		desInfoVect[des] = nullptr; // should already be nullptr
	return des;
}

/*
 * Function:	Create a new file and get its file descriptor. If needed, expand the vector
 * 				to the new descriptor number
 * Return:		return value of creat
 */
int myCreat(const char *pathname, mode_t mode)
{
	lock_guard<mutex> vectlk(vectMutex);
	int des = creat(pathname, mode);
	if((sdcVectType::size_type) des >= desInfoVect.size())
		desInfoVect.resize(des+1);
	return des;
}

/*
 * Function:	Create pair of sockets and put them in desInfoVect (expanding vector if necessary)
 * Return:		return an integer that indicate if it is successful (0) or not (-1)
 */
int mySocketpair(int domain, int type, int protocol, int des[2]) {
	lock_guard<mutex> vectlk(vectMutex);
	int returnVal = socketpair(domain, type, protocol, des);
	if(returnVal != -1) {
		int vectSize = desInfoVect.size();
		//	vector will expand if a
		//  descriptor number is bigger than or equal to the vector size
		int maxDes = max(des[0], des[1]);
		if(maxDes >= vectSize)
			desInfoVect.resize(maxDes+1);

		desInfoVect[des[0]] = new socketDrainClass(des[1]);
		desInfoVect[des[1]] = new socketDrainClass(des[0]);
	}
	return returnVal;
}

/*
 * Function:	Calling the reading member function to read
 * Return:		An integer with number of bytes read, or -1 for an error.
 */
int myReadcond(int des, void * buf, int n, int min, int time, int timeout) {
	if (!desInfoVect.at(des)) {
		COUT << "myReadcond currently only supported on sockets created with mySocketpair()" << endl;
		return wcsReadcond(des, buf, n, min, time, timeout); // should return -1 and set errno.
	}
	// Care here.  What if des is closed between these two lines?  We assume close will not be called
	// until other functions on the descriptor have returned.
	return desInfoVect.at(des)->reading(des, buf, n, min, time, timeout);
}

/*
 * Function:	Reading directly from the a file or calling myReadcond() to read from socket pair descriptors)
 * Return:		the number of bytes read , or -1 for an error
 */
ssize_t myRead(int des, void* buf, size_t nbyte) {
	//Check if des was obtained from mySocketpair or not
	if (!desInfoVect.at(des))
		// don't call mySocketpair() after myRead() to avoid mySocketpair() being called here. 
		return read(des, buf, nbyte); // des is not currently from a pair of sockets.
	// myRead (for sockets) usually reads a minimum of 1 byte
	return myReadcond(des, buf, nbyte, 1, 0, 0);
}

/*
 * Return:		the number of bytes written, or -1 for an error
 */
ssize_t myWrite(int des, const void* buf, size_t nbyte) {
	unique_lock<mutex> vectlk(vectMutex);
	//Check if the descriptor given is from a socket pair or not
	socketDrainClass* desInfoP = desInfoVect.at(des);
	if(desInfoP)
		if (desInfoP->pair == -1) {
			errno = EPIPE;
			return -1;
		} else
			// locking vectMutex above makes sure that desinfoP->pair is not closed here
			return desInfoVect[desInfoP->pair]->writing(des, buf, nbyte);
	else
		return write(des, buf, nbyte);  // des is not currently from a pair of sockets.
}

/*
 * Function:  make the calling thread wait for a reading thread to drain the data
 */
int myTcdrain(int des) {
	unique_lock<mutex> vectlk(vectMutex);
	socketDrainClass* desInfoP = desInfoVect.at(des); // chose not to write desInfoVect[des]
	if(desInfoP && desInfoP->pair != -1)
		// locking vectMutex above makes sure that desinfoP->pair is not closed here
		return desInfoVect[desInfoP->pair]->waitForDraining(vectlk);
	vectlk.unlock();
	return tcdrain(des); // des is not from a pair of sockets.
}

/*
 * Function:	Closing des
 * 				myClose() should not be called until all other calls using the descriptor have finished.
 */
int myClose(int des) {
	// lock vectMutex because we don't want socketpair() to be called at same time as close()
	//		and we don't want the paired socket to be closed at the same time either.
	lock_guard<mutex> vectlk(vectMutex);
	if(!desInfoVect.at(des))
		return close(des);
	return desInfoVect[des]->closing(des);
}
