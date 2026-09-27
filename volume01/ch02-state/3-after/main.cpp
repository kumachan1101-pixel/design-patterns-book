#include <iostream>
#include <string>
#include <map>
#include <list>

struct EventInfo {
    std::string title;   // イベント名
    int capacity;        // 定員
    int reserved;        // 現在の予約数
};

class EventDatabase {
private:
    std::map<std::string, EventInfo> records;
public:
    EventDatabase() {
        records["EVT001"] = {"春の音楽祭",  100,  20};
        records["EVT002"] = {"夏のフェス",  500, 499};
        records["EVT003"] = {"秋の映画会",   50,  50};  // 満席
    }

    bool exists(const std::string& id) const {
        return records.count(id) > 0;
    }

    EventInfo get(const std::string& id) const {
        return records.at(id);
    }

    bool hasCapacity(const std::string& id) const {
        const auto& e = records.at(id);

        return e.reserved < e.capacity;
    }

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

    void cancelSeat(const std::string& id) {
        auto& event = records.at(id);
        int before = event.reserved;

        if (event.reserved > 0) --event.reserved;
        std::cout << "[予約数] " << id << " "
                  << before << "/" << event.capacity
                  << " -> " << event.reserved << "/"
                  << event.capacity
                  << std::endl;
    }
};

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

    virtual ~IReservationState() = default;
};

// 予約クラス：状態を保持し操作を委譲する
class TicketReservation {
private:
    IReservationState* state;
    EventDatabase* db;           // 席数の保存データ（境界）
    std::string eventId;

public:
    TicketReservation(IReservationState* initialState,
                      EventDatabase* db,
                      const std::string& eventId)
        : state(initialState), db(db), eventId(eventId) {}

    // 状態遷移時に、共有状態オブジェクトへの借用ポインタを差し替える。
    // 状態は関数ローカルstaticが所有するため、ここではdeleteしない。
    void setState(IReservationState* nextState) {
        state = nextState;
    }

    // 状態遷移の副作用：予約数の増減
    void reserveSeat() { db->reserveSeat(eventId); }
    void cancelSeat()  { db->cancelSeat(eventId); }
    bool hasCapacity() const {
        return db->hasCapacity(eventId);
    }
    // イベントの有無は状態によらないので、ここで確かめる
    bool exists() const {
        if (db->exists(eventId)) return true;

        std::cout << "エラー：イベントID " << eventId
                  << " は存在しません\n";
        return false;
    }

    bool showAvailability() const {
        if (!exists()) return false;

        EventInfo info = db->get(eventId);
        int available = info.capacity - info.reserved;
        std::cout << "[席数確認] " << eventId << " "
                  << info.reserved << "/" << info.capacity
                  << "（空席" << available << "）";
        if (available == 0) std::cout << "（満席）";
        std::cout << std::endl;

        return true;
    }
    std::string eventTitle() const {
        return db->get(eventId).title;
    }

    // 操作は現在の状態へ委譲する。
    // 未登録のイベントIDだけは、状態に関係なく先に断る。
    void reserve() { if (exists()) state->reserve(this); }
    void pay()     { if (exists()) state->pay(this); }
    void cancel()  { if (exists()) state->cancel(this); }
    void hold()    { if (exists()) state->hold(this); }
    void expire()  { if (exists()) state->expire(this); }
};

IReservationState* availableState();
IReservationState* reservedState();
IReservationState* paidState();
IReservationState* heldState();

// Available（予約可能）：空席があれば予約し、満席なら断る
class AvailableState : public IReservationState {
public:
    void reserve(TicketReservation* reservation) override {
        if (!reservation->hasCapacity()) {
            std::cout << "エラー：" << reservation->eventTitle()
                      << " は満席です\n";
            return;
        }

        reservation->reserveSeat();
        std::cout << "予約完了しました\n";
        reservation->setState(reservedState());
    }
};

// Reserved（予約済み）：支払い、取消、一時保留、期限切れを処理する
class ReservedState : public IReservationState {
public:
    void pay(TicketReservation* reservation) override {
        std::cout << "支払い完了しました\n";
        reservation->setState(paidState());
    }

    void cancel(TicketReservation* reservation) override {
        reservation->cancelSeat();
        std::cout << "予約をキャンセルしました\n";
        reservation->setState(availableState());
    }

    void hold(TicketReservation* reservation) override {
        std::cout << "一時保留にしました（支払期限を24時間へ延長）\n";
        reservation->setState(heldState());
    }

    void expire(TicketReservation* reservation) override {
        reservation->cancelSeat();
        std::cout << "通常の決済期限が切れました\n";
        reservation->setState(availableState());
    }
};

// Paid（支払い済み）：完了状態のため、すべて既定の拒否を使う
class PaidState : public IReservationState {};

// Held（一時保留）：支払い、取消、期限切れを処理する
class HeldState : public IReservationState {
public:
    void pay(TicketReservation* reservation) override {
        std::cout << "保留から支払い完了しました\n";
        reservation->setState(paidState());
    }

    void cancel(TicketReservation* reservation) override {
        reservation->cancelSeat();
        std::cout << "保留からキャンセルしました\n";
        reservation->setState(availableState());
    }

    void expire(TicketReservation* reservation) override {
        reservation->cancelSeat();
        std::cout << "保留期限が切れました\n";
        reservation->setState(availableState());
    }
};

// 状態オブジェクト取得関数。関数ローカルstaticが所有する。
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
            std::cout << "予約対象：" << seat1.eventTitle() << "\n";
            seat1.reserve();
            seat1.pay();
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
            std::cout << "予約対象：" << seat3.eventTitle() << "\n";
            seat3.reserve();
            seat3.hold();
            seat3.pay();
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
        seat8.pay();      // Available では支払えない
        seat8.reserve();
        seat8.pay();
        seat8.cancel();   // Paid からは取り消せない

        // ケース9：存在しないイベントIDのエラー
        std::cout << "--- ケース9: 存在しないイベントID ---\n";
        TicketReservation& missing =
            assembly.startReservation("EVT999");
        missing.showAvailability();
        missing.reserve();
    }
};

int main() {
    BatchApplication app;
    app.run();

    return 0;
}
