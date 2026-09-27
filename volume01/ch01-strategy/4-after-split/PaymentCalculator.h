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

    PaymentResult calculate(int subtotal) const {
        return PaymentResult{subtotal, rule.apply(subtotal)};
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
            std::cout << "エラー: 顧客ID " << order.customerId
                << " は登録されていません\n";
            return;
        }
        if (order.items.empty()) {
            std::cout << "エラー: 注文が空です\n";
            return;
        }

        const CustomerInfo customer = db.get(order.customerId);
        // 会員種別と施策状態から、適用する契約を1つ選ぶ
        const IDiscountRule& rule =
            selector.select(customer.memberType, context);
        PaymentCalculator calculator(rule);

        int subtotal = 0;
        for (const auto& item : order.items) {
            subtotal += item.price;
        }
        const PaymentResult payment =
            calculator.calculate(subtotal);
        renderer.showOrderResult(customer, order,
                                 context, payment);
    }
};

#endif  // PAYMENTCALCULATOR_H_INCLUDED
