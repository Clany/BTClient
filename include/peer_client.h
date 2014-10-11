#ifndef PEER_CLIENT_H
#define PEER_CLIENT_H

#include <clany/dyn_bitset.hpp>
#include "metainfo.h"
#include "socket.hpp"

_CLANY_BEGIN
struct Peer
{
    string pid;
    string address;
    ushort port;
    bool is_connected;
};

inline bool operator==(const Peer& left, const Peer& right)
{
    return left.pid == right.pid &&
           left.address == right.address &&
           left.port == right.port;
}

inline bool operator!=(const Peer& left, const Peer& right)
{
    return !(left == right);
}

class PeerClient : public TCPSocket{
public:
    using Ptr = shared_ptr<PeerClient>;
    enum { HAVE = 4, BITFIELD = 5, REQUEST = 6, CANCEL = 8, PIECE = 7 };

    PeerClient(const MetaInfo& meta_info) : torrent_info(meta_info) {};
    PeerClient(const MetaInfo& meta_info, int sock, SockAddrIN addr, SockState state)
        : TCPSocket(sock, addr, state), torrent_info(meta_info) {}

    void setPeerInfo(const Peer& info) {
        peer_info = info;
    }
    Peer getPeerInfo() const {
        return peer_info;
    }

    bool hasPiece(int idx) const {
        return bit_field[idx];
    }

    // Message protocals
    // have: <len=0005><id=4><piece index>
    void sendPieceUpdate(int piece) const;
    // bitfield: <len=0001+X><id=5><bitfield>
    void sendAvailPieces(const BitField& bit_field) const;
    // request: <len=0013><id=6><index><begin><length>
    void requestBlock(int piece, int offset, int length) const;
    // cancel: <len=0013><id=8><index><begin><length>
    void cancelRequest(int piece, int offset, int length) const;
    // piece: <len=0009+X><id=7><index><begin><block>
    void sendBlock(int piece, int offset, const ByteArray& data) const;

    void setBitField(const ByteArray& buffer);
    void updatePiece(const ByteArray& buffer);

private:
    const MetaInfo& torrent_info;

    Peer peer_info;
    BitField bit_field;
};
_CLANY_END

#endif // PEER_CLIENT_H