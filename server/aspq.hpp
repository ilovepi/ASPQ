#ifndef ASPQ_HPP_
#define ASPQ_HPP_

#include <boost/asio.hpp>
#include <boost/lockfree/queue.hpp>
#include <atomic>

class spq {
public:
  spq();
  ~spq();

  void add();
  void reomve();

private:
  queue _high;

  queue _low;

  int high_capacity;
  int low_capacity;
};
class aspq : public spq {};

#endif
