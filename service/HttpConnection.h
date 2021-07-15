#ifndef __HTTP_CONNECTION__
#define __HTTP_CONNECTION__
#include "uv.h"
#include "http-parser/http_parser.h"
namespace ndcp {

class HttpConnection {
public:
	HttpConnection();
	virtual ~HttpConnection();
public:
	void OnUvReadAlloc(size_t suggestedSize, uv_buf_t *buf);
	void OnUvRead(uv_handle_t* handle, ssize_t nread, const uv_buf_t *buf);
	void OnUvClose(uv_handle_t* handle);
	void OnUvWrite(int status);
	void Start();
	void Close();
	void WriteResponse(const char *method, int responseCode);
	uv_tcp_t* GetHandle();

	/* Struct for the data field of uv_req_t when writing into the connection. */
	struct UvWriteData
	{
		explicit UvWriteData(size_t storeSize)
		{
			this->store = new uint8_t[storeSize];
		}

		// Disable copy constructor because of the dynamically allocated data (store).
		UvWriteData(const UvWriteData&) = delete;

		~UvWriteData()
		{
			delete[] this->store;
		}

		uv_write_t req;
		uint8_t* store{ nullptr };
	};

private:
	  uv_tcp_t handle;
	  http_parser parser;
	  http_parser_settings parser_settings;

private:
	bool isClosedByPeer { false };
	bool hasError { false };
	bool closed { false };


};
} // namespace ndcp

#endif//__HTTP_CONNECTION__
