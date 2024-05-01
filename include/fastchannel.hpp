#pragma once
#include <concurrentqueue.h>
#include <iostream>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>

#define SEND_CLOSED_ERR 2
#define RECV_CLOSED_ERR 3
#define CLOSE_CLOSED_ERR 4

template <typename T> struct fast_channel {
  size_t cap;

  moodycamel::ConcurrentQueue<T> q{};
  bool _closed = false;
  std::shared_mutex mu{};

  void check_closed(const std::string &msg, int exit_code = 1) {
    if (closed()) {
      std::cerr << msg << '\n';
      exit(exit_code);
    }
  }

public:
  fast_channel(size_t cap) : cap(cap){};
  /**
   * @brief Checks if the channel is closed.
   *
   * @return true if the channel is closed, false otherwise.
   */
  bool closed() {
    std::shared_lock _(mu);
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
    // moodycamel::ConcurrentQueue<T> empty_q{};
    check_closed("Attempt to close a closed channel", CLOSE_CLOSED_ERR);
    std::scoped_lock _(mu);
    _closed = true;
    flush();
    // q.swap(empty_q);
  }

  void flush() {
    T t;
    while (q.try_dequeue(t)) {
    };
  }
  bool send(const T &&t) {

    if (closed()) {
      return false;
    }
    while (cap > 0 && q.size_approx() > cap) {
    }
    if (closed()) {
      return false;
    }
    q.enqueue(t);
    return true;
  }

  bool send(const T &t) {
    if (closed()) {
      return false;
    }
    while (cap > 0 && q.size_approx() > cap) {
    }
    if (closed()) {
      return false;
    }
    q.enqueue(std::move(t));
    return true;
  }

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
    T t;

    if (q.try_dequeue(t)) {
      return t;
    }

    return {};
  }

  /**
   * @brief Receives an item from the channel, blocking if the channel is
   * empty.
   *
   *
   * @return The item received from the channel.
   * @exits with RECV_CLOSED_ERR if the channel is closed.
   */
  T recv() {
    // Check if the channel is closed. If it is, throw an exception.
    check_closed("Attempt to receive on a closed channel", RECV_CLOSED_ERR);

    // Wait until the queue is not empty.
    T t;
    while (!q.try_dequeue(t)) {
    }
    check_closed("Attempt to receive on a closed channel", RECV_CLOSED_ERR);
    return t;
  }
};
