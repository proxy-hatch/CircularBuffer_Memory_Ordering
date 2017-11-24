#ifndef MYSOCKET_H_
#define MYSOCKET_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <unistd.h> // for size_t

// for mode_t
//#include <fcntl.h>
#include <sys/stat.h>

/* int myOpen( const char * path,
          int oflag,
          ... )
; */
int myOpen(const char *pathname, int flags, mode_t mode);

int myCreat(const char *pathname, mode_t mode);
int mySocketpair( int domain, int type, int protocol, int des[2] );
ssize_t myRead( int des, void* buf, size_t nbyte );
ssize_t myWrite( int des, const void* buf, size_t nbyte );
int myClose(int des);
// The last two are not ordinarily used with sockets
int myTcdrain(int des); //is also included for purposes of the course.
int myReadcond(int des, void * buf, int n, int min, int time, int timeout);

#ifdef __cplusplus
}
#endif

#endif /*MYSOCKET_H_*/

