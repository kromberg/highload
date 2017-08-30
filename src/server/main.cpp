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

  size_t serversCount = 4;
  if (argc >= 2) {
    serversCount = std::stoul(argv[1]);
  }
  size_t acceptThreadsCount = 1;
  if (argc >= 3) {
    acceptThreadsCount = std::stoul(argv[1]);
  }

  db::StoragePtr storage(new db::Storage);
  storage->load("/tmp/data/data.zip");
  db::Storage::loadTime("/tmp/data/options.txt");

  running = true;
  std::vector<http::HttpServer*> servers(serversCount, nullptr);
  for (auto server : servers) {
    server = new http::HttpServer(acceptThreadsCount, storage);
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