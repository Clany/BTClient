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
    bool is_available;
    int  trying_times;
};

inline bool operator==(const Peer& left, const Peer& right)
{
    return left.pid == right.pid &&
           left.address == right.address;
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

    friend bool operator==(const PeerClient& left, const PeerClient& right);

    void listen();
    void request();

public:
    using Ptr = shared_ptr<PeerClient>;
    enum { CHOKE = 0, UNCHOKE = 1, INTERESTED = 2, NOT_INTERESTED = 3,
           HAVE = 4, BITFIELD = 5, REQUEST = 6, CANCEL = 8, PIECE = 7 };

    PeerClient(const MetaInfo& meta_info, BTClient* torrent_client)
        : torrent_info(meta_info), bit_field(meta_info.num_pieces),
          bt_client(torrent_client), am_choking(false), am_interested(false),
          peer_choking(true), peer_interested(false) {
        running = true;
    };
    PeerClient(const MetaInfo& meta_info, BTClient* torrent_client,
               int sock, SockAddrIN addr, SockState state)
        : TCPSocket(sock, addr, state), torrent_info(meta_info),
          bt_client(torrent_client), bit_field(meta_info.num_pieces),
          am_choking(false), am_interested(false),
          peer_choking(true), peer_interested(false) {
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

    void start();
    void stop() { running = false; }
    void wait() { peer_task.wait(); }
    bool isRunning() const { return running; }
    bool isSeeder()  const { return bit_field.all(); }

    // Message protocals
    // choke/unchoke: <len=0001><id=0/id=1>
    bool sendChoke(bool choking) const;
    // interested/not interested: <len=0001><id=2/id=3>
    bool sendInterested(bool interested) const;
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

private:
    void setBitField(const ByteArray& buffer, const vector<int>& needed_piece);
    void updatePiece(const ByteArray& buffer, const vector<int>& needed_piece);
    void handleRequest(const ByteArray& request_msg);
    void receiveBlock(const ByteArray& buffer);

    BTClient* bt_client;

    const MetaInfo& torrent_info;
    atm_bool running;
    tbb::task_group peer_task;

    string addr;
    string addr_id;
    char log_buffer[255];

    Peer peer_info;
    BitField bit_field;
    bool am_choking;
    bool am_interested;
    bool peer_choking;
    bool peer_interested;
};

inline bool operator==(const PeerClient& left, const PeerClient& right)
{
    return left.peer_info.pid == right.peer_info.pid &&
           left.peer_info.address == right.peer_info.address;
}
_CLANY_END

#endif // PEER_CLIENT_H