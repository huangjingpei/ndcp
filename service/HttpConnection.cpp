#include "HttpConnection.h"


int OnUrl(http_parser* parser, const char *at, size_t length) {
	auto *connection = static_cast<ndcp::HttpConnection*>(parser->data);
	printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	printf("at: %s length %ld\n",at, length);
	return 0;
}

int OnStatus(http_parser* parser, const char *at, size_t length) {
	auto *connection = static_cast<ndcp::HttpConnection*>(parser->data);
	printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	printf("at: %s length %ld\n",at, length);
	return 0;
}

int OnHeaderField(http_parser* parser, const char *at, size_t length) {
	auto *connection = static_cast<ndcp::HttpConnection*>(parser->data);
	printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	printf("at: %s length %ld\n",at, length);

	return 0;
}

int OnHeaderValue(http_parser* parser, const char *at, size_t length) {
	auto *connection = static_cast<ndcp::HttpConnection*>(parser->data);
	printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	printf("at: %s length %ld\n",at, length);

	return 0;
}

int OnHeaderComplete(http_parser* parser) {
	auto *connection = static_cast<ndcp::HttpConnection*>(parser->data);
	printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;

}

int OnBody(http_parser* parser, const char *at, size_t length) {
	auto *connection = static_cast<ndcp::HttpConnection*>(parser->data);
	printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	printf("at: %s length %ld\n",at, length);


	connection->WriteResponse("add", 200);


	return 0;
}

int OnMessageComplete(http_parser* parser) {
	auto *connection = static_cast<ndcp::HttpConnection*>(parser->data);
	return 0;

}
inline static void onAlloc(uv_handle_t *handle, size_t suggestedSize,
		uv_buf_t *buf) {
	auto *connection = static_cast<ndcp::HttpConnection*>(handle->data);

	if (connection)
		connection->OnUvReadAlloc(suggestedSize, buf);
}

inline static void onRead(uv_stream_t *handle, ssize_t nread,
		const uv_buf_t *buf) {
	auto *connection = static_cast<ndcp::HttpConnection*>(handle->data);

	if (connection) {
		connection->OnUvRead(reinterpret_cast<uv_handle_t*>(handle), nread, buf);
	}
}

inline static void onWrite(uv_write_t *req, int status) {
	auto *writeData = static_cast<ndcp::HttpConnection::UvWriteData*>(req->data);
	auto *handle = req->handle;
	auto *connection = static_cast<ndcp::HttpConnection*>(handle->data);

	if (connection)
		connection->OnUvWrite(status);

	// Delete the UvWriteData struct and the cb.
	delete writeData;
}


inline static void onClose(uv_handle_t* handle) {
	auto *connection = static_cast<ndcp::HttpConnection*>(handle->data);
	if (connection) {
		connection->OnUvClose(handle);
	}
}


inline static void onShutdown(uv_shutdown_t *req, int /*status*/) {
	auto *handle = req->handle;

	delete req;

	// Now do close the handle.
	uv_close(reinterpret_cast<uv_handle_t*>(handle),
			static_cast<uv_close_cb>(onClose));
}


namespace ndcp {
HttpConnection::HttpConnection() {
    http_parser_init(&parser, HTTP_REQUEST);
    parser.data = this;

	parser_settings.on_url = OnUrl;
	parser_settings.on_status = OnStatus;
	parser_settings.on_header_field = OnHeaderField;
	parser_settings.on_header_value = OnHeaderValue;
	parser_settings.on_headers_complete  = OnHeaderComplete ;
	parser_settings.on_body = OnBody;
	parser_settings.on_message_complete = OnMessageComplete;
}

HttpConnection::~HttpConnection() {

}


void HttpConnection::OnUvReadAlloc(size_t suggestedSize, uv_buf_t *buf) {
	  buf->base = new char(suggestedSize);
	  buf->len = suggestedSize;
}
void HttpConnection::OnUvRead(uv_handle_t* handle, ssize_t nread, const uv_buf_t *buf) {
	  size_t parsed;
	  HttpConnection* connection = (HttpConnection*)handle->data;
	  if (nread == 0) {
		  return;
	  }

	  if (nread >= 0) {
	    parsed = http_parser_execute(
	        &connection->parser, &connection->parser_settings, buf->base, nread);
	    if (parsed < nread) {
	      uv_close(reinterpret_cast<uv_handle_t*>( &connection->handle),
	  			static_cast<uv_close_cb>(onClose));
	    }
	  } else if (nread == UV_EOF || nread == UV_ECONNRESET) {// Client disconnected.
		  isClosedByPeer = true;
		  // Close server side of the connection.
		  Close();
	  } else {	// Some error.
		  hasError = true;
			// Close server side of the connection.
			Close();
	  }
	  delete buf->base;

}

void HttpConnection::OnUvWrite(int status) {

	if (status != 0) {
		if (status != UV_EPIPE && status != UV_ENOTCONN) {
			this->hasError = true;
		}

		printf("write error, closing the connection: %s\n",
				uv_strerror(status));

		Close();
	}
}

void HttpConnection::OnUvClose(uv_handle_t* handle) {

}

void after_write(uv_write_t* req, int status) {
  uv_close((uv_handle_t*)req->handle, nullptr);
}
void HttpConnection::WriteResponse(const char *method, int responseCode) {
	const char* RESPONSE = "HTTP/1.1 200 OK\r\n""Content-Type: text/plain\r\n""Content-Length: 14\r\n\r\n""Hello, World!\n";
	size_t len = strlen(RESPONSE) + 1;
	uv_buf_t buffer = uv_buf_init(
			reinterpret_cast<char*>(const_cast<char*>(RESPONSE)), len);
	int written = uv_try_write(reinterpret_cast<uv_stream_t*>(&handle),
			&buffer, 1);
	if (written == static_cast<int>(len)) {
		return;
	} else if (written == UV_EAGAIN || written == UV_ENOSYS) {
		// Cannot write any data at first time. Use uv_write().
		written = 0;
	} else if (written < 0) {
		Close();
		return;
	}
	size_t pendingLen = len - written;
	auto *writeData = new UvWriteData(pendingLen);

	buffer = uv_buf_init(reinterpret_cast<char*>(writeData->store), pendingLen);
	int err = uv_write(&writeData->req,
			reinterpret_cast<uv_stream_t*>(&handle), &buffer, 1,
			static_cast<uv_write_cb>(onWrite));
	if (err != 0) {
		printf("write failed %s\n", uv_strerror(err));
		delete writeData;
	}

}


void HttpConnection::Start() {
	int err;
	err = uv_read_start(reinterpret_cast<uv_stream_t*>(&handle),
				static_cast<uv_alloc_cb>(onAlloc), static_cast<uv_read_cb>(onRead));
}


void HttpConnection::Close() {

	if (closed)
		return;

	int err;
	closed = true;

	// Tell the UV handle that the TcpConnection has been closed.
	handle.data = nullptr;

	// Don't read more.
	err = uv_read_stop(reinterpret_cast<uv_stream_t*>(&handle));

	if (err != 0)
		printf("uv_read_stop() failed: %s\n", uv_strerror(err));

	// If there is no error and the peer didn't close its connection side then close gracefully.
	if (!this->hasError && !this->isClosedByPeer) {
		// Use uv_shutdown() so pending data to be written will be sent to the peer
		// before closing.
		auto req = new uv_shutdown_t;
		req->data = static_cast<void*>(this);
		err = uv_shutdown(req, reinterpret_cast<uv_stream_t*>(&handle),
				static_cast<uv_shutdown_cb>(onShutdown));

		if (err != 0) {
			printf("uv_shutdown() failed: %s\n", uv_strerror(err));
		}
	}
	// Otherwise directly close the socket.
	else {
		uv_close(reinterpret_cast<uv_handle_t*>(&handle),
				static_cast<uv_close_cb>(onClose));
	}
}

uv_tcp_t* HttpConnection::GetHandle() {
	return &handle;
}


} // namespace ndcp

