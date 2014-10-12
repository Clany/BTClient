#include <random>
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
const ushort LISTEN_PORT       = 6767;
const size_t MSG_SIZE_LIMITE   = 1 * 1024 * 1024;    // 1mb
const int    BUF_LEN           = 1024;
const int    HANDSHAKE_MSG_LEN = 68;
const int    MSG_HEADER_LEN = 5;
const double SLEEP_INTERVAL    = 0.3;
const size_t BLOCK_CHUNK_SIZE = 16 * 1024;

const llong     FILE_CHUNK_SIZE = 100 * 1024 * 1024; // 100 MB
const ByteArray FILE_CHUNK(FILE_CHUNK_SIZE);

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

    for (auto i = 0u; i < fsize / FILE_CHUNK_SIZE; ++i) {
        ofs.write(FILE_CHUNK.data(), FILE_CHUNK_SIZE);
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
    //////////////////////////////////////////////////////////////////////////////////////////
    // Hard code parameters
    max_connections = 8;
    //////////////////////////////////////////////////////////////////////////////////////////

    if (!download_file.empty()) {
        cerr << "Torrent already set!" << endl;
        return false;
    }

    MetaInfoParser parser;
    if (!parser.parse(readBinaryFile(torrent_name), meta_info)) return false;
    save_name = save_file_name.empty() ? meta_info.name : save_file_name;

    // Initialize piece status (bitfield), load (partial)downloaded file if exist
    bit_field.resize(meta_info.num_pieces);
    pieces_status.resize(meta_info.num_pieces);
    fill(pieces_status.begin(), pieces_status.end(), -1);
    if (!loadFile(save_name)) {
        download_file.create(save_name, meta_info.length);
    }
    DBGVAR(cout, bit_field);

    return true;
}

void BTClient::addPeerAddr(const string& address, ushort port)
{
    peer_list.push_back({"", address, port, false});
}

void BTClient::run()
{
    ATOMIC_PRINT("Starting Main Loop, press q/Q to exit the program\n");

    atomic<bool> running[4];
    fill(begin(running), end(running), true);

    task_group search_peers;
    search_peers.run(
        [this, &running]() { initiate(running[0]); }
    );
    search_peers.run(
        [this, &running]() { listen(running[1]); }
    );

    // Get what we don't have now
    vector<int> idx_vec;
    idx_vec.reserve(meta_info.num_pieces);
    for (auto idx = 0; idx < meta_info.num_pieces; ++idx) {
        if (!bit_field[idx]) idx_vec.push_back(idx);
    }
    shuffle(idx_vec.begin(), idx_vec.end(), rd_engine);

    torrent_task;
    torrent_task.run(
        [this, &running, &idx_vec]() { download(running[2], idx_vec); }
    );
    torrent_task.run(
        [this, &running]() { handleMsg(running[3]); }
    );

    string input_str;
    while(getline(cin, input_str)) {
        char c = input_str[0];
        // Exit the program if user press q/Q
        if (input_str.size() == 1 && c == 'q' || c == 'Q') {
            fill(begin(running), end(running), false);
            // Wait for all tasks to terminate
            search_peers.wait();
            torrent_task.wait();
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
        return PeerClient::Ptr(new PeerClient(meta_info, sock, client_addr,
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
            addPeerInfo(peer_client->getPeerInfo());
            addPeerClient(peer_client);
        }
    }
}

void BTClient::initiate(atm_bool& running)
{
    while (running) {
        // Sleep for 0.3s, prevent from using 100% CPU
        this_thread::sleep_for(tick_count::interval_t(SLEEP_INTERVAL));
        if (connection_list.size() >= max_connections) continue;

        // Iterate peer list to find available connection
        for (auto& peer : peer_list) {
            if (peer.is_connected) continue;

            auto peer_client = make_shared<PeerClient>(meta_info);
            if (!peer_client->isValid()) {
                ATOMIC_PRINT("Fail to create socket for download task!\n");
                continue;
            }

            if (peer_client->connect(peer.address, peer.port) &&
                handShake(*peer_client)) {
                ATOMIC_PRINT("Establish connection from %s:%d\n",
                             peer.address.c_str(), peer.port);
                peer_client->sendAvailPieces(bit_field);
                peer.pid = peer_client->getPeerInfo().pid;
                addPeerClient(peer_client);
                peer.is_connected = true;
            }
            if (connection_list.size() == max_connections) break;
        }
    }
}

void BTClient::addPeerClient(PeerClient::Ptr peer_client)
{
    mutex::scoped_lock(connection_mtx);
    connection_list.push_back(peer_client);
}

void BTClient::removePeerClient(PeerClient::Ptr peer_client)
{
    mutex::scoped_lock(connection_mtx);
    connection_list.remove(peer_client);
}

void BTClient::addPeerInfo(const Peer& peer)
{
    mutex::scoped_lock(peer_list_mtx);
    peer_list.push_back(peer);
}

void BTClient::removePeerInfo(const Peer& peer)
{
    mutex::scoped_lock(peer_list_mtx);
    peer_list.remove(peer);
}

void BTClient::download(atm_bool& running, const vector<int>& idx_vec)
{
    mutex idx_mtx;
    auto blocks_per_piece = meta_info.piece_length / BLOCK_CHUNK_SIZE;
    while (running) {
        // Break the loop if we've got all the pieces
        if (bit_field.all()) break;

        parallel_for_each(connection_list.begin(), connection_list.end(),
                          [&](PeerClient::Ptr peer) {
            auto idx_iter = idx_vec.begin();

            while (running/*&& connection is valid*/) {
                if (!peer->bitFieldAvail()) continue;

                // Find a piece to download
                idx_mtx.lock();
                idx_iter = find_if(idx_iter, idx_vec.end(), [this, &peer](int idx) {
                    return peer->hasPiece(idx) &&
                           pieces_status[idx] < 0;
                });
                int idx = *idx_iter;
                pieces_status[idx] = 0;
                idx_mtx.unlock();

                // Try to download the piece
                for (auto i = 0u; i < blocks_per_piece; ++i) {
                    peer->requestBlock(idx, i*BLOCK_CHUNK_SIZE, BLOCK_CHUNK_SIZE);
                }

                // Wait until we've downloaded the requested piece, no longer than 30s
                for (auto count = 0; count < 100; ++count) {
                    this_thread::sleep_for(tick_count::interval_t(SLEEP_INTERVAL));
                    if (pieces_status[idx] || !running) break;
                }

                // Revert piece status if we didn't get that piece
                if (pieces_status[idx] == 0) pieces_status[idx] = -1;

                // Sleep for 0.3s, prevent from using 100% CPU
                this_thread::sleep_for(tick_count::interval_t(SLEEP_INTERVAL));
            }
        });
    }
}

void BTClient::handleMsg(atm_bool& running)
{
    auto blocks_per_piece = meta_info.piece_length / BLOCK_CHUNK_SIZE;
    uint block_num = 0;
    while (running) {
        // Sleep for 0.3s, prevent from using 100% CPU
        this_thread::sleep_for(tick_count::interval_t(SLEEP_INTERVAL));
        parallel_for_each(connection_list.begin(), connection_list.end(),
                          [&](PeerClient::Ptr connection) {
            while (running /*&& connection is valid*/) {
                // Sleep for 0.3s, prevent from using 100% CPU
                this_thread::sleep_for(tick_count::interval_t(SLEEP_INTERVAL));

                ByteArray buffer;
                if (!recvMsg(*connection, buffer, MSG_HEADER_LEN, 0)) continue;

                int   msg_len = *reinterpret_cast<int*>(buffer.data()) - 1;
                uchar msg_id = buffer[4];

                buffer.resize(msg_len);
                if (!recvMsg(*connection, buffer, msg_len, 0)) continue;

                switch (msg_id) {
                case PeerClient::BITFIELD:
                    connection->setBitField(buffer);
                    break;
                case PeerClient::HAVE:
                    connection->updatePiece(buffer);
                    break;
                case PeerClient::REQUEST:
                    sendBlock(*connection, buffer);
                    break;
                case PeerClient::CANCEL:
                    // Cancel download task
                    break;
                case PeerClient::PIECE:
                    writeBlock(buffer);
                    ++block_num;
                    break;
                default:
                    ATOMIC_PRINT("Unknown message ID\n");
                    break;
                }
                if (block_num == blocks_per_piece) {
                    block_num = 0;
                    int piece_idx = *reinterpret_cast<int*>(buffer.data());
                    ByteArray piece(meta_info.piece_length);
                    mutex::scoped_lock(file_mtx);
                    download_file.read(piece_idx * meta_info.piece_length,
                                       meta_info.piece_length, piece);
                    validatePiece(piece, piece_idx);
                }
            }
        });
    }
}

bool BTClient::handShake(PeerClient& client_sock, bool is_initiator)
{
    auto sendMessage = [this, &client_sock]() {
        string peerid = pid;
        peerid.resize(20);
        // 1 + 19 + 8 + 20 + 20
        auto handshake_msg = string(1, char(19)) + "BitTorrent Protocol" + string(8, '0') +
                             static_cast<string>(meta_info.info_hash) + peerid;
        if (!client_sock.write(handshake_msg)) {
            ATOMIC_PRINT("ERROR, fail to sendback handshake message!\n");
            return false;
        }
        return true;
    };

    auto receiveMessge = [this, &client_sock](ByteArray& buffer) {
        // Wait for incoming handshake message, drop connection if no response within 10s
        recvMsg(client_sock, buffer, HANDSHAKE_MSG_LEN, 10);

        if (buffer.size() != HANDSHAKE_MSG_LEN) {
            ATOMIC_PRINT("Handshake message is invalid, drop connection from %s\n",
                         client_sock.peekAddress().c_str());
            // Drop connection
            client_sock.disconnect();
            return false;
        }
        client_sock.setPeerInfo({buffer.sub(48, 20), client_sock.peekAddress(),
                                 client_sock.port(), true});
        return true;
    };

    // initiator send handshake message first, then receive respond
    // recipient receive handshake message first, then send back respond
    ByteArray buffer;
    if (is_initiator) {
        if (!sendMessage()) return false;

        if (receiveMessge(buffer)) {
            // Extract info hash
            auto recv_sha1 = buffer.sub(28, 20);
            if (recv_sha1 != meta_info.info_hash) return false;
        };
    }
    else {
        if (!receiveMessge(buffer)) return false;

        // Extract info hash
        auto recv_sha1 = buffer.sub(28, 20);
        if (recv_sha1 == meta_info.info_hash) {
            if (!sendMessage()) return false;
        }
    }

    return true;
}

bool BTClient::hasIncomingData(const TCPSocket& client_sock) const
{
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(client_sock.sock(), &read_fds);
    timeval no_block {0, 0};

    return ::select(1, &read_fds, nullptr, nullptr, &no_block) > 0;
}

bool BTClient::recvMsg(const TCPSocket& client_sock, char* buffer,
                       size_t msg_len, double time_out) const
{
    if (msg_len > MSG_SIZE_LIMITE) return false;

    int count = 0;
    int max_count = time_out == 0 ?
        1 : static_cast<int>(ceil(time_out / SLEEP_INTERVAL));
    for (;;) {
        // Wait for incoming handshake message
        if (++count > max_count) return false;
        this_thread::sleep_for(tick_count::interval_t(SLEEP_INTERVAL));
        if (!hasIncomingData(client_sock)) continue;

        int num_bytes = 0;
        size_t idx = 0;
        do {
            num_bytes = ::recv(client_sock.sock(), buffer + idx, msg_len - num_bytes, 0);
            if (num_bytes <= 0) break;
            idx += num_bytes;
        } while (hasIncomingData(client_sock) && idx < msg_len);

        if (idx == msg_len) return true;
        return false;
    }
}

bool BTClient::recvMsg(const TCPSocket& client_sock, ByteArray& buffer,
                       size_t msg_len, double time_out) const
{
    buffer.resize(msg_len);
    return recvMsg(client_sock, buffer.data(), msg_len, time_out);
}

bool BTClient::recvMsg(const TCPSocket& client_sock, string& buffer,
                       size_t msg_len, double time_out) const
{
    buffer.resize(msg_len);
    return recvMsg(client_sock, &buffer[0], msg_len, time_out);
}

ByteArray BTClient::getBlock(int piece, int offset, int length) const
{
    mutex::scoped_lock(file_mtx);
    if (offset + length > meta_info.piece_length) {
        length = meta_info.piece_length - offset;
    }
    ByteArray data;
    download_file.read(piece*meta_info.piece_length + offset, length, data);
    return data;
}

ByteArray BTClient::getBlock(const int* block_header) const
{
    return getBlock(block_header[0], block_header[1], block_header[2]);
}

void BTClient::sendBlock(const PeerClient& peer_client,
                         const ByteArray& request_msg) const
{
    auto blk_header = reinterpret_cast<const int*>(request_msg.data());
    int piece_idx = blk_header[0];
    int offset = blk_header[1];
    int length = blk_header[2];
    if (bit_field[piece_idx]) {
        peer_client.sendBlock(piece_idx, offset, getBlock(blk_header));
    } else {
        peer_client.cancelRequest(piece_idx, offset, length);
    }
}

void BTClient::writeBlock(const ByteArray& block_msg)
{
    auto blk_header = reinterpret_cast<const int*>(block_msg.data());
    int piece_idx = blk_header[0];
    int offset = blk_header[1];

    download_file.write(piece_idx*meta_info.piece_length + offset, block_msg.sub(8));
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