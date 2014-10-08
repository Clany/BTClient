#ifndef PEER_CLIENT_H
#define PEER_CLIENT_H

#include <clany/dyn_bitset.hpp>
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
    enum { HAVE = 4, BITFIELD = 5, REQUEST = 6, CANCEL = 8, PIECE = 7 };

    PeerClient() = default;
    PeerClient(int sock, SockAddrIN addr, SockState state)
        : TCPSocket(sock, addr, state) {}

    // Message protocals
    // have: <len=0005><id=4><piece index>
    void sendPieceUpdate(int piece);
    // bitfield: <len=0001+X><id=5><bitfield>
    void sendAvailPieces(const BitField& bit_field);
    // request: <len=0013><id=6><index><begin><length>
    void requestBlock(int piece, int offset, int length);
    // cancel: <len=0013><id=8><index><begin><length>
    void cancelRequest(int piece, int offset, int length);
    // piece: <len=0009+X><id=7><index><begin><block>
    void sendBlock(int piece, int offset, const ByteArray& data);

    void parseMsg(const string& buffer, int& msg_id, string& payload);

private:
};
_CLANY_END

#endif // PEER_CLIENT_H