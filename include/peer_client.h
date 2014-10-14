#ifndef PEER_CLIENT_H
#define PEER_CLIENT_H

#include <clany/dyn_bitset.hpp>
#include "metainfo.h"
#include "socket.hpp"
#include <tbb/tbb.h>

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

inline bool operator<(const Peer& left, const Peer& right)
{
    return left.pid < right.pid;
}

inline bool operator>(const Peer& left, const Peer& right)
{
    return left.pid > right.pid;
}

class BTClient;

class PeerClient : public TCPSocket{
    using atm_bool = tbb::atomic<bool>;
    using atm_int = tbb::atomic<int>;

    void listen(BTClient* bt_client);
    void request(BTClient* bt_client);

public:
    using Ptr = shared_ptr<PeerClient>;
    enum { HAVE = 4, BITFIELD = 5, REQUEST = 6, CANCEL = 8, PIECE = 7 };

    PeerClient(const MetaInfo& meta_info)
        : torrent_info(meta_info), bit_field(meta_info.num_pieces),
          piece_avail(false) {
        running = true;
    };
    PeerClient(const MetaInfo& meta_info, int sock, SockAddrIN addr, SockState state)
        : TCPSocket(sock, addr, state), torrent_info(meta_info),
          bit_field(meta_info.num_pieces), piece_avail(false) {
        running = true;
    }

    void setPeerInfo(const Peer& info) {
        peer_info = info;
    }
    Peer getPeerInfo() const {
        return peer_info;
    }

    bool hasPiece(int idx) const {
        return bit_field[idx];
    }

    void start(BTClient* bt_client);
    void stop() { running = false; }
    void wait() { peer_task.wait(); }
    bool isRunning() const { return running; }

    // Message protocals
    // have: <len=0005><id=4><piece index>
    bool sendPieceUpdate(int piece) const;
    // bitfield: <len=0001+X><id=5><bitfield>
    bool sendAvailPieces(const BitField& bit_field) const;
    // request: <len=0013><id=6><index><begin><length>
    bool requestBlock(int piece, int offset, int length) const;
    // cancel: <len=0013><id=8><index><begin><length>
    bool cancelRequest(int piece, int offset, int length) const;
    // piece: <len=0009+X><id=7><index><begin><block>
    bool sendBlock(int piece, int offset, const ByteArray& data) const;

    void setBitField(const ByteArray& buffer);
    void updatePiece(const ByteArray& buffer);
    void handleRequest(const ByteArray& request_msg, BTClient* bt_client);

private:
    const MetaInfo& torrent_info;
    atm_bool running;
//    tbb::task_scheduler_init ts_init;
    tbb::task_group peer_task;

    Peer peer_info;
    BitField bit_field;
    bool piece_avail;
};

inline bool operator==(const PeerClient& left, const PeerClient& right)
{
    return left.getPeerInfo() == right.getPeerInfo();
}
_CLANY_END

#endif // PEER_CLIENT_H