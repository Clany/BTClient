#ifndef BT_CLIENT_H
#define BT_CLIENT_H

#include <list>
#include <clany/clany_defs.h>
#include <tbb/tbb.h>
#include <tbb/compat/thread>
#include "peer_client.h"
#include "tcp_server.hpp"
#include "metainfo.h"

_CLANY_BEGIN
class BTClient : public TCPServer {
private:
    using atm_bool = tbb::atomic<bool>;
    using atm_int  = tbb::atomic<int>;

    // Preallocated temporary file on disk
    struct TmpFile {
        TmpFile() = default;
        TmpFile(const string& file_name, llong file_size) { create(file_name, file_size); }

        bool create(const string& file_name, llong file_size);
        bool write(size_t idx, const ByteArray& data) const;

        llong size() const { return fsize; }
        bool empty() const { return fsize == 0; }

        string fname = "";
        llong  fsize = 0;
    };

    // Search for peer clients, fill connection list
    auto getIncomingPeer() const -> PeerClient::Ptr;
    using TCPServer::listen;
    void listen(atm_bool& running);
    void initiate(atm_bool& running);
    void addPeerClient(PeerClient::Ptr peer);
    void removePeerClient(PeerClient::Ptr peer);

    // Mange download and upload tasks
    void download(atm_bool& running);
    void upload(atm_bool& running);

    bool handShake(TCPSocket& client_sock, bool is_initiator = true);
    bool hasIncomingData(TCPSocket& client_sock) const;
    void recvMsg(TCPSocket& client_sock, string& buffer) const;
    auto getBitField() const -> ByteArray;

    // Load existing (partial)downloaded file
    bool loadFile(const string& file_name);

    // Return true if SHA1 value of piece is correct, also update pieces status
    bool checkPiece(const ByteArray& piece, int idx);

public:
    using Ptr = shared_ptr<BTClient>;

    BTClient(uint peer_id) : pid(peer_id) {};

    bool setTorrent(const string& torrent_name, const string& save_file_name = "");

    auto getMetaInfo() -> const MetaInfo& { return meta_info; }

    void run();

private:
    list<Peer> peer_list;
    list<PeerClient::Ptr> connection_list;
    size_t max_connections;

    uint pid;

    MetaInfo meta_info;
    TmpFile download_file;
    vector<atm_int> pieces_status;

    string save_name;
    string log_name;

    tbb::task_group dnload_tasks;
    vector<atm_int> dlt_status;
    tbb::task_group upload_tasks;
    vector<atm_int> upt_status;
    size_t max_dnload_size = 4;
    size_t max_upload_size = 4;
};
_CLANY_END

#endif // BT_CLIENT_H