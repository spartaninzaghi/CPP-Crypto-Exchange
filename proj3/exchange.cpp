/******************************************************************************/
/*                                                                            */
/*   SS 23 CSE 232 PROJECT 3                                                  */
/*                                                                            */
/*   -- Author --                : Mate Narh                                  */
/*                                                                            */
/******************************************************************************/
#include "exchange.hpp"
#include <algorithm>
#include <iterator>
#include <vector>

void Exchange::MakeDeposit(const std::string &username,
                           const std::string &asset, int amount) {
  if (Portfolios.count(username)) Portfolios[username][asset] += amount;
  else Portfolios[username][asset] = amount;
}

void Exchange::PrintUserPortfolios(std::ostream &os) const {
  os << "User Portfolios (in alphabetical order):" << std::endl;
  for (const auto &[username, assets] : Portfolios) {
    os << username << "'s Portfolio: ";
    for (const auto &[asset, amount] : assets) {
      if (amount) os << amount << ' ' << asset << ", ";
    }
    os << std::endl;
  }
}

bool Exchange::WithdrawalIsPossible(const std::string &username,
                                    const std::string &asset, int amount) {
  if (Portfolios.count(username)) {
    const std::map<std::string, int> assets = Portfolios.at(username);
    if (assets.count(asset)) {
      const int balance = Portfolios.at(username).at(asset);
      if ((balance - amount) >= 0) return true;
    }
  }
  return false;
}

bool Exchange::MakeWithdrawal(const std::string &username,
                              const std::string &asset, int amount) {

  if (WithdrawalIsPossible(username, asset, amount)) {
    Portfolios.at(username).at(asset) -= amount;
    return true;
  }
  return false;
};

std::vector<Order> Exchange::GetTradeParticipants(Order &taker) {
  std::vector<Order> filtered;
  std::copy_if(open_orders.cbegin(), open_orders.cend(),
               std::back_inserter(filtered), [taker](Order o) {
                 return (taker.amount) && (o.asset == taker.asset) &&
                        (o.side != taker.side) &&
                        ((taker.side == "Buy") ? (o.price <= taker.price)
                                               : (o.price >= taker.price));
               });
  return filtered;
}

bool Exchange::AddOrder(const Order &order) {
  if (order.side == "Sell") return AddSellOrder(order);
  else return AddBuyOrder(order);
}

bool Exchange::AddBuyOrder(const Order &order) {
  Order taker(order);
  if (!WithdrawalIsPossible(taker.username, "USD",
                            (taker.amount * taker.price))) {
    return false;
  }
  while (GetLowestAsk(taker) != open_orders.end()) ExecuteTakerBuy(taker);
  if (taker.amount) {
    MakeWithdrawal(taker.username, "USD", (taker.amount * taker.price));
    open_orders.push_back(taker);
  }
  return true;
}

bool Exchange::AddSellOrder(const Order &order) {
  Order taker(order);
  if (!WithdrawalIsPossible(taker.username, taker.asset, taker.amount)) {
    return false;
  }
  while (GetHighestBid(taker) != open_orders.end()) ExecuteTakerSell(taker);
  if (taker.amount) {
    MakeWithdrawal(taker.username, taker.asset, taker.amount);
    open_orders.push_back(taker);
  }
  return true;
}

void Exchange::PrintUsersOrders(std::ostream &os) const {
  os << "Users Orders (in alphabetical order):" << std::endl;
  for (const auto &[username, assets] : Portfolios) {
    os << username << "'s Open Orders (in chronological order):" << std::endl;
    for (const auto &o : open_orders) {
      if (username == o.username) os << o << std::endl;
    }
    os << username << "'s Filled Orders (in chronological order):" << std::endl;
    for (const auto &o : filled_orders) {
      if (username == o.username) os << o << std::endl;
    }
  }
}

void Exchange::PrintTradeHistory(std::ostream &os) const {
  os << "Trade History (in chronological order):" << std::endl;
  for (const auto &t : trade_history) {
    os << t.buyer_username << " Bought " << t.amount << " of " << t.asset
       << " From " << t.seller_username << " for " << t.price << " USD"
       << std::endl;
  }
}

void Exchange::PrintBidAskSpread(std::ostream &os) const {
  os << "Asset Bid Ask Spread (in alphabetical order):" << std::endl;
  for (const std::string &asset : GetNamesOfOpenAssets()) {
    const std::string highest_buy = GetHighestBuyForAsset(asset);
    const std::string lowest_sell = GetLowestSellForAsset(asset);
    os << asset << ": Highest Open Buy = " << highest_buy
       << " and Lowest Open Sell = " << lowest_sell << std::endl;
  }
}

void Exchange::TransactTakerBuy(Order &taker, int usd_payment,
                                int amount_sold) {
  auto lowest_seller = GetLowestAsk(taker);
  std::string buyer{taker.username}, seller{lowest_seller->username};
  MakeWithdrawal(buyer, "USD", usd_payment);
  MakeDeposit(seller, "USD", usd_payment);
  MakeDeposit(buyer, taker.asset, amount_sold);
}

void Exchange::TransactTakerSell(Order &taker, int usd_payment,
                                 int amount_bought) {
  auto highest_buyer = GetHighestBid(taker);
  std::string seller{taker.username}, buyer{highest_buyer->username};
  MakeWithdrawal(seller, taker.asset, amount_bought);
  MakeDeposit(buyer, taker.asset, amount_bought);
  MakeDeposit(seller, "USD", usd_payment);
}

void Exchange::FullTakerBuyPartialMakerSell(Order &taker, Trade &trade) {
  auto lowest_seller = GetLowestAsk(taker);
  const std::string buyer{taker.username}, seller{lowest_seller->username};
  const int amount_sold{taker.amount},
      usd_payment{(taker.amount * taker.price)};

  TransactTakerBuy(taker, usd_payment, amount_sold);
  
  Order partial_sell({seller, "Sell", taker.asset, taker.amount, taker.price});
  
  filled_orders.push_back(partial_sell);
  filled_orders.push_back(taker);
  trade_history.push_back(trade);
  
  lowest_seller->amount -= amount_sold;
  taker.amount = 0;
}

void Exchange::FullTakerBuyFullMakerSell(Order &taker, Trade &trade) {
  auto lowest_seller = GetLowestAsk(taker);
  const std::string buyer{taker.username}, seller{lowest_seller->username};
  const int amount_sold{taker.amount},
      usd_payment{(taker.amount * taker.price)};

  TransactTakerBuy(taker, usd_payment, amount_sold);
  
  Order complete_sell({seller, "Sell", taker.asset, taker.amount, taker.price});
  
  open_orders.erase(lowest_seller);
  filled_orders.push_back(complete_sell);
  filled_orders.push_back(taker);
  trade_history.push_back(trade);
  
  taker.amount = 0;
}

void Exchange::PartialTakerBuyFullMakerSell(Order &taker, Trade &trade) {
  auto lowest_seller = GetLowestAsk(taker);
  const std::string buyer{taker.username}, seller{lowest_seller->username};
  const int amount_sold{lowest_seller->amount},
      usd_payment{(amount_sold * taker.price)};
  trade.amount = amount_sold;

  TransactTakerBuy(taker, usd_payment, amount_sold);
  
  Order complete_sell({seller, "Sell", taker.asset, amount_sold, taker.price});
  Order partial_buy({buyer, "Buy", taker.asset, amount_sold, taker.price});
  
  open_orders.erase(lowest_seller);
  filled_orders.push_back(complete_sell);
  filled_orders.push_back(partial_buy);
  trade_history.push_back(trade);
  
  taker.amount -= amount_sold;
}

void Exchange::ExecuteTakerBuy(Order &taker) {
  auto lowest_seller = GetLowestAsk(taker);
  const std::string buyer{taker.username}, seller{lowest_seller->username};
  Trade trade = {buyer, seller, taker.asset, taker.amount, taker.price};

  if (taker.amount < lowest_seller->amount) FullTakerBuyPartialMakerSell(taker, trade);
  else if (taker.amount == lowest_seller->amount) FullTakerBuyFullMakerSell(taker, trade);
  else PartialTakerBuyFullMakerSell(taker, trade);
}

void Exchange::FullTakerSellPartialMakerBuy(Order &taker, Trade &trade) {
  auto highest_buyer = GetHighestBid(taker);
  const std::string seller{taker.username}, buyer{highest_buyer->username};
  const int amount_bought{taker.amount},
      usd_payment{(taker.amount * taker.price)};

  TransactTakerSell(taker, usd_payment, amount_bought);
  
  Order partial_maker_buy(
      {buyer, "Buy", taker.asset, taker.amount, taker.price});
  
  filled_orders.push_back(partial_maker_buy);
  filled_orders.push_back(taker);
  trade_history.push_back(trade);
  
  highest_buyer->amount -= amount_bought;
  taker.amount = 0;
}

void Exchange::FullTakerSellFullMakerBuy(Order &taker, Trade &trade) {
  auto highest_buyer = GetHighestBid(taker);
  const std::string seller{taker.username}, buyer{highest_buyer->username};
  const int amount_bought{taker.amount},
      usd_payment{(taker.amount * taker.price)};

  TransactTakerSell(taker, usd_payment, amount_bought);
  
  Order complete_maker_buy(
      {seller, "Sell", taker.asset, taker.amount, taker.price});
  
  open_orders.erase(highest_buyer);
  filled_orders.push_back(complete_maker_buy);
  filled_orders.push_back(taker);
  trade_history.push_back(trade);
  
  taker.amount = 0;
}

void Exchange::PartialTakerSellFullMakerBuy(Order &taker, Trade &trade) {
  auto highest_buyer = GetHighestBid(taker);
  const std::string seller{taker.username}, buyer{highest_buyer->username};
  const int amount_bought{highest_buyer->amount},
      usd_payment{(amount_bought * taker.price)};
  trade.amount = amount_bought;

  TransactTakerSell(taker, usd_payment, amount_bought);
  
  Order complete_buy({buyer, "Buy", taker.asset, amount_bought, taker.price});
  Order partial_sell({seller, "Sell", taker.asset, amount_bought, taker.price});
  
  open_orders.erase(highest_buyer);
  filled_orders.push_back(complete_buy);
  filled_orders.push_back(partial_sell);
  trade_history.push_back(trade);
  
  taker.amount -= amount_bought;
}

void Exchange::ExecuteTakerSell(Order &taker) {
  auto highest_buyer = GetHighestBid(taker);
  const std::string seller{taker.username}, buyer{highest_buyer->username};
  Trade trade = {buyer, seller, taker.asset, taker.amount, taker.price};

  if (taker.amount < highest_buyer->amount) FullTakerSellPartialMakerBuy(taker, trade);
  else if (taker.amount == highest_buyer->amount) FullTakerSellFullMakerBuy(taker, trade);
  else PartialTakerSellFullMakerBuy(taker, trade);
}

std::vector<int> Exchange::GetOpenBuysForAsset(const std::string &asset) const {
  std::vector<int> buys;
  for (const Order &o : open_orders) {
    if (o.side == "Buy" && o.asset == asset) buys.push_back(o.price);
  }
  return buys;
}

std::vector<int> Exchange::GetOpenSellsForAsset(const std::string &asset) const {
  std::vector<int> sells;
  for (const Order &o : open_orders) {
    if (o.side == "Sell" && o.asset == asset) sells.push_back(o.price);
  }
  return sells;
}

std::string Exchange::GetHighestBuyForAsset(const std::string &asset) const {
  std::ostringstream oss;
  std::vector<int> buys = GetOpenBuysForAsset(asset);
  if (buys.size()) oss << *std::max_element(buys.begin(), buys.end()) << " USD";
  else oss << "NA USD";
  return oss.str();
}

std::string Exchange::GetLowestSellForAsset(const std::string &asset) const {
  std::ostringstream oss;
  std::vector<int> sells = GetOpenSellsForAsset(asset);
  if (sells.size()) oss << *std::min_element(sells.begin(), sells.end()) << " USD";
  else oss << "NA USD";
  return oss.str();
}

std::set<std::string> Exchange::GetNamesOfOpenAssets() const {
  std::set<std::string> open_assets;
  for (const auto &o : open_orders) open_assets.insert(o.asset);
  return open_assets;
}
