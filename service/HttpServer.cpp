#include "HttpServer.h"
#include "uv.h"
#include "Looper.h"
#include "HttpConnection.h"

namespace ndcp {

inline static void onConnection(uv_stream_t *handle, int status) {
	auto *server = static_cast<HttpServer*>(handle->data);

	if (server == nullptr)
		return;

	server->processNewConnection(handle, status);
}


HttpServer::HttpServer() : loop_(Looper::getLooper()) {

}

HttpServer::~HttpServer() {

}

int HttpServer::start(const char *ip, short port) {
	int err = -1;
	do {
		err = uv_tcp_init(loop_, &server_);
		if (err != 0) {
			printf("error while initializing tcp server: %s", uv_strerror(err));
			break;
		}
	    struct sockaddr_in addr;
		err = uv_ip4_addr(ip, port, &addr);
		if (err!= 0) {
			printf("error while initializing tcp addr: %s", uv_strerror(err));
			break;
		}

		err = uv_tcp_bind(&server_, (struct sockaddr *) &addr, 0);
		if (err != 0) {
			printf("error while binding addr: %s", uv_strerror(err));
			break;
		}
		server_.data = this;
		err = uv_listen(reinterpret_cast<uv_stream_t*>(&server_), 3, static_cast<uv_connection_cb>(onConnection));
		err = 0;
	} while (0);

	return err;
}

int HttpServer::processNewConnection(uv_stream_t *handle, int status) {
	if (status != 0) {
		printf("error while receiving a new TCP connection: %s\n",
				uv_strerror(status));

		return -1;
	}
	int err;
    HttpConnection* connection = new HttpConnection;
    uv_tcp_init(loop_, connection->GetHandle());
    connection->GetHandle()->data = connection;

	// Accept the connection.
	err = uv_accept(reinterpret_cast<uv_stream_t*>(&server_),
			reinterpret_cast<uv_stream_t*>(connection->GetHandle()));

	if (err != 0) {
		printf("error while accepting the new connection: %s", uv_strerror(err));
		return -1;
	}

	connection->Start();
	return 0;

}



} //namespace oscp


