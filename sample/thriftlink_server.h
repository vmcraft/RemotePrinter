#ifndef __THRIFTLINK_SERVER_H_
#define __THRIFTLINK_SERVER_H_

#include "ApiForward.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/server/TThreadedServer.h>


using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace ::apache::thrift::concurrency;
using namespace  ::apiforward;
using boost::shared_ptr;

bool thrift_server_start(int port);


#endif // __THRIFTLINK_SERVER_H_