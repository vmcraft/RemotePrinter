#pragma once

#include "ApiForward.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace apiforward;

extern boost::shared_ptr<ApiForwardClient> _api;

bool thrift_connect(const char* ipaddr, int port);
void thrift_close();
bool ensure_connection();

