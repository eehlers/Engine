/*
 Copyright (C) 2018 Quaternion Risk Management Ltd
 All rights reserved.
*/

#include <ored/model/crcirbuilder.hpp>

#include <qle/models/cirppconstantfellerparametrization.hpp>
#include <qle/models/cirppconstantparametrization.hpp>

#include <qle/models/cdsoptionhelper.hpp>

#include <ql/math/optimization/levenbergmarquardt.hpp>

#include <ored/model/utilities.hpp>
#include <ored/utilities/log.hpp>
#include <ored/utilities/parsers.hpp>
#include <ored/utilities/strike.hpp>
#include <ored/utilities/to_string.hpp>

using namespace QuantLib;
using namespace QuantExt;
using namespace std;

namespace ore {
namespace data {

CrCirBuilder::CrCirBuilder(const boost::shared_ptr<ore::data::Market>& market, const boost::shared_ptr<CrCirData>& data,
                           const std::string& configuration)
    : market_(market), configuration_(configuration), data_(data),
      optimizationMethod_(boost::shared_ptr<OptimizationMethod>(new LevenbergMarquardt(1E-8, 1E-8, 1E-8))),
      endCriteria_(EndCriteria(1000, 500, 1E-8, 1E-8, 1E-8)),
      calibrationErrorType_(BlackCalibrationHelper::RelativePriceError) {

    LOG("CIR CR Calibration for name " << data_->name());

    rateCurve_ = market->discountCurve(data_->currency(), configuration);
    creditCurve_ = market->defaultCurve(data_->name(), configuration)->curve();
    recoveryRate_ = market->recoveryRate(data_->name(), configuration);

    registerWith(rateCurve_);
    registerWith(creditCurve_);
    registerWith(recoveryRate_);

    // shifted CIR model hard coded here
    parametrization_ = boost::make_shared<QuantExt::CrCirppConstantWithFellerParametrization>(
        parseCurrency(data_->currency()), creditCurve_, data_->reversionValue(), data_->longTermValue(),
        data_->volatility(), data_->startValue(), true, data_->relaxedFeller(), data_->fellerFactor(),
        data_->name());

    // alternatively, use unconstrained parametrization (only positivity of all parameters is implied)
    // parametrization_ = boost::make_shared<QuantExt::CrCirppConstantParametrization>(
    //     parseCurrency(data_->currency()), creditCurve_, data_->reversionValue(), data_->longTermValue(),
    //     data_->volatility(), data_->startValue(), false);

    model_ = boost::make_shared<QuantExt::CrCirpp>(parametrization_);
}

} // namespace data
} // namespace ore
