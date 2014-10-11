#include "peer_client.h"

using namespace std;
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
} // Unnamed namespace

void PeerClient::sendPieceUpdate(int/* piece*/) const
{
    // TODO
}

void PeerClient::sendAvailPieces(const BitField& bit_field) const
{
    // Do no send if we have no piece
    if (bit_field.none()) return;

    string payload = bit_field.toByteArray();
    MsgHeader header {1 + payload.length(), HAVE};
    string msg = string(header.data) + static_cast<string>(payload);
    write(msg);
}

void PeerClient::requestBlock(int piece, int offset, int length) const
{
    MsgHeader   msg_header {13, REQUEST};
    BlockHeader blk_header {piece, offset, length};
    string msg = string(msg_header.data) + string(blk_header.data, 12);
    write(msg);
}

void PeerClient::cancelRequest(int piece, int offset, int length) const
{
    MsgHeader   msg_header {13, CANCEL};
    BlockHeader blk_header {piece, offset, length};
    string msg = string(msg_header.data) + string(blk_header.data, 12);
    write(msg);
}

void PeerClient::sendBlock(int piece, int offset, const ByteArray& data) const
{
    string payload = data;
    MsgHeader   msg_header {9 + payload.length(), PIECE};
    BlockHeader blk_header {piece, offset, 0};
    string msg = string(msg_header.data) + string(blk_header.data, 8) + payload;
    write(msg);
}

void PeerClient::setBitField(const ByteArray& buffer)
{
    bit_field.fromByteArray(torrent_info.num_pieces, buffer);
}

void PeerClient::updatePiece(const ByteArray& buffer)
{
    int idx = *reinterpret_cast<const int*>(buffer.data());
    bit_field[idx] = 1;
}