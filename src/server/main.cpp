#include <memory>
#include <cstdint>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <common/Types.h>
#include <http/HttpServer.h>
#include <db/Storage.h>

volatile bool running = false;
void sig_handler(int signum)
{
  LOG(stderr, "Received signal %d\n", signum);
  running = false;
}

int main(int argc, char* argv[])
{
  using common::Result;
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  http::HttpServer server;

  Result res = server.start();
  if (Result::SUCCESS == res) {
    running = true;
  }

  db::Storage storage;
  storage.load("/tmp/data/data.zip");

  while (running) {
    usleep(1000000);
  }

  server.stop();

  return 0;
}