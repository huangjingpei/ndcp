#ifndef __NSCP_HTTP_SERVER_H__
#define __NSCP_HTTP_SERVER_H__
#include "uv.h"
#include "http-parser/http_parser.h"
namespace ndcp {

class HttpServer {
public:
	HttpServer();
	~HttpServer();

	int start(const char *addr, short port);
	int stop();

	int processNewConnection(uv_stream_t *handle, int status);
public:


private:

	uv_loop_t *loop_;
	uv_tcp_t server_;


};

} //namespace ndcp

#endif//__NSCP_HTTP_SERVER_H__
