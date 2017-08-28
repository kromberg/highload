#include <memory>
#include <cstdint>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <common/Types.h>
#include <http/HttpServer.h>
#include <db/Storage.h>

#include <common/Profiler.h>

volatile bool running = false;
void sig_handler(int signum)
{
  LOG_CRITICAL(stderr, "Received signal %d\n", signum);
  running = false;
}

int main(int argc, char* argv[])
{
  using common::Result;
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  db::StoragePtr storage(new db::Storage);
  storage->load("/tmp/data/data.zip");
  db::Storage::loadTime("/tmp/data/options.txt");

  running = true;
  std::vector<http::HttpServer*> servers(4, nullptr);
  for (auto server : servers) {
    server = new http::HttpServer(storage);
    Result res = server->start();
    if (Result::SUCCESS != res) {
      running = false;
      break;
    }
  }

  while (running) {
    usleep(10000000);
    common::TimeProfiler::print();
  }

  for (auto server : servers) {
    if (server) {
      server->stop();
      delete server;
    }
  }

  return 0;
}