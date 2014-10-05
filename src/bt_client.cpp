#include <openssl/sha.h>
#include "clany/file_operation.hpp"
#include "bt_client.h"

#define ATOMIC_PRINT(format, ...) { \
  mutex::scoped_lock(print_mtx); \
  printf((format), __VA_ARGS__); \
}

using namespace std;
using namespace tbb;
using namespace clany;

namespace {
const size_t    FILE_SIZE_LIMITE = 10 * 1024 * 1024;    // 10mb
const int       BUF_LEN = 1024;
const int       HANDSHAKE_MSG_LEN = 68;
const llong     DEFAULT_CHUNK_SIZE = 100 * 1024 * 1024; // 100 MB
const ByteArray DEFAULT_CHUNK(DEFAULT_CHUNK_SIZE);

tbb::mutex print_mtx;
} // Unnamed namespace

bool BTClient::TmpFile::create(const string& file_name, llong file_size)
{
    fname = file_name;
    fsize = file_size;
    ofstream ofs(fname, ios::binary);
    if (!ofs) return false;

    for (auto i = 0u; i < fsize / DEFAULT_CHUNK_SIZE; ++i) {
        ofs.write(DEFAULT_CHUNK.data(), DEFAULT_CHUNK_SIZE);
    }
    int rest_size = file_size % DEFAULT_CHUNK_SIZE;
    ofs.write(ByteArray(rest_size).data(), rest_size);

    return true;
}

bool BTClient::TmpFile::write(size_t idx, const ByteArray& data) const
{
    fstream fs(fname, ios::binary | ios::in | ios::out);
    if (!fs) return false;

    fs.seekp(idx);
    fs.write(data.data(), data.size());

    return true;
}
//////////////////////////////////////////////////////////////////////////////////////////
bool BTClient::loadFile(const string& file_name)
{
    download_file.fname = file_name;
    ifstream ifs(file_name, ios::binary | ios::ate);

    if (!ifs) return false;
    download_file.fsize = ifs.tellg();
    ifs.seekg(0);   // Set input position to start

    // Check hash
    ByteArray piece(meta_info.piece_length);
    int idx = 0;
    while (++idx < meta_info.num_pieces) {
        ifs.read(piece.data(), piece.size());
        checkPiece(piece, idx - 1);
    };
    // Last piece
    piece.resize(meta_info.length % meta_info.piece_length);
    ifs.read(piece.data(), piece.size());
    checkPiece(piece, idx - 1);

    return true;
}

bool BTClient::checkPiece(const ByteArray& piece, int idx)
{
    ByteArray sha1(20);
    SHA1((uchar*)piece.data(), piece.size(), (uchar*)sha1.data());
    if (sha1 == meta_info.sha1_vec[idx]) pieces_status[idx] = 1;
    return pieces_status[idx] == 1;
}

bool BTClient::setTorrent(const string& torrent_name, const string& save_file_name)
{
    if (!download_file.empty()) {
        cerr << "Torrent already set!" << endl;
        return false;
    }

    MetaInfoParser parser;
    if (!parser.parse(readBinaryFile(torrent_name), meta_info)) return false;
    save_name = save_file_name;

    // Check if we have already downloaded the file
    pieces_status.resize(meta_info.num_pieces);
    fill(pieces_status.begin(), pieces_status.end(), -1);
    auto file_name = save_name.empty() ? meta_info.name : save_name;
    if (!loadFile(file_name)) {
        download_file.create(file_name, meta_info.length);
    }

    return true;
}

void BTClient::run()
{
    ATOMIC_PRINT("Starting Main Loop, press q/Q to exit the program\n");

    atomic<bool> running[2];
    fill(begin(running), end(running), true);

    task_group down_up_taskgrp;
//     down_up_taskgrp.run(
//         [&]() { download(running[0]); }
//     );

    down_up_taskgrp.run(
        [&]() { upload(running[1]); }
    );

    string input_str;
    while(getline(cin, input_str)) {
        char c = input_str[0];
        // Exit the program if user press q/Q
        if (input_str.size() == 1 && c == 'q' || c == 'Q') {
            fill(begin(running), end(running), false);
            // Wait for all tasks to terminate
            down_up_taskgrp.wait();
            break;
        } else {
            ATOMIC_PRINT("Invalid input\n");
        }
    }
}

void BTClient::download(atomic<bool>& running)
{
    if (!client.isValid()) {
        ATOMIC_PRINT("Fail to create socket for download task!\n");
        return;
    }
    while (running) {
        // Choose a peer from list
        this_thread::sleep_for(tick_count::interval_t(0.1));
        if (client.connect("192.168.1.100", 6767) && handShake(client)) {
            ATOMIC_PRINT("Establish connection from 192.168.1.100:6767\n")
        }
    }

    dnload_tasks.wait();
}

void BTClient::upload(atomic<bool>& running)
{
    if (!server.listen(6767)) {
        ATOMIC_PRINT("Fail to establish uploading task!\n");
        return;
    }
    ATOMIC_PRINT("Waiting for incoming request...\n");

    while (running) {
        // Sleep for 0.1s, prevent from using 100% CPU
        this_thread::sleep_for(tick_count::interval_t(0.1));
        auto client_sock = server.nextPendingConnection();
        if (client_sock && handShake(*client_sock, false)) {
            ATOMIC_PRINT("Accept connection from %s\n", client_sock->peekAddress().c_str());
            // TODO
        }
    }

    upload_tasks.wait();
}

bool BTClient::handShake(TCPSocket& client_sock, bool is_initiator)
{
    auto sendMessage = [this, &client_sock]() {
        string peerid = to_string(pid);
        peerid.resize(20);
        auto handshake_msg = string(1, uchar(19)) + "BitTorrent Protocol" + string(8, '0') +
                             static_cast<string>(meta_info.info_hash) + peerid;
        if (!client_sock.write(handshake_msg)) {
            ATOMIC_PRINT("ERROR, fail to sendback handshake message!\n");
            return false;
        }
        return true;
    };

    auto receiveMessge = [this, &client_sock](string& buffer) {
        recvMsg(client_sock, buffer);

        if (buffer.length() != HANDSHAKE_MSG_LEN) {
            ATOMIC_PRINT("Handshake message is invalid, drop connection from %s\n",
                         client_sock.peekAddress().c_str());
            // Drop connection
            // is it right to call client_sock.disconnect(); ?
            return false;
        }
        return true;
    };

    // initiator send handshake message first, then receive respond
    // recipient receive handshake message first, then send back respond
    string buffer;
    if (is_initiator) {
        if (!sendMessage()) return false;

        if (receiveMessge(buffer)) {
            // Extract info hash
            string recv_sha1 = buffer.substr(28, 20);
            if (recv_sha1 != meta_info.info_hash) return false;
        };
    }
    else {
        if (!receiveMessge(buffer)) return false;

        // cout << buffer << endl;
        // Extract info hash
        string recv_sha1 = buffer.substr(28, 20);
        if (recv_sha1 == meta_info.info_hash) {
            if (!sendMessage()) return false;
        }
    }

    return true;
}

bool BTClient::hasIncomingData(TCPSocket& client_sock) const
{
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(client_sock.sock(), &read_fds);
    timeval no_block {0, 0};

    return select(1, &read_fds, nullptr, nullptr, &no_block) > 0;
}

void BTClient::recvMsg(TCPSocket& client_sock, string& buffer) const
{
    buffer.resize(FILE_SIZE_LIMITE);
    for (;;) {
        // Wait for incoming handshake message
        this_thread::sleep_for(tick_count::interval_t(0.1));
        if (!hasIncomingData(client_sock)) continue;

        int num_bytes = 0, idx = 0;
        while (hasIncomingData(client_sock)) {
            num_bytes = ::recv(client_sock.sock(), &buffer[idx], BUF_LEN, 0);
            if (num_bytes <= 0) break;
            idx += num_bytes;
        }

        buffer.resize(idx);
        break;
    }
}