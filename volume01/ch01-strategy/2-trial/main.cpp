#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

class Item {
public:
    std::string name;   // 商品名
    int price;          // 単価（円）
    Item(std::string n, int p) : name(n), price(p) {}
};

class Order {
public:
    std::string customerId;   // 注文した顧客のID
    std::vector<Item> items;  // カートに入っている商品の一覧
};

// CampaignContext クラスへの変更（サマーセールフラグの追加が必要）
class CampaignContext {
public:
    bool isCampaignActive = false;
    bool isSummerSale = false;   // ← 変更ID1で追加
};

struct CustomerInfo {
    std::string name;        // 顧客の氏名
    std::string memberType;  // "Premium" または "Regular"
};

class CustomerDatabase {
private:
    std::map<std::string, CustomerInfo> records;  // ID→顧客情報
public:
    CustomerDatabase() {
        records["C001"] = {"田中 一郎", "Premium"};
        records["C002"] = {"佐藤 花子", "Regular"};
        records["C003"] = {"鈴木 次郎", "Regular"};
    }

    bool exists(const std::string& id) const {
        return records.count(id) > 0;   // IDが登録済みか
    }

    CustomerInfo get(const std::string& id) const {
        return records.at(id);          // IDから顧客情報を取り出す
    }
};

class PaymentCalculator {
public:
    int calculate(const Order& order,
                  const std::string& memberType,
                  const CampaignContext& context) {
        // (1) 小計：商品の単価を全部足す
        int total = 0;

        for (const auto& item : order.items) {
            total += item.price;
        }

        // (2) 割引：会員種別とキャンペーンで割引率を決める
        // サマーセール対応：Regular会員向けに条件を追加
        if (memberType == "Premium") {
            total = total * 80 / 100;  // 20%引き（サマーセール対象外）
        // ← ここから追加。サマーセール単独と、
        //    キャンペーンとの重なりを別の枝にする
        } else if (context.isSummerSale && context.isCampaignActive) {
            total = (total * 90 / 100) * 95 / 100; // 逐次割引（Regular会員）
        } else if (context.isSummerSale) {
            total = total * 95 / 100;  // 5%引き（Regular会員）
        // ← ここまで
        } else if (context.isCampaignActive) {  // ← 変更
            total = total * 90 / 100;  // 10%引き
        }

        // どれにも当てはまらなければ割引なし（定価）

        return total;
    }
};

class CheckoutResultRenderer {
public:
    void showOrderResult(const CustomerInfo& customer,
                         const Order& order,
                         const CampaignContext& context,
                         int subtotal,
                         int finalPrice) {
        std::cout << customer.name << " さんの注文:";

        for (const auto& item : order.items) {
            std::cout << " " << item.name << " " << item.price
                      << "円";
        }

        std::cout << "\n  条件: 会員=" << customer.memberType
                  << ", キャンペーン="
                  << (context.isCampaignActive ? "あり" : "なし");
        std::cout << "\n  小計 " << subtotal << "円 → 支払金額 "
                  << finalPrice << "円\n";
    }
};

class OrderProcessor {
private:
    CustomerDatabase& db;
    CheckoutResultRenderer& renderer;
    PaymentCalculator calculator;
public:
    OrderProcessor(CustomerDatabase& db,
                   CheckoutResultRenderer& renderer)
        : db(db), renderer(renderer) {}

    void process(const Order& order,
                 const CampaignContext& context);
};

void OrderProcessor::process(const Order& order,
                             const CampaignContext& context) {
    // エラー条件1：顧客IDが存在しない
    if (!db.exists(order.customerId)) {
        std::cout << "エラー: 顧客ID " << order.customerId
                  << " は登録されていません\n";
        return;
    }

    // エラー条件2：注文が空
    if (order.items.empty()) {
        std::cout << "エラー: 注文が空です\n";
        return;
    }

    // 会員種別をIDから取得して計算へ渡す
    CustomerInfo customer = db.get(order.customerId);

    int finalPrice =
        calculator.calculate(order,
                             customer.memberType, context);

    // 表示形式はRenderer境界へ委ねる
    int subtotal = 0;

    for (const auto& item : order.items) {
        subtotal += item.price;
    }

    renderer.showOrderResult(customer, order, context,
                             subtotal, finalPrice);
}

// 変更後のクラスを使った動作確認
int main() {
    CustomerDatabase db;
    CheckoutResultRenderer renderer;
    OrderProcessor processor(db, renderer);
    CampaignContext context;
    Order order;
    order.items.push_back(Item("ワイヤレスイヤホン", 10000));

    order.customerId = "C001";
    context.isSummerSale = true;
    context.isCampaignActive = true;
    processor.process(order, context);

    order.customerId = "C002";
    context.isSummerSale = true;
    context.isCampaignActive = true;
    processor.process(order, context);

    context.isSummerSale = true;
    context.isCampaignActive = false;
    processor.process(order, context);

    context.isSummerSale = false;
    context.isCampaignActive = false;
    processor.process(order, context);

    return 0;
}
