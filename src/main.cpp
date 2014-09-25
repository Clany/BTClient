#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include "clany/file_operation.hpp"

#include "bt_client.h"
#include "bt_server.h"
#include "setup.hpp"

using namespace std;
using namespace clany;

#ifdef _WIN32
INIT_WINSOCK
#endif // _WIN32

int main(int argc, char* argv[])
{
    CmdArgs bt_args;
    parse_args(bt_args, argc, argv);

    if (bt_args.verbose) {
        cout.flags(ios::left);
        cout << "Args:" << endl;;
        cout << setw(12) << "verbose"      << ": " << bt_args.verbose      << endl;
        cout << setw(12) << "save_file"    << ": " << bt_args.save_file    << endl;
        cout << setw(12) << "log_file"     << ": " << bt_args.log_file     << endl;
        cout << setw(12) << "torrent_file" << ": " << bt_args.torrent_file << endl;

//         for (i = 0; i < MAX_CONNECTIONS; i++) {
//             if (bt_args.peers[i] != NULL)
//                 print_peer(bt_args.peers[i]);
//         }
        cout << string(70, '-') << endl;
    }

    //read and parse the torrent file here
    MetaInfoParser parser;
    MetaInfo info;

    parser.parse(readFile(bt_args.torrent_file), info);

    if (bt_args.verbose) {
        // print out the torrent file arguments here
        cout.flags(ios::left);
        cout << "Torrent info:" << endl;
        cout << setw(12) << "Announce"     << ": " << info.announce     << endl
             << setw(12) << "Name"         << ": " << info.name         << endl
             << setw(12) << "Length"       << ": " << info.length       << endl
             << setw(12) << "Piece length" << ": " << info.piece_length << endl
             << setw(12) << "Num pieces"   << ": " << info.num_pieces   << endl;

        cout.flags(ios::right | ios::hex);
        cout.fill('0');
        cout << "Pieces:" << endl;
        for (const auto& sha1 : info.sha1_sums) {
            for_each(sha1.begin(), sha1.end(), [](uchar c) {
                cout << setw(2) << (int)c << " ";
            });
            cout << endl;
        }
        cout << string(70, '-') << endl;
    }

    //main client loop
    cout << "Starting Main Loop" << endl;

    return 0;
}