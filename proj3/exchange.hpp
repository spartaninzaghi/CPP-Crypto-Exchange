#pragma once
#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "useraccount.hpp"
#include "utility.hpp"

class Exchange {
public:
  // 1 Portfolio Data Structure
  std::map<std::string, std::map<std::string, int>> Portfolios;

  // 2 Helper Containers
  std::vector<Order> open_orders = {};
  std::vector<Order> filled_orders = {};
  std::vector<Trade> trade_history = {};

  // 3 Depositor & Withdrawer
  void MakeDeposit(const std::string &username, const std::string &asset,
                   int amount);
  bool WithdrawalIsPossible(const std::string &username,
                            const std::string &asset, int amount);
  bool MakeWithdrawal(const std::string &username, const std::string &asset,
                      int amount);

  // 4 Order Adders
  bool AddOrder(const Order &order);
  bool AddBuyOrder(const Order &order);
  bool AddSellOrder(const Order &order);

  // 5 Judge For Order Entrants
  std::vector<Order> GetTradeParticipants(Order &taker);

  // 6 Order Executors
  void ExecuteTakerBuy(Order &order);
  void ExecuteTakerSell(Order &order);
  void TransactTakerBuy(Order &taker, int usd_payment, int amount_sold);
  void TransactTakerSell(Order &taker, int usd_payment, int amount_bought);

  // 6a Taker<buy> Helpers
  void FullTakerBuyPartialMakerSell(Order &taker, Trade &trade);
  void FullTakerBuyFullMakerSell(Order &taker, Trade &trade);
  void PartialTakerBuyFullMakerSell(Order &taker, Trade &trade);

  // 6b Taker<sell> Helpers
  void FullTakerSellPartialMakerBuy(Order &taker, Trade &trade);
  void FullTakerSellFullMakerBuy(Order &taker, Trade &trade);
  void PartialTakerSellFullMakerBuy(Order &taker, Trade &trade); 

  // 7 Printers
  void PrintUserPortfolios(std::ostream &os) const;
  void PrintUsersOrders(std::ostream &os) const;
  void PrintTradeHistory(std::ostream &os) const;
  void PrintBidAskSpread(std::ostream &os) const;

  // 7b PrintBidAskPrice() Helpers
  std::set<std::string> GetNamesOfOpenAssets() const;
  std::string GetHighestBuyForAsset(const std::string &asset) const;
  std::string GetLowestSellForAsset(const std::string &asset) const;
  std::vector<int> GetOpenBuysForAsset(const std::string &asset) const;
  std::vector<int> GetOpenSellsForAsset(const std::string &asset) const;

  // 8 Highest<bid> Lowest<ask> Filters
  auto GetLowestAsk(Order &taker) {
    auto p = GetTradeParticipants(taker);
    if (p.size()) {
      auto min_p_iter = std::min_element(p.begin(), p.end(),
                                         [](const Order &o1, const Order &o2) {
                                           return (o1.price < o2.price);
                                         });
      return std::find(open_orders.begin(), open_orders.end(), *min_p_iter);
    }
    return open_orders.end();
  }

  auto GetHighestBid(Order &taker) {
    auto p = GetTradeParticipants(taker);
    if (p.size()) {
      auto max_p_iter = std::max_element(p.begin(), p.end(),
                                         [](const Order &o1, const Order &o2) {
                                           if (o1.price != o2.price) {
                                             return o1.price < o2.price;
                                           } else {
                                             return o1.username < o2.username;
                                           }
                                         });
      return std::find(open_orders.begin(), open_orders.end(), *max_p_iter);
    }
    return open_orders.end();
  }
};
