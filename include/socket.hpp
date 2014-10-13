#ifndef SOCKET_HPP
#define SOCKET_HPP

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <ws2ipdef.h>
#  define CLOSESOCKET ::closesocket
#else
#  include <unistd.h>
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netdb.h>
#  include <arpa/inet.h>
#  define CLOSESOCKET ::close
   using SOCKET = int;
#  define INVALID_SOCKET -1
#endif

#define INIT_WINSOCK \
class WSA {\
public:\
    WSA() : wsa_err(WSAStartup(MAKEWORD(2, 2), &wsa_data)) {}\
    ~WSA() { WSACleanup(); }\
private:\
    WSADATA wsa_data;\
    int wsa_err;\
};\
const WSA wsa;

#include <cstring>
#include <string>
#include <iostream>
#include <memory>
#include <stdexcept>
#include "clany/byte_array.hpp"

_CLANY_BEGIN
using SockAddrIN  = sockaddr_in;
using SockAddr    = sockaddr;

class SocketError : public runtime_error {
public:
    SocketError(const string& err) : runtime_error(err.c_str()) {}
};

class AbstractSocket
{
public:
    enum SockState {
       UnconnectedState = 0,
       HostLookupState  = 1,
       ConnectingState  = 2,
       ConnectedState   = 3,
       BoundState       = 4,
       ClosingState     = 5,
       ListeningState   = -1
    };

    using Ptr = shared_ptr<AbstractSocket>;

    AbstractSocket(int domain, int type, int protocal)
      : handle(::socket(domain, type, protocal)), sock_state(UnconnectedState) {}
    AbstractSocket(int sock, SockAddrIN address, SockState state)
      : handle(sock), addr(address), sock_state(state) {}

    AbstractSocket(const AbstractSocket&) = delete;
    AbstractSocket& operator=(const AbstractSocket&) = delete;

    ~AbstractSocket() {
        if (sock_state != UnconnectedState) CLOSESOCKET(handle);
    }

    bool bind(const string& host_address, ushort port) {
        SockAddrIN host_addr;
        memset(&host_addr, 0, sizeof(host_addr));
        host_addr.sin_family = AF_INET;
        host_addr.sin_port   = port;
        if (!::inet_pton(AF_INET, host_address.c_str(), &host_addr.sin_addr)) {
            cerr << "Invalid address, conversion failed" << endl;
            return false;
        }
        if (::bind(handle, (SockAddr*)&host_addr, sizeof(host_addr)) < 0) {
            cerr << "Bind socket fail!" << endl;
            return false;
        }

        sock_state = BoundState;
        return true;
    }

    bool bind(ushort port) {
        SockAddrIN host_addr;
        memset(&host_addr, 0, sizeof(host_addr));
        host_addr.sin_family = AF_INET;
        host_addr.sin_port   = port;
        host_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (::bind(handle, (SockAddr*)&host_addr, sizeof(host_addr)) < 0) {
            cerr << "Bind socket fail!" << endl;
            return false;
        }

        sock_state = BoundState;
        return true;
    }

    virtual bool connect(const string& host_name, ushort port) {
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = port;

        sock_state = HostLookupState;
        if (!::inet_pton(AF_INET, host_name.c_str(), &addr.sin_addr)) {
            cerr << "Invalid address, conversion failed" << endl;
            sock_state = UnconnectedState;
            return false;
        }
        sock_state = ConnectingState;
        if (::connect(handle, (SockAddr*)&addr, sizeof(addr)) < 0) {
            cerr << "Fail to connect to host!" << endl;
            sock_state = UnconnectedState;
            return false;
        }

        sock_state = ConnectedState;
        return true;
    }

    virtual void disconnect() {
        close();
    }

    bool isValid() const { return handle >= 0; }

    virtual bool write(const string& message) const {
        return write(message.c_str(), message.length());
    }

    virtual bool write(const ByteArray& data) const {
        return write(data.data(), data.size());
    }

    virtual bool write(const char* message, size_t n) const {
        if (::send(handle, message, n, 0) < 0) {
            return false;
        }
        return true;
    }

    SOCKET sock() const { return handle; }

    string peekAddress() const {
        auto _addr = addr.sin_addr;
        char address[INET_ADDRSTRLEN];
        ::inet_ntop(AF_INET, &_addr, address, INET_ADDRSTRLEN);

        return string(address);
    }

    ushort port() const { return addr.sin_port; }

    SockState state() const { return sock_state; }

protected:
    void setState(SockState state) { sock_state = state; }

    void close() {
        sock_state = ClosingState;
        CLOSESOCKET(handle);
        sock_state = UnconnectedState;
    }

    SOCKET handle;
    SockAddrIN addr;
    SockState sock_state;
};

class TCPSocket : public AbstractSocket
{
    friend class TCPServer;
public:
    using Ptr = shared_ptr<TCPSocket>;

    TCPSocket() : AbstractSocket(AF_INET, SOCK_STREAM, 0) {}

    // Take ownership of existing resource
    TCPSocket(int sock, SockAddrIN addr, SockState state) : AbstractSocket(sock, addr, state) {}
};

class UDPSocket : public AbstractSocket
{
public:
    using Ptr = shared_ptr<UDPSocket>;

    UDPSocket() : AbstractSocket(AF_INET, SOCK_DGRAM, 0) {}

    // Take ownership of existing resource
    UDPSocket(int sock, SockAddrIN addr, SockState state) : AbstractSocket(sock, addr, state) {}
};
_CLANY_END

#endif // SOCKET_HPP