#include "peer_client.h"

using namespace std;
using namespace tbb;
using namespace clany;

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

const size_t MSG_HEADER_SIZE = 5;
} // Unnamed namespace

void PeerClient::sendPieceUpdate(int/* piece*/) const
{
    // TODO
}

void PeerClient::sendAvailPieces(const BitField& bit_field) const
{
    // Do no send if we have no piece
    if (bit_field.none()) return;

    ByteArray payload    { bit_field.toByteArray() };
    MsgHeader msg_header {1 + payload.size(), BITFIELD};
    auto msg = ByteArray(msg_header.data, MSG_HEADER_SIZE) + payload;
    write(msg);
}

void PeerClient::requestBlock(int piece, int offset, int length) const
{
    MsgHeader   msg_header {13, REQUEST};
    BlockHeader blk_header {piece, offset, length};
    auto msg = ByteArray(msg_header.data, MSG_HEADER_SIZE) + ByteArray(blk_header.data, 12);
    write(msg);
}

void PeerClient::cancelRequest(int piece, int offset, int length) const
{
    MsgHeader   msg_header {13, CANCEL};
    BlockHeader blk_header {piece, offset, length};
    auto msg = ByteArray(msg_header.data, MSG_HEADER_SIZE) + ByteArray(blk_header.data, 12);
    write(msg);
}

void PeerClient::sendBlock(int piece, int offset, const ByteArray& data) const
{
    MsgHeader   msg_header {9 + data.size(), PIECE};
    BlockHeader blk_header {piece, offset, 0};
    auto msg = ByteArray(msg_header.data, MSG_HEADER_SIZE) + ByteArray(blk_header.data, 8) + data;
    write(msg);
}

void PeerClient::setBitField(const ByteArray& buffer)
{
    mutex::scoped_lock(bf_mtx);
    bit_field.fromByteArray(torrent_info.num_pieces, buffer);
}

void PeerClient::updatePiece(const ByteArray& buffer)
{
    mutex::scoped_lock(bf_mtx);
    int idx = *reinterpret_cast<const int*>(buffer.data());
    bit_field[idx] = 1;
}