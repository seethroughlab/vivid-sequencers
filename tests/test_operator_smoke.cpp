#include "audio_op_harness.h"
#include "runtime/operator_registry.h"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

static int failures = 0;

int main() {
    std::string staging = "./.test_operator_smoke_staging";
    std::filesystem::create_directories(staging);

    const char* core_plugin_dir_env = std::getenv("VIVID_CORE_PLUGIN_DIR");
    std::string core_plugin_dir = core_plugin_dir_env ? core_plugin_dir_env : ".";

    // Copy all sequencer package operators
    const char* package_ops[] = {
        "drum_sequencer", "pattern_seq", "note_pattern", "note_duration",
        "arpeggiator", "chord_progression", "state_machine", "tracker",
    };
    for (const char* name : package_ops) {
        std::string src = std::string(".") + "/" + name + ".dylib";
        std::string dst = staging + "/" + name + ".dylib";
        std::error_code ec;
        std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            std::fprintf(stderr, "  SKIP: %s (not found: %s)\n", name, ec.message().c_str());
        }
    }

    // Copy core operators that might be available
    const char* core_ops[] = {
        "euclidean", "stack", "alternate", "pat_transform",
        "spread_source_op", "clock",
    };
    for (const char* name : core_ops) {
        std::string src = core_plugin_dir + "/" + name + ".dylib";
        std::string dst = staging + "/" + name + ".dylib";
        std::error_code ec;
        std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing, ec);
        // Silently skip missing core ops — this test focuses on package operators
    }

    vivid::OperatorRegistry registry;
    if (!registry.scan(staging.c_str())) {
        std::fprintf(stderr, "FAIL: registry.scan() failed\n");
        std::filesystem::remove_all(staging);
        return 1;
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
