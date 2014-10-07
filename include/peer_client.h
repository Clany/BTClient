#ifndef PEER_CLIENT_H
#define PEER_CLIENT_H

#include <clany/byte_array.hpp>
#include "socket.hpp"

_CLANY_BEGIN
struct Peer
{
    uint pid;
    string address;
    ushort port;
    bool is_connected;
};

class PeerClient : public TCPSocket{
public:
    using Ptr = shared_ptr<PeerClient>;

    PeerClient() = default;
    PeerClient(int sock, SockAddrIN addr, SockState state)
        : TCPSocket(sock, addr, state) {}

    // Message protocals
    void sendPieceUpdate(int piece);
    void sendAvailPieces(const ByteArray& bit_field);
    void requestBlock(int piece, int offset, int length);
    void cancelRequest(int piece, int offset, int length);
    void sendBlock(int piece, int offset, const ByteArray &data);

private:
};
_CLANY_END

#endif // PEER_CLIENT_H