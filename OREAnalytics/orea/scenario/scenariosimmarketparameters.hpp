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

/*! \file scenario/scenariosimmarketparameters.hpp
    \brief A class to hold Scenario parameters for scenarioSimMarket
    \ingroup scenario
*/

#pragma once

#include <ored/utilities/parsers.hpp>
#include <ored/utilities/xmlutils.hpp>
#include <qle/termstructures/dynamicstype.hpp>

using QuantLib::Period;
using QuantLib::Rate;
using std::vector;
using std::string;
using std::pair;
using ore::data::XMLSerializable;
using ore::data::XMLDocument;
using ore::data::XMLNode;
using ore::data::XMLUtils;

namespace ore {
namespace analytics {

//! ScenarioSimMarket description
/*! \ingroup scenario
 */
class ScenarioSimMarketParameters : public XMLSerializable {
public:
    //! Default constructor
    ScenarioSimMarketParameters(){};

    //! \name Inspectors
    //@{
    const string& baseCcy() const { return baseCcy_; }
    const vector<string>& ccys() const { return ccys_; }
    const vector<Period>& yieldCurveTenors() const { return yieldCurveTenors_; }
    const vector<string>& indices() const { return indices_; }
    const map<string, string>& swapIndices() const { return swapIndices_; }
    const string& interpolation() const { return interpolation_; }
    const bool& extrapolate() const { return extrapolate_; }

    const vector<Period>& swapVolTerms() const { return swapVolTerms_; }
    const vector<Period>& swapVolExpiries() const { return swapVolExpiries_; }
    const vector<string>& swapVolCcys() const { return swapVolCcys_; }
    const string& swapVolDecayMode() const { return swapVolDecayMode_; }

    const vector<string>& capFloorVolCcys() const { return capFloorVolCcys_; }
    const string& capFloorVolDecayMode() const { return capFloorVolDecayMode_; }

    const vector<string>& defaultNames() const { return defaultNames_; }
    const vector<Period>& defaultTenors() const { return defaultTenors_; }

    const vector<string>& equityNames() const { return eqNames_; }
    const vector<Period>& equityTenors() const { return eqTenors_; }

    bool simulateFXVols() const { return fxVolSimulate_; }
    const vector<Period>& fxVolExpiries() const { return fxVolExpiries_; }
    const string& fxVolDecayMode() const { return fxVolDecayMode_; }
    // TODO: rename fxVolCurrencyPairs or something to indicate this is for FX vols
    const vector<string>& ccyPairs() const { return ccyPairs_; }

    bool simulateEQVols() const { return eqVolSimulate_; }
    const vector<Period>& eqVolExpiries() const { return eqVolExpiries_; }
    const string& eqVolDecayMode() const { return eqVolDecayMode_; }
    const vector<string>& eqVolNames() const { return eqVolNames_; }

    const vector<string>& additionalScenarioDataIndices() const { return additionalScenarioDataIndices_; }
    const vector<string>& additionalScenarioDataCcys() const { return additionalScenarioDataCcys_; }
    //@}

    //! \name Setters
    //@{
    string& baseCcy() { return baseCcy_; }
    vector<string>& ccys() { return ccys_; }
    vector<Period>& yieldCurveTenors() { return yieldCurveTenors_; }
    vector<string>& indices() { return indices_; }
    map<string, string>& swapIndices() { return swapIndices_; }
    string& interpolation() { return interpolation_; }
    bool& extrapolate() { return extrapolate_; }

    vector<Period>& swapVolTerms() { return swapVolTerms_; }
    vector<string>& swapVolCcys() { return swapVolCcys_; }
    vector<Period>& swapVolExpiries() { return swapVolExpiries_; }
    string& swapVolDecayMode() { return swapVolDecayMode_; }

    vector<string>& capFloorVolCcys() { return capFloorVolCcys_; }
    string& capFloorVolDecayMode() { return capFloorVolDecayMode_; }

    vector<string>& defaultNames() { return defaultNames_; }
    vector<Period>& defaultTenors() { return defaultTenors_; }

    vector<string>& equityNames() { return eqNames_; }
    vector<Period>& equityTenors() { return eqTenors_; }

    bool& simulateFXVols() { return fxVolSimulate_; }
    vector<Period>& fxVolExpiries() { return fxVolExpiries_; }
    string& fxVolDecayMode() { return fxVolDecayMode_; }
    vector<string>& ccyPairs() { return ccyPairs_; }

    bool& simulateEQVols() { return eqVolSimulate_; }
    vector<Period>& eqVolExpiries() { return eqVolExpiries_; }
    string& eqVolDecayMode() { return eqVolDecayMode_; }
    vector<string>& eqVolNames() { return eqVolNames_; }

    vector<string>& additionalScenarioDataIndices() { return additionalScenarioDataIndices_; }
    vector<string>& additionalScenarioDataCcys() { return additionalScenarioDataCcys_; }
    //@}

    //! \name Serialisation
    //@{
    virtual void fromXML(XMLNode* node);
    virtual XMLNode* toXML(XMLDocument& doc);
    //@}

    //! \Equality Operators
    //@{
    bool operator==(const ScenarioSimMarketParameters& rhs);
    bool operator!=(const ScenarioSimMarketParameters& rhs);
    //@}

private:
    string baseCcy_;
    vector<string> ccys_; // may or may not include baseCcy;
    vector<Period> yieldCurveTenors_;
    vector<string> indices_;
    map<string, string> swapIndices_;
    string interpolation_;
    bool extrapolate_;

    vector<Period> swapVolTerms_;
    vector<string> swapVolCcys_;
    vector<Period> swapVolExpiries_;
    string swapVolDecayMode_;

    vector<string> capFloorVolCcys_;
    string capFloorVolDecayMode_;

    vector<string> defaultNames_;
    vector<Period> defaultTenors_;

    vector<string> eqNames_;
    vector<Period> eqTenors_;

    bool fxVolSimulate_;
    vector<Period> fxVolExpiries_;
    string fxVolDecayMode_;
    vector<string> ccyPairs_;

    bool eqVolSimulate_;
    vector<Period> eqVolExpiries_;
    string eqVolDecayMode_;
    vector<string> eqVolNames_;

    vector<string> additionalScenarioDataIndices_;
    vector<string> additionalScenarioDataCcys_;
};
} // namespace analytics
} // namespace ore
