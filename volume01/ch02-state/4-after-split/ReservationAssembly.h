#ifndef RESERVATIONASSEMBLY_H_INCLUDED
#define RESERVATIONASSEMBLY_H_INCLUDED

#include "States.h"
#include "TicketReservation.h"

// ReservationAssembly：共有実体の生成・所有・受け渡しを担う

class ReservationAssembly {
    EventDatabase db;
    std::list<TicketReservation> reservations;
public:
    TicketReservation& startReservation(
            const std::string& eventId) {
        reservations.emplace_back(
            availableState(), &db, eventId);
        return reservations.back();
    }
};

#endif  // RESERVATIONASSEMBLY_H_INCLUDED
