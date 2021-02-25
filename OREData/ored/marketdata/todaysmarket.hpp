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

/*! \file marketdata/todaysmarket.hpp
    \brief An concerte implementation of the Market class that loads todays market and builds the required curves
    \ingroup marketdata
*/

#pragma once

#include <ored/configuration/conventions.hpp>
#include <ored/configuration/curveconfigurations.hpp>
#include <ored/marketdata/curvespec.hpp>
#include <ored/marketdata/loader.hpp>
#include <ored/marketdata/marketimpl.hpp>
#include <ored/marketdata/todaysmarketcalibrationinfo.hpp>
#include <ored/marketdata/todaysmarketparameters.hpp>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/directed_graph.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/shared_ptr.hpp>

#include <map>

namespace ore {
namespace data {

class ReferenceDataManager;
class YieldCurve;
class FXSpot;
class FXVolCurve;
class SwaptionVolCurve;
class YieldVolCurve;
class CapFloorVolCurve;
class DefaultCurve;
class CDSVolCurve;
class BaseCorrelationCurve;
class InflationCurve;
class InflationCapFloorVolCurve;
class EquityCurve;
class EquityVolCurve;
class Security;
class CommodityCurve;
class CommodityVolCurve;
class CorrelationCurve;

// TODO: rename class
//! Today's Market
/*!
  Today's Market differes from MarketImpl in that it actually loads market data
  and builds term structure objects.

  We label this object Today's Market in contrast to the Simulation Market which can
  differ in composition and granularity. The Simulation Market is initialised using a Today's Market
  object.

  Today's market's purpose is t0 pricing, the Simulation Market's purpose is
  pricing under future scenarios.

  \ingroup marketdata
 */
class TodaysMarket : public MarketImpl {
public:
    //! Constructor taking pointers and allowing for a lazy build of the market objects
    TodaysMarket( //! Valuation date
        const Date& asof,
        //! Description of the market composition
        const boost::shared_ptr<TodaysMarketParameters>& params,
        //! Market data loader
        const boost::shared_ptr<Loader>& loader,
        //! Description of curve compositions
        const boost::shared_ptr<CurveConfigurations>& curveConfigs,
        //! Repository of market conventions
        const boost::shared_ptr<Conventions>& conventions,
        //! Continue even if build errors occur
        const bool continueOnError = false,
        //! Optional Load Fixings
        const bool loadFixings = true,
        //! If yes, build market objects lazily
        const bool lazyBuild = false,
        //! Optional reference data manager, needed to build fitted bond curves
        const boost::shared_ptr<ReferenceDataManager>& referenceData = nullptr,
        //! If true, preserve link to loader quotes, this might heavily interfere with XVA simulations!
        const bool preserveQuoteLinkage = false);

    boost::shared_ptr<TodaysMarketCalibrationInfo> calibrationInfo() const { return calibrationInfo_; }

private:
    // MarketImpl interface
    void require(const MarketObject o, const string& name, const string& configuration) const override;

    // input parameters

    const boost::shared_ptr<TodaysMarketParameters> params_;
    const boost::shared_ptr<Loader> loader_;
    const boost::shared_ptr<const CurveConfigurations> curveConfigs_;
    const boost::shared_ptr<Conventions> conventions_;

    const bool continueOnError_;
    const bool loadFixings_;
    const bool lazyBuild_;
    const bool preserveQuoteLinkage_;
    const boost::shared_ptr<ReferenceDataManager> referenceData_;

    // initialise market
    void initialise(const Date& asof);

    // build a graph whose vertices represent the market objects to build (DiscountCurve, IndexCurve, EquityVol, ...)
    // and an edge from x to y means that x must be built before y, since y depends on it. */
    void buildDependencyGraph(const std::string& configuration, std::map<std::string, std::string>& buildErrors);

    // data structure for a vertex in the graph
    struct Node {
        MarketObject obj;                       // the market object to build
        std::string name;                       // the LHS of the todays market mapping
        std::string mapping;                    // the RHS of the todays market mapping
        boost::shared_ptr<CurveSpec> curveSpec; // the parsed curve spec, if applicable, null otherwise
        bool built;                             // true if we have built this node
    };
    friend std::ostream& operator<<(std::ostream& o, const Node& n);

    // some typedefs for graph related data types
    using Graph = boost::directed_graph<Node>;
    using IndexMap = boost::property_map<Graph, boost::vertex_index_t>::type;
    using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
    using VertexIterator = boost::graph_traits<Graph>::vertex_iterator;

    // the dependency graphs for each configuration
    mutable std::map<std::string, Graph> dependencies_;

    // build a single market object
    void buildNode(const std::string& configuration, Node& node) const;

    // fx triangulation initially built using all fx spot quotes from the loader; this is provided to
    // curve builders that require fx spots (e.g. xccy discount curves)
    mutable FXTriangulation fxT_;

    // calibration results
    boost::shared_ptr<TodaysMarketCalibrationInfo> calibrationInfo_;

    // cached market objects, the key of the maps is the curve spec name, except for swap indices, see below
    mutable map<string, boost::shared_ptr<YieldCurve>> requiredYieldCurves_;
    mutable map<string, boost::shared_ptr<FXSpot>> requiredFxSpots_;
    mutable map<string, boost::shared_ptr<FXVolCurve>> requiredFxVolCurves_;
    mutable map<string, boost::shared_ptr<SwaptionVolCurve>> requiredSwaptionVolCurves_;
    mutable map<string, boost::shared_ptr<YieldVolCurve>> requiredYieldVolCurves_;
    mutable map<string, boost::shared_ptr<CapFloorVolCurve>> requiredCapFloorVolCurves_;
    mutable map<string, boost::shared_ptr<DefaultCurve>> requiredDefaultCurves_;
    mutable map<string, boost::shared_ptr<CDSVolCurve>> requiredCDSVolCurves_;
    mutable map<string, boost::shared_ptr<BaseCorrelationCurve>> requiredBaseCorrelationCurves_;
    mutable map<string, boost::shared_ptr<InflationCurve>> requiredInflationCurves_;
    mutable map<string, boost::shared_ptr<InflationCapFloorVolCurve>> requiredInflationCapFloorVolCurves_;
    mutable map<string, boost::shared_ptr<EquityCurve>> requiredEquityCurves_;
    mutable map<string, boost::shared_ptr<EquityVolCurve>> requiredEquityVolCurves_;
    mutable map<string, boost::shared_ptr<Security>> requiredSecurities_;
    mutable map<string, boost::shared_ptr<CommodityCurve>> requiredCommodityCurves_;
    mutable map<string, boost::shared_ptr<CommodityVolCurve>> requiredCommodityVolCurves_;
    mutable map<string, boost::shared_ptr<CorrelationCurve>> requiredCorrelationCurves_;
    // for swap indices we map the configuration name to a map (swap index name => index)
    mutable map<string, map<string, boost::shared_ptr<SwapIndex>>> requiredSwapIndices_;
};

std::ostream& operator<<(std::ostream& o, const TodaysMarket::Node& n);

} // namespace data
} // namespace ore
