#ifndef SERVER_HPP
#define SERVER_HPP

#include "socket.hpp"

_CLANY_BEGIN
class TCPServer {
public:
    TCPServer(int num_connections = 1) : max_queue_sz(num_connections) {}

    virtual TCPSocket::Ptr nextPendingConnection() const {
        SockAddrIN client_addr;
        socklen_t  addr_sz = sizeof(client_addr);
        memset(&client_addr, 0, addr_sz);
        if (hasPendingConnections()) {
            auto sock = ::accept(tcp_socket.sock(), (SockAddr*)&client_addr, &addr_sz);
            return TCPSocket::Ptr(new TCPSocket(sock, client_addr,
                                                TCPSocket::ConnectedState));
        }

        return nullptr;
    }

    virtual bool hasPendingConnections() const {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(tcp_socket.sock(), &read_fds);
        timeval no_block {0, 0};

        return ::select(tcp_socket.sock() + 1, &read_fds,
                        nullptr, nullptr, &no_block) > 0;
    }

    bool listen(const string& host_address, uint16_t port) {
        if (!tcp_socket.bind(host_address, port)) return false;
        if (::listen(tcp_socket.sock(), max_queue_sz) < 0) {
            if (verbose) cerr << "listen failed!" << endl;
            return false;
        }

        local_addr = host_address;
        listen_port = port;
        tcp_socket.setState(TCPSocket::ListeningState);
        return true;
    }

    bool listen(uint16_t port) {
        if (!tcp_socket.bind(port)) return false;
        if (::listen(tcp_socket.sock(), max_queue_sz) < 0) {
            if (verbose) cerr << "listen failed!" << endl;
            return false;
        }

        local_addr  = "0.0.0.0";
        listen_port = port;
        tcp_socket.setState(TCPSocket::ListeningState);
        return true;
    }

    bool isListening() const {
        return tcp_socket.state() == TCPSocket::ListeningState;
    }

    void close() {
        tcp_socket.close();
    }

    void setMaxConnection(int num_connections) {
        max_queue_sz = num_connections;
    }

    string  listenAddr() const { return local_addr; }
    int16_t listenPort() const { return listen_port; }

protected:
    TCPSocket tcp_socket;
    int max_queue_sz;

    string local_addr;
    uint16_t listen_port;

    bool verbose = false;
};
_CLANY_END

#endif // SERVER_HPP