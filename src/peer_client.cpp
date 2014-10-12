#include <clany/clany_defs.h>
#include <tbb/compat/thread>
#include "peer_client.h"
#include "bt_client.h"

using namespace std;
using namespace tbb;
using namespace clany;

#define ATOMIC_PRINT(format, ...) { \
  mutex::scoped_lock lock(print_mtx); \
  printf((format), __VA_ARGS__); \
}

namespace {
struct MsgHeader {
    union {
        struct {
            int   length;
            uchar msg_id;
        };
        char data[5];
    };
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
};

const double SLEEP_INTERVAL   = 0.1;
const size_t MSG_LEN_LIMIT    = 1024 * 1024;  // 1mb
const size_t BLOCK_CHUNK_SIZE = 16 * 1024;   // 16kb

tbb::mutex print_mtx;
} // Unnamed namespace

void PeerClient::listen(BTClient* bt_client)
{
    auto blocks_per_piece = torrent_info.piece_length / BLOCK_CHUNK_SIZE;
    uint blocks_num = 0;
    ByteArray piece;
    while (running /*&& connestion is valid*/) {
        ByteArray buffer;
        if (!bt_client->recvMsg(this, buffer, sizeof(MsgHeader), 0)) {
            // Sleep for a short time, prevent from using 100% CPU
            this_thread::sleep_for(tick_count::interval_t(SLEEP_INTERVAL));
            continue;
        }

        int   msg_len = *reinterpret_cast<int*>(buffer.data()) - 1;
        uchar msg_id = buffer[4];

        buffer.resize(msg_len);
        if (msg_len < 0 || msg_len > MSG_LEN_LIMIT ||
            !bt_client->recvMsg(this, buffer, msg_len)) continue;

        switch (msg_id) {
        case PeerClient::BITFIELD:
            setBitField(buffer);
            break;
        case PeerClient::HAVE:
            updatePiece(buffer);
            break;
        case PeerClient::REQUEST:
            sendBlock(buffer, bt_client->getBlock(buffer));
            break;
        case PeerClient::CANCEL:
            // Cancel download task
            break;
        case PeerClient::PIECE:
            bt_client->writeBlock(buffer);
            piece += buffer;
            ++blocks_num;
            break;
        default:
            ATOMIC_PRINT("Unknown message ID\n");
            break;
        }
        if (blocks_num == blocks_per_piece) {
            int piece_idx = *reinterpret_cast<int*>(buffer.data());
            if (bt_client->validatePiece(piece, piece_idx)) {
                sendPieceUpdate(piece_idx);
            };

            blocks_num = 0;
            piece.clear();
        }
    }
}

void PeerClient::download(BTClient* bt_client)
{
    auto blocks_per_piece = torrent_info.piece_length / BLOCK_CHUNK_SIZE;
    auto& p_status = bt_client->pieces_status;
    auto& idx_vec = bt_client->needed_piece;
    auto idx_iter = idx_vec.begin();

    while (running/*&& connestion is valid*/) {
        // Sleep for a short time, prevent from using 100% CPU
        this_thread::sleep_for(tick_count::interval_t(SLEEP_INTERVAL));

        // Break the loop if we've got all the pieces
        if (bt_client->bit_field.all()) break;

        if (!piece_avail) continue;

        // Find a piece to download
        idx_iter = find_if(idx_iter, idx_vec.end(), [this, &p_status](int idx) {
            return hasPiece(idx) &&
                   p_status[idx].compare_and_swap(0, -1) < 0;
        });
        if (idx_iter == idx_vec.end()) continue;
        int idx = *idx_iter;

        // Send download request, handle last piece separately
        if (idx == torrent_info.num_pieces) {
            // TODO
        }
        for (auto i = 0u; i < blocks_per_piece; ++i) {
            requestBlock(idx, i*BLOCK_CHUNK_SIZE, BLOCK_CHUNK_SIZE);
        }

        // Wait until we've got the requested piece, no longer than 30s
        double time_out = 30.0 / SLEEP_INTERVAL;
        for (auto count = 0; count < time_out; ++count) {
            this_thread::sleep_for(tick_count::interval_t(SLEEP_INTERVAL));
            if (p_status[idx] || !running) break;
        }

        // Revert piece status if we didn't get that piece
        p_status[idx].compare_and_swap(-1, 0);
    }
}

void PeerClient::start(BTClient* bt_client)
{
    parallel_invoke(
        [&]() { listen(bt_client); },
        [&]() { download(bt_client); }
    );
}

void PeerClient::sendPieceUpdate(int piece) const
{
    MsgHeader msg_header {5, HAVE};
    auto msg = ByteArray(msg_header.data, 5) +
               ByteArray(reinterpret_cast<char*>(&piece), sizeof(int));
    write(msg);
}

void PeerClient::sendAvailPieces(const BitField& bit_field) const
{
    // Do no send if we have no piece
    if (bit_field.none()) return;

    ByteArray payload    {bit_field.toByteArray()};
    MsgHeader msg_header {1 + payload.size(), BITFIELD};
    auto msg = ByteArray(msg_header.data, 5) + payload;
    write(msg);
}

void PeerClient::requestBlock(int piece, int offset, int length) const
{
    MsgHeader   msg_header {13, REQUEST};
    BlockHeader blk_header {piece, offset, length};
    auto msg = ByteArray(msg_header.data, 5) + ByteArray(blk_header.data, 12);
    write(msg);
}

void PeerClient::cancelRequest(int piece, int offset, int length) const
{
    MsgHeader   msg_header {13, CANCEL};
    BlockHeader blk_header {piece, offset, length};
    auto msg = ByteArray(msg_header.data, 5) + ByteArray(blk_header.data, 12);
    write(msg);
}

void PeerClient::sendBlock(int piece, int offset, const ByteArray& data) const
{
    MsgHeader   msg_header {9 + data.size(), PIECE};
    BlockHeader blk_header {piece, offset, 0};
    auto msg = ByteArray(msg_header.data, 5) + ByteArray(blk_header.data, 8) + data;
    write(msg);
}

void PeerClient::sendBlock(const ByteArray& request_msg, const ByteArray& data) const
{
    auto blk_header = reinterpret_cast<const int*>(request_msg.data());
    int piece_idx   = blk_header[0];
    int offset      = blk_header[1];
    int length      = blk_header[2];
    if (data.empty()) {
        cancelRequest(piece_idx, offset, length);
    } else {
        sendBlock(piece_idx, offset, data);
    }
}

void PeerClient::setBitField(const ByteArray& buffer)
{
    size_t bf_sz = torrent_info.num_pieces;
    bit_field.fromByteArray(bf_sz, buffer);
    piece_avail = true;
}

void PeerClient::updatePiece(const ByteArray& buffer)
{
    int idx = *reinterpret_cast<const int*>(buffer.data());
    bit_field[idx] = 1;
}