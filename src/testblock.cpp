#include <channels/channel.hpp>
#include <iostream>

int main() {
  auto ch = channel::channel<int>(3);
  auto [sch, rch] = ch.split();

  std::cout << "starting...\n";
  std::cout << "rch.wouldblock = " << rch.wouldblock()
            << ", ch.recv_wouldblock = " << ch.recv_wouldblock() << "\n";

  std::cout << "sch.wouldblock = " << sch.wouldblock()
            << ", ch.send_wouldblock = " << ch.send_wouldblock() << "\n";
  1 >> sch;
  2 >> sch;
  3 >> sch;

  std::cout << "sch.wouldblock = " << sch.wouldblock()
            << ", ch.send_wouldblock = " << ch.send_wouldblock() << "\n";

  std::cout << "rch.wouldblock = " << rch.wouldblock()
            << ", ch.recv_wouldblock = " << ch.recv_wouldblock() << "\n";
  std::cout << "exiting\n";
}
