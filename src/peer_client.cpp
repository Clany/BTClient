#include <clany/clany_defs.h>
#include "peer_client.h"
#include "bt_client.h"

using namespace std;
using namespace tbb;
using namespace cls;

#define ATOMIC_PRINT(format, ...) { \
  mutex::scoped_lock lock(print_mtx); \
  char buffer[256]; \
  sprintf(buffer, (format), ##__VA_ARGS__); \
  cout << buffer; \
}

#define THREAD_SLEEP(interval) \
  this_tbb_thread::sleep(tick_count::interval_t((interval)))

namespace {
struct MsgHeader {
    union {
        struct {
            int   length;
            uchar msg_id;
        };
        char data[5];
    };
    MsgHeader(int len, uchar id)
        : length(len), msg_id(id) {}
};

struct BlockHeader {
    union {
        struct {
            int piece_idx;
            int offset;
            int length;
        };
        char data[12];
    };
    BlockHeader(int idx, int begin, int len)
        : piece_idx(idx), offset(begin), length(len) {}
};

const double SLEEP_INTERVAL   = 0.01;
const int MSG_SIZE_LIMITE = 1024 * 1024;  // 1mb
const size_t BLOCK_CHUNK_SIZE = 32 * 1024;   // 32kb
const size_t BUFF_LEN = 255;

tbb::mutex print_mtx;
} // Unnamed namespace

void PeerClient::listen()
{
    int piece_idx = -1;
    int last_piece_len = static_cast<int>(torrent_info.length -
                        (torrent_info.num_pieces - 1) * torrent_info.piece_length);
    ByteArray piece;
    int piece_width = to_string(torrent_info.num_pieces).size();
    int data_width  = to_string(torrent_info.length / 0x100000).size() + 3;
    while (running && state() != UnconnectedState) {
        THREAD_SLEEP(SLEEP_INTERVAL);

        if (am_choking) continue;

        ByteArray buffer;
        int retval = bt_client->recvMsg(this, buffer, 5, 0);
        if (!retval) continue;

        if (retval < 0) {
            stop();
            break;
        }

        int   msg_len = *reinterpret_cast<int*>(buffer.data()) - 1;
        uchar msg_id = buffer[4];
        if (msg_len < 0 || msg_len > MSG_SIZE_LIMITE) {
            ATOMIC_PRINT("Message header is invalid\n");
            stop();
            break;
        }

        buffer.resize(msg_len);
        if (msg_len != 0 && bt_client->recvMsg(this, buffer, msg_len) < 0) {
            stop();
            break;
        }

        switch (msg_id) {
        case PeerClient::CHOKE:
            sprintf(log_buffer, "MESSAGE CHOKE FROM %s", addr_id.c_str());
            peer_choking = true;
            break;
        case PeerClient::UNCHOKE:
            sprintf(log_buffer, "MESSAGE UNCHOKE FROM %s", addr_id.c_str());
            peer_choking = false;
            break;
        case PeerClient::INTERESTED:
            sprintf(log_buffer, "MESSAGE INTERESTED FROM %s", addr_id.c_str());
            peer_interested = true;
            sendChoke(am_choking);
            break;
        case PeerClient::NOT_INTERESTED:
            sprintf(log_buffer, "MESSAGE NOT_INTERESTED FROM %s", addr_id.c_str());
            peer_interested = false;
            break;
        case PeerClient::HAVE:
            updatePiece(buffer, bt_client->needed_piece);
            break;
        case PeerClient::BITFIELD:
            setBitField(buffer, bt_client->needed_piece);
            break;
        case PeerClient::REQUEST:
            handleRequest(buffer);
            break;
        case PeerClient::CANCEL:
            // Cancel download task
            break;
        case PeerClient::PIECE:
            receiveBlock(buffer);
            piece_idx = *reinterpret_cast<int*>(buffer.data());
            piece += buffer.sub(8);
            break;
        default:
            ATOMIC_PRINT("Unknown message ID\n");
            stop();
            break;
        }

        bt_client->writeLog(log_buffer);

        if ((piece_idx == torrent_info.num_pieces - 1 &&
            piece.size() == (size_t)last_piece_len) ||
            piece.size() == (size_t)torrent_info.piece_length) {
            if (bt_client->validatePiece(piece, piece_idx)) {
                bt_client->broadcastPU(piece_idx);
                int piece_num = bt_client->bit_field.count();
                float dn_mb = bt_client->downloaded / 1024.f / 1024.f;
                float up_mb = bt_client->uploaded   / 1024.f / 1024.f;

                ATOMIC_PRINT("Piece %*d from %s, progress: %5.2f%%, "
                             "downloaded: %*.2f MB, uploaded: %*.2f MB\n",
                             piece_width, piece_idx, addr.c_str(),
                             100.0 *  piece_num / torrent_info.num_pieces,
                             data_width, dn_mb, data_width, up_mb);
                if (piece_num == torrent_info.num_pieces) {
                    ATOMIC_PRINT("Download complete, now seeding. Press q/Q to quit\n");
                    bt_client->is_complete = true;
                }
            }

            piece_idx = -1;
            piece.clear();
        }
    }
}

void PeerClient::request()
{
    auto blocks_per_piece = torrent_info.piece_length / BLOCK_CHUNK_SIZE;
    int last_piece_len = static_cast<int>(torrent_info.length -
                         (torrent_info.num_pieces - 1) * torrent_info.piece_length);
    auto& p_status = bt_client->pieces_status;
    auto& idx_vec = bt_client->needed_piece;

    while (running && state() != UnconnectedState) {
        THREAD_SLEEP(SLEEP_INTERVAL);

        if (peer_choking || !am_interested) continue;

        // Find a piece to download
        auto idx_iter = find_if(idx_vec.begin(), idx_vec.end(), [this, &p_status](int idx) {
            return hasPiece(idx) &&
                   p_status[idx].compare_and_swap(0, -1) < 0;
        });
        if (idx_iter == idx_vec.end()) continue;
        int idx = *idx_iter;

        // Send download request, handle last piece separately
        if (idx == torrent_info.num_pieces - 1) {
            uint iter_num = last_piece_len / BLOCK_CHUNK_SIZE;
            for (auto i = 0u; i < iter_num; ++i) {
                requestBlock(idx, i*BLOCK_CHUNK_SIZE, BLOCK_CHUNK_SIZE);
            }
            uint final_len = last_piece_len % BLOCK_CHUNK_SIZE;
            requestBlock(idx, last_piece_len - final_len, final_len);
        } else {
            for (auto i = 0u; i < blocks_per_piece; ++i) {
                requestBlock(idx, i*BLOCK_CHUNK_SIZE, BLOCK_CHUNK_SIZE);
            }
        }

        // Wait until we've got the requested piece, no longer than 20s
        double time_out = 20.0 / SLEEP_INTERVAL;
        for (auto count = 0; count < time_out; ++count) {
            THREAD_SLEEP(SLEEP_INTERVAL);
            if (p_status[idx] || !running || state() == UnconnectedState) break;
        }

        // Revert piece status if we didn't get that piece
        p_status[idx].compare_and_swap(-1, 0);
    }
}

void PeerClient::start()
{
    char buffer[BUFF_LEN];
    sprintf(buffer, "%s:%5d, pid: %s",
            peer_info.address.c_str(), peer_info.port, peer_info.pid);
    addr_id = buffer;
    addr = addr_id.substr(0, addr_id.find(','));

    peer_task.run([&]() { listen();  });
    peer_task.run([&]() { request(); });
    wait();
    stop();
}

bool PeerClient::sendChoke(bool choking) const
{
    char log_buffer[BUFF_LEN];

    ByteArray msg;
    if (choking) {
        MsgHeader msg_header {1, CHOKE};
        msg = ByteArray(msg_header.data, 5);
        sprintf(log_buffer, "MESSAGE CHOKE TO %s", addr_id.c_str());
    } else {
        MsgHeader msg_header {1, UNCHOKE};
        msg = ByteArray(msg_header.data, 5);
        sprintf(log_buffer, "MESSAGE UNCHOKE TO %s", addr_id.c_str());
    }

    bt_client->writeLog(log_buffer);

    return write(msg);
}

bool PeerClient::sendInterested(bool interested) const
{
    char log_buffer[BUFF_LEN];

    ByteArray msg;
    if (interested) {
        MsgHeader msg_header {1, INTERESTED};
        msg = ByteArray(msg_header.data, 5);
        sprintf(log_buffer, "MESSAGE INTERESTED TO %s", addr_id.c_str());
    } else {
        MsgHeader msg_header {1, NOT_INTERESTED};
        msg = ByteArray(msg_header.data, 5);
        sprintf(log_buffer, "MESSAGE NOT_INTERESTED TO %s", addr_id.c_str());
    }

    bt_client->writeLog(log_buffer);

    return write(msg);
}

bool PeerClient::sendPieceUpdate(int piece) const
{
    MsgHeader msg_header {5, HAVE};
    auto msg = ByteArray(msg_header.data, 5) +
               ByteArray(reinterpret_cast<char*>(&piece), sizeof(int));

    char log_buffer[BUFF_LEN];
    sprintf(log_buffer, "MESSAGE HAVE TO %s, piece: %d", addr_id.c_str(), piece);
    bt_client->writeLog(log_buffer);

    return write(msg);
}

bool PeerClient::sendAvailPieces(const BitField& bit_field) const
{
    // Do no send if we have no piece
    if (bit_field.none()) return false;

    ByteArray payload    {bit_field.toByteArray()};
    MsgHeader msg_header {1 + static_cast<int>(payload.size()), BITFIELD};
    auto msg = ByteArray(msg_header.data, 5) + payload;

    char log_buffer[BUFF_LEN];
    int avail_num = bit_field.count();
    sprintf(log_buffer, "MESSAGE BITFIELD TO %s, avail: %d, not avail: %d",
            addr_id.c_str(), avail_num, bit_field.size() - avail_num);
    bt_client->writeLog(log_buffer);

    return write(msg);
}

bool PeerClient::requestBlock(int piece, int offset, int length) const
{
    MsgHeader   msg_header {13, REQUEST};
    BlockHeader blk_header {piece, offset, length};
    auto msg = ByteArray(msg_header.data, 5) + ByteArray(blk_header.data, 12);

    char log_buffer[BUFF_LEN];
    sprintf(log_buffer, "MESSAGE REQUEST TO %s, piece: %d, offset: %d, length: %d",
            addr_id.c_str(), piece, offset, length);
    bt_client->writeLog(log_buffer);

    return write(msg);
}

bool PeerClient::cancelRequest(int piece, int offset, int length) const
{
    MsgHeader   msg_header {13, CANCEL};
    BlockHeader blk_header {piece, offset, length};
    auto msg = ByteArray(msg_header.data, 5) + ByteArray(blk_header.data, 12);

    char log_buffer[BUFF_LEN];
    sprintf(log_buffer, "MESSAGE CANCEL TO %s, piece: %d, offset: %d, length: %d",
            addr_id.c_str(), piece, offset, length);
    bt_client->writeLog(log_buffer);

    return write(msg);
}

bool PeerClient::sendBlock(int piece, int offset, const ByteArray& data) const
{
    MsgHeader   msg_header {9 + static_cast<int>(data.size()), PIECE};
    BlockHeader blk_header {piece, offset, 0};
    auto msg = ByteArray(msg_header.data, 5) + ByteArray(blk_header.data, 8) + data;

    char log_buffer[BUFF_LEN];
    sprintf(log_buffer, "MESSAGE PIECE TO %s, piece: %d, offset: %d, length: %d",
            addr_id.c_str(), piece, offset, data.size());
    bt_client->writeLog(log_buffer);

    return write(msg);
}

void PeerClient::setBitField(const ByteArray& buffer, const vector<int>& needed_piece)
{
    size_t bf_sz = torrent_info.num_pieces;
    bit_field.fromByteArray(bf_sz, buffer);

    int avail_num = bit_field.count();
    sprintf(log_buffer, "MESSAGE BITFIELD FROM %s, avail: %d, not avail: %d",
            addr_id.c_str(), avail_num, bf_sz - avail_num);

    auto iter = find_if(needed_piece.begin(), needed_piece.end(), [this](int idx) {
        return bit_field.test(idx);
    });
    if (iter != needed_piece.end()) am_interested = true;

    sendInterested(am_interested);
}

void PeerClient::updatePiece(const ByteArray& buffer, const vector<int>& needed_piece)
{
    int idx = *reinterpret_cast<const int*>(buffer.data());
    bit_field[idx] = 1;

    sprintf(log_buffer, "MESSAGE HAVE FROM %s, piece: %d", addr_id.c_str(), idx);

    if (!am_interested) {
        auto iter = find_if(needed_piece.begin(), needed_piece.end(), [this](int idx) {
            return bit_field.test(idx);
        });
        if (iter != needed_piece.end()) {
            am_interested = true;
            sendInterested(am_interested);
        }
    }
}

void PeerClient::handleRequest(const ByteArray& request_msg)
{
    auto blk_header = reinterpret_cast<const int*>(request_msg.data());
    int piece_idx = blk_header[0];
    int offset = blk_header[1];
    int length = blk_header[2];

    sprintf(log_buffer, "MESSAGE REQUEST FROM %s, piece: %d, offset: %d, length: %d",
            addr_id.c_str(), piece_idx, offset, length);

    if (!bt_client->bit_field[piece_idx]) {
        peer_task.run([=]() {
            cancelRequest(piece_idx, offset, length);
        });
    } else {
        peer_task.run([=]() {
            auto data = bt_client->getBlock(request_msg);
            if (sendBlock(piece_idx, offset, data)) bt_client->uploaded += data.size();
        });
    }
}

void PeerClient::receiveBlock(const ByteArray& buffer)
{
    auto blk_header = reinterpret_cast<const int*>(buffer.data());
    int piece_idx = blk_header[0];
    int offset    = blk_header[1];
    int length    = buffer.size() - 8;

    sprintf(log_buffer, "MESSAGE PIECE FROM %s, piece: %d, offset: %d, length: %d",
            addr_id.c_str(), piece_idx, offset, length);

    bt_client->writeBlock(piece_idx, offset, buffer.sub(8));
}