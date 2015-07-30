#include "thriftlink_server.h"
#include "userdef_server.h"

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "libeay32MT.lib")
#pragma comment(lib, "ssleay32MT.lib")



bool thrift_server_start(int port) {
    shared_ptr<ApiForwardHandler> handler(new ApiForwardHandler());
    shared_ptr<TProcessor> processor(new ApiForwardProcessor(handler));
    shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    TThreadedServer server(processor,
                           serverTransport,
                           transportFactory,
                           protocolFactory);

    printf("Starting server on port %d.\n", port);

    server.serve();
    return true;
}

