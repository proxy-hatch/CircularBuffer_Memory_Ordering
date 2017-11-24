/*
 * Medium.h
 *
 *      Author: Craig Scratchley
 *      Copyright(c) 2014 (Spring) Craig Scratchley
 */

#ifndef MEDIUM_H_
#define MEDIUM_H_

class Medium {
public:
	Medium(int d1, int d2, const char *fname);
	virtual ~Medium();

	void run();

private:
	int Term1D;	// descriptor for Term1
	int Term2D;	// descriptor for Term2
	const char* logFileName;
	int logFileD;	// descriptor for log file

	int byteCount;

	int ACKreceived;
	int ACKforwarded;
	bool sendExtraAck;

	bool MsgFromTerm1();
	bool MsgFromTerm2();
};

#endif /* MEDIUM_H_ */
