#include <stdio.h>
#include "../uvkits/Looper.h"
#include "../service/HttpServer.h"
#if defined(WIN)
#pragma comment(lib, "psapi")
#pragma comment(lib, "user32")
#pragma comment(lib, "advapi32")
#pragma comment(lib, "iphlpapi")
#pragma comment(lib, "userenv")
#pragma comment(lib, "ws2_32")
#endif
int main() {
  ndcp::HttpServer* server = new ndcp::HttpServer;
  server->start("0.0.0.0", 8090);
  ndcp::Looper::loop();
  return 0;
}