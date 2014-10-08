#include <clany/dyn_bitset.hpp>
#include "bt_client.h"
#include "setup.hpp"

using namespace std;
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