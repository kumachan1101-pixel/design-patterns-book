#ifndef STATES_H_INCLUDED
#define STATES_H_INCLUDED

#include "EventDatabase.h"
#include "IReservationState.h"

// Available（予約可能）：空席があれば予約し、満席なら断る
class AvailableState : public IReservationState {
public:
    void reserve(TicketReservation* reservation) override;
};

// Reserved（予約済み）：支払い、取消、一時保留、期限切れを処理する
class ReservedState : public IReservationState {
public:
    void pay(TicketReservation* reservation) override;

    void cancel(TicketReservation* reservation) override;

    void hold(TicketReservation* reservation) override;

    void expire(TicketReservation* reservation) override;
};

// Paid（支払い済み）：完了状態のため、すべて既定の拒否を使う
class PaidState : public IReservationState {};

// Held（一時保留）：支払い、取消、期限切れを処理する
class HeldState : public IReservationState {
public:
    void pay(TicketReservation* reservation) override;

    void cancel(TicketReservation* reservation) override;

    void expire(TicketReservation* reservation) override;
};

// 状態オブジェクト取得関数。関数ローカルstaticが所有する。
IReservationState* availableState();

IReservationState* reservedState();

IReservationState* paidState();

IReservationState* heldState();

#endif  // STATES_H_INCLUDED
