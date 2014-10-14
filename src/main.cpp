#include <clany/dyn_bitset.hpp>
#include "bt_client.h"
#include "setup.hpp"

using namespace std;
using namespace tbb;
using namespace clany;

#ifdef _WIN32
INIT_WINSOCK
#endif // _WIN32

int main(int argc, char* argv[])
{
    CmdArgs bt_args;
    parseArgs(bt_args, argc, argv);

    BTClient bt_client(bt_args.id);
    if (!bt_client.setTorrent(bt_args.torrent_file, bt_args.save_file)) {
        cerr << "Input torrent file is invalid!" << endl;
        exit(1);
    };

    if (!bt_client.setLogFile(bt_args.log_file)) {
        cerr << "Failed to set log file" << endl;
        exit(1);
    }

    for (const auto& peer_addr : bt_args.peers) {
        auto sep = peer_addr.find(':');
        string ip = peer_addr.substr(0, sep);
        ushort port = static_cast<ushort>(stoi(peer_addr.substr(sep + 1)));
        bt_client.addPeerAddr(ip, port);
    }

    if (bt_args.verbose) {
        printTorrentFileInfo(bt_client.getMetaInfo(), bt_args.log_file);
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