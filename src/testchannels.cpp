#include "channel.hpp"
#include <iostream>
#include <thread>
#include <unistd.h>

void helper(channel::send_channel<int> ch) {
  thread_local int i = 0;
  bool closed = ch.closed();
  while (!closed) {
    closed = !ch.send(i);
    // std::cout << "Helper sent: " << i << "\n";
    i += 2;
  }
  std::cout << "Helper: channel closed; exiting\n";
}
int main() {

  auto ch = channel::channel<int>(5);

  std::thread mythread(helper, ch);
  int r{};
  for (int i = 0; i < 10; ++i) {
    r = ch.recv();
    std::cout << "Main received " << r << "\n";
  }

  std::cout << "Main closing channel\n";
  ch.close();

  // std::optional<int> last = ch.recv_immed();
  // while (last.has_value()) {
  // std::cout << "Main received last: " << last.value() << "\n";
  // last = ch.recv_immed();
  // }

  mythread.join();
  std::cout << "Done!\n";
}
