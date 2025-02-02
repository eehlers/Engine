/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
  Copyright (C) 2020 Quaternion Risk Management Ltd.
  All rights reserved.
*/

#include <qle/models/crlgm1fparametrization.hpp>

#include <ored/model/crlgmbuilder.hpp>
#include <ored/utilities/log.hpp>

#include <ql/currencies/america.hpp>
#include <ql/termstructures/credit/flathazardrate.hpp>

using namespace QuantLib;
using namespace QuantExt;
using namespace std;

namespace ore {
namespace data {

CrLgmBuilder::CrLgmBuilder(const boost::shared_ptr<ore::data::Market>& market, const boost::shared_ptr<CrLgmData>& data,
                           const std::string& configuration)
    : market_(market), configuration_(configuration), data_(data) {

    string name = data->name();
    LOG("LgmCalibration for name " << name << ", configuration is " << configuration);

    modelDefaultCurve_ = RelinkableHandle<DefaultProbabilityTermStructure>(*market_->defaultCurve(name, configuration)->curve());

    QL_REQUIRE(!data_->calibrateA() && !data_->calibrateH(), "CrLgmBuilder does not support calibration currently");

    QL_REQUIRE(data_->aParamType() == ParamType::Constant, "CrLgmBuilder only supports constant volatility currently");
    QL_REQUIRE(data_->hParamType() == ParamType::Constant, "CrLgmBuilder only supports constant reversion currently");

    Array aTimes(data_->aTimes().begin(), data_->aTimes().end());
    Array hTimes(data_->hTimes().begin(), data_->hTimes().end());
    Array alpha(data_->aValues().begin(), data_->aValues().end());
    Array h(data_->hValues().begin(), data_->hValues().end());

    // the currency does not matter here
    parametrization_ =
        boost::make_shared<QuantExt::CrLgm1fConstantParametrization>(USDCurrency(), modelDefaultCurve_, alpha[0], h[0], name);

    LOG("Apply shift horizon and scale");

    QL_REQUIRE(data_->shiftHorizon() >= 0.0, "shift horizon must be non negative");
    QL_REQUIRE(data_->scaling() > 0.0, "scaling must be positive");

    if (data_->shiftHorizon() > 0.0) {
        LOG("Apply shift horizon " << data_->shiftHorizon() << " to the " << data_->qualifier() << " CR-LGM model");
        parametrization_->shift() = data_->shiftHorizon();
    }

    if (data_->scaling() != 1.0) {
        LOG("Apply scaling " << data_->scaling() << " to the " << data_->qualifier() << " CR-LGM model");
        parametrization_->scaling() = data_->scaling();
    }
}
} // namespace data
} // namespace ore
