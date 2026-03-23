#include "audio_op_harness.h"
#include "plugin_test_utils.h"
#include "runtime/graph.h"
#include "runtime/scheduler.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

static int failures = 0;

static void check(bool cond, const char* msg) {
    if (!cond) {
        std::fprintf(stderr, "  FAIL: %s\n", msg);
        failures++;
    } else {
        std::fprintf(stderr, "  PASS: %s\n", msg);
    }
}

static void check_float(float actual, float expected, const char* msg, float tol = 1e-4f) {
    if (std::fabs(actual - expected) > tol) {
        std::fprintf(stderr, "  FAIL: %s (expected %f, got %f)\n", msg, expected, actual);
        failures++;
    } else {
        std::fprintf(stderr, "  PASS: %s (%f)\n", msg, actual);
    }
}

static void test_euclidean(vivid::OperatorRegistry& registry) {
    std::fprintf(stderr, "\n=== Euclidean Algorithm Tests ===\n");

    auto run_euclidean = [&](int hits, int steps, int rotation,
                             const std::vector<int>& expected, const char* label) {
        vivid::Graph g;
        g.add_node("e", "Euclidean", {
            {"hits", static_cast<float>(hits)},
            {"steps", static_cast<float>(steps)},
            {"rotation", static_cast<float>(rotation)},
            {"gate_length", 0.5f},
            {"rate", 2.0f}
        });

        vivid::Scheduler sched;
        if (!sched.build(g, registry)) {
            std::fprintf(stderr, "  FAIL: %s - build failed\n", label);
            failures++;
            return;
        }

        sched.tick(0.0, 0.016, 0);

        auto* node = sched.find_node_mut("e");
        check(node != nullptr, label);
        if (!node) { sched.shutdown(); return; }

        auto& spread = node->output_spreads[3];
        char buf[256];
        std::snprintf(buf, sizeof(buf), "%s: spread length = %d (expected %d)",
                      label, static_cast<int>(spread.size()), steps);
        check(static_cast<int>(spread.size()) == steps, buf);

        if (static_cast<int>(spread.size()) == steps) {
            bool match = true;
            for (int i = 0; i < steps; ++i) {
                if (static_cast<int>(spread[i]) != expected[i]) {
                    match = false;
                    std::snprintf(buf, sizeof(buf), "%s: spread[%d] = %d, expected %d",
                                  label, i, static_cast<int>(spread[i]), expected[i]);
                    check(false, buf);
                }
            }
            if (match) {
                std::snprintf(buf, sizeof(buf), "%s: pattern correct", label);
                check(true, buf);
            }
        }

        sched.shutdown();
    };

    run_euclidean(3, 8, 0, {1,0,0,1,0,0,1,0}, "E(3,8)");
    run_euclidean(4, 8, 0, {1,0,1,0,1,0,1,0}, "E(4,8)");
    run_euclidean(5, 8, 0, {1,0,1,1,0,1,1,0}, "E(5,8)");
    run_euclidean(0, 8, 0, {0,0,0,0,0,0,0,0}, "E(0,8) all zeros");
    run_euclidean(8, 8, 0, {1,1,1,1,1,1,1,1}, "E(8,8) all ones");
    run_euclidean(3, 8, 1, {0,0,1,0,0,1,0,1}, "E(3,8) rot=1");
}

static void test_pattern_seq(vivid::OperatorRegistry& registry) {
    std::fprintf(stderr, "\n=== PatternSeq Tests ===\n");

    auto* loader = registry.find("PatternSeq");
    check(loader != nullptr, "PatternSeq loader found");
    if (!loader) return;

    {
        AudioOpHarness op(loader);
        op.set_param("steps", 4.0f);
        op.set_param("rate", 2.0f);
        op.set_param("gate_length", 0.8f);
        op.set_param("probability", 1.0f);
        op.set_param("val_0", 10.0f);
        op.set_param("val_1", 20.0f);
        op.set_param("val_2", 30.0f);
        op.set_param("val_3", 40.0f);

        op.input_floats[0] = 0.0f;
        op.process(0);
        check_float(op.output_floats[0], 10.0f, "seq step 0: value=10");
        check_float(op.output_floats[1], 1.0f, "seq step 0: trigger=1");
        check_float(op.output_floats[3], 0.0f, "seq step 0: step index=0");

        op.input_floats[0] = 0.99f;
        op.process(1);
        check_float(op.output_floats[1], 0.0f, "seq no retrigger within beat");

        op.input_floats[0] = 0.01f;
        op.process(2);
        check_float(op.output_floats[3], 1.0f, "seq after 1 beat: step=1");
        check_float(op.output_floats[0], 20.0f, "seq step 1: value=20");
        check_float(op.output_floats[1], 1.0f, "seq step 1: trigger=1");

        auto& spread = op.output_spreads[4];
        check(spread.length == 4, "seq spread has 4 elements");
        if (spread.length == 4) {
            check_float(spread.data[0], 10.0f, "seq spread[0]=10");
            check_float(spread.data[1], 20.0f, "seq spread[1]=20");
            check_float(spread.data[2], 30.0f, "seq spread[2]=30");
            check_float(spread.data[3], 40.0f, "seq spread[3]=40");
        }
    }

    {
        AudioOpHarness op(loader);
        op.set_param("steps", 4.0f);
        op.set_param("rate", 2.0f);
        op.set_param("gate_length", 0.8f);
        op.set_param("probability", 0.0f);
        op.set_param("val_0", 10.0f);
        op.set_param("val_1", 20.0f);
        op.set_param("val_2", 30.0f);
        op.set_param("val_3", 40.0f);

        op.input_floats[0] = 0.0f;
        op.process(0);
        check_float(op.output_floats[0], 0.0f, "seq prob=0: value=0 (silenced)");
        check_float(op.output_floats[1], 0.0f, "seq prob=0: trigger=0");
        check_float(op.output_floats[2], 0.0f, "seq prob=0: gate=0");
    }
}

static void test_sequencer(vivid::OperatorRegistry& registry) {
    std::fprintf(stderr, "\n=== Sequencer Tests ===\n");

    auto* loader = registry.find("Sequencer");
    check(loader != nullptr, "Sequencer loader found");
    if (!loader) return;

    {
        AudioOpHarness op(loader);
        op.set_param("steps", 4.0f);
        op.input_spreads[0].length = 4;
        op.input_spreads[0].data[0] = 10.0f;
        op.input_spreads[0].data[1] = 20.0f;
        op.input_spreads[0].data[2] = 30.0f;
        op.input_spreads[0].data[3] = 40.0f;
        op.input_spreads[1].length = 4;
        op.input_spreads[1].data[0] = 1.0f;
        op.input_spreads[1].data[1] = 1.0f;
        op.input_spreads[1].data[2] = 1.0f;
        op.input_spreads[1].data[3] = 1.0f;
        op.input_spreads[2].length = 4;
        op.input_spreads[2].data[0] = 1.0f;
        op.input_spreads[2].data[1] = 1.0f;
        op.input_spreads[2].data[2] = 1.0f;
        op.input_spreads[2].data[3] = 1.0f;

        op.input_floats[0] = 0.00f;
        op.input_floats[1] = 0.0f;
        op.process(0);
        check_float(op.output_floats[1], 0.0f, "baseline step=0");
        check_float(op.output_floats[0], 10.0f, "baseline value step0");
        check_float(op.output_floats[2], 1.0f, "baseline trigger on first step");

        op.input_floats[0] = 0.10f;
        op.process(1);
        check_float(op.output_floats[2], 0.0f, "baseline no retrigger within step");

        op.input_floats[0] = 0.30f;
        op.process(2);
        check_float(op.output_floats[1], 1.0f, "baseline step=1");
        check_float(op.output_floats[0], 20.0f, "baseline value step1");
        check_float(op.output_floats[2], 1.0f, "baseline trigger on step change");
    }

    {
        AudioOpHarness op(loader);
        op.set_param("steps", 4.0f);
        op.input_spreads[0].length = 4;
        op.input_spreads[0].data[0] = 10.0f;
        op.input_spreads[0].data[1] = 20.0f;
        op.input_spreads[0].data[2] = 30.0f;
        op.input_spreads[0].data[3] = 40.0f;
        op.input_spreads[1].length = 4;
        op.input_spreads[1].data[0] = 1.0f;
        op.input_spreads[1].data[1] = 0.0f;
        op.input_spreads[1].data[2] = 1.0f;
        op.input_spreads[1].data[3] = 1.0f;
        op.input_spreads[2].length = 4;
        op.input_spreads[2].data[0] = 1.0f;
        op.input_spreads[2].data[1] = 1.0f;
        op.input_spreads[2].data[2] = 1.0f;
        op.input_spreads[2].data[3] = 1.0f;

        op.input_floats[0] = 0.00f;
        op.input_floats[1] = 0.0f;
        op.process(0);
        op.input_floats[0] = 0.30f;
        op.process(1);

        check_float(op.output_floats[1], 1.0f, "probability test step=1");
        check_float(op.output_floats[0], 0.0f, "probability test value silenced");
        check_float(op.output_floats[2], 0.0f, "probability test trigger suppressed");
    }

    {
        AudioOpHarness op(loader);
        op.set_param("steps", 1.0f);
        op.input_spreads[0].length = 1;
        op.input_spreads[0].data[0] = 11.0f;
        op.input_spreads[1].length = 1;
        op.input_spreads[1].data[0] = 1.0f;
        op.input_spreads[2].length = 1;
        op.input_spreads[2].data[0] = 4.0f;

        op.input_floats[0] = 0.01f;
        op.input_floats[1] = 0.0f;
        op.process(0);
        check_float(op.output_floats[2], 1.0f, "ratchet trigger bucket 0");

        op.input_floats[0] = 0.12f;
        op.process(1);
        check_float(op.output_floats[2], 0.0f, "ratchet no retrigger within bucket");

        op.input_floats[0] = 0.26f;
        op.process(2);
        check_float(op.output_floats[2], 1.0f, "ratchet trigger bucket 1");

        op.input_floats[0] = 0.51f;
        op.process(3);
        check_float(op.output_floats[2], 1.0f, "ratchet trigger bucket 2");

        op.input_floats[0] = 0.76f;
        op.process(4);
        check_float(op.output_floats[2], 1.0f, "ratchet trigger bucket 3");
        check_float(op.output_floats[0], 11.0f, "ratchet keeps step value");
    }
}

static void test_stack(vivid::OperatorRegistry& registry) {
    std::fprintf(stderr, "\n=== Stack Tests ===\n");

    {
        vivid::Graph g;
        g.add_node("src_a", "SpreadSourceOp", {{"base", 1.0f}, {"count", 3.0f}});
        g.add_node("src_b", "SpreadSourceOp", {{"base", 2.0f}, {"count", 2.0f}});
        g.add_node("stk", "Stack", {{"mode", 0.0f}});
        g.add_connection("src_a", "out", "stk", "a");
        g.add_connection("src_b", "out", "stk", "b");

        vivid::Scheduler sched;
        check(sched.build(g, registry), "Stack Concat build");
        sched.tick(0.0, 0.016, 0);

        auto* stk = sched.find_node_mut("stk");
        check(stk != nullptr, "Stack node found");
        if (!stk) { sched.shutdown(); return; }

        auto& out = stk->output_spreads[0];
        check(out.size() == 5, "Concat: 3+2=5 elements");
        if (out.size() == 5) {
            check_float(out[0], 1.0f, "concat[0]=1");
            check_float(out[1], 2.0f, "concat[1]=2");
            check_float(out[2], 3.0f, "concat[2]=3");
            check_float(out[3], 2.0f, "concat[3]=2");
            check_float(out[4], 4.0f, "concat[4]=4");
        }
        sched.shutdown();
    }

    {
        vivid::Graph g;
        g.add_node("src_a", "SpreadSourceOp", {{"base", 1.0f}, {"count", 3.0f}});
        g.add_node("src_b", "SpreadSourceOp", {{"base", 2.0f}, {"count", 3.0f}});
        g.add_node("stk", "Stack", {{"mode", 1.0f}});
        g.add_connection("src_a", "out", "stk", "a");
        g.add_connection("src_b", "out", "stk", "b");

        vivid::Scheduler sched;
        check(sched.build(g, registry), "Stack Interleave build");
        sched.tick(0.0, 0.016, 0);

        auto* stk = sched.find_node_mut("stk");
        check(stk != nullptr, "Stack interleave node found");
        if (!stk) { sched.shutdown(); return; }

        auto& out = stk->output_spreads[0];
        check(out.size() == 6, "Interleave: 3+3=6 elements");
        if (out.size() == 6) {
            check_float(out[0], 1.0f, "interleave[0]=1 (a[0])");
            check_float(out[1], 2.0f, "interleave[1]=2 (b[0])");
            check_float(out[2], 2.0f, "interleave[2]=2 (a[1])");
            check_float(out[3], 4.0f, "interleave[3]=4 (b[1])");
            check_float(out[4], 3.0f, "interleave[4]=3 (a[2])");
            check_float(out[5], 6.0f, "interleave[5]=6 (b[2])");
        }
        sched.shutdown();
    }

    {
        vivid::Graph g;
        g.add_node("src_a", "SpreadSourceOp", {{"base", 5.0f}, {"count", 2.0f}});
        g.add_node("stk", "Stack", {{"mode", 0.0f}});
        g.add_connection("src_a", "out", "stk", "a");

        vivid::Scheduler sched;
        check(sched.build(g, registry), "Stack empty inputs build");
        sched.tick(0.0, 0.016, 0);

        auto* stk = sched.find_node_mut("stk");
        auto& out = stk->output_spreads[0];
        check(out.size() == 2, "Stack only a connected: 2 elements");
        if (out.size() == 2) {
            check_float(out[0], 5.0f, "single input[0]=5");
            check_float(out[1], 10.0f, "single input[1]=10");
        }
        sched.shutdown();
    }
}

static void test_alternate(vivid::OperatorRegistry& registry) {
    std::fprintf(stderr, "\n=== Alternate Tests ===\n");

    vivid::Graph g;
    g.add_node("phase_src", "SpreadSourceOp", {{"base", 0.0f}, {"count", 1.0f}});
    g.add_node("src_a", "SpreadSourceOp", {{"base", 1.0f}, {"count", 2.0f}});
    g.add_node("src_b", "SpreadSourceOp", {{"base", 10.0f}, {"count", 2.0f}});
    g.add_node("alt", "Alternate", {{"cycle", 0.0f}});
    g.add_connection("phase_src", "out", "alt", "beat_phase");
    g.add_connection("src_a", "out", "alt", "a");
    g.add_connection("src_b", "out", "alt", "b");

    vivid::Scheduler sched;
    check(sched.build(g, registry), "Alternate build");

    auto* src = sched.find_node_mut("phase_src");
    auto* alt = sched.find_node_mut("alt");
    check(alt != nullptr, "Alternate node found");
    if (!alt || !src) { sched.shutdown(); return; }

    src->param_values[0] = 0.0f;
    sched.tick(0.0, 0.016, 0);

    auto& out = alt->output_spreads[0];
    check(out.size() == 2, "alt beat 0: 2 elements");
    if (out.size() >= 2) {
        check_float(out[0], 1.0f, "alt beat 0: [0]=1 (from a)");
        check_float(out[1], 2.0f, "alt beat 0: [1]=2 (from a)");
    }

    src->param_values[0] = 0.99f;
    sched.tick(0.016, 0.016, 1);
    src->param_values[0] = 0.01f;
    sched.tick(0.032, 0.016, 2);

    auto& out2 = alt->output_spreads[0];
    check(out2.size() == 2, "alt beat 1: 2 elements");
    if (out2.size() >= 2) {
        check_float(out2[0], 10.0f, "alt beat 1: [0]=10 (from b)");
        check_float(out2[1], 20.0f, "alt beat 1: [1]=20 (from b)");
    }

    sched.shutdown();
}

static void test_pat_transform(vivid::OperatorRegistry& registry) {
    std::fprintf(stderr, "\n=== PatTransform Tests ===\n");

    auto run_transform = [&](float base, int count,
                             float reverse, float rotate, float scale, float offset,
                             float probability,
                             const std::vector<float>& expected,
                             const char* label) {
        vivid::Graph g;
        g.add_node("src", "SpreadSourceOp", {{"base", base}, {"count", static_cast<float>(count)}});
        g.add_node("pt", "PatTransform", {
            {"reverse", reverse},
            {"rotate", rotate},
            {"scale", scale},
            {"offset", offset},
            {"probability", probability}
        });
        g.add_connection("src", "out", "pt", "pattern");

        vivid::Scheduler sched;
        if (!sched.build(g, registry)) {
            std::fprintf(stderr, "  FAIL: %s - build failed\n", label);
            failures++;
            return;
        }
        sched.tick(0.0, 0.016, 0);

        auto* pt = sched.find_node_mut("pt");
        check(pt != nullptr, label);
        if (!pt) { sched.shutdown(); return; }

        auto& out = pt->output_spreads[0];
        char buf[256];
        std::snprintf(buf, sizeof(buf), "%s: length %d (expected %d)",
                      label, static_cast<int>(out.size()), static_cast<int>(expected.size()));
        check(out.size() == expected.size(), buf);

        if (out.size() == expected.size()) {
            bool match = true;
            for (size_t i = 0; i < expected.size(); ++i) {
                if (std::fabs(out[i] - expected[i]) > 0.01f) {
                    match = false;
                    std::snprintf(buf, sizeof(buf), "%s: [%d] = %f, expected %f",
                                  label, static_cast<int>(i), out[i], expected[i]);
                    check(false, buf);
                }
            }
            if (match) {
                std::snprintf(buf, sizeof(buf), "%s: values correct", label);
                check(true, buf);
            }
        }

        sched.shutdown();
    };

    run_transform(1.0f, 3, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
                  {3.0f, 2.0f, 1.0f}, "Reverse [1,2,3]");
    run_transform(1.0f, 4, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
                  {2.0f, 3.0f, 4.0f, 1.0f}, "Rotate [1,2,3,4] by 1");
    run_transform(1.0f, 3, 0.0f, 0.0f, 2.0f, 0.0f, 1.0f,
                  {2.0f, 4.0f, 6.0f}, "Scale [1,2,3] * 2");
    run_transform(1.0f, 3, 0.0f, 0.0f, 1.0f, 10.0f, 1.0f,
                  {11.0f, 12.0f, 13.0f}, "Offset [1,2,3] + 10");
    run_transform(1.0f, 3, 1.0f, 1.0f, 2.0f, 0.0f, 1.0f,
                  {4.0f, 2.0f, 6.0f}, "Combined: reverse+rotate+scale");
}

static void test_integration(vivid::OperatorRegistry& registry) {
    std::fprintf(stderr, "\n=== Integration Test ===\n");

    vivid::Graph g;
    g.add_node("clock", "Clock", {{"bpm", 120.0f}});
    g.add_node("euclid", "Euclidean", {
        {"hits", 3.0f}, {"steps", 8.0f}, {"rotation", 0.0f},
        {"gate_length", 0.5f}, {"rate", 2.0f}
    });
    g.add_node("src", "SpreadSourceOp", {{"base", 1.0f}, {"count", 4.0f}});
    g.add_node("stk", "Stack", {{"mode", 0.0f}});
    g.add_node("xform", "PatTransform", {
        {"reverse", 0.0f}, {"rotate", 0.0f}, {"scale", 2.0f},
        {"offset", 0.0f}, {"probability", 1.0f}
    });

    g.add_connection("clock", "beat_phase", "euclid", "beat_phase");
    g.add_connection("euclid", "pattern", "stk", "a");
    g.add_connection("src", "out", "stk", "b");
    g.add_connection("stk", "output", "xform", "pattern");

    vivid::Scheduler sched;
    check(sched.build(g, registry), "integration build");

    for (int i = 0; i < 5; ++i) {
        sched.tick(i * 0.016, 0.016, static_cast<uint64_t>(i));
    }

    auto* stk = sched.find_node_mut("stk");
    auto* xform = sched.find_node_mut("xform");
    check(stk != nullptr, "stack node found");
    check(xform != nullptr, "transform node found");

    if (stk && xform) {
        auto& stk_out = stk->output_spreads[0];
        check(stk_out.size() == 12, "stack output: 8+4=12 elements");

        auto& xf_out = xform->output_spreads[0];
        check(xf_out.size() == 12, "transform output: 12 elements");

        if (stk_out.size() == 12 && xf_out.size() == 12) {
            bool all_doubled = true;
            for (size_t i = 0; i < 12; ++i) {
                if (std::fabs(xf_out[i] - stk_out[i] * 2.0f) > 0.01f) {
                    all_doubled = false;
                    break;
                }
            }
            check(all_doubled, "transform doubles all stack values");
        }
    }

    sched.shutdown();
}

int main() {
    namespace vstest = vivid_sequencers::test;

    std::string staging = "./.test_pattern_algebra_staging";
    std::filesystem::create_directories(staging);

    const char* core_plugin_dir_env = std::getenv("VIVID_CORE_PLUGIN_DIR");
    std::string core_plugin_dir = core_plugin_dir_env ? core_plugin_dir_env : ".";

    try {
        vstest::copy_plugin_or_throw(".", staging, "pattern_seq");
        vstest::copy_plugin_or_throw(".", staging, "sequencer");
        vstest::copy_plugin_or_throw(".", staging, "euclidean");
        vstest::copy_plugin_or_throw(".", staging, "pat_transform");
        vstest::copy_plugin_or_throw(core_plugin_dir, staging, "stack");
        vstest::copy_plugin_or_throw(core_plugin_dir, staging, "alternate");
        vstest::copy_plugin_or_throw(core_plugin_dir, staging, "spread_source_op");
        vstest::copy_plugin_or_throw(core_plugin_dir, staging, "clock");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        std::filesystem::remove_all(staging);
        return 1;
    }

    vivid::OperatorRegistry registry;
    check(registry.scan(staging.c_str()), "registry.scan() succeeds");
    check(registry.find("Euclidean") != nullptr, "Euclidean registered");
    check(registry.find("PatternSeq") != nullptr, "PatternSeq registered");
    check(registry.find("Sequencer") != nullptr, "Sequencer registered");
    check(registry.find("Stack") != nullptr, "Stack registered");
    check(registry.find("Alternate") != nullptr, "Alternate registered");
    check(registry.find("PatTransform") != nullptr, "PatTransform registered");
    check(registry.find("SpreadSourceOp") != nullptr, "SpreadSourceOp registered");
    check(registry.find("Clock") != nullptr, "Clock registered");

    test_euclidean(registry);
    test_pattern_seq(registry);
    test_sequencer(registry);
    test_stack(registry);
    test_alternate(registry);
    test_pat_transform(registry);
    test_integration(registry);

    std::filesystem::remove_all(staging);

    std::fprintf(stderr, "\n=== %s (%d failures) ===\n\n",
        failures == 0 ? "ALL PASSED" : "SOME FAILED", failures);
    return failures == 0 ? 0 : 1;
}
