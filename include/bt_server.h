#ifndef BT_SERVER_H
#define BT_SERVER_H

#include <list>
#include "tcp_server.hpp"
#include "bt_client.h"

_CLANY_BEGIN
class BTServer : public TCPServer {
public:
    void addClient();
    void removeClient();

private:
    list<BTClient::Ptr> client_list;
};
_CLANY_END

#endif // BT_SERVER_H