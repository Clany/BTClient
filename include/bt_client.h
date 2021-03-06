#ifndef BT_CLIENT_H
#define BT_CLIENT_H

#include <list>
#include <fstream>
#include <chrono>
#include <clany/file_operation.hpp>
#include "peer_client.h"
#include "tcp_server.hpp"
#include "metainfo.h"

_CLANY_BEGIN
class BTClient : public TCPServer {
    friend class PeerClient;

    using atm_bool = tbb::atomic<bool>;
    using atm_int  = tbb::atomic<int>;

    // Preallocated temporary file on disk
    struct TmpFile {
        TmpFile() = default;
        TmpFile(const string& file_name, llong file_size) { create(file_name, file_size); }

        bool create(const string& file_name, llong file_size);
        bool write(size_t idx, const ByteArray& data) const;
        bool read(size_t idx, size_t length, ByteArray& data) const;

        llong size() const { return fsize; }
        bool empty() const { return fsize == 0; }

        string fname = "";
        llong  fsize = 0;
    };

    // Search for peer clients, fill connection list
    using TCPServer::listen;
    void listen(atm_bool& running);
    void initiate(atm_bool& running);
    bool addPeerClient(PeerClient::Ptr peer_client);
    void addPeerInfo(const Peer& peer);
    void removePeerClient(PeerClient::Ptr peer_client);
    void removePeerInfo(const Peer& peer);

    // Mange torrent task
    auto getIncomingPeer() -> PeerClient::Ptr;
    bool handShake(PeerClient* peer_client, bool is_initiator);
    void broadcastPU(int piece_idx) const;

    bool hasIncomingData(const TCPSocket* client_sock) const;
    int recvMsg(const TCPSocket* client_sock, char* buffer,
                size_t msg_len = string::npos, double time_out = 3.0) const;
    int recvMsg(const TCPSocket* client_sock, ByteArray& buffer,
                size_t msg_len = string::npos, double time_out = 3.0) const;
    int recvMsg(const TCPSocket* client_sock, string& buffer,
                size_t msg_len = string::npos, double time_out = 3.0) const;

    auto getBlock(int piece, int offset, int length) const -> ByteArray;
    auto getBlock(const ByteArray& block_header) const -> ByteArray;
    void writeBlock(int piece, int offset, const ByteArray& block_data);

    // Load existing (partial) downloaded file
    bool loadFile(const string& file_name);

    // Return true if SHA1 value of piece is correct, update pieces accordingly
    bool validatePiece(const ByteArray& piece, int idx);

public:
    using Ptr = shared_ptr<BTClient>;

    BTClient(const string& peer_id, const string& ip = "", int16_t port = 6767)
        : max_connections(4), ts_init(16), pid(peer_id),
          start(chrono::system_clock::now()) {
        // Set peer id to bt_client:port if not provided
        listen_port = port;
        local_addr  = ip;
        if (pid.empty()) pid = string("bt_client") + ":" + to_string(listen_port);

        downloaded = 0;
        uploaded   = 0;
    };

    bool setTorrent(const string& torrent_name, const string& save_file_name = "");

    bool setLogFile(const string& file_name);

    void writeLog(const string& message);

    void setMaxConnection(int max_connections) {
        this->max_connections = max_connections;
    }

    void addPeerAddr(const string& address, ushort port) {
        // Peer ID, ip, port, is_connected, is_available, trying times
        peer_list.push_back({"", address, port, false, true, 0});
    }

    auto getMetaInfo() -> const MetaInfo& { return meta_info; }

    void run();

private:
    list<Peer> peer_list;
    list<PeerClient::Ptr> connection_list;
    size_t max_connections;
    tbb::task_scheduler_init ts_init;
    tbb::task_group torrent_task;

    string pid;

    MetaInfo meta_info;
    TmpFile download_file;
    BitField bit_field;
    vector<atm_int> pieces_status;
    vector<int> needed_piece;

    string save_name;
    pair<string, ofstream> log_file;
    string log_buffer;
    tbb::mutex log_mtx;
    chrono::time_point<chrono::system_clock> start;

    atm_int downloaded;
    atm_int uploaded;
    bool is_complete = false;
    bool verbose     = false;
};
_CLANY_END

#endif // BT_CLIENT_H