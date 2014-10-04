#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include "clany/file_operation.hpp"

#include "bt_client.h"
#include "bt_server.h"
#include "setup.hpp"

using namespace std;
using namespace clany;

#ifdef _WIN32
INIT_WINSOCK
#endif // _WIN32

const size_t FILE_SIZE_LIMITE = 1024 * 1024 * 10; // 10mb
const int BUF_LEN = 1024;

void clientMode(const CmdArgs& bt_args)
{
    TCPSocket client;
    if (!client.isValid()) {
        cerr << "Fail to create socket!" << endl;
        exit(1);
    }

    if (!client.connect("127.0.0.1", 6767)) exit(1);


    string handshake_msg;
    string pstrlen(1, char(19));
    string pstr = "BitTorrent Protocol";
    string reserved(8, '0');
    string SHA1(20, '0');
    string peerid = "anshi";
    peerid.resize(20);

    handshake_msg = pstrlen + pstr + reserved + SHA1 + peerid;
    cout << handshake_msg.size() << endl;
    if (!client.write(handshake_msg)) exit(1);

    string buffer(FILE_SIZE_LIMITE, '\0');
    int num_bytes = 0, idx = 0;
    while ((num_bytes = ::recv(client.sock(), &buffer[idx], BUF_LEN, 0)) > 0) {
        idx += num_bytes;
        if (idx == 68) break;
    }
    buffer.resize(idx);
    cout << buffer << endl;

    string revsha1 = buffer.substr(28, 20);
    if (revsha1 == SHA1){
        cout << "Fuck!!!" << endl;
    }
}

int main(int argc, char* argv[])
{
    CmdArgs bt_args;
    parseArgs(bt_args, argc, argv);

    BTClient bt_client(bt_args.id);
    if (!bt_client.setTorrent(bt_args.torrent_file, bt_args.save_file)) {
        cerr << "Input torrent file is invalid!" << endl;
        exit(1);
    };

    if (bt_args.verbose) {
        printTorrentFileInfo(bt_client.getMetaInfo());
    }

    try {
        bt_client.run();
    }
    catch (const SocketError& err) {
        cerr << err.what() << endl;
    }
    catch (...) {
        cerr << "Unknown exception!" << endl;
    }

    return 0;
}