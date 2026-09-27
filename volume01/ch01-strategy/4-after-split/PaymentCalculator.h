#ifndef PAYMENTCALCULATOR_H_INCLUDED
#define PAYMENTCALCULATOR_H_INCLUDED

#include "Order.h"
#include "IDiscountRule.h"
#include "RuleSelector.h"

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

#endif  // PAYMENTCALCULATOR_H_INCLUDED
