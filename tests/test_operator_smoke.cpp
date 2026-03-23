#include "audio_op_harness.h"
#include "plugin_test_utils.h"
#include "runtime/operator_registry.h"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

static int failures = 0;

struct ExpectedPlugin {
    const char* stem;
    const char* type_name;
};

int main() {
    namespace vstest = vivid_sequencers::test;

    std::string staging = "./.test_operator_smoke_staging";
    std::filesystem::create_directories(staging);

    const char* core_plugin_dir_env = std::getenv("VIVID_CORE_PLUGIN_DIR");
    std::string core_plugin_dir = core_plugin_dir_env ? core_plugin_dir_env : ".";

    const ExpectedPlugin package_ops[] = {
        {"sequencer", "Sequencer"},
        {"drum_sequencer", "DrumSequencer"},
        {"pattern_seq", "PatternSeq"},
        {"note_pattern", "NotePattern"},
        {"note_duration", "NoteDuration"},
        {"arpeggiator", "Arpeggiator"},
        {"chord_progression", "ChordProgression"},
        {"state_machine", "StateMachine"},
        {"tracker", "Tracker"},
        {"euclidean", "Euclidean"},
        {"pat_transform", "PatTransform"},
        {"phase_to_midi", "PhaseToMidi"},
    };
    for (const auto& plugin : package_ops) {
        std::error_code ec;
        vstest::copy_plugin_if_exists(".", staging, plugin.stem, ec);
        if (ec) {
            std::fprintf(stderr, "  SKIP: %s (not found: %s)\n", plugin.stem, ec.message().c_str());
        }
    }

    const char* core_ops[] = {
        "stack", "alternate", "spread_source_op", "clock",
    };
    for (const char* name : core_ops) {
        std::error_code ec;
        vstest::copy_plugin_if_exists(core_plugin_dir, staging, name, ec);
    }

    vivid::OperatorRegistry registry;
    if (!registry.scan(staging.c_str())) {
        std::fprintf(stderr, "FAIL: registry.scan() failed\n");
        std::filesystem::remove_all(staging);
        return 1;
    }

    for (const auto& plugin : package_ops) {
        if (!registry.find(plugin.type_name)) {
            std::fprintf(stderr, "  FAIL: %s — package plugin did not load\n", plugin.stem);
            ++failures;
        }
    }

    auto names = registry.type_names();
    std::fprintf(stderr, "\n=== Operator Smoke Test (%zu operators) ===\n",
                 names.size());

    for (const auto& name : names) {
        auto* loader = registry.find(name.c_str());
        if (!loader) {
            std::fprintf(stderr, "  FAIL: %s — loader not found\n", name.c_str());
            ++failures;
            continue;
        }

        const auto* desc = loader->descriptor();
        if (!desc) {
            std::fprintf(stderr, "  FAIL: %s — no descriptor\n", name.c_str());
            ++failures;
            continue;
        }

        // Skip control-domain-only operators (no process_audio)
        if (desc->domain == VIVID_DOMAIN_CONTROL) {
            std::fprintf(stderr, "  SKIP: %s (control domain)\n", name.c_str());
            continue;
        }

        AudioOpHarness harness(loader);
        if (!harness.instance) {
            std::fprintf(stderr, "  FAIL: %s — create_instance returned null\n", name.c_str());
            ++failures;
            continue;
        }

        // Call process once — if it doesn't crash, that's a pass
        harness.process(0);
        std::fprintf(stderr, "  PASS: %s\n", name.c_str());
    }

    std::filesystem::remove_all(staging);

    std::fprintf(stderr, "\n=== %s (%d failures) ===\n\n",
                 failures == 0 ? "ALL PASSED" : "SOME FAILED", failures);
    return failures == 0 ? 0 : 1;
}
