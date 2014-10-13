#include <clany/clany_defs.h>
#include <tbb/compat/thread>
#include "peer_client.h"
#include "bt_client.h"

using namespace std;
using namespace tbb;
using namespace clany;

#define ATOMIC_PRINT(format, ...) { \
  mutex::scoped_lock lock(print_mtx); \
  printf((format), ##__VA_ARGS__); \
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
const int MSG_LEN_LIMIT       = 1024 * 1024;  // 1mb
const size_t BLOCK_CHUNK_SIZE = 32 * 1024;   // 16kb

tbb::mutex print_mtx;
} // Unnamed namespace

void PeerClient::listen(BTClient* bt_client)
{
    int piece_idx = -1;
    ByteArray piece;
    while (running && state() != UnconnectedState) {
        ByteArray buffer;
        int retval = bt_client->recvMsg(this, buffer, 5, 0);
        if (!retval) {
            // Sleep for a short time, prevent from using 100% CPU
            this_thread::sleep_for(tick_count::interval_t(SLEEP_INTERVAL));
            continue;
        }
        if (retval < 0) {
            disconnect();
            break;
        }

        int   msg_len = *reinterpret_cast<int*>(buffer.data()) - 1;
        uchar msg_id = buffer[4];
        if (msg_len < 0 || msg_len > MSG_LEN_LIMIT) continue;

        buffer.resize(msg_len);
        if (bt_client->recvMsg(this, buffer, msg_len) < 0) {
            disconnect();
            break;
        }

        switch (msg_id) {
        case PeerClient::BITFIELD:
            setBitField(buffer);
            break;
        case PeerClient::HAVE:
            updatePiece(buffer);
            break;
        case PeerClient::REQUEST:
            handleRequest(buffer, bt_client);
            break;
        case PeerClient::CANCEL:
            // Cancel download task
            break;
        case PeerClient::PIECE:
            bt_client->writeBlock(buffer);
            piece_idx = *reinterpret_cast<int*>(buffer.data());
            piece += buffer.sub(8);
            break;
        default:
            ATOMIC_PRINT("Unknown message ID\n");
            break;
        }
        if (piece.size() == torrent_info.piece_length ||
            piece_idx == torrent_info.num_pieces - 1) {
            if (bt_client->validatePiece(piece, piece_idx)) {
                sendPieceUpdate(piece_idx);
                ATOMIC_PRINT("Piece %d from %s download complete, progress: %.2f%%\n",
                piece_idx, peekAddress().c_str(),
                100.0 * bt_client->bit_field.count() / torrent_info.num_pieces);
            };

            piece.clear();
        }
    }
}

void PeerClient::download(BTClient* bt_client)
{
    auto blocks_per_piece = torrent_info.piece_length / BLOCK_CHUNK_SIZE;
    int last_piece_len = static_cast<int>(torrent_info.length -
                         (torrent_info.num_pieces - 1) * torrent_info.piece_length);
    auto& p_status = bt_client->pieces_status;
    auto& idx_vec = bt_client->needed_piece;
    auto idx_iter = idx_vec.begin();

    while (running && state() != UnconnectedState) {
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
        if (idx == torrent_info.num_pieces - 1) {
            requestBlock(idx, 0, last_piece_len);
        } else {
            for (auto i = 0u; i < blocks_per_piece; ++i) {
                requestBlock(idx, i*BLOCK_CHUNK_SIZE, BLOCK_CHUNK_SIZE);
            }
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
//    parallel_invoke(
//        [&]() { listen(bt_client); },
//        [&]() { download(bt_client); }
//    );
    thread in(mem_fn(&PeerClient::listen), this, bt_client);
    thread out(mem_fn(&PeerClient::download), this, bt_client);
    in.join();
    out.join();
    running = false;
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

void PeerClient::handleRequest(const ByteArray& request_msg, BTClient* bt_client) const
{
    auto data = bt_client->getBlock(request_msg);

    auto blk_header = reinterpret_cast<const int*>(request_msg.data());
    int piece_idx   = blk_header[0];
    int offset      = blk_header[1];
    int length      = blk_header[2];
    if (data.empty()) {
        thread cancel([&]() {
            sendBlock(piece_idx, offset, length);
        });
        cancel.detach();
    } else {
        thread send([&, data]() {
            sendBlock(piece_idx, offset, data);
        });
        send.detach();
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