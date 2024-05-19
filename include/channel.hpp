#pragma once
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>

namespace channel {
constexpr int SEND_CLOSED_ERR = 2;
constexpr int RECV_CLOSED_ERR = 3;
constexpr int CLOSE_CLOSED_ERR = 4;

// template <typename T> class channel;
template <typename T> class channel {
  // friend class channel<T>;
  size_t cap{};
  std::mutex mu{};
  std::condition_variable _full{};
  std::condition_variable _empty{};
  std::queue<T> q{};
  bool _closed = false;

  void check_closed(const std::string &msg, int exit_code = 1) {
    if (_closed) {
      std::cerr << msg << '\n';
      exit(exit_code);
    }
  }

public:
  channel(size_t cap) : cap(cap){};
  ~channel() {
    if (!closed()) {
      close();
    }
    flush();
  }
  // channels cannot be copied.
  channel(const channel &) = delete;
  void operator=(const channel &) = delete;

  void flush() {
    std::scoped_lock _(mu);
    while (!q.empty()) {
      q.pop();
    }
    _full.notify_one();
  }

  bool closed() {
    std::scoped_lock _(mu);
    return _closed;
  }

  size_t size() {
    std::scoped_lock _(mu);
    return q.size();
  }

  void close() {
    std::queue<T> empty_q{};
    std::scoped_lock _(mu);
    check_closed("Attempt to close a closed channel", CLOSE_CLOSED_ERR);
    _closed = true;
    std::swap(q, empty_q);
    _full.notify_all();
    _empty.notify_one();
  }

  bool send(T t) {
    std::unique_lock l(mu);
    if (_closed) {
      return false;
    }
    _full.wait(l, [this] { return _closed || (cap == 0 || q.size() < cap); });
    if (_closed) {
      return false;
    }
    q.push(std::move(t));
    _empty.notify_one();
    return true;
  }

  friend void operator>>(T &&t, channel<T> &ch) { ch.send(std::move(t)); }
  friend void operator<<(T &t, channel<T> &ch) { t = ch.recv(); }

  std::optional<T> recv_immed() {
    std::scoped_lock _(mu);
    if (q.empty()) {
      return {};
    }
    T t = std::move(q.front());
    q.pop();
    _full.notify_one();

    return t;
  }

  T recv() {
    // Check if the channel is closed. If it is, throw an exception.
    check_closed("Attempt to receive on a closed channel", RECV_CLOSED_ERR);

    // Wait until the queue is not empty.
    std::unique_lock l(mu);
    _empty.wait(l, [this] { return _closed || !q.empty(); });
    check_closed("Attempt to receive on a closed channel", RECV_CLOSED_ERR);
    T t = std::move(q.front());
    q.pop();
    _full.notify_one();
    return t;
  }
};

// template <typename T, typename Fn> struct channel_group {
//   std::condition_variable _switched{};
//   std::map<channel<T>, Fn> group;

// public:
//   channel_group() = default;
//   void add_channel(channel<T> &ch, const Fn &fn) {
//     group.emplace_back(std::move(ch), fn);
//   };
//   channel<T> pop_channel() {
//     channel<T> ch = group.begin().first;
//     group.erase(ch);
//     return ch;
//   };

//   void wait_on() {

// }
// };

} // namespace channel
