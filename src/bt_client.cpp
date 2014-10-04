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
    down_up_taskgrp.run(
        [&]() { download(running[0]); }
    );

    down_up_taskgrp.run(
        [&]() { upload(running[1]); }
    );

    string input_str;
    while(getline(cin, input_str)) {
        char c = input_str[0];
        if (input_str.size() == 1 && c == 'q' || c == 'Q') {
            fill(begin(running), end(running), false);
            break;
        } else {
            ATOMIC_PRINT("Invalid input\n");
        }
    }
}

void BTClient::download(atomic<bool>& running)
{
    while (running) {
        // TODO
    }
}

void BTClient::upload(atomic<bool>& running)
{
    if (!server.listen(6767)) {
        ATOMIC_PRINT("Fail to establish uploading task\n");
    }
    ATOMIC_PRINT("Waiting for incoming message\n");

    while (running) {
        auto client_sock = server.nextPendingConnection();
        if (client_sock && handShake(client_sock)) {
            // TODO
        }
    }
}

bool BTClient::handShake(TCPSocket::Ptr client_sock)
{
    string buffer(FILE_SIZE_LIMITE, '\0');
    for (;;) {
        // Wait for incoming handshake message
        if (!hasIncomingData(client_sock)) continue;

        int num_bytes = 0, idx = 0;
        while ((num_bytes = ::recv(client_sock->sock(), &buffer[idx], BUF_LEN, 0)) > 0) {
            num_bytes = ::recv(client_sock->sock(), &buffer[idx], BUF_LEN, 0);
            idx += num_bytes;
        }
        if (idx != 68) {
            ATOMIC_PRINT("Handshake message is invalid, drop connection from %s\n", client_sock->peekAddress().c_str());
            // TODO
            // Drop connection
            return false;
        }
        buffer.resize(idx);
        break;
    }

    cout << buffer << endl;
    string recv_sha1 = buffer.substr(28, 20);
    if (recv_sha1 == meta_info.info_hash) {
        string peerid = to_string(pid);
        peerid.resize(20);
        auto handshake_msg = string(1, uchar(19)) + "BitTorrent Protocol" + string(8, '0') +
                             static_cast<string>(meta_info.info_hash) + peerid;
        if (!client_sock->write(handshake_msg)) {
            ATOMIC_PRINT("ERROR, fail to sendback handshake message!\n");
            return false;
        }
    }

    return true;
}

bool BTClient::hasIncomingData(TCPSocket::Ptr client_sock)
{
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(client_sock->sock(), &read_fds);
    timeval no_block {0, 0};

    return select(1, &read_fds, nullptr, nullptr, &no_block) > 0;
}