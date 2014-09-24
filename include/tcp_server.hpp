#ifndef SERVER_HPP
#define SERVER_HPP

#include "socket.hpp"

_CLANY_BEGIN
class TCPServer
{
public:
    TCPServer(int num_connections = 1) : max_queue_sz(num_connections) {}

    TCPSocket::Ptr nextPendingConnection() {
        SockAddrIN client_addr;
        socklen_t  addr_sz = sizeof(client_addr);
        memset(&client_addr, 0, addr_sz);
        return make_shared<TCPSocket>(::accept(tcp_socket.sock(), (SockAddr*)&client_addr, &addr_sz),
                                      client_addr, AbstractSocket::ConnectedState);
    }

    bool listen(const string& host_address, uint16_t port) {
        if (!tcp_socket.bind(host_address, port)) return false;
        if (::listen(tcp_socket.sock(), max_queue_sz) < 0) {
            cerr << "listen failed!" << endl;
            return false;
        }

        tcp_socket.setState(TCPSocket::ListeningState);
        cout << "Waiting for client request..." << endl;
        return true;
    }

    bool listen(uint16_t port) {
        if (!tcp_socket.bind(port)) return false;
        if (::listen(tcp_socket.sock(), max_queue_sz) < 0) {
            cerr << "listen failed!" << endl;
            return false;
        }

        tcp_socket.setState(TCPSocket::ListeningState);
        cout << "Waiting for client request..." << endl;
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

private:
    TCPSocket tcp_socket;
    int max_queue_sz;
};
_CLANY_END

#endif // SERVER_HPP