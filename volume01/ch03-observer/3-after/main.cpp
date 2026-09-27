#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <stdexcept>

// 商品マスタの1件分
struct ProductInfo {
    std::string name;            // 商品名
    int         stock;           // 在庫数
    int         alertThreshold;  // これ以下になったら通知する在庫数
};

// 商品マスタ（データ駆動バリデーション用）
class ProductDatabase {
private:
    std::map<std::string, ProductInfo> records;
public:
    ProductDatabase() {
        records["PRD001"] = {"ワイヤレスマウス", 50, 10};
        records["PRD002"] = {"USBハブ",           3,  5}; // 閾値以下
        records["PRD003"] = {"キーボード",         0,  5}; // 在庫なし
    }

    bool exists(const std::string& id) const {
        return records.count(id) > 0;
    }

    ProductInfo get(const std::string& id) const {
        return records.at(id);
    }

    void save(const std::string& id, const ProductInfo& info) {
        records[id] = info;           // 実行中の商品マスターへ追加
    }

    bool isBelowThreshold(const std::string& id,
                          int currentStock) const {
        return currentStock <= records.at(id).alertThreshold;
    }
};

// ログや結果で使う通知手段名を一か所に定義する
namespace ChannelName {
    const char* const EMAIL     = "Email";      // メール通知
    const char* const DASHBOARD = "Dashboard";  // ダッシュボード表示
    const char* const CHAT      = "Chat";       // 社内チャット
    const char* const SMS       = "SMS";        // SMS
}

// 通知手段ごとに表現を変えるための、共通の在庫警告データ
struct StockAlert {
    std::string productId;    // 商品コード
    std::string productName;  // 商品名
    int         stock;        // 更新後の在庫数
};

// 通知1件の結果。手段ごとに違う戻り値を、この1つの形へそろえる
struct DeliveryResult {
    bool        sent;      // 送れたか
    std::string channel;   // どの通知手段か
};

// 通知先が満たす必要がある契約（インターフェース）
class INotification {
public:
    virtual ~INotification() = default;
    // 在庫警告を受け取り、手段別に表現して結果を1つ返す
    virtual DeliveryResult send(const StockAlert& alert) = 0;
};

// 通知先1：メール通知
class EmailNotifier : public INotification {
    std::vector<std::string> inbox;

    // 現状コードと同じメール基盤の操作
    bool sendMail(const std::string& subject,
                  const std::string& body) {
        inbox.push_back(body);
        std::cout << "Email(" << inbox.size()
                  << "件) [" << subject << "] "
                  << body << std::endl;
        return true;
    }
public:
    DeliveryResult send(const StockAlert& a) override {
        std::string body =
            "商品 " + a.productId + "（" + a.productName
            + "） の在庫が閾値以下です。";
        bool ok = sendMail("在庫アラート", body);

        return {ok, ChannelName::EMAIL};
    }
};

// 通知先2：ダッシュボード更新
class DashboardUpdater : public INotification {
    int refreshCount;

    // 現状コードと同じ画面更新。戻り値が無い
    void refreshStockWidget(const std::string& productCode,
                            int stock) {
        ++refreshCount;
        std::cout << "Dashboard(" << refreshCount << "件): "
             << productCode
             << " の在庫表示を " << stock << " に更新" << std::endl;
    }
public:
    DashboardUpdater() : refreshCount(0) {}
    DeliveryResult send(const StockAlert& a) override {
        refreshStockWidget(a.productId, a.stock);
        // 呼べたことをもって成功とする。この割り切りはここに閉じる
        return {true, ChannelName::DASHBOARD};
    }
};

// 通知先3：チャット通知
class ChatNotifier : public INotification {
    std::vector<std::string> posted;

    // 現状コードと同じチャット基盤。投稿IDを返す
    std::string postMessage(const std::string& channel,
                            const std::string& text) {
        posted.push_back(text);
        std::string postId =
            "POST-" + std::to_string(posted.size());
        std::cout << "Chat(" << posted.size()
                  << "件) #" << channel << "\n"
                  << "  " << text << " -> " << postId
                  << std::endl;
        return postId;
    }
public:
    DeliveryResult send(const StockAlert& a) override {
        std::string text =
            "商品 " + a.productId + "（" + a.productName
            + "） の在庫が閾値以下です。";
        std::string postId =
            postMessage("inventory-alert", text);

        return {!postId.empty(), ChannelName::CHAT};
    }
};

// 通知先4：SMS通知。既存3手段と同じく、その場で成否が決まる
class SMSNotifier : public INotification {
    std::vector<std::string> inbox;

    // SMS基盤の操作。送れたかどうかだけを返す
    bool sendSMS(const std::string& text) {
        inbox.push_back(text);
        std::cout << "SMS(" << inbox.size() << "件): "
                  << text << std::endl;
        return true;
    }
public:
    DeliveryResult send(const StockAlert& a) override {
        std::string text = "在庫警告 " + a.productId + " 残"
            + std::to_string(a.stock);
        bool ok = sendSMS(text);

        return {ok, ChannelName::SMS};
    }
};

// 通知元クラス（Subject に相当）
class InventoryManager {
private:
    // 非所有ポインタ。登録中の通知先はInventoryManagerより長く生存すること。
    std::vector<INotification*> observers;
    ProductDatabase& db;

public:
    explicit InventoryManager(ProductDatabase& database)
        : db(database) {}

    // nullと重複登録を拒否する
    bool attach(INotification* o) {
        if (o == nullptr) return false;

        if (std::find(observers.begin(), observers.end(), o)
                != observers.end()) {
            return false;
        }

        observers.push_back(o);

        return true;
    }

    void reduceStock(std::string productId, int quantity) {
        if (!db.exists(productId)) {
            std::cout << "[エラー] 商品ID " << productId
                 << " はマスタに存在しません。処理を中断します。"
                 << std::endl;
            return;
        }

        ProductInfo info = db.get(productId);

        if (quantity <= 0 || quantity > info.stock) {
            std::cout << "[エラー] 商品 " << productId
                 << "（" << info.name << "）"
                 << " は " << quantity << " 個出庫できません。現在在庫: "
                 << info.stock << std::endl;
            return;
        }

        int before = info.stock;
        info.stock -= quantity;
        db.save(productId, info);
        std::cout << "商品 " << productId
             << "（" << info.name << "）"
             << " の在庫を " << quantity << " 減らしました。"
             << " 在庫: " << before
             << " -> " << info.stock << std::endl;

        if (db.isBelowThreshold(productId, info.stock)) {
            notifyAll({productId, info.name, info.stock});
        }
    }

    void replenishStock(std::string productId, int quantity) {
        if (!db.exists(productId)) {
            std::cout << "[エラー] 商品ID " << productId
                 << " はマスタに存在しません。処理を中断します。"
                 << std::endl;
            return;
        }

        ProductInfo info = db.get(productId);

        if (quantity <= 0) {
            std::cout << "[エラー] 商品 " << productId
                 << "（" << info.name << "） は " << quantity
                 << " 個補充できません。現在在庫: "
                 << info.stock << std::endl;
            return;
        }

        int before = info.stock;
        info.stock += quantity;
        db.save(productId, info);
        std::cout << "商品 " << productId
             << "（" << info.name << "）\n"
             << "  在庫を " << quantity
             << " 補充しました。在庫: " << before
             << " -> " << info.stock
             << "（通知なし）" << std::endl;
    }

private:
    // 各通知先の結果を集計する。通知先の種類ごとに分岐しない
    void notifyAll(const StockAlert& alert) {
        int sent   = 0;  // 送れた件数
        int failed = 0;  // 送れなかった件数

        for (auto* o : observers) {
            DeliveryResult r = o->send(alert);

            if (r.sent) {
                sent++;
            } else {
                failed++;
                std::cout << "  失敗: " << r.channel << std::endl;
            }
        }

        std::cout << "[通知結果] 成功:" << sent
             << " 失敗:" << failed << std::endl;
    }
};

// 生成・所有・登録を一か所に閉じるアプリケーションの組み立て役
class InventoryApplication {
    // 上から生成され、下から破棄される。
    // InventoryManagerより通知先を先に宣言し、借用先の寿命を保証する。
    ProductDatabase productDatabase;
    EmailNotifier email;
    DashboardUpdater dashboard;
    ChatNotifier chat;
    SMSNotifier sms;
    InventoryManager manager;

    void registerNotifications() {
        bool registered = manager.attach(&email)
                       && manager.attach(&dashboard)
                       && manager.attach(&chat)
                       && manager.attach(&sms);
        if (!registered) {
            throw std::logic_error("通知先の初期登録に失敗しました");
        }
    }

public:
    InventoryApplication()
        : manager(productDatabase) {
        registerNotifications();
    }

    InventoryManager& inventory() { return manager; }
};

int main() {
    // mainは具体的な通知先や登録順を知らない
    InventoryApplication app;

    // PRD001: 在庫50、閾値10 → 5減らしても閾値超えのまま
    std::cout << "--- ケース1: 在庫が閾値を超えたまま減少"
                 "（通知なし） ---" << std::endl;
    app.inventory().reduceStock("PRD001", 5);
    std::cout << std::endl;

    // PRD002: 在庫3、閾値5 → 最初から閾値以下。4手段へ通知
    std::cout << "--- ケース2: 在庫が閾値以下に減少（4手段へ通知） ---"
              << std::endl;
    app.inventory().reduceStock("PRD002", 1);
    std::cout << std::endl;

    std::cout << "--- ケース3: 在庫が補充された（閾値超え） ---"
              << std::endl;
    app.inventory().replenishStock("PRD001", 20);
    std::cout << std::endl;

    // PRD003: 在庫0 → 出庫エラー
    std::cout << "--- ケース4: 在庫0の出庫操作 ---" << std::endl;
    app.inventory().reduceStock("PRD003", 1);
    std::cout << std::endl;

    // ケース5: 存在しない商品IDのエラー確認
    std::cout << "--- ケース5: 存在しない商品IDを操作する ---"
              << std::endl;
    app.inventory().reduceStock("PRD999", 1);
    std::cout << std::endl;

    std::cout << "--- ケース6: 0個の補充を拒否する ---" << std::endl;
    app.inventory().replenishStock("PRD001", 0);

    return 0;
}
