/*
 Copyright (C) 2016 Quaternion Risk Management Ltd
 All rights reserved.

 This file is part of ORE, a free-software/open-source library
 for transparent pricing and risk analysis - http://opensourcerisk.org

 ORE is free software: you can redistribute it and/or modify it
 under the terms of the Modified BSD License.  You should have received a
 copy of the license along with this program.
 The license is also available online at <http://opensourcerisk.org>

 This program is distributed on the basis that it will form a useful
 contribution to risk analytics and model standardisation, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE. See the license for more details.
*/

#include <ored/marketdata/defaultcurve.hpp>
#include <ored/utilities/log.hpp>

#include <qle/termstructures/defaultprobabilityhelpers.hpp>
#include <qle/termstructures/probabilitytraits.hpp>

#include <ql/math/interpolations/backwardflatinterpolation.hpp>
#include <ql/math/interpolations/loginterpolation.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>

#include <algorithm>

using namespace QuantLib;
using namespace std;

namespace ore {
namespace data {

DefaultCurve::DefaultCurve(Date asof, DefaultCurveSpec spec, const Loader& loader,
    const CurveConfigurations& curveConfigs, const Conventions& conventions,
    map<string, boost::shared_ptr<YieldCurve>>& yieldCurves) {

    try {

        const boost::shared_ptr<DefaultCurveConfig>& config = curveConfigs.defaultCurveConfig(spec.curveConfigID());

        // Set the recovery rate if necessary
        recoveryRate_ = Null<Real>();
        if (!config->recoveryRateQuote().empty()) {
            QL_REQUIRE(loader.has(config->recoveryRateQuote(), asof),
                "There is no market data for the requested recovery rate " << config->recoveryRateQuote());
            recoveryRate_ = loader.get(config->recoveryRateQuote(), asof)->quote()->value();
        }

        // Build the default curve of the requested type
        switch (config->type()) {
        case DefaultCurveConfig::Type::SpreadCDS:
            buildCdsCurve(*config, asof, spec, loader, conventions, yieldCurves);
            break;
        case DefaultCurveConfig::Type::HazardRate:
            buildHazardRateCurve(*config, asof, spec, loader, conventions);
            break;
        case DefaultCurveConfig::Type::Benchmark:
            buildBenchmarkCurve(*config, asof, spec, loader, conventions, yieldCurves);
            break;
        default:
            QL_FAIL("The DefaultCurveConfig type " << static_cast<int>(config->type()) << " was not recognised");
        }

    } catch (exception& e) {
        QL_FAIL("default curve building failed for " << spec.curveConfigID() << ": " << e.what());
    } catch (...) {
        QL_FAIL("default curve building failed for " << spec.curveConfigID() << ": unknown error");
    }
}

void DefaultCurve::buildCdsCurve(DefaultCurveConfig& config, const Date& asof,
    const DefaultCurveSpec& spec, const Loader& loader, const Conventions& conventions,
    map<string, boost::shared_ptr<YieldCurve>>& yieldCurves) {

    QL_REQUIRE(config.type() == DefaultCurveConfig::Type::SpreadCDS,
        "DefaultCurve::buildCdsCurve expected a default curve configuration with type SpreadCDS");
    QL_REQUIRE(recoveryRate_ != Null<Real>(), "DefaultCurve: recovery rate needed to build SpreadCDS curve");

    // Get the CDS spread curve conventions
    QL_REQUIRE(conventions.has(config.conventionID()), "No conventions found with id " << config.conventionID());
    boost::shared_ptr<CdsConvention> cdsConv = boost::dynamic_pointer_cast<CdsConvention>(
        conventions.get(config.conventionID()));
    QL_REQUIRE(cdsConv, "SpreadCDS curves require CDS convention");

    // Get the discount curve for use in the CDS spread curve bootstrap
    auto it = yieldCurves.find(config.discountCurveID());
    QL_REQUIRE(it != yieldCurves.end(), "The discount curve, " << config.discountCurveID() <<
        ", required in the building of the curve, " << spec.name() << ", was not found.");
    Handle<YieldTermStructure> discountCurve = it->second->handle();

    // Get the CDS spread curve quotes
    map<Period, Real> quotes = getConfiguredQuotes(config, asof, loader);

    // Create the CDS instrument helpers
    vector<boost::shared_ptr<QuantExt::DefaultProbabilityHelper>> helpers;
    for (auto quote : quotes) {
        helpers.push_back(boost::make_shared<QuantExt::SpreadCdsHelper>(
            quote.second, quote.first, cdsConv->settlementDays(), cdsConv->calendar(), cdsConv->frequency(),
            cdsConv->paymentConvention(), cdsConv->rule(), cdsConv->dayCounter(), recoveryRate_, discountCurve,
            config.startDate(), cdsConv->settlesAccrual(), cdsConv->paysAtDefaultTime()));
    }

    // Create the default probability term structure 
    boost::shared_ptr<DefaultProbabilityTermStructure> tmp =
        boost::make_shared<PiecewiseDefaultCurve<QuantExt::SurvivalProbability, LogLinear>>(
            asof, helpers, config.dayCounter());

    // As for yield curves we need to copy the piecewise curve because on eval date changes the relative date 
    // helpers with trigger a bootstrap.
    vector<Date> dates;
    vector<Real> survivalProbs;
    dates.push_back(asof);
    survivalProbs.push_back(1.0);
    for (Size i = 0; i < helpers.size(); ++i) {
        if (helpers[i]->latestDate() > asof) {
            dates.push_back(helpers[i]->latestDate());
            survivalProbs.push_back(tmp->survivalProbability(dates.back()));
        }
    }
    QL_REQUIRE(dates.size() >= 2, "Need at least 2 points to build the default curve");

    LOG("DefaultCurve: copy piecewise curve to interpolated survival probability curve");
    curve_ = boost::make_shared<InterpolatedSurvivalProbabilityCurve<LogLinear>>(
        dates, survivalProbs, config.dayCounter());
    if (config.extrapolation()) {
        curve_->enableExtrapolation();
        DLOG("DefaultCurve: Enabled Extrapolation");
    }

    // Force bootstrap so that errors are thrown during the build, not later
    curve_->survivalProbability(QL_EPSILON);
}

void DefaultCurve::buildHazardRateCurve(DefaultCurveConfig& config, const Date& asof,
    const DefaultCurveSpec& spec, const Loader& loader, const Conventions& conventions) {

    QL_REQUIRE(config.type() == DefaultCurveConfig::Type::HazardRate,
        "DefaultCurve::buildHazardRateCurve expected a default curve configuration with type HazardRate");

    // Get the hazard rate curve conventions
    QL_REQUIRE(conventions.has(config.conventionID()), "No conventions found with id " << config.conventionID());
    boost::shared_ptr<CdsConvention> cdsConv = boost::dynamic_pointer_cast<CdsConvention>(
        conventions.get(config.conventionID()));
    QL_REQUIRE(cdsConv, "HazardRate curves require CDS convention");

    // Get the hazard rate quotes
    map<Period, Real> quotes = getConfiguredQuotes(config, asof, loader);

    // Build the hazard rate curve
    Calendar cal = cdsConv->calendar();
    vector<Date> dates;
    vector<Real> quoteValues;

    // If first term is not zero, add asof point
    if (quotes.begin()->first != 0 * Days) {
        LOG("DefaultCurve: add asof (" << asof << "), hazard rate " << quotes.begin()->second << ", as not given");
        dates.push_back(asof);
        quoteValues.push_back(quotes.begin()->second);
    }

    for (auto quote : quotes) {
        dates.push_back(cal.advance(asof, quote.first, Following, false));
        quoteValues.push_back(quote.second);
    }

    LOG("DefaultCurve: set up interpolated hazard rate curve");
    curve_ = boost::make_shared<InterpolatedHazardRateCurve<BackwardFlat>>(
        dates, quoteValues, config.dayCounter(), BackwardFlat());

    if (config.extrapolation()) {
        curve_->enableExtrapolation();
        DLOG("DefaultCurve: Enabled Extrapolation");
    }

    if (recoveryRate_ == Null<Real>()) {
        LOG("DefaultCurve: setting recovery rate to 0.0 for hazard rate curve, because none is given.");
        recoveryRate_ = 0.0;
    }

    // Force bootstrap so that errors are thrown during the build, not later
    curve_->survivalProbability(QL_EPSILON);
}

void DefaultCurve::buildBenchmarkCurve(DefaultCurveConfig& config, const Date& asof, const DefaultCurveSpec& spec,
    const Loader& loader, const Conventions& conventions,
    map<string, boost::shared_ptr<YieldCurve>>& yieldCurves) {

    QL_REQUIRE(config.type() == DefaultCurveConfig::Type::Benchmark,
        "DefaultCurve::buildBenchmarkCurve expected a default curve configuration with type Benchmark");

    QL_REQUIRE(recoveryRate_ == Null<Real>(), "DefaultCurve: recovery rate must not be given "
        "for benchmark implied curve type, it is assumed to be 0.0");
    recoveryRate_ = 0.0;

    // Populate benchmark yield curve
    auto it = yieldCurves.find(config.benchmarkCurveID());
    QL_REQUIRE(it != yieldCurves.end(), "The benchmark curve, " << config.benchmarkCurveID() <<
        ", required in the building of the curve, " << spec.name() << ", was not found.");
    boost::shared_ptr<YieldCurve> benchmarkCurve = it->second;

    // Populate source yield curve
    it = yieldCurves.find(config.sourceCurveID());
    QL_REQUIRE(it != yieldCurves.end(), "The source curve, " << config.sourceCurveID() <<
        ", required in the building of the curve, " << spec.name() << ", was not found.");
    boost::shared_ptr<YieldCurve> sourceCurve = it->second;

    // Parameters from the configuration
    vector<Period> pillars = parseVectorOfValues<Period>(config.pillars(), &parsePeriod);
    Calendar cal = config.calendar();
    Size spotLag = config.spotLag();
    
    // Create the implied survival curve
    vector<Date> dates;
    vector<Real> impliedSurvProb;
    Date spot = cal.advance(asof, spotLag * Days);
    for (Size i = 0; i < pillars.size(); ++i) {
        dates.push_back(cal.advance(spot, pillars[i]));
        Real tmp = sourceCurve->handle()->discount(dates[i]) / benchmarkCurve->handle()->discount(dates[i]);
        impliedSurvProb.push_back(dates[i] == asof ? 1.0 : tmp);
    }
    QL_REQUIRE(dates.size() > 0, "DefaultCurve (Benchmark): no dates given");

    // Insert SP = 1.0 at asof if asof date is not in the pillars
    if (dates[0] != asof) {
        dates.insert(dates.begin(), asof);
        impliedSurvProb.insert(impliedSurvProb.begin(), 1.0);
    }

    LOG("DefaultCurve: set up interpolated surv prob curve as yield over benchmark");
    curve_ = boost::make_shared<InterpolatedSurvivalProbabilityCurve<LogLinear>>(
        dates, impliedSurvProb, config.dayCounter());

    if (config.extrapolation()) {
        curve_->enableExtrapolation();
        DLOG("DefaultCurve: Enabled Extrapolation");
    }

    // Force bootstrap so that errors are thrown during the build, not later
    curve_->survivalProbability(QL_EPSILON);
}

map<Period, Real> DefaultCurve::getConfiguredQuotes(DefaultCurveConfig& config,
    const Date& asof, const Loader& loader) const {

    QL_REQUIRE(config.type() == DefaultCurveConfig::Type::SpreadCDS ||
        config.type() == DefaultCurveConfig::Type::HazardRate,
        "DefaultCurve::getConfiguredQuotes expected a default curve configuration with type SpreadCDS or HazardRate");

    map<Period, Real> result;
    for (const auto& p : config.cdsQuotes()) {
        if (boost::shared_ptr<MarketDatum> md = loader.get(p, asof)) {
            Period tenor;
            if (config.type() == DefaultCurveConfig::Type::SpreadCDS) {
                tenor = boost::dynamic_pointer_cast<CdsSpreadQuote>(md)->term();
            } else {
                tenor = boost::dynamic_pointer_cast<HazardRateQuote>(md)->term();
            }

            // Add to quotes, with a check that we have no duplicate tenors
            auto r = result.insert(make_pair(tenor, md->quote()->value()));
            QL_REQUIRE(r.second, "duplicate term in quotes found ("
                << tenor << ") while loading default curve " << config.curveID());
        }
    }

    QL_REQUIRE(!result.empty(), "No market points found for curve config " << config.curveID());
    LOG("DefaultCurve: using " << result.size() << " default quotes of " <<
        config.cdsQuotes().size() << " requested quotes.");

    return result;
}

} // namespace data
} // namespace ore
