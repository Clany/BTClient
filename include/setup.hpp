#ifndef SETUP_HPP
#define SETUP_HPP

#include "clany/cmdparser.hpp"

_CLANY_BEGIN
typedef struct {
    int verbose;        // verbose level
    string save_file;   // filename to save to
    string log_file;    // log file name
    string torrent_file;// torrent file name
    uint id;            // this bt_clients id
//     FILE * f_save;
//     peer_t * peers[MAX_CONNECTIONS]; // array of peer_t pointers
//     int sockets[MAX_CONNECTIONS]; //Array of possible sockets
//     struct pollfd poll_sockets[MAX_CONNECTIONS]; //Array of pollfd for polling for input
} CmdArgs;

inline void usage(ostream& file)
{
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

inline void parse_args(CmdArgs& bt_args, int argc, char* argv[])
{
    int ch; //ch for each flag
//    int n_peers = 0;

    /* set the default args */
    bt_args.verbose = 0; //no verbosity

//     //null out file pointers
//     bt_args.f_save = NULL;
//
//     //default lag file
//     strncpy(bt_args.log_file, "bt-client.log", FILE_NAME_MAX);

    bt_args.id = 0;
    CmdLineParser cmd_parser(argc, argv, "hp:s:l:vI:");
    while ((ch = cmd_parser.get()) != -1) {
        switch (ch) {
        case 'h': //help
            usage(cout);
            exit(0);
            break;
        case 'v': //verbose
            bt_args.verbose = 1;
            break;
        case 's': //save file
            bt_args.save_file = cmd_parser.getArg<string>();
            break;
        case 'l': //log file
            bt_args.log_file = cmd_parser.getArg<string>();
            break;
        case 'p': //peer
//             n_peers++;
//             //check if we are going to overflow
//             if (n_peers > MAX_CONNECTIONS) {
//                 fprintf(stderr, "ERROR: Can only support %d initial peers", MAX_CONNECTIONS);
//                 usage(stderr);
//                 exit(1);
//             }
//
//             bt_args.peers[n_peers] = malloc(sizeof(peer_t));
//
//             //parse peers
//             __parse_peer(bt_args.peers[n_peers], optarg);
            break;
        case 'I':
            bt_args.id = cmd_parser.getArg<int>();
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

    //copy torrent file over
    bt_args.torrent_file = argv[0];

    return;
}
_CLANY_END

#endif // SETUP_HPP