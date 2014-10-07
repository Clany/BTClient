#include "peer_client.h"

using namespace std;
using namespace clany;

void PeerClient::sendPieceUpdate(int piece)
{
}

void PeerClient::sendAvailPieces(const ByteArray& bit_field)
{
}

void PeerClient::requestBlock(int piece, int offset, int length)
{
}

void PeerClient::cancelRequest(int piece, int offset, int length)
{
}

void PeerClient::sendBlock(int piece, int offset, const ByteArray &data)
{
}