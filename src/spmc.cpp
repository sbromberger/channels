#include "channel.hpp"
#include <iostream>
#include <string>
#include <thread>

// this helper consumes integers until the producer closes.
void helper(channel::recv_channel<int> &ch, const std::string &id) {
  std::cout << "Helper " << id << " starting" << std::endl;
  while (!ch.finished()) {
    auto r = ch.recv();
    std::cout << "Helper " << id << ": received " << r << std::endl;
  };
  std::cout << "Helper " << id << ": channel finished; exiting\n";
  std::cout << "Helper: queue size now " << ch.size() << "\n";
}
int main() {

  using namespace std::chrono_literals;
  auto ch = channel::channel<int>(3);
  auto [sch, rch] = ch.split();

  std::thread mythread1([&] { helper(rch, "consumer 1"); });
  std::thread mythread2([&] { helper(rch, "consumer 2"); });
  std::thread mythread3([&] { helper(rch, "consumer 3"); });

  std::this_thread::sleep_for(2000ms);
  bool s{true};
  for (int i = 0; i < 10 && s; ++i) {
    s = sch.send(i);
    std::cout << "Main sent " << i << "; sch size now " << sch.size()
              << ", ch.size now " << ch.size() << "\n";
  }

  std::cout << "Main closing channel\n";
  std::cout << "Main size now " << sch.size() << "\n";
  sch.close();

  mythread1.join();
  mythread2.join();
  mythread3.join();
  std::cout << "Done!\n";

  std::cout << "rch size is " << rch.size() << "\n";
  std::cout << "ch size is " << ch.size() << "\n";
}
