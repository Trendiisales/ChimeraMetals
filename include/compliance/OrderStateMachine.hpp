#pragma once
#include <unordered_map>
#include <string>

enum class OrderStatus
{
    NEW,
    ACKED,
    PARTIAL,
    FILLED,
    CANCELED,
    REJECTED
};

struct OrderRecord
{
    std::string clOrdId;
    OrderStatus status;
    double filled_qty{0.0};
    double avg_price{0.0};
};

class OrderStateMachine
{
public:
    void onNew(const std::string& id)
    {
        orders_[id] = {id, OrderStatus::NEW};
    }

    void onAck(const std::string& id)
    {
        orders_[id].status = OrderStatus::ACKED;
    }

    void onPartial(const std::string& id, double qty, double price)
    {
        orders_[id].status = OrderStatus::PARTIAL;
        orders_[id].filled_qty += qty;
        orders_[id].avg_price = price;
    }

    void onFill(const std::string& id, double qty, double price)
    {
        orders_[id].status = OrderStatus::FILLED;
        orders_[id].filled_qty += qty;
        orders_[id].avg_price = price;
    }

    void onReject(const std::string& id)
    {
        orders_[id].status = OrderStatus::REJECTED;
    }

    bool isRejected(const std::string& id) const
    {
        auto it = orders_.find(id);
        if (it == orders_.end()) return false;
        return it->second.status == OrderStatus::REJECTED;
    }

private:
    std::unordered_map<std::string, OrderRecord> orders_;
};
