#ifndef PRODUCTDATABASE_H_INCLUDED
#define PRODUCTDATABASE_H_INCLUDED

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <stdexcept>

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

// 通知先1：メール通知

// 通知先2：ダッシュボード更新

// 通知先3：チャット通知

// 通知先4：SMS通知。既存3手段と同じく、その場で成否が決まる

// 通知元クラス（Subject に相当）

// 生成・所有・登録を一か所に閉じるアプリケーションの組み立て役

#endif  // PRODUCTDATABASE_H_INCLUDED
