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

    std::filesystem::path staging = "./.test_audio_operator_contracts_staging";
    std::filesystem::create_directories(staging);

    try {
        vstest::copy_plugin_or_throw(".", staging, "sequencer");
        vstest::copy_plugin_or_throw(".", staging, "drum_sequencer");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        std::filesystem::remove_all(staging);
        return 1;
    }

    vivid::OperatorRegistry registry;
    check(registry.scan(staging.c_str()), "registry.scan() succeeds");

    std::fprintf(stderr, "\n=== Sequencer Contract ===\n");
    {
        auto* loader = registry.find("Sequencer");
        check(loader != nullptr, "Sequencer loader found");
        if (loader) {
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
            check_float(op.output_floats[0], 10.0f, "step 0 value");
            check_float(op.output_floats[1], 0.0f, "step 0 index");
            check_float(op.output_floats[2], 1.0f, "step 0 trigger");

            op.input_floats[0] = 0.30f;
            op.process(1);
            check_float(op.output_floats[0], 0.0f, "step 1 probability mutes value");
            check_float(op.output_floats[1], 1.0f, "step 1 index");
            check_float(op.output_floats[2], 0.0f, "step 1 probability suppresses trigger");
        }
    }

    std::fprintf(stderr, "\n=== DrumSequencer Contract ===\n");
    {
        auto* loader = registry.find("DrumSequencer");
        check(loader != nullptr, "DrumSequencer loader found");
        if (loader) {
            AudioOpHarness op(loader);
            op.set_param("steps", 4.0f);
            op.set_param("kick_note", 36.0f);
            op.set_param("kick_0", 1.0f);
            op.set_param("kick_ma_0", 0.75f);
            op.set_param("kick_mb_0", 0.25f);

            op.input_floats[0] = 0.01f;
            op.input_floats[1] = 0.0f;
            op.process(0);
            check_float(op.output_floats[0], 1.0f, "kick trigger fires on active step");
            check_float(op.output_floats[6], 0.0f, "step output is 0");
            check_float(op.output_floats[7], 0.75f, "kick mod A output");
            check_float(op.output_floats[13], 0.25f, "kick mod B output");

            auto& gates = op.output_spreads[19];
            auto& notes = op.output_spreads[20];
            auto& vels = op.output_spreads[21];
            check(gates.length == 6, "drum gates spread length");
            check(notes.length == 6, "drum notes spread length");
            check(vels.length == 6, "drum velocities spread length");
            if (gates.length == 6 && notes.length == 6 && vels.length == 6) {
                check_float(gates.data[0], 1.0f, "gates[0] mirrors kick trigger");
                check_float(notes.data[0], 36.0f, "notes[0] mirrors kick note");
                check_float(vels.data[0], 0.75f, "velocities[0] mirrors mod A");
            }

            op.input_floats[0] = 0.10f;
            op.process(1);
            check_float(op.output_floats[0], 0.0f, "kick trigger does not retrigger within step");
        }
    }

    std::filesystem::remove_all(staging);

    std::fprintf(stderr, "\n=== %s (%d failures) ===\n\n",
                 failures == 0 ? "ALL PASSED" : "SOME FAILED", failures);
    return failures == 0 ? 0 : 1;
}
