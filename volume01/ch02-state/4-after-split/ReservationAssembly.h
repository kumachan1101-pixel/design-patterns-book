#ifndef RESERVATIONASSEMBLY_H_INCLUDED
#define RESERVATIONASSEMBLY_H_INCLUDED

#include "States.h"
#include "TicketReservation.h"

// ReservationAssembly：共有実体の生成・所有・受け渡しを担う

class ReservationAssembly {
    EventDatabase db;
    // 追加後も、先に返した予約への参照が無効にならない列
    std::list<TicketReservation> reservations;
public:
    TicketReservation& startReservation(
            const std::string& eventId) {
        // 予約をlistの末尾に直接生成する
        reservations.emplace_back(
            availableState(), &db, eventId);
        // 直前に追加した末尾の予約を参照で返す。削除はしない
        return reservations.back();
    }
};

#endif  // RESERVATIONASSEMBLY_H_INCLUDED
