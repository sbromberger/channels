#pragma once
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <unordered_map>

namespace channel {

constexpr int RECV_CLOSED_ERR = 3;
constexpr int CLOSE_CLOSED_ERR = 4;

class ChannelClosedException : public std::runtime_error {
public:
  ChannelClosedException(const std::string &msg) : std::runtime_error(msg){};
};

static const std::unordered_map<int, std::string> errs = {
    {RECV_CLOSED_ERR, "Attempt to receive on closed channel"},
    {CLOSE_CLOSED_ERR, "Attempt to close a previously-closed channel"},
};

template <typename T> struct buffer {
  size_t cap{}; // if the capacity is set to zero, unlimited capacity.
  std::mutex mu{};
  std::condition_variable _waiting_to_send{};
  std::condition_variable _waiting_to_receive{};
  std::queue<T> q{};
  bool _closed = false;

  // constructor
  explicit buffer(size_t cap) : cap(cap){};
  buffer(const buffer &) = delete;
  void operator=(const buffer &) = delete;

  // reports the number of elements in the channel.
  size_t size() {
    std::unique_lock _(mu);
    return q.size();
  }

  bool empty() {
    std::unique_lock _(mu);
    return q.empty();
  }

  /**
   * @brief Checks if the channel is closed and empty.
   *
   * @return true if the channel is closed and empty, false otherwise.
   */
  bool finished() {
    std::unique_lock _(mu);
    return _closed && q.empty();
  }

  /**
   * @brief Checks if the channel is closed.
   *
   * @return true if the channel is closed, false otherwise.
   */
  bool closed() {
    std::unique_lock _(mu);
    return _closed;
  }

  /**
   * @brief Closes the channel.
   *
   * This function closes the channel. Elements in the channel are
   * not discarded and can be subsequently received.
   *
   * @exits with ChannelClosedException if the channel is already closed.
   */
  void close() {
    std::unique_lock _(mu);
    if (_closed) {
      throw ChannelClosedException(errs.at(CLOSE_CLOSED_ERR));
    }
    _closed = true;
    _waiting_to_send.notify_all();
    _waiting_to_receive.notify_all();
  }

  bool send(T el) {
    std::unique_lock l(mu);
    _waiting_to_send.wait(
        l, [this] { return _closed || (cap == 0 || q.size() < cap); });
    if (_closed) {
      return false;
    }

    q.push(std::move(el));
    _waiting_to_receive.notify_all();
    return true;
  }

  T recv() {
    std::unique_lock l(mu);
    _waiting_to_receive.wait(l, [this] { return _closed || !q.empty(); });
    if (q.finished()) {
      throw ChannelClosedException(
          "Attempt to receive on a closed empty channel");
    }
    T t = std::move(q.front());
    q.pop();
    if (!_closed) {
      _waiting_to_send.notify_all();
    }
    return t;
  }

  std::optional<T> try_recv() {
    std::unique_lock _(mu);
    if (q.empty()) {
      return {};
    }
    T t = std::move(q.front());
    q.pop();
    if (!_closed) {
      _waiting_to_send.notify_all();
    }
    return t;
  }
};

template <class> class send_channel;
template <class> class recv_channel;

template <typename T> class channel {
  std::shared_ptr<buffer<T>> buf_p;

public:
  friend class send_channel<T>;
  friend class recv_channel<T>;

  friend void operator>>(T &&t, channel<T> &ch) { ch.send(std::move(t)); }
  bool send(T el) { return buf_p->send(el); }
  T recv() { return buf_p->recv(); }
  std::optional<T> try_recv() { return buf_p->try_recv(); }
  friend void operator<<(T &t, channel<T> &ch) { t = ch.recv(); }
  void close() { buf_p->close(); }
  // we can make these const even though buf_p->closed() cannot be due to the
  // mutex.
  [[nodiscard]] bool closed() const { return buf_p->closed(); }
  [[nodiscard]] size_t size() const { return buf_p->size(); }
  [[nodiscard]] bool finished() const { return buf_p->finished(); }
  [[nodiscard]] bool empty() const { return buf_p->empty(); }

  std::pair<send_channel<T>, recv_channel<T>> split() const {
    return std::make_pair(*this, *this);
  }

  // constructor
  explicit channel(size_t cap) : buf_p(std::make_shared<buffer<T>>(cap)) {}
};

template <typename T> class send_channel {
  std::shared_ptr<buffer<T>> buf_p;

public:
  // let's make the copy internally so the user doesn't have to do it.
  send_channel(const channel<T> &ch) : buf_p(ch.buf_p){};
  // we move here because in split, we pass a copy which already increases the
  // refct.
  // send_channel(channel<T> ch) : buf_p(std::move(ch.buf_p)){};
  friend void operator>>(T &&t, send_channel<T> &ch) { ch.send(std::move(t)); }
  friend void operator>>(const T &t, send_channel<T> &ch) { ch.send(t); }
  // TODO: what is the proper way to do send?
  // do we need a T &&el as well for temporary variables?
  bool send(T el) { return buf_p->send(std::move(el)); }
  bool send(T &&el) { return buf_p->send(std::move(el)); }
  void close() { buf_p->close(); }
  [[nodiscard]] bool closed() const { return buf_p->closed(); }
  [[nodiscard]] bool finished() const { return buf_p->finished(); }
  [[nodiscard]] size_t size() const { return buf_p->size(); }
  [[nodiscard]] bool empty() const { return buf_p->empty(); }
};

template <typename T> class recv_channel {
  std::shared_ptr<buffer<T>> buf_p;

public:
  recv_channel(const channel<T> &ch) : buf_p(ch.buf_p){};
  // recv_channel(channel<T> ch) : buf_p(std::move(ch.buf_p)){};
  friend void operator<<(T &t, recv_channel<T> &ch) { t = ch.recv(); }
  T recv() { return buf_p->recv(); }
  std::optional<T> recv_immed() { return buf_p->try_recv(); }
  void close() { buf_p->close(); }
  [[nodiscard]] bool closed() const { return buf_p->closed(); }
  [[nodiscard]] bool finished() const { return buf_p->finished(); }
  [[nodiscard]] size_t size() const { return buf_p->size(); }
  [[nodiscard]] bool empty() const { return buf_p->empty(); }
};

template <typename T>
static std::pair<send_channel<T>, recv_channel<T>> channels(size_t cap) {
  auto c = channel<T>(cap);
  return c.split();
}

} // namespace channel
