/*
 Copyright (C) 2019 Quaternion Risk Management Ltd
 All rights reserved.
*/

/*! \file ored/marketdata/compositeloader.hpp
    \brief Loader that is a composite of two loaders
    \ingroup marketdata
*/

#pragma once

#include <boost/shared_ptr.hpp>
#include <ored/marketdata/loader.hpp>

namespace ore {
namespace data {

class CompositeLoader : public Loader {
public:
    CompositeLoader(const boost::shared_ptr<Loader>& a, const boost::shared_ptr<Loader>& b) : a_(a), b_(b) {
        QL_REQUIRE(a_ || b_, "CompositeLoader(): at least one loader must be not null");
    }

    std::vector<boost::shared_ptr<MarketDatum>> loadQuotes(const QuantLib::Date& d) const override {
        if (!b_)
            return a_->loadQuotes(d);
        if (!a_)
            return b_->loadQuotes(d);
        std::vector<boost::shared_ptr<MarketDatum>> data;
        auto tmp1 = a_->loadQuotes(d);
        data.insert(data.end(), tmp1.begin(), tmp1.end());
        auto tmp2 = b_->loadQuotes(d);
        data.insert(data.end(), tmp2.begin(), tmp2.end());
        return data;
    }

    boost::shared_ptr<MarketDatum> get(const std::string& name, const QuantLib::Date& d) const override {
        if (a_ && a_->has(name, d))
            return a_->get(name, d);
        if (b_ && b_->has(name, d))
            return b_->get(name, d);
        QL_FAIL("No MarketDatum for name " << name << " and date " << d);
    }

    std::set<boost::shared_ptr<MarketDatum>> get(const std::set<std::string>& names,
                                                 const QuantLib::Date& asof) const override {
        std::set<boost::shared_ptr<MarketDatum>> result;
        if (a_) {
            auto tmp = a_->get(names, asof);
            result.insert(tmp.begin(), tmp.end());
        }
        if (b_) {
            auto tmp = b_->get(names, asof);
            result.insert(tmp.begin(), tmp.end());
        }
        return result;
    }

    std::set<boost::shared_ptr<MarketDatum>> get(const Wildcard& wildcard, const QuantLib::Date& asof) const override {
        std::set<boost::shared_ptr<MarketDatum>> result;
        if (a_) {
            auto tmp = a_->get(wildcard, asof);
            result.insert(tmp.begin(), tmp.end());
        }
        if (b_) {
            auto tmp = b_->get(wildcard, asof);
            result.insert(tmp.begin(), tmp.end());
        }
        return result;
    }

    bool has(const std::string& name, const QuantLib::Date& d) const override {
        return (a_ && a_->has(name, d)) || (b_ && b_->has(name, d));
    }

    std::set<Fixing> loadFixings() const override {
        if (!b_)
            return a_->loadFixings();
        if (!a_)
            return b_->loadFixings();
        std::set<Fixing> fixings;
        auto tmp1 = a_->loadFixings();
        auto tmp2 = b_->loadFixings();
        fixings.insert(tmp1.begin(), tmp1.end());
        fixings.insert(tmp2.begin(), tmp2.end());
        return fixings;
    }

    std::set<Fixing> loadDividends() const override {
        if (!b_)
            return a_->loadDividends();
        if (!a_)
            return b_->loadDividends();
        std::set<Fixing> dividends;
        auto tmp1 = a_->loadDividends();
        auto tmp2 = b_->loadDividends();
        dividends.insert(tmp1.begin(), tmp1.end());
        dividends.insert(tmp2.begin(), tmp2.end());
        return dividends;
    }

private:
    const boost::shared_ptr<Loader> a_, b_;
};

} // namespace data
} // namespace ore
