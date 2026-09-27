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

struct PaymentResult {
    int subtotal;    // 割引前の小計
    int finalPrice;  // 割引後の支払金額
};

class CampaignContext {
public:
    bool isCampaignActive = false;  // キャンペーン期間中なら true
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
    PaymentResult calculate(int subtotal,
                            const std::string& memberType,
                            const CampaignContext& context) {
        int finalPrice = subtotal;

        // 会員種別とキャンペーンで割引率を決める
        if (memberType == "Premium") {
            finalPrice = subtotal * 80 / 100; // プレミアム割引 20%引き
        } else if (memberType == "Regular" &&
                   context.isCampaignActive) {
            finalPrice = subtotal * 90 / 100; // キャンペーン割引 10%引き
        }

        // どちらにも当てはまらなければ割引なし（定価）

        return PaymentResult{subtotal, finalPrice};
    }
};

class CheckoutResultRenderer {
public:
    void showOrderResult(const CustomerInfo& customer,
                         const Order& order,
                         const CampaignContext& context,
                         const PaymentResult& payment) {
        std::cout << customer.name << " さんの注文:";

        for (const auto& item : order.items) {
            std::cout << " " << item.name << " " << item.price
                      << "円";
        }

        std::cout << "\n  条件: 会員=" << customer.memberType
                  << ", キャンペーン="
                  << (context.isCampaignActive ? "あり" : "なし");
        std::cout << "\n  小計 " << payment.subtotal
                  << "円 → 支払金額 "
                  << payment.finalPrice << "円\n";
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

    // 会員種別をIDから取得する
    CustomerInfo customer = db.get(order.customerId);

    // 商品単価を合算し、注文全体ではなく小計を支払計算へ渡す
    int subtotal = 0;

    for (const auto& item : order.items) {
        subtotal += item.price;
    }

    const PaymentResult payment =
        calculator.calculate(subtotal,
                             customer.memberType, context);

    // 表示形式はRenderer境界へ委ねる
    renderer.showOrderResult(customer, order, context, payment);
}

int main() {
    CustomerDatabase db;
    CheckoutResultRenderer renderer;
    OrderProcessor processor(db, renderer);
    CampaignContext context;

    // ケース1：C001（Premium）/ キャンペーンなし → 20%引き
    std::cout << "--- ケース1: Premium会員・キャンペーンなし ---\n";
    Order order1;
    order1.customerId = "C001";
    order1.items.push_back(Item("ワイヤレスイヤホン", 10000));
    context.isCampaignActive = false;
    processor.process(order1, context);

    // ケース2：同じPremium会員にキャンペーンを当てても優先は変わらない
    std::cout << "--- ケース2: Premium会員・キャンペーンあり ---\n";
    context.isCampaignActive = true;
    processor.process(order1, context);   // → 8000（キャンペーン無効）
    context.isCampaignActive = false;

    // ケース3：C002（Regular）/ キャンペーンあり → 10%引き
    std::cout << "--- ケース3: Regular会員・キャンペーンあり ---\n";
    Order order2;
    order2.customerId = "C002";
    order2.items.push_back(Item("ワイヤレスイヤホン", 10000));
    context.isCampaignActive = true;
    processor.process(order2, context);

    // ケース4：C003（Regular）/ キャンペーンなし → 割引なし
    std::cout << "--- ケース4: Regular会員・キャンペーンなし ---\n";
    Order order3;
    order3.customerId = "C003";
    order3.items.push_back(Item("スマホケース", 3000));
    order3.items.push_back(Item("USBケーブル", 1000));
    context.isCampaignActive = false;
    processor.process(order3, context);

    // ケース5：エラー条件（存在しない顧客ID）
    std::cout << "--- ケース5: 未登録の顧客ID ---\n";
    Order order4;
    order4.customerId = "UNKNOWN";
    order4.items.push_back(Item("USBケーブル", 1000));
    processor.process(order4, context);

    return 0;
}
