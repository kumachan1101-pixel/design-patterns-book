#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <stdexcept>

// 会員種別（ルール判定で使う直文字列を名前へ置き換える）
namespace MemberType {
    const std::string Premium = "Premium";  // 優待会員
    const std::string Regular = "Regular";  // 一般会員
}

// 開催中の施策を表すコード（入力と判定で同じ名前を使う）
namespace CampaignCode {
    // 通常キャンペーン
    const std::string RegularCampaign = "REGULAR_CAMPAIGN";
    // サマーセール
    const std::string SummerSale = "SUMMER_SALE";
}

class Item {
public:
    std::string name;
    int price;
    Item(std::string n, int p) : name(n), price(p) {}
};

class CampaignContext {
private:
    std::vector<std::string> activeCampaigns;
public:
    void activate(const std::string& code) {
        activeCampaigns.push_back(code);
    }

    bool isActive(const std::string& code) const {
        for (const auto& active : activeCampaigns) {
            if (active == code) return true;
        }

        return false;
    }
};

class Order {
public:
    std::string customerId;
    std::vector<Item> items;
};

// 顧客1件分の情報
struct CustomerInfo {
    std::string name;        // 顧客の氏名
    std::string memberType;  // 会員種別（MemberType の値）
};

// 金額計算の結果（小計と支払金額を一組で返す）
struct PaymentResult {
    int subtotal;    // 割引前の小計
    int finalPrice;  // 割引後の支払金額
};

class CustomerDatabase {
private:
    std::map<std::string, CustomerInfo> records;
public:
    CustomerDatabase() {
        records["C001"] = {"田中 一郎", "Premium"};
        records["C002"] = {"佐藤 花子", "Regular"};
        records["C003"] = {"鈴木 次郎", "Regular"};
    }

    bool exists(const std::string& id) const {
        return records.count(id) > 0;
    }
    CustomerInfo get(const std::string& id) const {
        return records.at(id);
    }
};

class IDiscountRule {
public:
    virtual bool matches(const std::string& memberType,
                         const CampaignContext& context) const =
                             0;
    virtual int apply(int total) const = 0;
    virtual ~IDiscountRule() = default;
};

class NoDiscount : public IDiscountRule {
public:
    bool matches(const std::string&,
                 const CampaignContext&) const override {
        return true;
    }

    int apply(int total) const override { return total; }
};

class PremiumDiscount : public IDiscountRule {
public:
    bool matches(const std::string& memberType,
                 const CampaignContext&) const override {
        return memberType == MemberType::Premium;
    }

    int apply(int total) const override {
        return total * 80 / 100;
    }
};

class SummerSaleAndCampaignDiscount : public IDiscountRule {
public:
    bool matches(const std::string& memberType,
                 const CampaignContext& context)
                 const override {
        return memberType == MemberType::Regular
            && context.isActive(CampaignCode::SummerSale)
            && context.isActive(CampaignCode::RegularCampaign);
    }

    int apply(int total) const override {
        return (total * 90 / 100) * 95 / 100;
    }
};

class SummerSaleDiscount : public IDiscountRule {
public:
    bool matches(const std::string& memberType,
                 const CampaignContext& context)
                 const override {
        return memberType == MemberType::Regular
            && context.isActive(CampaignCode::SummerSale);
    }

    int apply(int total) const override {
        return total * 95 / 100;
    }
};

class CampaignDiscount : public IDiscountRule {
public:
    bool matches(const std::string& memberType,
                 const CampaignContext& context)
                 const override {
        return memberType == MemberType::Regular
            && context.isActive(CampaignCode::RegularCampaign);
    }

    int apply(int total) const override {
        return total * 90 / 100;
    }
};

class PaymentCalculator {
private:
    const IDiscountRule& rule;
public:
    explicit PaymentCalculator(const IDiscountRule& r)
            : rule(r) {}

    PaymentResult calculate(const Order& order) const {
        int subtotal = 0;

        for (const auto& item : order.items) subtotal +=
            item.price;
        return PaymentResult{subtotal, rule.apply(subtotal)};
    }
};

class RuleSelector {
private:
    std::vector<std::reference_wrapper<
            const IDiscountRule>> rules;
public:
    void add(const IDiscountRule& rule) {
        rules.push_back(std::cref(rule));
    }

    const IDiscountRule& select(
            const std::string& memberType,
            const CampaignContext& context) const {
        // 適用する割引の優先順は、ここでの登録順で表す。
        // Selectorは個別条件を知らず、最初に一致したものを返す。
        for (const auto& registered : rules) {
            const IDiscountRule& rule = registered.get();

            if (rule.matches(memberType, context)) return rule;
        }

        throw std::logic_error("適用可能な割引ルールがありません");
    }
};

class DiscountRuleSet {
private:
    PremiumDiscount premium;
    SummerSaleAndCampaignDiscount summerAndCampaign;
    SummerSaleDiscount summer;
    CampaignDiscount campaign;
    NoDiscount none;
    RuleSelector ruleSelector;
public:
    DiscountRuleSet() {
        // Premiumは他施策と併用しない
        ruleSelector.add(premium);
        ruleSelector.add(summerAndCampaign); // 複合条件を単独条件より先にする
        ruleSelector.add(summer);
        ruleSelector.add(campaign);
        ruleSelector.add(none);              // 必ず一致するため最後にする
    }

    // ルールの実体はこのクラスが所有し、Selectorはその参照だけを持つ。
    // コピーすると複製側のSelectorが元の実体を指したままになるため、禁じる。
    DiscountRuleSet(const DiscountRuleSet&) = delete;
    DiscountRuleSet& operator=(const DiscountRuleSet&) = delete;

    const RuleSelector& selector() const {
        return ruleSelector;
    }
};

class CheckoutResultRenderer {
public:
    void showError(const std::string& message) {
        std::cout << "エラー: " << message << "\n";
    }

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
          << (context.isActive(CampaignCode::RegularCampaign)
                      ? "あり" : "なし");
        std::cout << "\n  小計 " << payment.subtotal
                  << "円 → 支払金額 "
                  << payment.finalPrice << "円\n";
    }
};

class OrderProcessor {
private:
    CustomerDatabase& db;
    const RuleSelector& selector;
    CheckoutResultRenderer& renderer;
public:
    OrderProcessor(CustomerDatabase& db,
                   const RuleSelector& selector,
                   CheckoutResultRenderer& renderer)
        : db(db), selector(selector), renderer(renderer) {}

    void process(const Order& order,
                 const CampaignContext& context) {
        if (!db.exists(order.customerId)) {
            renderer.showError(
                "顧客ID " + order.customerId
                + " は登録されていません");
            return;
        }
        if (order.items.empty()) {
            renderer.showError("注文が空です");
            return;
        }

        const CustomerInfo customer = db.get(order.customerId);
        // 会員種別と施策状態から、適用する契約を1つ選ぶ
        const IDiscountRule& rule =
            selector.select(customer.memberType, context);
        PaymentCalculator calculator(rule);

        const PaymentResult payment =
            calculator.calculate(order);
        renderer.showOrderResult(customer, order,
                                 context, payment);
    }
};

int main() {
    CustomerDatabase db;
    CheckoutResultRenderer renderer;

    DiscountRuleSet discountRules;

    OrderProcessor processor(db, discountRules.selector(),
                             renderer);

    // C001（Premium）/ キャンペーンなし / サマーセールなし → 20%引き
    std::cout << "--- ケース1: Premium割引 ---\n";
    Order order1;
    order1.customerId = "C001";
    order1.items.push_back(Item("ワイヤレスイヤホン", 10000));
    CampaignContext context1;
    processor.process(order1, context1);

    // C001（Premium）/ キャンペーンあり / サマーセール中 → Premium優先
    std::cout << "\n--- ケース2: Premium排他 ---\n";
    Order order2;
    order2.customerId = "C001";
    order2.items.push_back(Item("ワイヤレスイヤホン", 10000));
    CampaignContext context2;
    context2.activate(CampaignCode::RegularCampaign);
    context2.activate(CampaignCode::SummerSale);
    processor.process(order2, context2);

    // C002（Regular）/ キャンペーンあり / サマーセール中 → 逐次割引
    std::cout << "\n--- ケース3: 逐次割引 ---\n";
    Order order3;
    order3.customerId = "C002";
    order3.items.push_back(Item("ワイヤレスイヤホン", 10000));
    CampaignContext context3;
    context3.activate(CampaignCode::RegularCampaign);
    context3.activate(CampaignCode::SummerSale);
    processor.process(order3, context3);

    // C002（Regular）/ サマーセールのみ → 5%引き
    std::cout << "\n--- ケース4: サマーセール単独 ---\n";
    Order order4;
    order4.customerId = "C002";
    order4.items.push_back(Item("ワイヤレスイヤホン", 10000));
    CampaignContext context4;
    context4.activate(CampaignCode::SummerSale);
    processor.process(order4, context4);

    // C002（Regular）/ キャンペーンのみ → 10%引き（変更前と同じ）
    std::cout << "\n--- ケース4b: キャンペーン単独 ---\n";
    Order order4b;
    order4b.customerId = "C002";
    order4b.items.push_back(Item("ワイヤレスイヤホン", 10000));
    CampaignContext context4b;
    context4b.activate(CampaignCode::RegularCampaign);
    processor.process(order4b, context4b);

    // C003（Regular）/ 割引なし
    std::cout << "\n--- ケース5: 割引なし ---\n";
    Order order5;
    order5.customerId = "C003";
    order5.items.push_back(Item("スマホケース", 3000));
    order5.items.push_back(Item("USBケーブル", 1000));
    CampaignContext context5;
    processor.process(order5, context5);

    // エラー条件も、正常系と同じ最終コードで確認する
    std::cout << "\n--- ケース6: 未登録顧客 ---\n";
    Order unknown;
    unknown.customerId = "UNKNOWN";
    unknown.items.push_back(Item("USBケーブル", 1000));
    processor.process(unknown, context5);

    std::cout << "\n--- ケース7: 空注文 ---\n";
    Order empty;
    empty.customerId = "C002";
    processor.process(empty, context5);

    return 0;
}
