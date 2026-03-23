#include "../src/tracker_data.h"
#include "audio_op_harness.h"
#include "plugin_test_utils.h"
#include "runtime/operator_registry.h"
#include <cstdio>
#include <filesystem>
#include <string>

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

int main() {
    namespace vstest = vivid_sequencers::test;

    std::filesystem::path staging = "./.test_tracker_contract_staging";
    std::filesystem::create_directories(staging);

    try {
        vstest::copy_plugin_or_throw(".", staging, "tracker");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        std::filesystem::remove_all(staging);
        return 1;
    }

    vivid::OperatorRegistry registry;
    check(registry.scan(staging.c_str()), "registry.scan() succeeds");

    auto* loader = registry.find("Tracker");
    check(loader != nullptr, "Tracker loader found");
    if (!loader) {
        std::filesystem::remove_all(staging);
        return 1;
    }

    AudioOpHarness op(loader);
    op.set_param("rate", 2.0f);
    op.set_param("speed", 1.0f);

    tracker::TrackerSong song;
    song.num_patterns = 2;
    song.arrangement_length = 2;
    song.arrangement[0] = 0;
    song.arrangement[1] = 1;
    song.patterns[0].num_rows = 2;
    song.patterns[1].num_rows = 2;
    op.set_string_param("pattern_data", tracker::serialize_song(song));

    uint64_t frame = 0;
    auto advance_beat = [&]() {
        op.input_floats[0] = 0.99f;
        op.process(frame++);
        op.input_floats[0] = 0.01f;
        op.process(frame++);
    };

    op.input_floats[0] = 0.0f;
    op.input_floats[1] = 0.0f;
    op.process(frame++);
    check_float(op.output_floats[0], 0.0f, "initial row = 0");
    check_float(op.output_floats[1], 0.0f, "initial pattern = 0");
    check_float(op.output_floats[2], 0.0f, "initial order = 0");

    advance_beat();
    check_float(op.output_floats[0], 1.0f, "after one beat row = 1");
    check_float(op.output_floats[1], 0.0f, "after one beat pattern stays 0");
    check_float(op.output_floats[2], 0.0f, "after one beat order stays 0");

    advance_beat();
    check_float(op.output_floats[0], 0.0f, "after two beats row wraps to 0");
    check_float(op.output_floats[1], 1.0f, "after two beats pattern = 1");
    check_float(op.output_floats[2], 1.0f, "after two beats order = 1");

    advance_beat();
    advance_beat();
    check_float(op.output_floats[0], 0.0f, "after four beats row returns to 0");
    check_float(op.output_floats[1], 0.0f, "after four beats pattern loops to 0");
    check_float(op.output_floats[2], 0.0f, "after four beats order loops to 0");

    op.input_floats[0] = 0.0f;
    op.input_floats[1] = 1.0f;
    op.process(frame++);
    check_float(op.output_floats[0], 0.0f, "reset row = 0");
    check_float(op.output_floats[1], 0.0f, "reset pattern = 0");
    check_float(op.output_floats[2], 0.0f, "reset order = 0");

    std::filesystem::remove_all(staging);

    std::fprintf(stderr, "\n=== %s (%d failures) ===\n\n",
                 failures == 0 ? "ALL PASSED" : "SOME FAILED", failures);
    return failures == 0 ? 0 : 1;
}
