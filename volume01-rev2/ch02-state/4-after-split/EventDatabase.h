#ifndef EVENTDATABASE_H_INCLUDED
#define EVENTDATABASE_H_INCLUDED

#include <iostream>
#include <string>
#include <map>
#include <list>

class TicketReservation;
class IReservationState;

struct EventInfo {
    std::string title;   // イベント名
    int capacity;        // 定員
    int reserved;        // 現在の予約数
};

class EventDatabase {
private:
    // イベントIDをキーに、イベント情報を保持する
    std::map<std::string, EventInfo> records;
public:
    EventDatabase() {
        // []で、キーに対応する値を登録する
        records["EVT001"] = {"春の音楽祭",  100,  20};
        records["EVT002"] = {"夏のフェス",  500, 499};
        records["EVT003"] = {"秋の映画会",   50,  50};  // 満席
    }

    bool exists(const std::string& id) const {
        // countは、キーがあれば1、なければ0を返す
        return records.count(id) > 0;
    }

    EventInfo get(const std::string& id) const {
        // atで、登録済みキーに対応する値を取得する
        return records.at(id);
    }

    bool hasCapacity(const std::string& id) const {
        const auto& e = records.at(id);

        return e.reserved < e.capacity;
    }

    void reserveSeat(const std::string& id) {
        // 値そのものを参照し、map内の予約数を書き換える
        auto& event = records.at(id);
        int before = event.reserved;
        ++event.reserved;
        std::cout << "[予約数] " << id << " "
                  << before << "/" << event.capacity
                  << " -> " << event.reserved << "/"
                  << event.capacity;
        if (event.reserved == event.capacity)
            std::cout << "（満席）";
        std::cout << std::endl;
    }

    void cancelSeat(const std::string& id) {
        auto& event = records.at(id);
        int before = event.reserved;

        if (event.reserved > 0) --event.reserved;
        std::cout << "[予約数] " << id << " "
                  << before << "/" << event.capacity
                  << " -> " << event.reserved << "/"
                  << event.capacity
                  << std::endl;
    }
};

#endif  // EVENTDATABASE_H_INCLUDED
