#include <iostream>
#include <string>
#include <vector>
#include <map>


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

class InventoryManager {
private:
    EmailNotifier    email;
    DashboardUpdater dashboard;
    ChatNotifier     chat;
    ProductDatabase  db;

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
    // 通知先が増えるたびに、ここが修正される。
    // 3つの基盤は引数の形も戻り値の意味も違うので、
    // 呼び分けと結果の解釈をこのメソッドが全部引き受けている。
    std::string message =
        "商品 " + productId + "（" + info.name + "）"
        + " の在庫が閾値以下です。";

    int sent   = 0;
    int failed = 0;

    // メールは件名と本文に分け、真偽値で成否を見る
    if (!email.sendMail("在庫アラート", message)) {
        std::cout << "[通知受付失敗] Email" << std::endl;
        failed++;
    } else {
        sent++;
    }

    // ダッシュボードは文言を受け取らず、成否も返さない。
    // 送れたかどうかを確かめる手段がない
    dashboard.refreshStockWidget(productId, info.stock);
    sent++;

    // チャットは投稿先が要り、空の投稿IDが失敗を表す
    std::string postId = chat.postMessage("inventory-alert",
                                     message);

    if (postId.empty()) {
        std::cout << "[通知受付失敗] Chat" << std::endl;
        failed++;
    } else {
        sent++;
    }

    std::cout << "[通知結果] 成功:" << sent
         << " 失敗:" << failed << std::endl;
}

int main() {
    InventoryManager manager;

    std::cout << "--- ケース1: PRD001を5減らす ---" << std::endl;
    manager.reduceStock("PRD001", 5);

    std::cout << "--- ケース2: PRD002を1減らす ---" << std::endl;
    manager.reduceStock("PRD002", 1);

    std::cout << "--- ケース3: PRD001を20補充する ---" << std::endl;
    manager.replenishStock("PRD001", 20);

    std::cout << "--- ケース4: PRD003を1減らす ---" << std::endl;
    manager.reduceStock("PRD003", 1);

    std::cout << "--- ケース5: 存在しない商品IDを操作する ---" << std::endl;
    manager.reduceStock("PRD999", 1);

    std::cout << "--- ケース6: 0個の補充を拒否する ---" << std::endl;
    manager.replenishStock("PRD001", 0);

    return 0;
}
