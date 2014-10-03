#ifndef BT_CLIENT_H
#define BT_CLIENT_H

#include "socket.hpp"
#include "metainfo.h"
#include <tbb/tbb.h>

_CLANY_BEGIN
// struct ByteArray : vector<char> {
//    ByteArray() = default;
//    ByteArray(const string& str)
//        : vector<char>(str.begin(), str.end()) {}
//
//    operator string() const {
//        return string(begin(), end());
//    }
//};
using ByteArray = string;

class BTClient {
private:
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

    // Load existing (partial)downloaded file
    bool loadFile(const string& file_name);

    // Return true if SHA1 value of piece is correct, also update pieces status
    bool checkPiece(const string& piece, int idx);

    void download();

    void upload();

public:
    using Ptr = shared_ptr<BTClient>;

    BTClient(int peer_id) : pid(peer_id) {};

    bool setTorrent(const string& torrent_name, const string& save_file_name = "");

    auto getMetaInfo() -> const MetaInfo& { return meta_info; }

    void run();

private:
    TCPSocket sock;
    uint pid;

    MetaInfo meta_info;
    TmpFile download_file;
    vector<tbb::atomic<int>> pieces_status;

    string save_name;
    string log_name;

    tbb::task_group dnload_tasks;
    tbb::task_group upload_tasks;
};
_CLANY_END

#endif // BT_CLIENT_H