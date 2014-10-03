#include <openssl/sha.h>
#include "clany/file_operation.hpp"
#include "bt_client.h"

using namespace std;
using namespace tbb;
using namespace clany;

const size_t DEFAULT_CHUNK_SIZE = 100 * 1000 * 1000;
const string DEFAULT_CHUNK = string(DEFAULT_CHUNK_SIZE, '0');

bool BTClient::TmpFile::create(const string& file_name, llong file_size)
{
    fname = file_name;
    fsize = file_size;
    ofstream ofs(fname, ios::binary);
    if (!ofs) return false;

    for (auto i = 0u; i < fsize / DEFAULT_CHUNK_SIZE; ++i) {
        ofs.write(DEFAULT_CHUNK.c_str(), DEFAULT_CHUNK_SIZE);
    }
    int rest_size = file_size % DEFAULT_CHUNK_SIZE;
    ofs.write(string(rest_size, '0').c_str(), rest_size);

    return true;
}

bool BTClient::TmpFile::write(size_t idx, const ByteArray& data) const
{
    fstream fs(fname, ios::binary | ios::in | ios::out);
    if (!fs) return false;

    fs.seekp(idx);
    fs.write(data.c_str(), data.size());

    return true;
}
//////////////////////////////////////////////////////////////////////////////////////////
bool BTClient::setTorrent(const string& torrent_name, const string& save_file_name)
{
    if (!download_file.empty()) {
        cerr << "Torrent already set!" << endl;
        return false;
    }

    MetaInfoParser parser;
    if (!parser.parse(readFile(torrent_name), meta_info)) return false;
    save_name = save_file_name;

    // Check if we have already downloaded the file
    pieces_status.resize(meta_info.num_pieces);
    fill(pieces_status.begin(), pieces_status.end(), -1);
    auto file_name = save_name.empty() ? meta_info.name : save_name;
    if (!loadFile(file_name)) {
        download_file.create(file_name, meta_info.length);
    }

    return true;
}

void BTClient::run()
{
    parallel_invoke(
        [this]() { download(); },
        [this]() { upload(); }
    );
}

bool BTClient::loadFile(const string& file_name)
{
    download_file.fname = file_name;
    ifstream ifs(file_name, ios::binary | ios::ate);

    if (!ifs) return false;
    download_file.fsize = ifs.tellg();
    ifs.seekg(0);   // Set input position to start

    // Check hash
    string piece(meta_info.piece_length, '0');
    int idx = 0;
    while (++idx < meta_info.num_pieces) {
        ifs.read((char*)piece.c_str(), piece.length());
        checkPiece(piece, idx - 1);
    };
    // Last piece
    piece.resize(meta_info.length % meta_info.piece_length);
    ifs.read((char*)piece.c_str(), piece.length());
    checkPiece(piece, idx - 1);

    return true;
}

bool BTClient::checkPiece(const string& piece, int idx)
{
    string sha1(20, '0');
    SHA1((uchar*)piece.c_str(), piece.length(), (uchar*)sha1.c_str());
    if (sha1 == meta_info.sha1_sums[idx]) pieces_status[idx] = 1;
    return pieces_status[idx] == 1;
}

void BTClient::download()
{
}

void BTClient::upload()
{
}