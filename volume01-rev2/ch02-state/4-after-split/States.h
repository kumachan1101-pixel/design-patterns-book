#ifndef STATES_H_INCLUDED
#define STATES_H_INCLUDED

#include "EventDatabase.h"
#include "IReservationState.h"

class AvailableState : public IReservationState {
public:
    const char* stateName() const override {
        return "Available";
    }

    void reserve(TicketReservation* reservation) override;
};

class ReservedState : public IReservationState {
public:
    const char* stateName() const override {
        return "Reserved";
    }

    void pay(TicketReservation* reservation) override;

    void cancel(TicketReservation* reservation) override;

    void hold(TicketReservation* reservation) override;

    void expire(TicketReservation* reservation) override;
};

class PaidState : public IReservationState {
public:
    const char* stateName() const override {
        return "Paid";
    }
};

// Held（一時保留）：支払い、取消、期限切れを処理する

class HeldState : public IReservationState {
public:
    const char* stateName() const override {
        return "Held";
    }

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
