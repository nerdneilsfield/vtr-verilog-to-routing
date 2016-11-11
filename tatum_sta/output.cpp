#include <fstream>
#include <cmath>
#include <vector>
#include <algorithm>

#include "output.hpp"

#include "tatum_assert.hpp"
#include "TimingGraph.hpp"

#include "TimingConstraints.hpp"
#include "TimingTags.hpp"
#include "timing_analyzers.hpp"

using tatum::NodeId;
using tatum::EdgeId;
using tatum::DomainId;
using tatum::TimingTags;
using tatum::TimingTag;
using tatum::TimingGraph;
using tatum::TimingConstraints;

void write_tags(std::ostream& os, const std::string& type, const TimingTags& tags, const NodeId node_id);

void write_timing_graph(std::ostream& os, const TimingGraph& tg) {
    os << "timing_graph:" << "\n";

    //We manually iterate to write the nodes in ascending order
    for(size_t node_idx = 0; node_idx < tg.nodes().size(); ++node_idx) {
        NodeId node_id(node_idx);

        os << " node: " << size_t(node_id) << "\n";

        os << "  type: " << tg.node_type(node_id);
        os << "\n";

        os << "  in_edges: ";
        auto in_edges = tg.node_in_edges(node_id);
        std::vector<EdgeId> edges(in_edges.begin(), in_edges.end());
        std::sort(edges.begin(), edges.end()); //sort the edges for consitent output
        for(EdgeId edge_id : edges) {
            os << size_t(edge_id) << " ";
        }
        os << "\n";

        os << "  out_edges: ";  
        auto out_edges = tg.node_out_edges(node_id);
        edges =  std::vector<EdgeId>(out_edges.begin(), out_edges.end());
        std::sort(edges.begin(), edges.end()); //sort the edges for consitent output
        for(EdgeId edge_id : edges) {
            os << size_t(edge_id) << " ";
        }
        os << "\n";

    }

    //We manually iterate to write the edges in ascending order
    for(size_t edge_idx = 0; edge_idx < tg.edges().size(); ++edge_idx) {
        EdgeId edge_id(edge_idx);

        os << " edge: " << size_t(edge_id) << "\n";
        os << "  src_node: " << size_t(tg.edge_src_node(edge_id)) << "\n";
        os << "  sink_node: " << size_t(tg.edge_sink_node(edge_id)) << "\n";
    }
    os << "\n";
}

void write_timing_constraints(std::ostream& os, const TimingConstraints& tc) {
    os << "timing_constraints:\n";    

    for(auto domain_id : tc.clock_domains()) {
        os << " type: CLOCK domain: " << size_t(domain_id) << " name: \"" << tc.clock_domain_name(domain_id) << "\"\n";
    }

    for(auto domain_id : tc.clock_domains()) {
        NodeId source_node_id = tc.clock_domain_source_node(domain_id);
        if(source_node_id) {
            os << " type: CLOCK_SOURCE node: " << size_t(source_node_id) << " domain: " << size_t(domain_id) << "\n";
        }
    }

    for(auto node_id : tc.constant_generators()) {
        os << " type: CONSTANT_GENERATOR node: " << size_t(node_id) << "\n";
    }

    for(auto kv : tc.input_constraints()) {
        auto node_id = kv.first;
        auto domain_id = kv.second.domain;
        auto constraint = kv.second.constraint;
        if(!isnan(constraint)) {
            os << " type: INPUT_CONSTRAINT node: " << size_t(node_id) << " domain: " << size_t(domain_id) << " constraint: " << constraint << "\n";
        }
    }

    for(auto kv : tc.output_constraints()) {
        auto node_id = kv.first;
        auto domain_id = kv.second.domain;
        auto constraint = kv.second.constraint;
        if(!isnan(constraint)) {
            os << " type: OUTPUT_CONSTRAINT node: " << size_t(node_id) << " domain: " << size_t(domain_id) << " constraint: " << constraint << "\n";
        }
    }

    for(auto kv : tc.setup_constraints()) {
        auto key = kv.first;
        auto constraint = kv.second;
        if(!isnan(constraint)) {
            os << " type: SETUP_CONSTRAINT";
            os << " src_domain: " << size_t(key.src_domain_id);
            os << " sink_domain: " << size_t(key.sink_domain_id);
            os << " constraint: " << constraint;
            os << "\n";
        }
    }

    for(auto kv : tc.hold_constraints()) {
        auto key = kv.first;
        auto constraint = kv.second;
        if(!isnan(constraint)) {
            os << " type: HOLD_CONSTRAINT";
            os << " src_domain: " << size_t(key.src_domain_id);
            os << " sink_domain: " << size_t(key.sink_domain_id);
            os << " constraint: " << constraint;
            os << "\n";
        }
    }
    os << "\n";
}

void write_analysis_result(std::ostream& os, const TimingGraph& tg, const std::shared_ptr<tatum::TimingAnalyzer> analyzer) {
    os << "analysis_result:\n";

    auto setup_analyzer = std::dynamic_pointer_cast<tatum::SetupTimingAnalyzer>(analyzer);
    if(setup_analyzer) {
        for(size_t node_idx = 0; node_idx < tg.nodes().size(); ++node_idx) {
            NodeId node_id(node_idx);
            write_tags(os, "SETUP_DATA", setup_analyzer->get_setup_data_tags(node_id), node_id);
        }
        for(size_t node_idx = 0; node_idx < tg.nodes().size(); ++node_idx) {
            NodeId node_id(node_idx);
            write_tags(os, "SETUP_CLOCK", setup_analyzer->get_setup_clock_tags(node_id), node_id);
        }
    }
    auto hold_analyzer = std::dynamic_pointer_cast<tatum::HoldTimingAnalyzer>(analyzer);
    if(hold_analyzer) {
        for(size_t node_idx = 0; node_idx < tg.nodes().size(); ++node_idx) {
            NodeId node_id(node_idx);
            write_tags(os, "HOLD_DATA", hold_analyzer->get_hold_data_tags(node_id), node_id);
        }
        for(size_t node_idx = 0; node_idx < tg.nodes().size(); ++node_idx) {
            NodeId node_id(node_idx);
            write_tags(os, "HOLD_CLOCK", hold_analyzer->get_hold_clock_tags(node_id), node_id);
        }
    }
    os << "\n";
}

void write_tags(std::ostream& os, const std::string& type, const TimingTags& tags, const NodeId node_id) {
    for(const auto& tag : tags) {
        float arr = tag.arr_time().value();
        float req = tag.req_time().value();

        if(!isnan(arr) || !isnan(req)) {
            os << " type: " << type;
            os << " node: " << size_t(node_id);
            os << " domain: " << size_t(tag.clock_domain());

            if(!isnan(arr)) {
                os << " arr: " << arr;
            }

            if(!isnan(arr)) {
                os << " req: " << tag.req_time().value();
            }

            os << "\n";
        }
    }
}
