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
    Paid
};

class TicketReservation {
private:
    // --- データ ---
    EventDatabase& db;   // 共有の在庫データ（外部から注入）
    std::string eventId; // 予約対象のイベント
    ReservationStatus status; // 現在の予約状態

    // 各状態で操作できないときのエラー出力
    void handleReserveError() { std::cout << "現在予約できません\n"; }
    void handlePayError() {
        std::cout << "支払いに適した状態ではありません\n";
    }
    void handleCancelError()  { std::cout << "キャンセルできません\n"; }

public:
    TicketReservation(EventDatabase& db,
                      const std::string& eventId)
        : db(db), eventId(eventId),
          status(ReservationStatus::Available) {}

    bool showAvailability() const;
    void reserve();
    void pay();
    void cancel();
};

bool TicketReservation::showAvailability() const {
    if (!db.exists(eventId)) {
        std::cout << "エラー：イベントID " << eventId
                  << " は存在しません\n";
        return false;
    }

    EventInfo info = db.get(eventId);
    int available = info.capacity - info.reserved;
    std::cout << "[席数確認] " << eventId << " "
              << info.reserved << "/" << info.capacity
              << "（空席" << available << "）";
    if (available == 0) std::cout << "（満席）";
    std::cout << std::endl;
    return true;
}

void TicketReservation::reserve() {
    if (!db.exists(eventId)) {
        std::cout << "エラー：イベントID " << eventId << " は存在しません\n";
        return;
    }

    if (!db.hasCapacity(eventId)) {
        std::cout << "エラー：" << db.get(eventId).title
                  << " は満席です\n";
        return;
    }

    if (status == ReservationStatus::Available) {
        std::cout << "予約対象：" << db.get(eventId).title << "\n";
        db.reserveSeat(eventId);
        status = ReservationStatus::Reserved;
        std::cout << "予約完了しました\n";
    } else {
        handleReserveError();
    }
}

void TicketReservation::pay() {
    if (status == ReservationStatus::Reserved) {
        status = ReservationStatus::Paid;
        std::cout << "支払い完了しました\n";
    } else {
        handlePayError();
    }
}

void TicketReservation::cancel() {
    if (status == ReservationStatus::Reserved) {
        db.cancelSeat(eventId);
        status = ReservationStatus::Available;
        std::cout << "予約をキャンセルしました\n";
    } else {
        handleCancelError();
    }
}

int main() {
    // イベント台帳を用意する。生成時に3件のイベントと
    // 現在の予約数が登録される（EVT001は定員100・予約20）
    EventDatabase db;

    // ケース1: 正常な予約から支払いまで
    std::cout << "--- ケース1: EVT001 予約 → 支払い ---\n";
    TicketReservation seat1(db, "EVT001"); // 状態: Available
    seat1.showAvailability();  // 空席数の表示。状態は不変
    seat1.reserve();  // Available → Reserved（予約数+1）
    seat1.pay();      // Reserved  → Paid

    // ケース2: 予約からキャンセルまで
    std::cout << "\n--- ケース2: EVT001 予約 → キャンセル ---\n";
    TicketReservation seat2(db, "EVT001"); // 状態: Available
    seat2.showAvailability();  // 空席数の表示。状態は不変
    seat2.reserve();  // Available → Reserved（予約数+1）
    seat2.cancel();   // Reserved  → Available（予約数-1）

    // ケース3: 満席イベントへの予約試み
    std::cout << "\n--- ケース3: EVT003 満席イベントへの予約 ---\n";
    TicketReservation seat3(db, "EVT003");
    seat3.showAvailability();
    seat3.reserve();  // エラー（満席）

    // ケース4: 存在しないイベントへの予約試み
    std::cout << "\n--- ケース4: UNKNOWN 存在しないイベントへの予約 ---\n";
    TicketReservation seat4(db, "UNKNOWN");
    if (seat4.showAvailability()) {
        seat4.reserve();
    }

    // ケース5: 状態エラー — 予約前に支払いを試みる
    std::cout << "\n--- ケース5: 予約なしで支払いを試みる ---\n";
    TicketReservation seat5(db, "EVT001");
    seat5.pay();      // エラー（Available状態）

    // ケース6: 状態エラー — 支払い済みをキャンセルしようとする
    std::cout << "\n--- ケース6: 支払い済みをキャンセルしようとする ---\n";
    TicketReservation seat6(db, "EVT001");
    seat6.showAvailability();
    seat6.reserve();  // Available → Reserved
    seat6.pay();      // Reserved  → Paid
    seat6.cancel();   // エラー（Paid状態）

    return 0;
}
