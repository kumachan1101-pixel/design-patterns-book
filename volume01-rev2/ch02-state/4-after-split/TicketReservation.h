#ifndef TICKETRESERVATION_H_INCLUDED
#define TICKETRESERVATION_H_INCLUDED

#include "EventDatabase.h"
#include "IReservationState.h"

class TicketReservation;

// 状態ごとの共通操作と、許可されない操作の既定処理を持つ基底クラス

class TicketReservation {
private:
    IReservationState* state;
    EventDatabase* db;           // 席数の保存データ（境界）
    std::string eventId;

public:
    TicketReservation(IReservationState* initialState,
                      EventDatabase* db,
                      const std::string& eventId)
        : state(initialState), db(db), eventId(eventId) {}

    // 状態遷移時に、共有状態オブジェクトへの借用ポインタを差し替える。
    // 状態は関数ローカルstaticが所有するため、ここではdeleteしない。
    void setState(IReservationState* nextState) {
        state = nextState;
    }

    // 状態遷移の副作用：予約数の増減
    void reserveSeat() { db->reserveSeat(eventId); }
    void cancelSeat()  { db->cancelSeat(eventId); }
    bool hasCapacity() const {
        return db->hasCapacity(eventId);
    }
    // イベントの有無は状態によらないので、ここで確かめる
    bool exists() const {
        if (db->exists(eventId)) return true;

        std::cout << "エラー：イベントID " << eventId
                  << " は存在しません\n";
        return false;
    }

    bool showAvailability() const {
        if (!exists()) return false;

        EventInfo info = db->get(eventId);
        int available = info.capacity - info.reserved;
        std::cout << "[席数確認] " << eventId << " "
                  << info.reserved << "/" << info.capacity
                  << "（空席" << available << "）";
        if (available == 0) std::cout << "（満席）";
        std::cout << std::endl;

        return true;
    }
    std::string eventTitle() const {
        return db->get(eventId).title;
    }
    const char* currentStateName() const {
        return state->stateName();
    }
    int reservedCount() const {
        return db->get(eventId).reserved;
    }

    // 操作は現在の状態へ委譲する。
    // 未登録のイベントIDだけは、状態に関係なく先に断る。
    void reserve() { if (exists()) state->reserve(this); }
    void pay()     { if (exists()) state->pay(this); }
    void cancel()  { if (exists()) state->cancel(this); }
    void hold()    { if (exists()) state->hold(this); }
    void expire()  { if (exists()) state->expire(this); }
};

#endif  // TICKETRESERVATION_H_INCLUDED
