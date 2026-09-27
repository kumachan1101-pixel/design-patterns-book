#include "States.h"
#include "TicketReservation.h"

void AvailableState::reserve(TicketReservation* reservation) {
        if (!reservation->hasCapacity()) {
            std::cout << "エラー：" << reservation->eventTitle()
                      << " は満席です\n";
            return;
        }

        std::cout << "予約対象："

                  << reservation->eventTitle() << "\n";

        reservation->reserveSeat();
        std::cout << "予約完了しました\n";
        reservation->setState(reservedState());
    }

void ReservedState::pay(TicketReservation* reservation) {
        std::cout << "支払い完了しました\n";
        reservation->setState(paidState());
    }

void ReservedState::cancel(TicketReservation* reservation) {
        reservation->cancelSeat();
        std::cout << "予約をキャンセルしました\n";
        reservation->setState(availableState());
    }

void ReservedState::hold(TicketReservation* reservation) {
        std::cout << "一時保留にしました（支払期限を24時間へ延長）\n";
        reservation->setState(heldState());
    }

void ReservedState::expire(TicketReservation* reservation) {
        reservation->cancelSeat();
        std::cout << "通常の決済期限が切れました\n";
        reservation->setState(availableState());
    }

void HeldState::pay(TicketReservation* reservation) {
        std::cout << "保留から支払い完了しました\n";
        reservation->setState(paidState());
    }

void HeldState::cancel(TicketReservation* reservation) {
        reservation->cancelSeat();
        std::cout << "保留からキャンセルしました\n";
        reservation->setState(availableState());
    }

void HeldState::expire(TicketReservation* reservation) {
        reservation->cancelSeat();
        std::cout << "保留期限が切れました\n";
        reservation->setState(availableState());
    }

IReservationState* availableState() {
    static AvailableState state;

    return &state;
}
IReservationState* reservedState() {
    static ReservedState state;

    return &state;
}
IReservationState* paidState() {
    static PaidState state;

    return &state;
}
IReservationState* heldState() {
    static HeldState state;

    return &state;
}
