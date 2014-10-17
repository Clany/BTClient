#include <random>
#include <openssl/sha.h>
#include <clany/algorithm.hpp>
#include "bt_client.h"

#define ATOMIC_PRINT(format, ...) { \
  mutex::scoped_lock lock(print_mtx); \
  char buffer[256]; \
  sprintf(buffer, (format), ##__VA_ARGS__); \
  cout << buffer; \
}

using namespace std;
using namespace tbb;
using namespace cls;

namespace {
const size_t MSG_SIZE_LIMITE   = 1 * 1024 * 1024;    // 1mb
const int    HANDSHAKE_MSG_LEN = 68;
const int    MAX_TRYING_NUM    = 5;
const double SLEEP_INTERVAL    = 0.1;
const llong  FILE_CHUNK_SIZE   = 100 * 1024 * 1024; // 100 MB
const size_t BUFF_LEN          = 255;

const uint SEED = random_device()();
auto  rd_engine = default_random_engine(SEED);

tbb::mutex print_mtx;
tbb::mutex peer_list_mtx;
tbb::mutex connection_mtx;
tbb::mutex file_mtx;
} // Unnamed namespace

bool BTClient::TmpFile::create(const string& file_name, llong file_size)
{
    fname = file_name;
    fsize = file_size;
    ofstream ofs(fname, ios::binary);
    if (!ofs) return false;

    const ByteArray file_chunk(FILE_CHUNK_SIZE);
    for (auto i = 0u; i < fsize / FILE_CHUNK_SIZE; ++i) {
        ofs.write(file_chunk.data(), FILE_CHUNK_SIZE);
    }
    int rest_size = file_size % FILE_CHUNK_SIZE;
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

    data.resize(length);
    ifs.seekg(idx);
    ifs.read(data.data(), length);

    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
// BTClient interface
bool BTClient::setTorrent(const string& torrent_name, const string& save_file_name)
{
    if (!download_file.empty()) {
        cerr << "Torrent already set!" << endl;
        return false;
    }

    MetaInfoParser parser;
    if (!parser.parse(readBinaryFile(torrent_name), meta_info)) return false;
    save_name = save_file_name.empty() ? meta_info.name : save_file_name;

    // Initialize piece status (-1: not have, 0: downloading, 1: have)
    // and bitfield, load (partial)downloaded file if exist
    bit_field.resize(meta_info.num_pieces);
    pieces_status.resize(meta_info.num_pieces);
    fill(pieces_status.begin(), pieces_status.end(), -1);
    if (!loadFile(save_name)) {
        download_file.create(save_name, meta_info.length);
    }
    DBGVAR(cout, bit_field);

    return true;
}

void BTClient::run()
{
    ATOMIC_PRINT("Starting Main Loop, press q/Q to exit the program\n");
    if (bit_field.all()) {
        is_complete = true;
        ATOMIC_PRINT("Already have the file, now seeding\n");
    }

    atm_bool running[2];
    fill(begin(running), end(running), true);

    task_group search_peers;
    search_peers.run(
        [this, &running]() { initiate(running[0]); }
    );
    search_peers.run(
        [this, &running]() { listen(running[1]); }
    );

    // Get what we don't have now
    needed_piece.clear();
    needed_piece.reserve(meta_info.num_pieces);
    for (auto idx = 0; idx < meta_info.num_pieces; ++idx) {
        if (!bit_field[idx]) needed_piece.push_back(idx);
    }
    shuffle(needed_piece.begin(), needed_piece.end(), rd_engine);

    string input_str;
    while(getline(cin, input_str)) {
        char c = input_str[0];
        // Exit the program if user press q/Q
        if (input_str.size() == 1 && (c == 'q' || c == 'Q')) {
            fill(begin(running), end(running), false);
            for_each(connection_list, mem_fn(&PeerClient::stop));
            break;
        } else {
            ATOMIC_PRINT("Invalid input\n");
        }
    }

    // Wait for all tasks to terminate
    search_peers.wait();
    torrent_task.wait();

    log_file.second << log_buffer;
}

//////////////////////////////////////////////////////////////////////////////////////////
// BTClient private methods
auto BTClient::getIncomingPeer() -> PeerClient::Ptr
{
    SockAddrIN client_addr;
    socklen_t  addr_sz = sizeof(client_addr);
    memset(&client_addr, 0, addr_sz);
    if (hasPendingConnections()) {
        auto sock = ::accept(tcp_socket.sock(), (SockAddr*)&client_addr, &addr_sz);
        return PeerClient::Ptr(new PeerClient(meta_info, this, sock, client_addr,
                                              PeerClient::ConnectedState));
    }

    return nullptr;
}

void BTClient::listen(atm_bool& running)
{
    if (!listen(listenPort())) {
        ATOMIC_PRINT("Fail to listen on port %d\n", listenPort());
        return;
    }
    ATOMIC_PRINT("Waiting for incoming request...\n");

    while (running) {
        // Sleep for a short time, prevent from using 100% CPU
        this_tbb_thread::sleep(tick_count::interval_t(SLEEP_INTERVAL));

        // When download complete, drop connection from seeders
        if (is_complete) {
            for (const auto& peers : connection_list) {
                if (peers->isSeeder()) peers->stop();
            }
        }

        // Remove disconnected peer from connection list
        for (auto iter = connection_list.begin(); iter != connection_list.end();) {
            if (!(*iter)->isRunning()) {
                (*iter)->wait();
                auto peer_iter = find(peer_list.begin(), peer_list.end(),
                                      (*iter)->getPeerInfo());
                if (peer_iter != peer_list.end()) peer_iter->is_connected = false;
                char log_buffer[BUFF_LEN];
                sprintf(log_buffer, "Disconnected from %s", (*iter)->peekAddress().c_str());
                ATOMIC_PRINT("%s\n", log_buffer);
                writeLog(log_buffer);
                iter = connection_list.erase(iter);
                continue;
            }
            ++iter;
        }

        if (connection_list.size() > max_connections) continue;

        auto peer_client = getIncomingPeer();
        if (peer_client && handShake(peer_client.get(), false)) {
            // Skip if connection is duplicate
            if (!addPeerClient(peer_client)) continue;

            ATOMIC_PRINT("Accept connection from %s:%d\n",
                         peer_client->peekAddress().c_str(), peer_client->port());
            peer_client->sendAvailPieces(bit_field);

            // Start torrent task for this connection
            torrent_task.run([this, peer_client]() {
                peer_client->start();
            });
        }
    }
}

void BTClient::initiate(atm_bool& running)
{
    while (running && !is_complete) {
        // Sleep for a short time, prevent from using 100% CPU
        this_tbb_thread::sleep(tick_count::interval_t(1.0));
        if (connection_list.size() >= max_connections) continue;

        // Iterate peer list to find available connection
        for (auto& peer : peer_list) {
            if (peer.is_connected || !peer.is_available) continue;
            if (!running || is_complete) break;

            auto peer_client = make_shared<PeerClient>(meta_info, this);
            if (!peer_client->isValid()) {
                ATOMIC_PRINT("Fail to create socket for download task!\n");
                continue;
            }

            if (!peer_client->connect(peer.address, peer.port)) {
                ATOMIC_PRINT("Peer not available: %s:%d, trying %d/%d\n",
                             peer.address.c_str(), peer.port,
                             ++peer.trying_num, MAX_TRYING_NUM);
                if (peer.trying_num == MAX_TRYING_NUM) {
                    ATOMIC_PRINT("Reach max trying number, delete peer from list\n");
                    peer.is_available = false;
                }
                continue;
            }

            if (handShake(peer_client.get(), true)) {
                // Save peer_info, skip if connection is duplicate
                peer.pid = peer_client->getPeerInfo().pid;
                peer.is_connected = true;
                peer.trying_num   = 0;
                if (!addPeerClient(peer_client)) continue;

                ATOMIC_PRINT("Establish connection to %s:%d\n",
                             peer.address.c_str(), peer.port);
                peer_client->sendAvailPieces(bit_field);

                // Start torrent task for this connection
                torrent_task.run([this, peer_client]() {
                    peer_client->start();
                });
            }
            if (connection_list.size() >= max_connections) break;
        }
    }
}

bool BTClient::addPeerClient(PeerClient::Ptr peer_client)
{
    mutex::scoped_lock lock(connection_mtx);
    for (const auto& peer : connection_list) {
        if (*peer == *peer_client) return false;
    }
    connection_list.push_back(peer_client);
    return true;
}

void BTClient::addPeerInfo(const Peer& peer)
{
    mutex::scoped_lock lock(peer_list_mtx);
    peer_list.push_back(peer);
}

void BTClient::removePeerInfo(const Peer& peer)
{
    mutex::scoped_lock lock(peer_list_mtx);
    peer_list.remove(peer);
}

bool BTClient::handShake(PeerClient* client_sock, bool is_initiator)
{
    string peer_id;

    auto sendMessage = [this, &client_sock]() {
        string peerid = pid;
        peerid.resize(20);
        // 1 + 19 + 8 + 20 + 20
        auto handshake_msg = string(1, char(19)) + "BitTorrent Protocol" + string(8, '0') +
                             static_cast<string>(meta_info.info_hash) + peerid;
        if (!client_sock->write(handshake_msg)) {
            ATOMIC_PRINT("ERROR, fail to sendback handshake message!\n");
            return false;
        }
        return true;
    };

    auto receiveMessge = [this, &client_sock, &peer_id](ByteArray& buffer) {
        // Wait for incoming handshake message, drop connection if no response within 10s
        recvMsg(client_sock, buffer, HANDSHAKE_MSG_LEN, 10);

        auto recv_sha1 = buffer.sub(28, 20);
        if (recv_sha1 != meta_info.info_hash) {
            ATOMIC_PRINT("Handshake message is invalid, drop connection from %s\n",
                         client_sock->peekAddress().c_str());
            // Drop connection
            client_sock->disconnect();
            return false;
        }
        peer_id = buffer.sub(48, 20);
        client_sock->setPeerInfo({peer_id, client_sock->peekAddress(),
                                 client_sock->port(), true});
        return true;
    };

    // initiator send handshake message first, then receive respond
    // recipient receive handshake message first, then send back respond
    char log_buffer[255];
    sprintf(log_buffer, "HANDSHAKE INIT ip: %s, port: %d",
            client_sock->peekAddress().c_str(), client_sock->port());
    writeLog(log_buffer);

    ByteArray buffer;
    bool is_success = true;
    if (is_initiator) {
        if (!sendMessage()) is_success = false;
        if (!receiveMessge(buffer))  is_success = false;
    }
    else {
        if (!receiveMessge(buffer)) is_success = false;
        if (!sendMessage()) is_success = false;
    }

    if (is_success) {
        sprintf(log_buffer, "HANDSHAKE SUCCESS ip: %s:%d, pid: %s",
                client_sock->peekAddress().c_str(), client_sock->port(), peer_id.c_str());
        writeLog(log_buffer);
    } else {
        sprintf(log_buffer, "HANDSHAKE FAIL ip: %s:%d",
                client_sock->peekAddress().c_str(), client_sock->port());
        writeLog(log_buffer);
    }

    return is_success;
}

void BTClient::broadcastPU(int idx) const
{
    for (const auto& peer : connection_list) {
        peer->sendPieceUpdate(idx);
    }
}

bool BTClient::hasIncomingData(const TCPSocket* client_sock) const
{
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(client_sock->sock(), &read_fds);
    timeval no_block {0, 0};

    return ::select(client_sock->sock() + 1, &read_fds,
                    nullptr, nullptr, &no_block) > 0;
}

int BTClient::recvMsg(const TCPSocket* client_sock, char* buffer,
                       size_t msg_len, double time_out) const
{
    if (msg_len > MSG_SIZE_LIMITE) return 0;

    int count = 0;
    int max_count = time_out == 0 ?
        1 : static_cast<int>(ceil(time_out / SLEEP_INTERVAL));
    for (;;) {
        // Wait for incoming handshake message
        if (++count > max_count) return 0;
        this_tbb_thread::sleep(tick_count::interval_t(SLEEP_INTERVAL));
        if (!hasIncomingData(client_sock)) continue;

        int num_bytes = 0;
        size_t idx = 0;
        do {
            num_bytes = ::recv(client_sock->sock(), buffer + idx, msg_len - num_bytes, 0);
            if (num_bytes <= 0) return -1;
            idx += num_bytes;
        } while (hasIncomingData(client_sock) && idx < msg_len);

        if (idx == msg_len) return 1;
        return 0;
    }
}

int BTClient::recvMsg(const TCPSocket* client_sock, ByteArray& buffer,
                       size_t msg_len, double time_out) const
{
    buffer.resize(msg_len);
    return recvMsg(client_sock, buffer.data(), msg_len, time_out);
}

int BTClient::recvMsg(const TCPSocket* client_sock, string& buffer,
                       size_t msg_len, double time_out) const
{
    buffer.resize(msg_len);
    return recvMsg(client_sock, &buffer[0], msg_len, time_out);
}

ByteArray BTClient::getBlock(int piece, int offset, int length) const
{
    // Return empty data if we don't have this piece
    if (!bit_field[piece]) return ByteArray();

    mutex::scoped_lock lock(file_mtx);
    if (offset + length > meta_info.piece_length) {
        length = meta_info.piece_length - offset;
    }
    ByteArray data;
    download_file.read(piece*meta_info.piece_length + offset, length, data);
    return data;
}

ByteArray BTClient::getBlock(const ByteArray& block_header) const
{
    auto header = reinterpret_cast<const int*>(block_header.data());
    return getBlock(header[0], header[1], header[2]);
}

void BTClient::writeBlock(int piece, int offset, const ByteArray& block_data)
{
    downloaded += block_data.size();
    download_file.write(piece*meta_info.piece_length + offset, block_data);
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
        bit_field[idx] = 1;
        pieces_status[idx] = 1;
    }
    return bit_field[idx];
}