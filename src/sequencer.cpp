#include "operator_api/operator.h"
#include "operator_api/midi_types.h"
#include "operator_api/type_id.h"
#include "midi_helpers.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

struct Sequencer : vivid::AudioOperatorBase {
    static constexpr const char* kName   = "Sequencer";
    static constexpr bool kTimeDependent = false;

    vivid::Param<int>   steps {"steps",  8, 1, 16};
    vivid::Param<float> val_0 {"val_0",  0.0f, -10000.0f, 10000.0f};
    vivid::Param<float> val_1 {"val_1",  0.0f, -10000.0f, 10000.0f};
    vivid::Param<float> val_2 {"val_2",  0.0f, -10000.0f, 10000.0f};
    vivid::Param<float> val_3 {"val_3",  0.0f, -10000.0f, 10000.0f};
    vivid::Param<float> val_4 {"val_4",  0.0f, -10000.0f, 10000.0f};
    vivid::Param<float> val_5 {"val_5",  0.0f, -10000.0f, 10000.0f};
    vivid::Param<float> val_6 {"val_6",  0.0f, -10000.0f, 10000.0f};
    vivid::Param<float> val_7 {"val_7",  0.0f, -10000.0f, 10000.0f};
    vivid::Param<float> val_8 {"val_8",  0.0f, -10000.0f, 10000.0f};
    vivid::Param<float> val_9 {"val_9",  0.0f, -10000.0f, 10000.0f};
    vivid::Param<float> val_10{"val_10", 0.0f, -10000.0f, 10000.0f};
    vivid::Param<float> val_11{"val_11", 0.0f, -10000.0f, 10000.0f};
    vivid::Param<float> val_12{"val_12", 0.0f, -10000.0f, 10000.0f};
    vivid::Param<float> val_13{"val_13", 0.0f, -10000.0f, 10000.0f};
    vivid::Param<float> val_14{"val_14", 0.0f, -10000.0f, 10000.0f};
    vivid::Param<float> val_15{"val_15", 0.0f, -10000.0f, 10000.0f};
    vivid::Param<float> prob_0 {"prob_0",  1.0f, 0.0f, 1.0f};
    vivid::Param<float> prob_1 {"prob_1",  1.0f, 0.0f, 1.0f};
    vivid::Param<float> prob_2 {"prob_2",  1.0f, 0.0f, 1.0f};
    vivid::Param<float> prob_3 {"prob_3",  1.0f, 0.0f, 1.0f};
    vivid::Param<float> prob_4 {"prob_4",  1.0f, 0.0f, 1.0f};
    vivid::Param<float> prob_5 {"prob_5",  1.0f, 0.0f, 1.0f};
    vivid::Param<float> prob_6 {"prob_6",  1.0f, 0.0f, 1.0f};
    vivid::Param<float> prob_7 {"prob_7",  1.0f, 0.0f, 1.0f};
    vivid::Param<float> prob_8 {"prob_8",  1.0f, 0.0f, 1.0f};
    vivid::Param<float> prob_9 {"prob_9",  1.0f, 0.0f, 1.0f};
    vivid::Param<float> prob_10{"prob_10", 1.0f, 0.0f, 1.0f};
    vivid::Param<float> prob_11{"prob_11", 1.0f, 0.0f, 1.0f};
    vivid::Param<float> prob_12{"prob_12", 1.0f, 0.0f, 1.0f};
    vivid::Param<float> prob_13{"prob_13", 1.0f, 0.0f, 1.0f};
    vivid::Param<float> prob_14{"prob_14", 1.0f, 0.0f, 1.0f};
    vivid::Param<float> prob_15{"prob_15", 1.0f, 0.0f, 1.0f};
    vivid::Param<int>   ratchet_0 {"ratchet_0",  1, 1, 8};
    vivid::Param<int>   ratchet_1 {"ratchet_1",  1, 1, 8};
    vivid::Param<int>   ratchet_2 {"ratchet_2",  1, 1, 8};
    vivid::Param<int>   ratchet_3 {"ratchet_3",  1, 1, 8};
    vivid::Param<int>   ratchet_4 {"ratchet_4",  1, 1, 8};
    vivid::Param<int>   ratchet_5 {"ratchet_5",  1, 1, 8};
    vivid::Param<int>   ratchet_6 {"ratchet_6",  1, 1, 8};
    vivid::Param<int>   ratchet_7 {"ratchet_7",  1, 1, 8};
    vivid::Param<int>   ratchet_8 {"ratchet_8",  1, 1, 8};
    vivid::Param<int>   ratchet_9 {"ratchet_9",  1, 1, 8};
    vivid::Param<int>   ratchet_10{"ratchet_10", 1, 1, 8};
    vivid::Param<int>   ratchet_11{"ratchet_11", 1, 1, 8};
    vivid::Param<int>   ratchet_12{"ratchet_12", 1, 1, 8};
    vivid::Param<int>   ratchet_13{"ratchet_13", 1, 1, 8};
    vivid::Param<int>   ratchet_14{"ratchet_14", 1, 1, 8};
    vivid::Param<int>   ratchet_15{"ratchet_15", 1, 1, 8};
    vivid::Param<int>   midi_channel{"midi_channel", 1, 1, 16};

    Sequencer() {
        vivid::semantic_tag(steps, "count");
        vivid::semantic_shape(steps, "int");
        vivid::semantic_intent(steps, "sequence_length");

        for (auto* p : {&prob_0, &prob_1, &prob_2, &prob_3, &prob_4, &prob_5, &prob_6, &prob_7,
                        &prob_8, &prob_9, &prob_10, &prob_11, &prob_12, &prob_13, &prob_14, &prob_15}) {
            vivid::semantic_tag(*p, "probability_01");
            vivid::semantic_shape(*p, "scalar");
        }

        for (auto* p : {&ratchet_0, &ratchet_1, &ratchet_2, &ratchet_3, &ratchet_4, &ratchet_5, &ratchet_6, &ratchet_7,
                        &ratchet_8, &ratchet_9, &ratchet_10, &ratchet_11, &ratchet_12, &ratchet_13, &ratchet_14, &ratchet_15}) {
            vivid::semantic_tag(*p, "count");
            vivid::semantic_shape(*p, "int");
            vivid::semantic_intent(*p, "ratchet_subdivisions");
        }
    }

    void collect_params(std::vector<vivid::ParamBase*>& out) override {
        out.push_back(&steps);                          // 0
        out.push_back(&val_0);  out.push_back(&val_1); // 1..16
        out.push_back(&val_2);  out.push_back(&val_3);
        out.push_back(&val_4);  out.push_back(&val_5);
        out.push_back(&val_6);  out.push_back(&val_7);
        out.push_back(&val_8);  out.push_back(&val_9);
        out.push_back(&val_10); out.push_back(&val_11);
        out.push_back(&val_12); out.push_back(&val_13);
        out.push_back(&val_14); out.push_back(&val_15);

        out.push_back(&prob_0);  out.push_back(&prob_1); // 17..32
        out.push_back(&prob_2);  out.push_back(&prob_3);
        out.push_back(&prob_4);  out.push_back(&prob_5);
        out.push_back(&prob_6);  out.push_back(&prob_7);
        out.push_back(&prob_8);  out.push_back(&prob_9);
        out.push_back(&prob_10); out.push_back(&prob_11);
        out.push_back(&prob_12); out.push_back(&prob_13);
        out.push_back(&prob_14); out.push_back(&prob_15);

        out.push_back(&ratchet_0);  out.push_back(&ratchet_1); // 33..48
        out.push_back(&ratchet_2);  out.push_back(&ratchet_3);
        out.push_back(&ratchet_4);  out.push_back(&ratchet_5);
        out.push_back(&ratchet_6);  out.push_back(&ratchet_7);
        out.push_back(&ratchet_8);  out.push_back(&ratchet_9);
        out.push_back(&ratchet_10); out.push_back(&ratchet_11);
        out.push_back(&ratchet_12); out.push_back(&ratchet_13);
        out.push_back(&ratchet_14); out.push_back(&ratchet_15);

        out.push_back(&midi_channel); // 49
    }

    void collect_ports(std::vector<VividPortDescriptor>& out) override {
        out.push_back({"beat_phase",   VIVID_PORT_FLOAT, VIVID_PORT_INPUT});
        out.push_back({"reset",   VIVID_PORT_FLOAT, VIVID_PORT_INPUT});
        out.push_back({"value",   VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        out.push_back({"step",    VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        out.push_back({"trigger", VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        out.push_back(VIVID_CUSTOM_REF_PORT("midi_out", VIVID_PORT_OUTPUT, VividMidiBuffer));
    }

    void process_audio(const VividAudioContext* ctx) override {
        float phase = ctx->input_float_values[0];
        bool reset = ctx->input_float_values[1] > 0.5f;

        // Rising-edge reset: capture current phase as offset
        if (reset && !prev_reset_)
            phase_offset_ = phase;
        prev_reset_ = reset;

        float adj_phase = std::fmod(phase - phase_offset_ + 1.0f, 1.0f);
        int n = std::max(steps.int_value(), 1);
        float scaled = adj_phase * static_cast<float>(n);
        int step = static_cast<int>(scaled);
        step = std::clamp(step, 0, n - 1);

        bool step_changed = (step != prev_step_);
        if (step_changed) {
            float prob = std::clamp(ctx->param_values[kProbBase + step], 0.0f, 1.0f);
            step_active_ = (prob >= 1.0f) || (next_random() < prob);
            int raw_ratchet = static_cast<int>(std::lround(ctx->param_values[kRatchetBase + step]));
            current_ratchet_ = std::clamp(raw_ratchet, 1, 8);
            prev_ratchet_index_ = -1;
            prev_step_ = step;
        }

        float value = step_active_ ? ctx->param_values[kValueBase + step] : 0.0f;
        float step_phase = std::clamp(scaled - static_cast<float>(step), 0.0f, 0.999999f);
        int ratchet_index = static_cast<int>(step_phase * static_cast<float>(current_ratchet_));
        ratchet_index = std::clamp(ratchet_index, 0, current_ratchet_ - 1);
        bool ratchet_trigger = (ratchet_index != prev_ratchet_index_);
        if (ratchet_trigger) prev_ratchet_index_ = ratchet_index;

        bool trigger = step_active_ && ratchet_trigger;
        ctx->output_float_values[0] = value;
        ctx->output_float_values[1] = static_cast<float>(step);
        ctx->output_float_values[2] = trigger ? 1.0f : 0.0f;

        // MIDI output: note-on on trigger, legato note-off before new note
        uint8_t ch = static_cast<uint8_t>(midi_channel.int_value() - 1);
        midi_buf_.count = 0;
        if (trigger) {
            uint8_t note = static_cast<uint8_t>(std::clamp(static_cast<int>(value), 0, 127));
            if (prev_midi_note_ >= 0) {
                vivid_sequencers::midi_note_off(midi_buf_,
                    static_cast<uint8_t>(prev_midi_note_), ch);
            }
            vivid_sequencers::midi_note_on(midi_buf_, note, 100, ch);
            prev_midi_note_ = note;
        }
        if (ctx->custom_outputs && ctx->custom_output_count > 0) {
            ctx->custom_outputs[0] = &midi_buf_;
        }
    }

private:
    static constexpr int kValueBase = 1;
    static constexpr int kProbBase = 17;
    static constexpr int kRatchetBase = 33;

    float next_random() {
        rng_state_ ^= (rng_state_ << 13);
        rng_state_ ^= (rng_state_ >> 17);
        rng_state_ ^= (rng_state_ << 5);
        constexpr float kInvMax = 1.0f / 4294967295.0f;
        return static_cast<float>(rng_state_) * kInvMax;
    }

    int prev_step_ = -1;
    float phase_offset_ = 0.0f;
    bool prev_reset_ = false;
    bool step_active_ = true;
    int current_ratchet_ = 1;
    int prev_ratchet_index_ = -1;
    uint32_t rng_state_ = 0xA5C31E59u;
    int prev_midi_note_ = -1;
    VividMidiBuffer midi_buf_ = {};
};

VIVID_REGISTER(Sequencer)
