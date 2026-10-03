#ifndef BATCHAPPLICATION_H_INCLUDED
#define BATCHAPPLICATION_H_INCLUDED

#include "ReservationAssembly.h"

// BatchApplication：入力例の実行と結果表示を担う

class BatchApplication {
    ReservationAssembly assembly;

public:
    void run() {
        // ケース1：通常予約フロー (Available → Reserved → Paid)
        std::cout << "--- ケース1: 通常予約 ---\n";

        TicketReservation& seat1 =
            assembly.startReservation("EVT001");
        if (seat1.showAvailability()) {
            seat1.reserve();
            seat1.pay();
            std::cout << "[状態確認] 状態="
                      << seat1.currentStateName()
                      << " 予約数="
                      << seat1.reservedCount() << "\n";
        }

        // ケース2：通常キャンセル (Available → Reserved → Available)
        std::cout << "--- ケース2: 通常キャンセル ---\n";

        TicketReservation& seat2 =
            assembly.startReservation("EVT001");
        if (seat2.showAvailability()) {
            seat2.reserve();
            seat2.cancel();
        }

        // ケース3：一時保留と支払い
        // (Available → Reserved → Held → Paid)
        std::cout << "--- ケース3: 一時保留と支払い ---\n";

        TicketReservation& seat3 =
            assembly.startReservation("EVT002");
        if (seat3.showAvailability()) {
            seat3.reserve();
            seat3.hold();
            seat3.pay();
            std::cout << "[状態確認] 状態="
                      << seat3.currentStateName()
                      << " 予約数="
                      << seat3.reservedCount() << "\n";
        }

        // ケース4：通常の決済期限切れ (Reserved → Available)
        std::cout << "--- ケース4: 通常の決済期限切れ ---\n";

        TicketReservation& seat4 =
            assembly.startReservation("EVT001");
        if (seat4.showAvailability()) {
            seat4.reserve();
            // 実行例から「15分経過」を即時再現する。
            // 本番ではタイマー基盤が同じexpire()を呼ぶ。
            seat4.expire();
        }

        // ケース5：保留期限切れ
        // (Available → Reserved → Held → Available)
        std::cout << "--- ケース5: 保留期限切れ ---\n";

        TicketReservation& seat5 =
            assembly.startReservation("EVT001");
        if (seat5.showAvailability()) {
            seat5.reserve();
            seat5.hold();
            // 「24時間経過」を同じ入口で再現する
            seat5.expire();
        }

        // ケース6：一時保留からのキャンセル (Held → Available)
        std::cout << "--- ケース6: 保留からのキャンセル ---\n";

        TicketReservation& seat6 =
            assembly.startReservation("EVT001");
        if (seat6.showAvailability()) {
            seat6.reserve();
            seat6.hold();
            seat6.cancel();
        }

        // ケース7：満席イベントの予約 (既存動作の回帰確認)
        std::cout << "--- ケース7: 満席イベントの予約 ---\n";

        TicketReservation& full =
            assembly.startReservation("EVT003");
        full.showAvailability();
        full.reserve();

        // ケース8：無効な操作の拒否 (Available → pay、Paid → cancel)
        std::cout << "--- ケース8: 無効な操作の拒否 ---\n";

        TicketReservation& seat8 =
            assembly.startReservation("EVT001");
        std::cout << "[不変確認] Availableでpay前 状態="
                  << seat8.currentStateName()
                  << " 予約数=" << seat8.reservedCount() << "\n";
        seat8.pay();      // Available では支払えない
        std::cout << "[不変確認] Availableでpay後 状態="
                  << seat8.currentStateName()
                  << " 予約数=" << seat8.reservedCount() << "\n";
        seat8.reserve();
        seat8.pay();
        std::cout << "[不変確認] Paidでcancel前 状態="
                  << seat8.currentStateName()
                  << " 予約数=" << seat8.reservedCount() << "\n";
        seat8.cancel();   // Paid からは取り消せない
        std::cout << "[不変確認] Paidでcancel後 状態="
                  << seat8.currentStateName()
                  << " 予約数=" << seat8.reservedCount() << "\n";

        // ケース9：存在しないイベントIDのエラー
        std::cout << "--- ケース9: 存在しないイベントID ---\n";
        TicketReservation& missing =
            assembly.startReservation("EVT999");
        missing.reserve();

        // ケース10：未登録IDへ支払う（状態へ渡る前に断る）
        std::cout << "--- ケース10: 未登録IDへ支払う ---\n";
        TicketReservation& missingPay =
            assembly.startReservation("EVT999");
        missingPay.pay();
    }
};

#endif  // BATCHAPPLICATION_H_INCLUDED
