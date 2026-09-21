#include <iostream>
#include <string>
#include <map>

struct EventInfo {
    std::string title;   // イベント名
    int capacity;        // 定員
    int reserved;        // 現在の予約数
};

class EventDatabase {
private:
    std::map<std::string, EventInfo> records;
public:
    // この章で使うイベント台帳を初期化する
    EventDatabase() {
        records["EVT001"] = {"春の音楽祭",  100,  20};
        records["EVT002"] = {"夏のフェス",  500, 499};
        records["EVT003"] = {"秋の映画会",   50,  50};  // 満席
    }

    // 指定したイベントIDが台帳にあるかを返す
    bool exists(const std::string& id) const {
        return records.count(id) > 0;
    }

    // 指定したイベントの現在値を返す
    EventInfo get(const std::string& id) const {
        return records.at(id);
    }

    // 定員と予約数を比較し、1席以上空いているかを返す
    bool hasCapacity(const std::string& id) const {
        const auto& e = records.at(id);

        return e.reserved < e.capacity;
    }

    // 予約成立時に予約数を1増やす
    void reserveSeat(const std::string& id) {
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

    // 取消成立時に予約数を1減らす
    void cancelSeat(const std::string& id) {
        auto& event = records.at(id);
        int before = event.reserved;

        if (event.reserved > 0) --event.reserved;
        std::cout << "[予約数] " << id << " "
                  << before << "/" << event.capacity
                  << " -> " << event.reserved << "/"
                  << event.capacity << std::endl;
    }
};

enum class ReservationStatus {
    Available,
    Reserved,
    Paid,
    Held        // ← 追加
};

// 変更後の TicketReservation（Held を追加した状態）
// 在庫（EventDatabase）は現状コードのものをそのまま使う想定で、この抜粋では
// 状態遷移に絡む部分だけを示す。
class TicketReservation {
    // --- データ ---
    EventDatabase& db;                 // 現状と同じイベント台帳
    std::string eventId;               // 現状と同じ予約対象
    ReservationStatus status;          // 現在の予約状態

    // 各状態で操作できないときのエラー出力
    void handleReserveError()  { std::cout << "現在予約できません\n"; }
    void handleHoldError()     { std::cout << "保留できません\n"; }
    void handlePayError() {
        std::cout << "支払いに適した状態ではありません\n";
    }
    void handleCancelError()   { std::cout << "キャンセルできません\n"; }
    void handleExpireError() {
        std::cout << "期限切れ処理は行えません\n";
    }
public:
    TicketReservation(EventDatabase& db,
                      const std::string& eventId)
        : db(db), eventId(eventId),
          status(ReservationStatus::Available) {}

    void reserve();
    void hold();                    // ← 追加
    void pay();
    void cancel();
    void expire();                  // ← 追加
};

// 通常の予約要求。現状と同じEventDatabaseで存在と空席を確認する。
void TicketReservation::reserve() {
    if (!db.exists(eventId)) {
        std::cout << "エラー：イベントID " << eventId
                  << " は存在しません\n";
        return;
    }

    if (!db.hasCapacity(eventId)) {
        std::cout << "エラー：" << db.get(eventId).title
                  << " は満席です\n";
        return;
    }

    if (status == ReservationStatus::Available) {
        std::cout << "予約対象："
                  << db.get(eventId).title << "\n";
        db.reserveSeat(eventId);
        status = ReservationStatus::Reserved;
        std::cout << "予約完了しました\n";
    } else {
        handleReserveError();
    }
}

// 一時保留する（Reserved のときだけ24時間の保留枠へ）
void TicketReservation::hold() {
    if (status == ReservationStatus::Reserved) {
        status = ReservationStatus::Held;
        std::cout << "一時保留にしました（支払期限を24時間へ延長）\n";
    } else {
        handleHoldError();
    }
}

// 支払う（Reserved と Held の両方から支払済みへ）
void TicketReservation::pay() {
    if (status == ReservationStatus::Reserved) {
        status = ReservationStatus::Paid;
        std::cout << "支払い完了しました\n";
    } else if (status == ReservationStatus::Held) {
        status = ReservationStatus::Paid;
        std::cout << "保留から支払い完了しました\n";
    } else {
        handlePayError();
    }
}

// キャンセルする（Reserved と Held から予約可能へ）
void TicketReservation::cancel() {
    if (status == ReservationStatus::Reserved) {
        db.cancelSeat(eventId);
        status = ReservationStatus::Available;
        std::cout << "予約をキャンセルしました\n";
    } else if (status == ReservationStatus::Held) {
        db.cancelSeat(eventId);
        status = ReservationStatus::Available;
        std::cout << "保留からキャンセルしました\n";
    } else {
        handleCancelError();
    }
}

// 決済期限切れ（Reservedは15分、Heldは24時間で枠を解放）
void TicketReservation::expire() {
    if (status == ReservationStatus::Reserved) {
        db.cancelSeat(eventId);
        status = ReservationStatus::Available;
        std::cout << "通常の決済期限が切れました\n";
    } else if (status == ReservationStatus::Held) {
        db.cancelSeat(eventId);
        status = ReservationStatus::Available;
        std::cout << "保留期限が切れました\n";
    } else {
        handleExpireError();
    }
}

int main() {
    EventDatabase db;  // 現状コードと同じイベント台帳

    // ケース1：予約 → 一時保留 → 支払い
    TicketReservation t1(db, "EVT001");
    t1.reserve();  // Available → Reserved（予約数+1）
    t1.hold();     // Reserved  → Held（席は確保したまま）
    t1.pay();      // Held      → Paid

    std::cout << "---" << std::endl;
    // ケース2：予約 → 一時保留 → 期限切れ → Available に戻る
    TicketReservation t2(db, "EVT001");
    t2.reserve();  // Available → Reserved（予約数+1）
    t2.hold();     // Reserved  → Held（期限は24時間）
    // 24時間の経過を、タイマーの代わりにここで再現する
    t2.expire();   // Held      → Available（予約数-1）

    std::cout << "---" << std::endl;
    // ケース3：保留しないまま15分が過ぎ、席が戻る
    TicketReservation t3(db, "EVT001");
    t3.reserve();  // Available → Reserved（予約数+1）
    // 15分の経過を、タイマーの代わりにここで再現する
    t3.expire();   // Reserved  → Available（予約数-1）
    t3.hold();     // 席を戻したあとなので受け付けない

    return 0;
}
