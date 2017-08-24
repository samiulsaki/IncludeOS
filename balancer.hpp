#pragma once
#define READQ_PER_CLIENT     4096
#define MAX_READQ_PER_NODE   8192
#define READQ_FOR_NODES      8192
#define MAX_OUTGOING_ATTEMPTS 100
// checking if nodes are dead or not
#define ACTIVE_INITIAL_PERIOD     8s
#define ACTIVE_CHECK_PERIOD      30s
// connection attempt timeouts
#define CONNECT_TIMEOUT          10s
#define CONNECT_THROW_PERIOD     20s
#define INITIAL_SESSION_TIMEOUT   5s
#define ROLLING_SESSION_TIMEOUT  60s

#include <net/inet4>
typedef net::Inet<net::IP4> netstack_t;
typedef net::tcp::Connection_ptr tcp_ptr;
typedef std::vector< std::pair<net::tcp::buffer_t, size_t> > queue_vector_t;
typedef delegate<void()> pool_signal_t;

struct Waiting {
  Waiting(tcp_ptr);

  tcp_ptr conn;
  queue_vector_t readq;
  int total = 0;
};

struct Nodes;
struct Session {
  Session(Nodes&, int idx, tcp_ptr inc, tcp_ptr out);
  void handle_timeout();

  Nodes&    parent;
  const int self;
  int       timeout_timer;
  tcp_ptr   incoming;
  tcp_ptr   outgoing;
};

struct Node {
  Node(netstack_t& stk, net::Socket a, pool_signal_t sig);

  auto address() const noexcept { return this->addr; }
  int  connection_attempts() const noexcept { return this->connecting; }
  int  pool_size() const noexcept { return pool.size(); }
  bool is_active() const noexcept { return active; };

  void    restart_active_check();
  void    perform_active_check();
  void    stop_active_check();
  void    connect();
  tcp_ptr get_connection();

private:
  netstack_t& stack;
  net::Socket addr;
  pool_signal_t pool_signal;
  std::vector<tcp_ptr> pool;
  bool        active = false;
  int         active_timer = -1;
  signed int  connecting = 0;
};

struct Nodes {
  typedef std::deque<Node> nodevec_t;
  typedef nodevec_t::iterator iterator;
  typedef nodevec_t::const_iterator const_iterator;
  Nodes() {}

  size_t   size() const noexcept;
  const_iterator begin() const;
  const_iterator end() const;

  int32_t open_sessions() const;
  int64_t total_sessions() const;
  int  pool_connecting() const;
  int  pool_size() const;

  template <typename... Args>
  void add_node(Args&&... args);
  void create_connections(int total);
  bool assign(tcp_ptr, queue_vector_t&);
  void create_session(tcp_ptr inc, tcp_ptr out);
  void close_session(int);

private:
  nodevec_t nodes;
  int64_t   session_total = 0;
  int       session_cnt = 0;
  int       conn_iterator = 0;
  int       algo_iterator = 0;
  std::deque<Session> sessions;
  std::vector<int> free_sessions;
};

struct Balancer {
  Balancer(netstack_t& in, uint16_t port,
           netstack_t& out, std::vector<net::Socket> nodes);

  void incoming(tcp_ptr);
  int  wait_queue() const;

  netstack_t& netin;
  Nodes nodes;

private:
  void handle_connections();
  void handle_queue();

  std::deque<Waiting> queue;
  int rethrow_timer = -1;
};
