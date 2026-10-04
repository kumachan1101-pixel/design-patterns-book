#ifndef IRESERVATIONSTATE_H_INCLUDED
#define IRESERVATIONSTATE_H_INCLUDED

#include "EventDatabase.h"

class TicketReservation;

// 状態ごとの共通操作と、許可されない操作の既定処理を持つ基底クラス
class IReservationState {
public:
    // 引数は操作対象の予約コンテキスト（TicketReservation*）。
    // 既定実装では使わないため仮引数名を省略する。
    // 派生側では reservation と名付ける。

    virtual void reserve(TicketReservation*) {
        std::cout << "現在予約できません\n";
    }

    virtual void pay(TicketReservation*) {
        std::cout << "支払いに適した状態ではありません\n";
    }

    virtual void cancel(TicketReservation*) {
        std::cout << "キャンセルできません\n";
    }

    virtual void hold(TicketReservation*) {
        std::cout << "保留できません\n";
    }

    virtual void expire(TicketReservation*) {
        std::cout << "期限切れ処理は行えません\n";
    }

    // 表示・検証専用。呼び出し側はこの名前で処理を分岐しない。
    virtual const char* stateName() const = 0;

    virtual ~IReservationState() = default;
};

#endif  // IRESERVATIONSTATE_H_INCLUDED
