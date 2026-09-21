#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>


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
        records[id] = info;           // 実行中の商品マスタへ追加
    }

    bool isBelowThreshold(const std::string& id,
                          int currentStock) const {
        return currentStock <= records.at(id).alertThreshold;
    }
};

// メール基盤：件名と本文が分かれ、送れたかどうかだけを返す
class EmailNotifier {
    std::vector<std::string> inbox;
public:
    bool sendMail(
        const std::string& subject,
        const std::string& body) {
        inbox.push_back(body);
        std::cout << "Email(" << inbox.size()
                  << "件) [" << subject << "] "
                  << body << std::endl;
        return true;
    }
};

// 社内ダッシュボード：文言ではなく商品コードと在庫数を受け取る。
// 画面を描き直すだけなので、成否を返さない
class DashboardUpdater {
    int refreshCount;
public:
    DashboardUpdater() : refreshCount(0) {}
    void refreshStockWidget(const std::string& productCode,
                            int stock) {
        ++refreshCount;
        std::cout << "Dashboard(" << refreshCount << "件): "
             << productCode
             << " の在庫表示を " << stock << " に更新" << std::endl;
    }
};

// チャット基盤：投稿先チャンネルが要り、投稿IDを返す。
// 空の投稿IDが失敗を表す
class ChatNotifier {
    std::vector<std::string> posted;
public:
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
};

// SMS基盤：在庫警告の本文を受け取り、送れたかどうかだけを返す
class SMSNotifier {
    std::vector<std::string> inbox;
public:
    bool sendSMS(const std::string& text) {
        inbox.push_back(text);
        std::cout << "SMS(" << inbox.size() << "件): "
                  << text << std::endl;
        return true;
    }
};

class InventoryManager {
private:
    EmailNotifier    email;
    DashboardUpdater dashboard;
    ChatNotifier     chat;
    ProductDatabase  db;
    SMSNotifier      sms;   // ← 追加

public:
    void reduceStock(std::string productId, int quantity);
    void replenishStock(std::string productId, int quantity);

private:
    void notifyAll(const std::string& productId,
                   const ProductInfo& info);
};

void InventoryManager::reduceStock(std::string productId,
                                   int quantity) {
    if (!db.exists(productId)) {
        std::cout << "[エラー] 商品ID " << productId
             << " はマスタに存在しません。処理を中断します。"
             << std::endl;
        return;
    }

    ProductInfo info = db.get(productId);

    if (quantity <= 0 || quantity > info.stock) {
        std::cout << "[エラー] 商品 " << productId
             << "（" << info.name
             << "）"
             << " は " << quantity << " 個出庫できません。現在在庫: "
             << info.stock << std::endl;
        return;
    }

    int before = info.stock;
    info.stock -= quantity;
    db.save(productId, info);
    std::cout << "商品 " << productId << "（" << info.name << "）"
         << " の在庫を " << quantity << " 減らしました。在庫: "
         << before << " -> " << info.stock << std::endl;

    if (db.isBelowThreshold(productId, info.stock)) {
        notifyAll(productId, info);
    }
}

void InventoryManager::replenishStock(std::string productId,
                                      int quantity) {
    if (!db.exists(productId)) {
        std::cout << "[エラー] 商品ID " << productId
             << " はマスタに存在しません。処理を中断します。"
             << std::endl;
        return;
    }

    ProductInfo info = db.get(productId);

    if (quantity <= 0) {
        std::cout << "[エラー] 商品 " << productId
             << "（" << info.name
             << "） は " << quantity
             << " 個補充できません。現在在庫: "
             << info.stock << std::endl;
        return;
    }

    int before = info.stock;
    info.stock += quantity;
    db.save(productId, info);
    std::cout << "商品 " << productId << "（" << info.name << "）\n"
         << "  在庫を " << quantity << " 補充しました。在庫: "
         << before << " -> " << info.stock << std::endl;
}

void InventoryManager::notifyAll(const std::string& productId,
                                 const ProductInfo& info) {
    // ← 追加。4手段の結果を数える
    int sent   = 0;  // 送れた件数
    int failed = 0;  // 送れなかった件数

    std::string message =
        "商品 " + productId + "（" + info.name + "）"
        + " の在庫が閾値以下です。";

    if (!email.sendMail("在庫アラート", message)) {
        std::cout << "[通知失敗] Email" << std::endl;
        failed++;                       // ← 追加
    } else {
        sent++;                         // ← 追加
    }

    // ダッシュボードは成否を返さないので、成功として数えるしかない
    dashboard.refreshStockWidget(productId, info.stock);
    sent++;                             // ← 追加

    std::string postId = chat.postMessage("inventory-alert",
                                     message);

    if (postId.empty()) {
        std::cout << "[通知失敗] Chat" << std::endl;
        failed++;                       // ← 追加
    } else {
        sent++;                         // ← 追加
    }

    // ← ここから追加。SMSだけは本文の作り方が違う
    std::string smsText =
        "在庫警告 " + productId + " 残"
        + std::to_string(info.stock);

    if (!sms.sendSMS(smsText)) {
        std::cout << "[通知失敗] SMS" << std::endl;
        failed++;
    } else {
        sent++;
    }

    std::cout << "[通知結果] 成功:" << sent
         << " 失敗:" << failed << std::endl;
    // ← ここまで
}

int main() {
    InventoryManager manager;

    std::cout << "--- ケース2: PRD002を1減らす ---" << std::endl;
    manager.reduceStock("PRD002", 1);

    return 0;
}
