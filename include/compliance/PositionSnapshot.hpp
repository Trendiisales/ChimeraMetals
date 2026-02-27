#pragma once
#include <fstream>
#include <string>

struct PositionSnapshot
{
    std::string symbol;
    int direction;
    double size;
    double avg_price;
    double daily_pnl;
};

class PositionPersistence
{
public:
    void save(const PositionSnapshot& p)
    {
        std::ofstream out("position_snapshot.dat", std::ios::trunc);
        out << p.symbol << "\n"
            << p.direction << "\n"
            << p.size << "\n"
            << p.avg_price << "\n"
            << p.daily_pnl << "\n";
    }

    bool load(PositionSnapshot& p)
    {
        std::ifstream in("position_snapshot.dat");
        if (!in.good()) return false;

        in >> p.symbol;
        in >> p.direction;
        in >> p.size;
        in >> p.avg_price;
        in >> p.daily_pnl;
        return true;
    }

    void clear()
    {
        std::ofstream out("position_snapshot.dat", std::ios::trunc);
    }
};
