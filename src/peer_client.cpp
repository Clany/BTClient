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

void PeerClient::sendPieceUpdate(int/* piece*/)
{
    // TODO
}

void PeerClient::sendAvailPieces(const BitField& bit_field)
{
    // Do no send if we have no piece
    if (bit_field.none()) return;

    string payload = bit_field.toByteArray();
    MsgHeader header {1 + payload.length(), HAVE};
    string msg = string(header.data) + static_cast<string>(payload);
    write(msg);
}

void PeerClient::requestBlock(int piece, int offset, int length)
{
    MsgHeader   msg_header {13, REQUEST};
    BlockHeader blk_header {piece, offset, length};
    string msg = string(msg_header.data) + string(blk_header.data, 12);
    write(msg);
}

void PeerClient::cancelRequest(int piece, int offset, int length)
{
    MsgHeader   msg_header {13, CANCEL};
    BlockHeader blk_header {piece, offset, length};
    string msg = string(msg_header.data) + string(blk_header.data, 12);
    write(msg);
}

void PeerClient::sendBlock(int piece, int offset, const ByteArray& data)
{
    string payload = data;
    MsgHeader   msg_header {9 + payload.length(), PIECE};
    BlockHeader blk_header {piece, offset, 0};
    string msg = string(msg_header.data) + string(blk_header.data, 8) + payload;
    write(msg);
}

void PeerClient::parseMsg(const string& buffer, int& msg_id, string& payload)
{
    int len = *reinterpret_cast<const int*>(buffer.substr(0, 4).c_str());
    if (len == 0) {
        msg_id = -1;
        payload = "";
    } else {
        msg_id = buffer[4];
        payload = buffer.substr(5, len - 1);
    }
}