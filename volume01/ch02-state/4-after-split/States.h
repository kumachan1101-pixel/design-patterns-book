#ifndef STATES_H_INCLUDED
#define STATES_H_INCLUDED

#include "EventDatabase.h"
#include "IReservationState.h"

class AvailableState : public IReservationState {
public:
    void reserve(TicketReservation* reservation) override;
};

class ReservedState : public IReservationState {
public:
    void pay(TicketReservation* reservation) override;

    void cancel(TicketReservation* reservation) override;

    void hold(TicketReservation* reservation) override;

    void expire(TicketReservation* reservation) override;
};

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
