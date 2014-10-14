#ifndef SETUP_HPP
#define SETUP_HPP

#include <iostream>
#include <iomanip>
#include <string>
#include "clany/cmdparser.hpp"
#include "metainfo.h"

_CLANY_BEGIN
struct CmdArgs {
    int    verbose       = 0;    // verbose level
    string save_file     = "";   // filename to save to
    string log_file      = "";   // log file name
    string torrent_file  = "";   // torrent file name
    string id            = "";   // this bt_clients id
    vector<string> peers = {};
};

inline void printLineSep(ostream& os = cout, int len = 79)
{
    os << string(len, '-') << endl;
}

inline void usage(ostream& file)
{
    file << "bt-client [OPTIONS] file.torrent\n"
         << "  -h            \t Print this help screen\n"
         << "  -b ip         \t Bind to this ip for incoming connections, ports\n"
         << "                \t are selected automatically\n"
         << "  -s save_file  \t Save the torrent in directory save_dir (dflt: .)\n"
         << "  -l log_file   \t Save logs to log_filw (dflt: bt-client.log)\n"
         << "  -p ip:port    \t Instead of contacting the tracker for a peer list,\n"
         << "                \t use this peer instead, ip:port (ip or hostname)\n"
         << "                \t (include multiple -p for more than 1 peer)\n"
         << "  -I id         \t Set the node identifier to id (dflt: random)\n"
         << "  -v            \t verbose, print additional verbose info\n";
}

inline void parseArgs(CmdArgs& bt_args, int argc, char* argv[])
{
    // default log file
    bt_args.log_file = "bt-client.log";

    CmdLineParser cmd_parser(argc, argv, "hp:s:l:vI:");
    int ch = 0; //ch for each flag
    while ((ch = cmd_parser.get()) != -1) {
        switch (ch) {
        case 'h': // help
            usage(cout);
            exit(0);
            break;
        case 'v': // verbose
            bt_args.verbose = 1;
            break;
        case 's': // save file
            bt_args.save_file = cmd_parser.getArg<string>();
            break;
        case 'l': // log file
            bt_args.log_file = cmd_parser.getArg<string>();
            break;
        case 'p': // peer
            bt_args.peers.push_back(cmd_parser.getArg<string>());
            break;
        case 'I':
            bt_args.id = cmd_parser.getArg<string>();
            break;
        case ':':
            cerr << "ERROR: Invalid option, missing argument!" << endl;
            usage(cout);
            exit(1);
        default:
            cerr << "ERROR: Unknown option '-" << ch << "'" << endl;
            usage(cout);
            exit(1);
        }
    }

    argc -= cmd_parser.getIndex();
    argv += cmd_parser.getIndex();

    if (argc == 0) {
        cerr << "ERROR: Require torrent file" << endl;
        usage(cerr);
        exit(1);
    }

    // copy torrent file over
    bt_args.torrent_file = argv[0];

    stringstream ss;
    ss.flags(ios::left);
    ss << "Arguments" << endl;
    ss << setw(12) << "peer id"      << ": " << bt_args.id           << endl;
    ss << setw(12) << "verbose"      << ": " << bt_args.verbose      << endl;
    ss << setw(12) << "save_file"    << ": " << bt_args.save_file    << endl;
    ss << setw(12) << "log_file"     << ": " << bt_args.log_file     << endl;
    ss << setw(12) << "torrent_file" << ": " << bt_args.torrent_file << endl;

    ss << setw(12) << "peers" << ": " << endl;
    for (const auto& peer : bt_args.peers) {
        ss << peer << endl;
    }
    printLineSep(ss);

    if (bt_args.verbose) cout << ss.str();

    ofstream ofs(bt_args.log_file);
    if (ofs) ofs << ss.str();

    return;
}

inline string printHash(const string& hash) {
    stringstream ss;
    ss.flags(ios::right | ios::hex);
    ss.fill('0');
    for_each(hash.begin(), hash.end(), [&ss](uchar c) {
        ss << setw(2) << (int)c << " ";
    });
    return ss.str();
};

inline void printTorrentFileInfo(const MetaInfo& info, const string& log_file)
{
    stringstream ss;
    ss.flags(ios::left);
    ss << "Torrent info" << endl;
    ss << setw(12) << "Announce"     << ": " << info.announce             << endl
       << setw(12) << "Name"         << ": " << info.name                 << endl
       << setw(12) << "Length"       << ": " << info.length               << endl
       << setw(12) << "Piece length" << ": " << info.piece_length         << endl
       << setw(12) << "Num pieces"   << ": " << info.num_pieces           << endl
       << setw(12) << "Info hash"    << ": " << printHash(info.info_hash) << endl;

#ifndef NDEBUG
    ss << "Pieces:" << endl;
    for (const auto& sha1 : info.sha1_vec) {
        ss << printHash(sha1) << endl;
    }
#endif
    printLineSep(ss);

    cout << ss.str();

    ofstream ofs(log_file, ios::app);
    if (ofs) ofs << ss.str();
}
_CLANY_END

#endif // SETUP_HPP