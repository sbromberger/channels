#pragma once
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <string>

#define SEND_CLOSED_ERR 2
#define RECV_CLOSED_ERR 3
#define CLOSE_CLOSED_ERR 4

template <typename T> struct channel {
  size_t cap{};
  std::mutex mu{};
  std::condition_variable cv{};
  std::queue<T> q{};
  bool _closed = false;

  /**
   * @brief Checks if the channel is closed and if so, outputs a message and
   * terminates the program.
   *
   * This function checks the _closed member variable. If it is true, it means
   * the channel is closed. In this case, it outputs a provided error message to
   * the standard error stream and then terminates the program with a provided
   * exit code. If no exit code is provided, the default is 1.
   *
   * @param msg The error message to be output if the channel is closed.
   * @param exit_code The exit code with which the program should be terminated.
   * Default is 1.
   */
  void check_closed(const std::string &msg, int exit_code = 1) {
    if (_closed) {
      std::cerr << msg << '\n';
      exit(exit_code);
    }
  }

public:
  channel(size_t cap) : cap(cap){};
  /**
   * @brief Checks if the channel is closed.
   *
   * @return true if the channel is closed, false otherwise.
   */
  bool closed() {
    std::scoped_lock _(mu);
    return _closed;
  }

  /**
   * @brief Closes the channel.
   *
   * This function closes and flushes the channel. All elements in the channel
   * are discarded.
   *
   * @exits with CLOSE_CLOSED_ERR If the channel is already closed.
   */
  void close() {
    std::queue<T> empty_q{};
    std::scoped_lock _(mu);
    check_closed("Attempt to close a closed channel", CLOSE_CLOSED_ERR);
    _closed = true;
    std::swap(q, empty_q);
    cv.notify_one();
  }

  void flush() {
    std::scoped_lock _(mu);
    while (!q.empty()) {
      q.pop();
    }
    cv.notify_one();
  }

  /**
   * @brief Sends a value to the channel, moving the value.
   *
   * @param t The value to be moved into the channel.
   * @exits with SEND_CLOSED_ERR If the channel is closed at the time of
   * sending.
   */
  bool send(T t) {
    std::unique_lock l(mu);
    if (_closed) {
      return false;
    }
    cv.wait(l, [this] { return _closed || (cap == 0 || q.size() < cap); });
    if (_closed) {
      return false;
    }
    q.push(std::move(t));
    cv.notify_one();
    return true;
  }

  friend void operator>>(T &&t, channel<T> &ch) { ch.send(std::move(t)); }
  friend void operator<<(T &t, channel<T> &ch) { t = ch.recv(); }
  /**
   * @brief Receives an item from the channel immediately.
   *
   * This function tries to receive an item from the channel immediately.
   * If the channel is empty, it returns an empty std::optional.
   * If the channel is not empty, it removes and returns the item from the
   * channel,
   *
   * @return A std::optional containing the received item, or an empty
   * std::optional if the channel was empty.
   */
  std::optional<T> recv_immed() {
    std::scoped_lock _(mu);
    if (q.empty()) {
      return {};
    }
    T t = std::move(q.front());
    q.pop();
    cv.notify_one();

    return t;
  }

  /**
   * @brief Receives an item from the channel, blocking if the channel is empty.
   *
   *
   * @return The item received from the channel.
   * @exits with RECV_CLOSED_ERR if the channel is closed.
   */
  T recv() {
    // Check if the channel is closed. If it is, throw an exception.
    check_closed("Attempt to receive on a closed channel", RECV_CLOSED_ERR);

    // Wait until the queue is not empty.
    std::unique_lock l(mu);
    cv.wait(l, [this] { return _closed || !q.empty(); });
    check_closed("Attempt to receive on a closed channel", RECV_CLOSED_ERR);
    T t = std::move(q.front());
    q.pop();
    cv.notify_one();
    return t;
  }
};
