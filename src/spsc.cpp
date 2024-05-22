#include <channels/channel.hpp>
#include <iostream>
#include <unistd.h>

// this helper provides an infinite series of integers to a channel.
void producer(channel::send_channel<int> &ch, int start, int step) {
  thread_local int i = start;
  // send returns false when channel is closed & blocks if channel is full.
  while (ch.send(i)) {
    i += step;
  };
  std::cout << "Helper: channel closed; exiting\n";
  std::cout << "Helper: queue size now " << ch.size() << "\n";
}

int main() {

  auto ch = channel::channel<int>(3);
  auto [sch, rch] = ch.split();

  std::thread mythread([&] { producer(sch, 1, 2); });
  int r{};
  for (int i = 0; i < 10; ++i) {
    r = rch.recv();
    std::cout << "Main received " << r << "; rch size now " << rch.size()
              << ", ch.size now " << ch.size() << "\n";
  }

  std::cout << "Main closing channel\n";
  std::cout << "Main size now " << rch.size() << "\n";
  rch.close();

  mythread.join();
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
