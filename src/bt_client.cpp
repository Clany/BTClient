#include <openssl/sha.h>
#include <clany/file_operation.hpp>
#include <clany/timer.hpp>
#include "bt_client.h"

#define ATOMIC_PRINT(format, ...) { \
  mutex::scoped_lock(print_mtx); \
  printf((format), __VA_ARGS__); \
}

using namespace std;
using namespace tbb;
using namespace clany;

namespace {
const ushort    LISTEN_PORT       = 6767;
const size_t    FILE_SIZE_LIMITE  = 10 * 1024 * 1024;    // 10mb
const int       BUF_LEN           = 1024;
const int       HANDSHAKE_MSG_LEN = 68;
const double    SLEEP_INTERVAL    = 0.3;

const llong     DEFAULT_CHUNK_SIZE = 100 * 1024 * 1024; // 100 MB
const ByteArray DEFAULT_CHUNK(DEFAULT_CHUNK_SIZE);

tbb::mutex print_mtx;
tbb::mutex connection_mtx;
tbb::mutex file_mtx;
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

bool BTClient::TmpFile::read(size_t idx, size_t length, ByteArray& data) const
{
    ifstream ifs(fname, ios::binary);
    if (!ifs) return false;

    ifs.seekg(idx);
    ifs.read(data.data(), length);

    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
// BTClient interface
bool BTClient::setTorrent(const string& torrent_name, const string& save_file_name)
{
    //////////////////////////////////////////////////////////////////////////////////////////
    // Hard code parameters
    peer_list.push_back({150, "192.168.1.100", 6767, false});
    max_connections = 8;
    //////////////////////////////////////////////////////////////////////////////////////////

    if (!download_file.empty()) {
        cerr << "Torrent already set!" << endl;
        return false;
    }

    MetaInfoParser parser;
    if (!parser.parse(readBinaryFile(torrent_name), meta_info)) return false;
    save_name = save_file_name;

    // Initialize piece status (bitfield), load (partial)downloaded file if exist
    bit_field.resize(meta_info.num_pieces);
    pieces_status.resize(meta_info.num_pieces);
    fill(pieces_status.begin(), pieces_status.end(), -1);
    auto file_name = save_name.empty() ? meta_info.name : save_name;
    if (!loadFile(file_name)) {
        download_file.create(file_name, meta_info.length);
    }
    DBGVAR(cout, bit_field);

    return true;
}

void BTClient::run()
{
    ATOMIC_PRINT("Starting Main Loop, press q/Q to exit the program\n");

    atomic<bool> running[4];
    fill(begin(running), end(running), true);

    task_group search_peers;
//     search_peers.run(
//         [this, &running]() { initiate(running[1]); }
//     );
    search_peers.run(
        [this, &running]() { listen(running[0]); }
    );

    // Sleep for 1s, then initialize download/upload tasks
    this_thread::sleep_for(tick_count::interval_t(1.0));
    task_group down_up_tasks;
//     down_up_tasks.run(
//         [this, &running]() { download(running[2]); }
//     );
//     down_up_tasks.run(
//         [this, &running]() { upload(running[3]); }
//     );

    string input_str;
    while(getline(cin, input_str)) {
        char c = input_str[0];
        // Exit the program if user press q/Q
        if (input_str.size() == 1 && c == 'q' || c == 'Q') {
            fill(begin(running), end(running), false);
            // Wait for all tasks to terminate
            search_peers.wait();
            down_up_tasks.wait();
            break;
        } else {
            ATOMIC_PRINT("Invalid input\n");
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// BTClient private methods
auto BTClient::getIncomingPeer() const -> PeerClient::Ptr
{
    SockAddrIN client_addr;
    socklen_t  addr_sz = sizeof(client_addr);
    memset(&client_addr, 0, addr_sz);
    if (hasPendingConnections()) {
        auto sock = ::accept(tcp_socket.sock(), (SockAddr*)&client_addr, &addr_sz);
        return PeerClient::Ptr(new PeerClient(sock, client_addr,
                                              PeerClient::ConnectedState));
    }

    return nullptr;
}

void BTClient::listen(atm_bool& running)
{
    if (!listen(LISTEN_PORT)) {
        ATOMIC_PRINT("Fail to establish peer searching task!\n");
        return;
    }
    ATOMIC_PRINT("Waiting for incoming request...\n");

    while (running) {
        // Sleep for 0.3s, prevent from using 100% CPU
        this_thread::sleep_for(tick_count::interval_t(SLEEP_INTERVAL));
        if (connection_list.size() > max_connections) continue;

        auto peer_client = getIncomingPeer();
        if (peer_client && handShake(*peer_client, false)) {
            ATOMIC_PRINT("Accept connection from %s\n",
                         peer_client->peekAddress().c_str());
            peer_client->sendAvailPieces(bit_field);
            addPeerClient(peer_client);
        }
    }
}

void BTClient::initiate(atm_bool& running)
{
    while (running) {
        if (connection_list.size() >= max_connections) continue;

        // Iterate peer list to find available connection
        for (auto& peer : peer_list) {
            if (peer.is_connected) continue;

            auto peer_client = make_shared<PeerClient>();
            if (!peer_client->isValid()) {
                ATOMIC_PRINT("Fail to create socket for download task!\n");
                continue;
            }

            if (peer_client->connect(peer.address, peer.port) &&
                handShake(*peer_client)) {
                ATOMIC_PRINT("Establish connection from %s:%d\n",
                             peer.address.c_str(), peer.port);
                peer_client->sendAvailPieces(bit_field);
                addPeerClient(peer_client);
                peer.is_connected = true;
            }
        }
        // Re-scan every 30 seconds
        this_thread::sleep_for(tick_count::interval_t(30.0));
    }
}

void BTClient::addPeerClient(PeerClient::Ptr peer)
{
    mutex::scoped_lock(connection_mtx);
    connection_list.push_back(peer);
}

void BTClient::removePeerClient(PeerClient::Ptr peer)
{
    mutex::scoped_lock(connection_mtx);
    connection_list.remove(peer);
}

void BTClient::download(atm_bool& running)
{
    while (running) {
        // Sleep for 0.3s, prevent from using 100% CPU
        this_thread::sleep_for(tick_count::interval_t(SLEEP_INTERVAL));
        if (connection_list.empty()) continue;

#ifndef NDEBUG
        for_each(connection_list.begin(), connection_list.end(),
#else
        parallel_for_each(connection_list.begin(), connection_list.end(),
#endif
                          [this](PeerClient::Ptr connection) {
        });
    }
}

void BTClient::upload(atm_bool& running)
{
    while (running) {
        // Sleep for 0.3s, prevent from using 100% CPU
        this_thread::sleep_for(tick_count::interval_t(SLEEP_INTERVAL));
        if (connection_list.empty()) continue;

#ifndef NDEBUG
        for_each(connection_list.begin(), connection_list.end(),
#else
        parallel_for_each(connection_list.begin(), connection_list.end(),
#endif
                 [this](PeerClient::Ptr connection) {
            string msg;
            recvMsg(*connection, msg, string::npos, 0);
            if (msg.empty()) return;

            int msg_id;
            string payload;
            connection->parseMsg(msg, msg_id, payload);
            if (msg_id == PeerClient::REQUEST && payload.size() == 12) {
                auto blk_header = reinterpret_cast<const int*>(payload.c_str());
                int piece_idx = blk_header[0];
                int offset    = blk_header[1];
                int length    = blk_header[2];
                if (bit_field[piece_idx]) {
                    connection->sendBlock(piece_idx, offset, getBlock(blk_header));
                } else {
                    connection->cancelRequest(piece_idx, offset, length);
                }
            }
        });
    }
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
            if (recv_sha1 != meta_info.info_hash.to_string()) return false;
        };
    }
    else {
        if (!receiveMessge(buffer)) return false;

        // cout << buffer << endl;
        // Extract info hash
        string recv_sha1 = buffer.substr(28, 20);
        if (recv_sha1 == meta_info.info_hash.to_string()) {
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

void BTClient::recvMsg(TCPSocket& client_sock, string& buffer,
                       size_t n, double time_out) const
{
    int count = 0;
    int max_count = time_out == 0 ?
                    1 : static_cast<int>(ceil(time_out / SLEEP_INTERVAL));
    buffer.resize(FILE_SIZE_LIMITE);
#ifndef NDEBUG
    ScopeTimer timer;
#endif
    for (;;) {
        // Wait for incoming handshake message
        if (++count > max_count) break;
        this_thread::sleep_for(tick_count::interval_t(SLEEP_INTERVAL));
        if (!hasIncomingData(client_sock)) continue;

        int num_bytes = 0;
        size_t idx = 0;
        while (hasIncomingData(client_sock)) {
            num_bytes = ::recv(client_sock.sock(), &buffer[idx], BUF_LEN, 0);
            if (num_bytes <= 0) break;
            idx += num_bytes;
            if (idx >= n) break;
        }

        buffer.resize(idx);
        break;
    }
}

ByteArray BTClient::getBlock(int piece, int offset, int length) const
{
    mutex::scoped_lock(file_mtx);
    ByteArray data;
    download_file.read(piece*meta_info.piece_length + offset, length, data);
    return data;
}

ByteArray BTClient::getBlock(const int* block_header) const
{
    return getBlock(block_header[0], block_header[1], block_header[2]);
}

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
        validatePiece(piece, idx - 1);
    };
    // Last piece
    piece.resize(meta_info.length % meta_info.piece_length);
    ifs.read(piece.data(), piece.size());
    validatePiece(piece, idx - 1);

    return true;
}

bool BTClient::validatePiece(const ByteArray& piece, int idx)
{
    ByteArray sha1(20);
    SHA1((uchar*)piece.data(), piece.size(), (uchar*)sha1.data());
    if (sha1 == meta_info.sha1_vec[idx]) {
        pieces_status[idx] = 1;
        bit_field[idx]     = 1;
    }
    return pieces_status[idx] == 1;
}