#pragma once
#include <iostream>
#include <sstream>
#include <string>

class Order {
public:
  std::string username;
  std::string side; // Can be "Buy" or "Sell"
  std::string asset;
  int amount;
  int price;

  // Constructors
  // 5-Arg constructor
  Order(const std::string &u, const std::string &s, const std::string &a, int q,
        int p)
      : username(u), side(s), asset(a), amount(q), price(p) {}

  // copy constructor
  Order(const Order &o)
      : username(o.username), side(o.side), asset(o.asset), amount(o.amount),
        price(o.price) {}

  // operator== `method`
  bool operator==(const Order &o) const;

  // friend: operator<< `function`
  friend std::ostream &operator<<(std::ostream &os, const Order &o);
};

struct Trade {
  std::string buyer_username;
  std::string seller_username;
  std::string asset;
  int amount;
  int price;
};
