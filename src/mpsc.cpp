#include <channels/channel.hpp>
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <unistd.h>

// this helper provides an infinite series of integers to a channel.
void producer(channel::send_channel<int> &ch, const std::string &id, int start,
              int step) {
  std::cout << "Helper " << id << " starting\n";
  thread_local std::random_device rd;
  thread_local std::mt19937 gen(rd());
  thread_local int i = start;
  // send returns false when channel is closed & blocks if channel is full.
  while (ch.send(i)) {
    i += step;

    std::uniform_int_distribution distrib(0, 80);
    std::this_thread::sleep_for(std::chrono::milliseconds(distrib(gen)));
  };
  std::cout << "Helper " << id << ": channel closed; exiting\n";
  std::cout << "Helper: queue size now " << ch.size() << "\n";
}
int main() {

  auto ch = channel::channel<int>(3);
  auto [sch, rch] = ch.split();

  std::thread mythread1([&] { producer(sch, "producer 1", 0, 3); });
  std::thread mythread2([&] { producer(sch, "producer 2", 1, 3); });
  std::thread mythread3([&] { producer(sch, "producer 3", 2, 3); });
  int r{};
  for (int i = 0; i < 10; ++i) {
    r = rch.recv();
    std::cout << "Main received " << r << "; rch size now " << rch.size()
              << ", ch.size now " << ch.size() << "\n";
  }

  std::cout << "Main closing channel\n";
  std::cout << "Main size now " << rch.size() << "\n";
  rch.close();

  mythread1.join();
  mythread2.join();
  mythread3.join();
  std::cout << "Done!\n";

  std::cout << "size is " << rch.size() << "\n";
  std::cout << rch.recv() << "\n";
  std::cout << "size is " << rch.size() << "\n";
  std::cout << rch.recv() << "\n";
  std::cout << "size is " << rch.size() << "\n";
  std::cout << rch.recv() << "\n";
  std::cout << "size is " << rch.size() << "\n";
  std::cout << "This should throw...\n";
  std::cout << "rch size is " << rch.size() << "\n";
  std::cout << "ch size is " << ch.size() << "\n";
  std::cout << rch.recv() << "\n";
}
