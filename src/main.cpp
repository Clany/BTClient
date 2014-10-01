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

void serverMode(const CmdArgs& bt_args)
{
	TCPServer server;
	//char host_name[INET_ADDRSTRLEN];
	string SHA1(20, '0');

	if (server.listen("127.0.0.1", 6767)){
		for (;;) {
			auto client_sock = server.nextPendingConnection();
			if (client_sock->sock() < 0)
				throw SocketError("Connection establish failed!");
			string buffer(FILE_SIZE_LIMITE, '\0');
			for (;;) {				
				int num_bytes = 0, idx = 0;
				while ((num_bytes = ::recv(client_sock->sock(), &buffer[idx], BUF_LEN, 0)) > 0) {
					idx += num_bytes;
					if (idx == 68) break;
				}
				buffer.resize(idx);
				if (idx <= 0 || idx == 68) break;
			}
			cout << buffer << endl;
			string revsha1 = buffer.substr(28, 20);
			if (revsha1 == SHA1){
				string handshake_msg;
				string pstrlen(1, char(19));
				string pstr = "BitTorrent Protocol";
				string reserved(8, '0');
				string SHA1(20, '0');
				string peerid = "anshi";
				peerid.resize(20);

				handshake_msg = pstrlen + pstr + reserved + SHA1 + peerid;
				if (!server.send(handshake_msg, *client_sock)) exit(1);
			}
		}
	}
}

	/*
	auto addr = bt_args.addr.sin_addr;
	inet_ntop(AF_INET, &addr, host_name, INET_ADDRSTRLEN);
	if (!server.listen(host_name, nc_args.port)) exit(1);

	while (true) {
		auto client_sock = server.nextPendingConnection();
		if (nc_args.verbose) cout << "Connection from " + client_sock->peekAddress() + " accepted" << endl;
		handleClient(*client_sock, nc_args);
		if (nc_args.verbose) cout << "Disconnected from " + client_sock->peekAddress() << endl;
	}
	*/


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


	/*
	char host_name[INET_ADDRSTRLEN];
	auto addr = nc_args.addr.sin_addr;
	inet_ntop(AF_INET, &addr, host_name, INET_ADDRSTRLEN);

	if (!client.connect(host_name, nc_args.port)) exit(1);
	if (nc_args.verbose) cout << "Successfully connect to " + string(host_name) << endl;

	string msg;
	if (nc_args.message_mode) msg = nc_args.message;
	else                      msg = fileToString(nc_args.filename);

	auto digest = HMAC(EVP_sha1(), &key, sizeof(key), (unsigned char*)msg.c_str(), msg.length(), NULL, NULL);
	string pkey((char*)digest, SHA1_SIZE);

	size_t msg_sz = msg.size();
	if (nc_args.n_bytes) msg_sz = nc_args.n_bytes;

	if (!client.write(msg.substr(nc_args.offset, msg_sz) + string("\0", 1) + pkey)) exit(1);
	if (nc_args.verbose) cout << "Sending message complete" + string(host_name) << endl;
	*/
}

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
	
	try {
		// Server mode
		if (bt_args.server_mode) serverMode(bt_args);
		// Client mode
		else clientMode(bt_args);
	}
	catch (const SocketError& err) {
		cerr << err.what() << endl;
	}
	catch (...) {
		cerr << "Unknown exception!" << endl;
	}
	
    return 0;
}