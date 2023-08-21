#include "utility.hpp"

bool Order::operator==(const Order &o) const {
  return (username == o.username) && (side == o.side) && (asset == o.asset) &&
         (amount == o.amount) && (price == o.price);
};

std::ostream &operator<<(std::ostream &os, const Order &o) {
  os << o.side << ' ' << o.amount << ' ' << o.asset << " at " << o.price
     << " USD by " << o.username;
  return os;
}