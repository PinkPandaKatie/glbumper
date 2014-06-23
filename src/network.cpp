
#include "common.h"
#include "network.h"



#define CHECKERR(expr)				\
do {						\
    set_sock_errno(0);				\
    expr;					\
    if (sock_errno != 0)			\
	throw SocketError();			\
} while (0)

#define REPEATAGAIN(expr)					\
do {								\
    errno = 0;							\
    expr;							\
    if (sock_errno == EAGAIN || sock_errno == EINTR) continue;	\
    if (sock_errno != 0)					\
	throw SocketError();					\
    break;							\
} while (1)


/*
 *
 * Address
 *
 */
Address::Address(string host) {
    int port = 0;
    unsigned int pos = host.find_last_of(':');
    
    if (pos != string::npos) {
	port = atoi(host.substr(pos+1).c_str());
	host = host.substr(0,pos);
    }
    set_host_port(host, port);
}

void Address::set_host_port(string host, int port) {
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    if (host.empty()) {
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = 0;
    } else {
	struct hostent *hp;
	if (!inet_aton(host.c_str(), &addr.sin_addr)) {
		hp = gethostbyname(host.c_str());
	    if (hp && hp->h_length == sizeof(addr.sin_addr)) {
		memcpy(&addr.sin_addr, hp->h_addr, sizeof(addr.sin_addr));
	    } else {
		throw SocketError(string("could not resolve ") + host);
	    }
	}
    }

    addr.sin_port = htons(port);

}

/*
 *
 * Socket
 *
 */

#include objdef(Socket)



Socket::Socket(int type, int port) : opened(false) {
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    
    CHECKERR(fd = socket(PF_INET, type, 0));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(port);
    
    CHECKERR(bind(fd, (struct sockaddr*)&saddr, sizeof(struct sockaddr_in)));
    
    opened = true;
}

void Socket::close() {
    if (opened) {
	close_socket(fd);
	opened = false;
    }
}


void Socket::init() {
#ifdef WIN32

    WORD version_wanted = MAKEWORD(1,1);
    WSADATA wsaData;
    
    if (WSAStartup(version_wanted, &wsaData) != 0) {
	throw SocketError("couldn't initialize Winsuck");
    }

#endif
} 


/*
 *
 * UDPSocket
 *
 */

#include objdef(UDPSocket)

int UDPSocket::recv(Uint8* buffer, int size, Address& from) {
    int ret;
    struct sockaddr_in sfrom;
    socklen_t fromlen = sizeof(struct sockaddr_in);
    memset(&sfrom, 0, sizeof(sfrom));
    
    REPEATAGAIN(ret = recvfrom(fd, (char*)buffer, size, 0, 
				   (struct sockaddr*)&sfrom, &fromlen));
    
    from = Address(sfrom);
    
    return ret;
}

void UDPSocket::send(const Uint8* buffer, int size, const Address& to) {
    int ret;
     
    REPEATAGAIN(ret = sendto(fd, (const char*)buffer, size, 0, 
			     (const struct sockaddr*)to.get_sockaddr(), 
			     sizeof(struct sockaddr_in)));
}

/*
 *
 * TCPServerSocket
 *
 */

#include objdef(TCPServerSocket)

TCPServerSocket::TCPServerSocket(int port) 
    : Socket(SOCK_STREAM, port), port(port) { 
    
    listen(fd, 1);
}



/*
 *
 * TCPSocket
 *
 */

#include objdef(TCPSocket)

TCPSocket::TCPSocket(TCPServerSocket* ssock) {
    struct sockaddr_in saddr;
    socklen_t fromsize;
    memset(&saddr, 0, sizeof(saddr));
    CHECKERR(fd = accept(ssock->fileno(), 
			 (struct sockaddr*)&saddr, &fromsize));
    if (fromsize != sizeof(struct sockaddr_in)) {
	throw SocketError("WTF? fromsize != sizeof(sockaddr_in)! This shouldn't happen!");
    }
    opened = true;
    connected = true;
    peer = saddr;
}

int TCPSocket::recv(Uint8* buffer, int size) {
    int ret;
    REPEATAGAIN(ret = ::recv(fd, (char*)buffer, size, 0));
    return ret;
}

int TCPSocket::send(const Uint8* buffer, int size) {
    int ret;
    REPEATAGAIN(ret = ::send(fd, (const char*)buffer, size, 0));
    return ret;
}

void TCPSocket::connect(const Address& to) {
    if (connected)
	throw SocketError("already connected");
    
    CHECKERR(::connect(fd, (struct sockaddr*)to.get_sockaddr(),
		       sizeof(struct sockaddr_in)));
    connected = true;
}


/*
 *
 * SocketGroup
 *
 */

#include objdef(SocketGroup)

void SocketGroup::poll(double timeout) {
    struct timeval timeval;
    fd_set readfds;
    fd_set writefds;
    int highest = 0;
    int r = 0;
    timeval.tv_sec = (int)(timeout);
    timeval.tv_usec = (int)((timeout - timeval.tv_sec) * 1000000.0);
    
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    
    FILTER(itr, socks, , (*itr)->isopen(), );

    FOREACH(itr, socks) {
	Socket* s = *itr;
	int fd = s->fileno();
	if (fd == -1) continue;

	FD_SET(fd, &readfds);
	if (s->want_to_write())
	    FD_SET(fd, &writefds);
	if (fd > highest)
	    highest = fd;

    }
    
    CHECKERR(r = select(highest + 1, &readfds, &writefds, NULL, &timeval));
    
    FOREACH(itr, socks) {
	Socket* s = *itr;
	int fd = s->fileno();
	if (fd == -1) continue;

	if (FD_ISSET(fd, &readfds)) {
	    s->readready(this);
	}
	if (FD_ISSET(fd, &writefds)) {
	    s->writeready(this);
	}
    }
}

int SocketGroup::run() {
    while (!should_stop()) {
	try {
	    poll(0.1);
	} catch (exception& ex) {
	    cout <<"Poll thread caught exception: " << ex.what() << endl;
	}
    }
    return 0;
}

/*
 *
 * BufferedSocket
 *
 */

#include objdef(BufferedSocket)


bool BufferedSocket::want_to_write() { 
    return _close_when_empty || !writebuf.empty();
}
void BufferedSocket::close_when_empty() {
    _close_when_empty = true;
}

void BufferedSocket::write(string dat) {
    writebuf += dat;
}
void BufferedSocket::writeready(SocketGroup* sg) {
    int sent = send((const Uint8*)writebuf.data(), writebuf.size());
    writebuf = writebuf.substr(sent);
    wrotedata();
    if (writebuf.empty() && _close_when_empty) {
	close();
    }
}
void BufferedSocket::readready(SocketGroup* sg) {
    Uint8 buffer[1025];
    int size = recv(buffer, 1024);
    if (size == 0) {
	close();
	return;
    }
    readbuf += string((char*)buffer, size);
    
    gotdata();
    
}

/*
 *
 * LineBufferedSocket
 *
 */

#include objdef(LineBufferedSocket)


void LineBufferedSocket::gotdata() {
    unsigned int p, lp;
    lp = 0;
    while ((p = readbuf.find('\n', lp)) != string::npos) {
	int siz = p - lp;
	if (readbuf[p - 1] == '\r') --siz;
	if (max_line_len && siz > max_line_len) siz = max_line_len;
	gotline(readbuf.substr(lp, siz));
	lp = p + 1;
    }
    if (lp != 0)
	readbuf = readbuf.substr(lp);
    if (max_line_len && readbuf.size() > (unsigned)max_line_len)
	readbuf = readbuf.substr(0, max_line_len);
}


#ifdef WIN32
// from the descriptions at
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winsock/winsock/windows_sockets_error_codes_2.asp

const char* winsock_strerror(int eno) {
    switch(eno) {
    case 0:
	return "Error occured, but did not set error code. This should not happen.";
    case WSAEINTR:
	return "Interrupted function call";
    case WSAEACCES:
	return "Permission denied";
    case WSAEFAULT:
	return "Bad address";
    case WSAEINVAL:
	return "Invalid argument";
    case WSAEMFILE:
	return "Too many open files";
    case WSAEWOULDBLOCK:
	return "Resource temporarily unavailable";
    case WSAEINPROGRESS:
	return "Operation now in progress";
    case WSAEALREADY:
	return "Operation already in progress";
    case WSAENOTSOCK:
	return "Socket operation on nonsocket";
    case WSAEDESTADDRREQ:
	return "Destination address required";
    case WSAEMSGSIZE:
	return "Message too long";
    case WSAEPROTOTYPE:
	return "Protocol wrong type for socket";
    case WSAENOPROTOOPT:
	return "Bad protocol option";
    case WSAEPROTONOSUPPORT:
	return "Protocol not supported";
    case WSAESOCKTNOSUPPORT:
	return "Socket type not supported";
    case WSAEOPNOTSUPP:
	return "Operation not supported";
    case WSAEPFNOSUPPORT:
	return "Protocol family not supported";
    case WSAEAFNOSUPPORT:
	return "Address family not supported by protocol family";
    case WSAEADDRINUSE:
	return "Address already in use";
    case WSAEADDRNOTAVAIL:
	return "Cannot assign requested address";
    case WSAENETDOWN:
	return "Network is down";
    case WSAENETUNREACH:
	return "Network is unreachable";
    case WSAENETRESET:
	return "Network dropped connection on reset";
    case WSAECONNABORTED:
	return "Software caused connection abort";
    case WSAECONNRESET:
	return "Connection reset by peer";
    case WSAENOBUFS:
	return "No buffer space available";
    case WSAEISCONN:
	return "Socket is already connected";
    case WSAENOTCONN:
	return "Socket is not connected";
    case WSAESHUTDOWN:
	return "Cannot send after socket shutdown";
    case WSAETIMEDOUT:
	return "Connection timed out";
    case WSAECONNREFUSED:
	return "Connection refused";
    case WSAEHOSTDOWN:
	return "Host is down";
    case WSAEHOSTUNREACH:
	return "No route to host";
    case WSAEPROCLIM:
	return "Too many processes";
    case WSASYSNOTREADY:
	return "Network subsystem is unavailable";
    case WSAVERNOTSUPPORTED:
	return "Winsock.dll version out of range";
    case WSANOTINITIALISED:
	return "Successful WSAStartup not yet performed";
    case WSAEDISCON:
	return "Graceful shutdown in progress";
    case WSATYPE_NOT_FOUND:
	return "Class type not found";
    case WSAHOST_NOT_FOUND:
	return "Host not found";
    case WSATRY_AGAIN:
	return "Nonauthoritative host not found";
    case WSANO_RECOVERY:
	return "This is a nonrecoverable error";
    case WSANO_DATA:
	return "Valid name, no data record of requested type";
    case WSA_INVALID_HANDLE:
	return "Specified event object handle is invalid";
    case WSA_INVALID_PARAMETER:
	return "One or more parameters are invalid";
    case WSA_IO_INCOMPLETE:
	return "Overlapped I/O event object not in signaled state";
    case WSA_IO_PENDING:
	return "Overlapped operations will complete later";
    case WSA_NOT_ENOUGH_MEMORY:
	return "Insufficient memory available";
    case WSA_OPERATION_ABORTED:
	return "Overlapped operation aborted";
#ifdef WSAINVALIDPROCTABLE
    case WSAINVALIDPROCTABLE:
	return "Invalid procedure table from service provider";
#endif
#ifdef WSAINVALIDPROVIDER
    case WSAINVALIDPROVIDER:
	return "Invalid service provider version number";
#endif
#ifdef WSAPROVIDERFAILEDINIT
    case WSAPROVIDERFAILEDINIT:
	return "Unable to initialize a service provider";
#endif
    case WSASYSCALLFAILURE:
	return "System call failure";
    }
    return "Unknown error";
}
#endif
