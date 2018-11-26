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

#include <test/curveconfig.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <ored/configuration/basecorrelationcurveconfig.hpp>
#include <ored/configuration/capfloorvolcurveconfig.hpp>
#include <ored/configuration/cdsvolcurveconfig.hpp>
#include <ored/configuration/curveconfigurations.hpp>
#include <ored/configuration/defaultcurveconfig.hpp>
#include <ored/configuration/equitycurveconfig.hpp>
#include <ored/configuration/equityvolcurveconfig.hpp>
#include <ored/configuration/fxspotconfig.hpp>
#include <ored/configuration/fxvolcurveconfig.hpp>
#include <ored/configuration/inflationcapfloorpricesurfaceconfig.hpp>
#include <ored/configuration/inflationcurveconfig.hpp>
#include <ored/configuration/securityconfig.hpp>
#include <ored/configuration/swaptionvolcurveconfig.hpp>
#include <ored/configuration/yieldcurveconfig.hpp>
#include <ored/utilities/parsers.hpp>
#include <ored/utilities/csvfilereader.hpp>
#include <oret/datapaths.hpp>

#include <ql/time/calendar.hpp>
#include <ql/time/daycounter.hpp>

using namespace QuantLib;
using namespace QuantExt;
using namespace boost::unit_test_framework;
using namespace std;
using namespace ore;
using namespace ore::data;

namespace {

set<string> readQuotes(const string& filename) {
    // Read the quotes from the file
    CSVFileReader reader(filename, false, ",");

    // Insert the quotes in a set
    set<string> quotes;
    while (reader.next()) {
        quotes.insert(reader.get(0));
    }

    return quotes;
}

}

namespace testsuite {

void CurveConfigTest::testCurveConfigQuotesAll() {

    SavedSettings backup;

    // Read curve configurations from file
    CurveConfigurations curveConfigs;
    curveConfigs.fromFile(TEST_INPUT_FILE("curve_config.xml"));

    // Ask the curve configurations object for all of its quotes
    set<string> quotes = curveConfigs.quotes();

    // Read the expected set of quotes from the file
    set<string> expectedQuotes = readQuotes(TEST_INPUT_FILE("expected_quotes_all.csv"));

    // Check that the quotes match the expected quotes
    BOOST_REQUIRE_EQUAL(quotes.size(), expectedQuotes.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(quotes.begin(), quotes.end(), expectedQuotes.begin(), expectedQuotes.end());
}

void CurveConfigTest::testCurveConfigQuotesSimpleTodaysMarket() {

    SavedSettings backup;

    // Read curve configurations from file
    CurveConfigurations curveConfigs;
    curveConfigs.fromFile(TEST_INPUT_FILE("curve_config.xml"));

    // TodayMarketParameters input files and expected output files
    vector<pair<string, string>> files = {
        make_pair("todays_market_only_ir.xml", "expected_quotes_only_ir.csv"),
        make_pair("todays_market_with_fx_vol_smile.xml", "expected_quotes_with_fx_vol_smile.csv"),
        make_pair("todays_market_with_fx_vol_atm.xml", "expected_quotes_with_fx_vol_atm.csv"),
        make_pair("todays_market_single_config_gbp.xml", "expected_quotes_tmp_single_gbp.csv")
    };

    for (const auto& p : files) {

        BOOST_TEST_MESSAGE("Testing with todaysmarket file: " << p.first);

        // Read the simple, single default configuration, TodaysMarketParameters instance from file
        boost::shared_ptr<TodaysMarketParameters> tmp = boost::make_shared<TodaysMarketParameters>();
        tmp->fromFile(TEST_INPUT_FILE(p.first));

        // Ask the curve configurations object for its quotes, restricted by the TodaysMarketParameters instance
        set<string> quotes = curveConfigs.quotes(tmp, { Market::defaultConfiguration });

        // Read the expected set of quotes from the file
        set<string> expectedQuotes = readQuotes(TEST_INPUT_FILE(p.second));

        // Check that the quotes match the expected quotes
        BOOST_REQUIRE_EQUAL(quotes.size(), expectedQuotes.size());
        BOOST_CHECK_EQUAL_COLLECTIONS(quotes.begin(), quotes.end(), expectedQuotes.begin(), expectedQuotes.end());
    }
}

void CurveConfigTest::testCurveConfigQuotesTodaysMarketMultipleConfigs() {

    SavedSettings backup;

    // Read curve configurations from file
    CurveConfigurations curveConfigs;
    curveConfigs.fromFile(TEST_INPUT_FILE("curve_config.xml"));

    // Read the TodaysMarketParameters instance, containing multiple configurations, from file
    boost::shared_ptr<TodaysMarketParameters> tmp = boost::make_shared<TodaysMarketParameters>();
    tmp->fromFile(TEST_INPUT_FILE("todays_market_multiple_configs.xml"));

    BOOST_REQUIRE_EQUAL(tmp->configurations().size(), 4);

    // Check the quotes for each configuration in turn
    for (const auto& kv : tmp->configurations()) {

        BOOST_TEST_MESSAGE("Checking quotes for configuration: " << kv.first);

        // Ask the curve configurations object for its quotes, restricted by the TodaysMarketParameters 
        // instance and the configuration
        set<string> quotes = curveConfigs.quotes(tmp, { kv.first });

        // Read the expected set of quotes from the file
        set<string> expectedQuotes;
        if (kv.first == "collateral_inccy") {
            expectedQuotes = readQuotes(TEST_INPUT_FILE("expected_quotes_tmp_multiple_collateral_inccy.csv"));
        } else {
            expectedQuotes = readQuotes(TEST_INPUT_FILE("expected_quotes_tmp_multiple.csv"));
        }

        // Check that the quotes match the expected quotes
        BOOST_REQUIRE_EQUAL(quotes.size(), expectedQuotes.size());
        BOOST_CHECK_EQUAL_COLLECTIONS(quotes.begin(), quotes.end(), expectedQuotes.begin(), expectedQuotes.end());
    }
}

void CurveConfigTest::testDiscountRatioSegmentFromXml() {

    // XML string
    string xml;
    xml.append("<DiscountRatio>");
    xml.append("  <Type>Discount Ratio</Type>");
    xml.append("  <BaseCurve currency=\"EUR\">EUR1D</BaseCurve>");
    xml.append("  <NumeratorCurve currency=\"BRL\">BRL-IN-USD</NumeratorCurve>");
    xml.append("  <DenominatorCurve currency=\"EUR\">EUR-IN-USD</DenominatorCurve>");
    xml.append("</DiscountRatio>");

    // XMLDocument from string
    XMLDocument doc;
    doc.fromXMLString(xml);

    // Populate empty segment from XML node
    DiscountRatioYieldCurveSegment seg;
    seg.fromXML(doc.getFirstNode(""));

    // Perform the checks
    BOOST_CHECK(seg.type() == YieldCurveSegment::Type::DiscountRatio);
    BOOST_CHECK_EQUAL(seg.typeID(), "Discount Ratio");
    BOOST_CHECK_EQUAL(seg.conventionsID(), "");
    BOOST_CHECK(seg.quotes().empty());

    BOOST_CHECK_EQUAL(seg.baseCurveId(), "EUR1D");
    BOOST_CHECK_EQUAL(seg.baseCurveCurrency(), "EUR");
    BOOST_CHECK_EQUAL(seg.numeratorCurveId(), "BRL-IN-USD");
    BOOST_CHECK_EQUAL(seg.numeratorCurveCurrency(), "BRL");
    BOOST_CHECK_EQUAL(seg.denominatorCurveId(), "EUR-IN-USD");
    BOOST_CHECK_EQUAL(seg.denominatorCurveCurrency(), "EUR");
}

void CurveConfigTest::testDiscountRatioSegmentToXml() {
    // Create a discount ratio segment
    DiscountRatioYieldCurveSegment seg("Discount Ratio", "EUR1D", "EUR", 
        "BRL-IN-USD", "BRL", "EUR-IN-USD", "EUR");

    // Create an XML document from the segment using toXML
    XMLDocument doc;
    doc.appendNode(seg.toXML(doc));

    // Create new segment using fromXML and check entries
    DiscountRatioYieldCurveSegment newSeg;
    BOOST_CHECK_NO_THROW(newSeg.fromXML(doc.getFirstNode("")));
    BOOST_CHECK(newSeg.type() == YieldCurveSegment::Type::DiscountRatio);
    BOOST_CHECK_EQUAL(newSeg.typeID(), "Discount Ratio");
    BOOST_CHECK_EQUAL(newSeg.conventionsID(), "");
    BOOST_CHECK(newSeg.quotes().empty());

    BOOST_CHECK_EQUAL(newSeg.baseCurveId(), "EUR1D");
    BOOST_CHECK_EQUAL(newSeg.baseCurveCurrency(), "EUR");
    BOOST_CHECK_EQUAL(newSeg.numeratorCurveId(), "BRL-IN-USD");
    BOOST_CHECK_EQUAL(newSeg.numeratorCurveCurrency(), "BRL");
    BOOST_CHECK_EQUAL(newSeg.denominatorCurveId(), "EUR-IN-USD");
    BOOST_CHECK_EQUAL(newSeg.denominatorCurveCurrency(), "EUR");
}


test_suite* CurveConfigTest::suite() {
    test_suite* suite = BOOST_TEST_SUITE("CurveConfigTest");

    suite->add(BOOST_TEST_CASE(&CurveConfigTest::testCurveConfigQuotesAll));
    suite->add(BOOST_TEST_CASE(&CurveConfigTest::testCurveConfigQuotesSimpleTodaysMarket));
    suite->add(BOOST_TEST_CASE(&CurveConfigTest::testCurveConfigQuotesTodaysMarketMultipleConfigs));
    suite->add(BOOST_TEST_CASE(&CurveConfigTest::testDiscountRatioSegmentFromXml));
    suite->add(BOOST_TEST_CASE(&CurveConfigTest::testDiscountRatioSegmentToXml));

    return suite;
}
} // namespace testsuite
