#include <iostream>
#include <fstream>
#include "clany/file_operation.hpp"

#include "bt_client.h"
#include "bt_server.h"

using namespace std;
using namespace std::tr2;
using namespace clany;

#ifdef _WIN32
INIT_WINSOCK
#endif // _WIN32

void usage(ostream& file) {
    file << "bt-client [OPTIONS] file.torrent\n"
         << "  -h            \t Print this help screen\n"
         << "  -b ip         \t Bind to this ip for incoming connections, ports\n"
         << "                \t are selected automatically\n"
         << "  -s save_file  \t Save the torrent in directory save_dir (dflt: .)\n"
         << "  -l log_file   \t Save logs to log_filw (dflt: bt-client.log)\n"
         << "  -p ip:port    \t Instead of contacing the tracker for a peer list,\n"
         << "                \t use this peer instead, ip:port (ip or hostname)\n"
         << "                \t (include multiple -p for more than 1 peer)\n"
         << "  -I id         \t Set the node identifier to id (dflt: random)\n"
         << "  -v            \t verbose, print additional verbose info\n";
}

int main(int argc, char* argv[])
{
//     MetaInfoParser parser;
//     MetaInfo info;
//
//     parser.parse(readFile("../test/moby_dick.txt.torrent"), info);
//     parser.parse(readFile("../test/download.mp3.torrent"), info);

    return 0;
}