/*
 Copyright (C) 2019, 2021 Quaternion Risk Management Ltd
 Copyright (C) 2021 Skandinaviska Enskilda Banken AB (publ)
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

#include <ored/portfolio/strike.hpp>
#include <ored/utilities/parsers.hpp>
#include <ored/utilities/to_string.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/export.hpp>

using namespace QuantLib;
using std::ostream;
using std::ostringstream;
using std::string;
using std::vector;

namespace ore {
namespace data {

Strike::Strike() {}

Strike::Strike(Real strike, std::string strikeCurrency) : strike_(strike), strikeCurrency_(strikeCurrency) {}

Real Strike::strike() const { return strike_; }
std::string Strike::strikeCurrency() const { return strikeCurrency_; }

} // namespace data
} // namespace ore

// BOOST_CLASS_EXPORT_IMPLEMENT(ore::data::AbsoluteStrike);