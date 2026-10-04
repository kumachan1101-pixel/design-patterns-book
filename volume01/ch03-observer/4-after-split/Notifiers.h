#ifndef NOTIFIERS_H_INCLUDED
#define NOTIFIERS_H_INCLUDED

#include "ProductDatabase.h"
#include "INotification.h"

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

#endif  // NOTIFIERS_H_INCLUDED
