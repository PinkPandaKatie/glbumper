// -*- c++ -*-

#ifndef _NETWORK_H
#define _NETWORK_H

#include "common.h"

#ifdef WIN32

#include <winsock2.h>

#ifndef EAGAIN
#define EAGAIN WSAEWOULDBLOCK
#define EINTR WSAEINTR
#endif

extern const char* winsock_strerror(int eno);
typedef int socklen_t;

static void sock_init() {
    WSADATA dat;
    WSAStartup(MAKEWORD(2, 0), &dat);
}

#define close_socket ::closesocket
#define sock_errno WSAGetLastError()
#define set_sock_errno(eno) WSASetLastError(eno)
#define sock_strerror(eno) winsock_strerror(eno)

static int inet_aton(const char* n, struct in_addr* addr) {
    addr->s_addr = inet_addr(n);
    return addr->s_addr != INADDR_NONE;
    
}

#else



#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>



#ifdef linux
#include <netinet/tcp.h>
#endif



#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>


#define sock_init()
#define close_socket ::close
#define sock_errno errno
#define set_sock_errno(eno) (errno = (eno))
#define sock_strerror(eno) strerror(eno)



#endif


#include <map>
#include <set>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <vector>

#include <SDL.h>

#include "object.h"
#include "vector.h"

class Address {
    struct sockaddr_in addr;

    void set_host_port(string host, int port);

    inline int compare(const Address& b) const {
	if (addr.sin_addr.s_addr < b.addr.sin_addr.s_addr)
	    return -1;
	if (addr.sin_addr.s_addr > b.addr.sin_addr.s_addr)
	    return 1;

	if (addr.sin_port < b.addr.sin_port)
	    return -1;
	if (addr.sin_port > b.addr.sin_port)
	    return 1;

	return 0;
    }

public:
    Address(string host, int port) {
	set_host_port(host, port);
    }

    Address(string host = "");

    Address(const struct sockaddr_in& saddr) : addr(saddr) { }

    string str() const {
	ostringstream oss;
	oss << inet_ntoa(addr.sin_addr) << ':' << ntohs(addr.sin_port);
	return oss.str();
    }

    string hoststr() const {
	return inet_ntoa(addr.sin_addr);
    }

    const struct sockaddr_in* get_sockaddr() const {
	return &addr;
    }

    int getport() {
	return ntohs(addr.sin_port);
    }

    bool operator<(const Address& b) const {
	return compare(b) < 0;
    }
    bool operator<=(const Address& b) const {
	return compare(b) <= 0;
    }
    bool operator>(const Address& b) const {
	return compare(b) > 0;
    }
    bool operator>=(const Address& b) const {
	return compare(b) >= 0;
    }
    bool operator==(const Address& b) const {
	return compare(b) == 0;
    }
    bool operator!=(const Address& b) const {
	return compare(b) != 0;
    }
};



class SocketError : public exception {
    string reason;
    int errnoval;
public:
    SocketError() throw() {
	errnoval = sock_errno;
	reason = sock_strerror(errnoval);
    }
    ~SocketError() throw() { }
    SocketError(string reason) throw() : reason(reason), errnoval(0) { }
    const char* what() const throw() {
	return reason.c_str();
    }
};

class SocketGroup;

class Socket : virtual public Object {
protected:
    int fd;
    bool opened;


    Socket(int type, int port);
    Socket() : fd(0), opened(false) { }

public:


    int fileno() const {
	return fd;
    }
    
    void close();
    bool isopen() { return opened; }

    virtual bool want_to_write() { return false; }

    virtual void readready(SocketGroup* group) { }
    virtual void writeready(SocketGroup* group) { }

    static void init();

    ~Socket() {	close(); }
    
    DECLARE_OBJECT;
};

class UDPSocket : public Socket {
public:

    UDPSocket(int port = 0) : Socket(SOCK_DGRAM, port) { 
    }


    int recv(Uint8* buffer, int size, Address& from);

    void send(const Uint8* buffer, int size, const Address& to);

    DECLARE_OBJECT;
};

class TCPServerSocket : public Socket {
    int port;

public:
    TCPServerSocket(int port = 0); 
    
    int getport() {
	return port;
    }

    DECLARE_OBJECT;
};

class TCPSocket : public Socket {
    Address peer;
    bool connected;


public:

    TCPSocket(TCPServerSocket* ssock);
    TCPSocket() : Socket(SOCK_STREAM, 0), connected(false) { }

    int recv(Uint8* buffer, int size);
    int send(const Uint8* buffer, int size);
    virtual void connect(const Address& to);

    bool isconnected() {
	return isopen() && connected;
    }

    Address getpeer() {
	return peer;
    }

    DECLARE_OBJECT;
};

class SocketGroup : public Thread {
    set< sptr<Socket> > socks;
public:

    SocketGroup() { };
    void add(Socket* sock) { socks.insert(sock); }
    void remove(Socket* sock) { socks.erase(sock); }
    void poll(double timeout = 0.0);
    int run();
    bool empty() const { return socks.empty(); }
    const set< sptr<Socket> >& getsockets() const { return socks; }
    ~SocketGroup() { };
    
    DECLARE_OBJECT;
};

class BufferedSocket : public TCPSocket { 
    bool _close_when_empty;
    
protected:
    string readbuf;
    string writebuf;

public:
    BufferedSocket(TCPServerSocket* ssock) : TCPSocket(ssock), _close_when_empty(false) { }
    BufferedSocket() : _close_when_empty(false) { }

    bool want_to_write();
    void close_when_empty();
    virtual void gotdata() { }
    virtual void wrotedata() { }
    void write(string dat);
    void writeready(SocketGroup* sg);
    void readready(SocketGroup* sg);
    
    DECLARE_OBJECT;
};

class LineBufferedSocket : public BufferedSocket {
protected:
    int max_line_len;

public: 

    LineBufferedSocket(TCPServerSocket* ssock) : BufferedSocket(ssock), max_line_len(0) { }
    LineBufferedSocket() : max_line_len(0) { }


    virtual void gotline(string line) { }
    virtual void gotdata();

    DECLARE_OBJECT;
};

#endif
