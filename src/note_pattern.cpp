#include "operator_api/operator.h"
#include "operator_api/midi_types.h"
#include "operator_api/type_id.h"
#include "midi_helpers.h"
#include <algorithm>
#include <cmath>

namespace note_insp {
static const char* kNoteNames[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
static const char* kChordAbbr[] = {"M","m","dim","aug","7","m7","M7"};
static constexpr float kTypeColors[7][3] = {
    {0.39f, 0.63f, 0.86f}, {0.63f, 0.39f, 0.78f}, {0.78f, 0.39f, 0.39f},
    {0.86f, 0.71f, 0.31f}, {0.31f, 0.71f, 0.63f}, {0.55f, 0.47f, 0.78f},
    {0.31f, 0.55f, 0.86f},
};
static constexpr float kPreviewH = 60.0f;
static constexpr float kLineH = 18.0f;
} // namespace note_insp

struct NotePattern : vivid::AudioOperatorBase {
    static constexpr const char* kName   = "NotePattern";
    static constexpr bool kTimeDependent = true;

    vivid::Param<int>   steps        {"steps",          4, 1, 8};
    vivid::Param<int>   root_0       {"root_0",         0, {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}};
    vivid::Param<int>   root_1       {"root_1",         0, {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}};
    vivid::Param<int>   root_2       {"root_2",         0, {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}};
    vivid::Param<int>   root_3       {"root_3",         0, {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}};
    vivid::Param<int>   root_4       {"root_4",         0, {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}};
    vivid::Param<int>   root_5       {"root_5",         0, {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}};
    vivid::Param<int>   root_6       {"root_6",         0, {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}};
    vivid::Param<int>   root_7       {"root_7",         0, {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}};
    vivid::Param<int>   type_0       {"type_0",         0, {"Major","Minor","Dim","Aug","Dom7","Min7","Maj7"}};
    vivid::Param<int>   type_1       {"type_1",         0, {"Major","Minor","Dim","Aug","Dom7","Min7","Maj7"}};
    vivid::Param<int>   type_2       {"type_2",         0, {"Major","Minor","Dim","Aug","Dom7","Min7","Maj7"}};
    vivid::Param<int>   type_3       {"type_3",         0, {"Major","Minor","Dim","Aug","Dom7","Min7","Maj7"}};
    vivid::Param<int>   type_4       {"type_4",         0, {"Major","Minor","Dim","Aug","Dom7","Min7","Maj7"}};
    vivid::Param<int>   type_5       {"type_5",         0, {"Major","Minor","Dim","Aug","Dom7","Min7","Maj7"}};
    vivid::Param<int>   type_6       {"type_6",         0, {"Major","Minor","Dim","Aug","Dom7","Min7","Maj7"}};
    vivid::Param<int>   type_7       {"type_7",         0, {"Major","Minor","Dim","Aug","Dom7","Min7","Maj7"}};
    vivid::Param<int>   octave       {"octave",         4, 2, 7};
    vivid::Param<int>   beats_per_step{"beats_per_step", 4, 1, 16};
    vivid::Param<float> gate_length  {"gate_length",    0.8f, 0.01f, 1.0f};
    vivid::Param<float> velocity     {"velocity",       0.8f, 0.0f, 1.0f};
    vivid::Param<int>   midi_channel {"midi_channel",   1, 1, 16};

    // Internal state
    int beat_count_ = 0;
    float prev_phase_ = 0.0f;
    bool prev_gate_ = false;
    int prev_notes_[5] = {-1, -1, -1, -1, -1};
    int prev_note_count_ = 0;
    VividMidiBuffer midi_buf_ = {};

    // Chord interval tables: [type][interval_count], terminated by -1
    // 0=major, 1=minor, 2=dim, 3=aug, 4=dom7, 5=min7, 6=maj7
    static constexpr int kChordIntervals[7][5] = {
        {0, 4, 7, -1, -1},    // major
        {0, 3, 7, -1, -1},    // minor
        {0, 3, 6, -1, -1},    // dim
        {0, 4, 8, -1, -1},    // aug
        {0, 4, 7, 10, -1},    // dom7
        {0, 3, 7, 10, -1},    // min7
        {0, 4, 7, 11, -1},    // maj7
    };

    void collect_params(std::vector<vivid::ParamBase*>& out) override {
        out.push_back(&steps);
        // Hide step params — rendered by custom inspector
        size_t hidden_start = out.size();
        out.push_back(&root_0); out.push_back(&root_1);
        out.push_back(&root_2); out.push_back(&root_3);
        out.push_back(&root_4); out.push_back(&root_5);
        out.push_back(&root_6); out.push_back(&root_7);
        out.push_back(&type_0); out.push_back(&type_1);
        out.push_back(&type_2); out.push_back(&type_3);
        out.push_back(&type_4); out.push_back(&type_5);
        out.push_back(&type_6); out.push_back(&type_7);
        for (size_t i = hidden_start; i < out.size(); ++i)
            out[i]->display_hint = VIVID_DISPLAY_HIDDEN;
        out.push_back(&octave);
        out.push_back(&beats_per_step);
        out.push_back(&gate_length);
        out.push_back(&velocity);
        out.push_back(&midi_channel);
    }

    void collect_ports(std::vector<VividPortDescriptor>& out) override {
        out.push_back({"beat_phase", VIVID_PORT_FLOAT, VIVID_PORT_INPUT});
        out.push_back({"notes",      VIVID_PORT_SPREAD, VIVID_PORT_OUTPUT});
        out.push_back({"velocities", VIVID_PORT_SPREAD, VIVID_PORT_OUTPUT});
        out.push_back({"gates",      VIVID_PORT_SPREAD, VIVID_PORT_OUTPUT});
        out.push_back(VIVID_CUSTOM_REF_PORT("midi_out", VIVID_PORT_OUTPUT, VividMidiBuffer));
    }

    void process_audio(const VividAudioContext* ctx) override {
        float beat_phase = ctx->input_float_values[0];
        int num_steps = steps.int_value();
        int oct = octave.int_value();
        int bps = beats_per_step.int_value();
        float gl = gate_length.value;
        float vel = velocity.value;

        // Detect beat_phase wraps (delta < -0.5) → increment beat_count_
        float delta = beat_phase - prev_phase_;
        if (delta < -0.5f) {
            beat_count_++;
        }
        prev_phase_ = beat_phase;

        // Current step
        int current_step = (beat_count_ / bps) % num_steps;

        // Look up root and chord type for current step
        // Params are ordered: steps, root_0..root_7, type_0..type_7, octave, beats_per_step, ...
        // root_0 is param index 1, type_0 is param index 9
        int root = static_cast<int>(ctx->param_values[1 + current_step]);
        int chord_type = static_cast<int>(ctx->param_values[9 + current_step]);
        if (chord_type < 0) chord_type = 0;
        if (chord_type > 6) chord_type = 6;

        // Count chord intervals
        const int* intervals = kChordIntervals[chord_type];
        int chord_size = 0;
        for (int i = 0; i < 5 && intervals[i] >= 0; ++i) {
            chord_size++;
        }

        // Gate: 1.0 if beat_phase < gate_length, else 0.0
        float gate_val = (beat_phase < gl) ? 1.0f : 0.0f;

        // Write output spreads
        if (ctx->output_spreads) {
            auto& notes_sp = ctx->output_spreads[0];
            auto& vel_sp   = ctx->output_spreads[1];
            auto& gates_sp = ctx->output_spreads[2];

            uint32_t len = static_cast<uint32_t>(chord_size);
            if (notes_sp.capacity >= len) {
                notes_sp.length = len;
                vel_sp.length   = len;
                gates_sp.length = len;
                for (uint32_t i = 0; i < len; ++i) {
                    notes_sp.data[i] = static_cast<float>(root + oct * 12 + intervals[i]);
                    vel_sp.data[i]   = vel;
                    gates_sp.data[i] = gate_val;
                }
            }
        }

        // MIDI output: polyphonic note-on/off on gate edges
        uint8_t ch = static_cast<uint8_t>(midi_channel.int_value() - 1);
        uint8_t midi_vel = static_cast<uint8_t>(std::clamp(static_cast<int>(vel * 127.0f), 0, 127));
        midi_buf_.count = 0;
        bool gate_high = (gate_val > 0.5f);
        if (gate_high && !prev_gate_) {
            // Gate rising: note-off previous chord, note-on new chord
            for (int i = 0; i < prev_note_count_; ++i) {
                if (prev_notes_[i] >= 0) {
                    vivid_sequencers::midi_note_off(midi_buf_,
                        static_cast<uint8_t>(prev_notes_[i]), ch);
                }
            }
            prev_note_count_ = chord_size;
            for (int i = 0; i < chord_size && i < 5; ++i) {
                int note = std::clamp(root + oct * 12 + intervals[i], 0, 127);
                vivid_sequencers::midi_note_on(midi_buf_,
                    static_cast<uint8_t>(note), midi_vel, ch);
                prev_notes_[i] = note;
            }
        } else if (!gate_high && prev_gate_) {
            // Gate falling: note-off all
            for (int i = 0; i < prev_note_count_; ++i) {
                if (prev_notes_[i] >= 0) {
                    vivid_sequencers::midi_note_off(midi_buf_,
                        static_cast<uint8_t>(prev_notes_[i]), ch);
                    prev_notes_[i] = -1;
                }
            }
            prev_note_count_ = 0;
        }
        prev_gate_ = gate_high;
        if (ctx->custom_outputs && ctx->custom_output_count > 0) {
            ctx->custom_outputs[0] = &midi_buf_;
        }

        // Scalar fallback: first note
        if (chord_size > 0) {
            ctx->output_float_values[0] = static_cast<float>(root + oct * 12 + intervals[0]);
            ctx->output_float_values[1] = vel;
            ctx->output_float_values[2] = gate_val;
        }
    }

    void draw_inspector(VividInspectorContext* ctx) override {
        namespace ni = note_insp;
        auto& d = ctx->draw;
        void* o = d.opaque;
        const auto& th = ctx->theme;

        float px = ctx->content_x;
        float py = ctx->content_y;
        float w = ctx->content_width;
        float h = ni::kPreviewH;

        int num_steps = (ctx->param_count > 0) ? static_cast<int>(ctx->param_values[0]) : 4;
        num_steps = std::max(1, std::min(8, num_steps));
        float cell_w = w / static_cast<float>(num_steps);

        // Detect current step from first output note (scalar fallback)
        int current_step = -1;
        if (ctx->output_count > 0) {
            float out_note = ctx->output_values[0];
            int oct = (ctx->param_count > 17) ? static_cast<int>(ctx->param_values[17]) : 4;
            for (int s = 0; s < num_steps; ++s) {
                int root = static_cast<int>(ctx->param_values[1 + s]);
                int chord_type = std::clamp(static_cast<int>(ctx->param_values[9 + s]), 0, 6);
                float expected = static_cast<float>(root + oct * 12 + kChordIntervals[chord_type][0]);
                if (std::fabs(out_note - expected) < 0.5f) {
                    current_step = s;
                    break;
                }
            }
        }

        py += 4;

        // Dark background
        d.draw_rect(o, px, py, w, h, {th.dark_bg.r, th.dark_bg.g, th.dark_bg.b, 0.9f});

        for (int s = 0; s < num_steps; ++s) {
            int root = std::clamp(static_cast<int>(ctx->param_values[1 + s]), 0, 11);
            int chord_type = std::clamp(static_cast<int>(ctx->param_values[9 + s]), 0, 6);

            float cx = px + s * cell_w;
            bool is_current = (s == current_step);

            // Current step highlight
            if (is_current) {
                d.draw_rect(o, cx, py, cell_w, h, {0.18f, 0.22f, 0.30f, 0.6f});
            }

            // Note name centered
            const char* note = ni::kNoteNames[root];
            float nw = d.text_width(o, note, 1.0f);
            float text_x = cx + (cell_w - nw) * 0.5f;
            float text_y = py + 6;
            float bright = is_current ? 1.0f : 0.85f;
            d.draw_text(o, text_x, text_y, note, {bright, bright, bright, 1.0f}, 1.0f);

            // Chord abbreviation below in type color
            const char* chord = ni::kChordAbbr[chord_type];
            float cw2 = d.text_width(o, chord, 1.0f);
            float chord_x = cx + (cell_w - cw2) * 0.5f;
            float chord_y = text_y + ni::kLineH;
            const float* tc = ni::kTypeColors[chord_type];
            d.draw_text(o, chord_x, chord_y, chord,
                        {tc[0], tc[1], tc[2], is_current ? 1.0f : 0.7f}, 1.0f);

            // Colored bar at bottom
            float bar_h = 4.0f;
            float bar_y = py + h - bar_h - 2.0f;
            d.draw_rect(o, cx + 2, bar_y, cell_w - 4, bar_h,
                        {tc[0], tc[1], tc[2], is_current ? 0.9f : 0.6f});

            // Cell divider
            if (s > 0) {
                d.draw_rect(o, cx, py, 1, h,
                            {th.separator.r, th.separator.g, th.separator.b, 0.5f});
            }
        }

        ctx->consumed_height = 4.0f + h + 4.0f;
    }

    void draw_thumbnail(const VividThumbnailContext* ctx) override {
        // Param layout: steps=0, root_0..root_7=1..8, type_0..type_7=9..16,
        //               octave=17, beats_per_step=18, gate_length=19, velocity=20
        int num_steps = (ctx->param_count > 0) ? static_cast<int>(ctx->param_values[0]) : 4;
        num_steps = std::max(1, std::min(8, num_steps));
        int bps = (ctx->param_count > 18) ? static_cast<int>(ctx->param_values[18]) : 4;
        if (bps < 1) bps = 1;

        // Detect current step from first output note value
        // output[0] = root + octave*12 + intervals[0] = root + octave*12 (for major/minor root)
        int current_step = -1;
        if (ctx->output_count > 0) {
            float out_note = ctx->output_values[0];
            int oct = (ctx->param_count > 17) ? static_cast<int>(ctx->param_values[17]) : 4;
            for (int s = 0; s < num_steps; ++s) {
                int root = static_cast<int>(ctx->param_values[1 + s]);
                int chord_type = static_cast<int>(ctx->param_values[9 + s]);
                if (chord_type < 0) chord_type = 0;
                if (chord_type > 6) chord_type = 6;
                float expected = static_cast<float>(root + oct * 12 + kChordIntervals[chord_type][0]);
                if (std::fabs(out_note - expected) < 0.5f) {
                    current_step = s;
                    break;
                }
            }
        }

        // 7-color palette for chord types (RGBA8)
        static constexpr uint8_t kTypeColors[7][3] = {
            {100, 160, 220},  // Major  — blue
            {160, 100, 200},  // Minor  — purple
            {200, 100, 100},  // Dim    — red
            {220, 180, 80},   // Aug    — gold
            { 80, 180, 160},  // Dom7   — teal
            {140, 120, 200},  // Min7   — lavender
            { 80, 140, 220},  // Maj7   — sky blue
        };

        float w = static_cast<float>(ctx->width);
        float h = static_cast<float>(ctx->height);
        float pad = 4.0f;
        float plot_w = w - 2.0f * pad;
        float plot_h = h - 2.0f * pad;
        float col_w = plot_w / static_cast<float>(num_steps);

        // Background
        const uint8_t bg_r = 18, bg_g = 20, bg_b = 23, bg_a = 230;
        for (uint32_t y = 0; y < ctx->height; ++y) {
            uint8_t* row = ctx->pixels + y * ctx->stride;
            for (uint32_t x = 0; x < ctx->width; ++x) {
                uint8_t* px = row + x * 4;
                px[0] = bg_r; px[1] = bg_g; px[2] = bg_b; px[3] = bg_a;
            }
        }

        // Draw each step as a colored bar
        for (int s = 0; s < num_steps; ++s) {
            int root = static_cast<int>(ctx->param_values[1 + s]);
            int chord_type = static_cast<int>(ctx->param_values[9 + s]);
            root = std::max(0, std::min(11, root));
            chord_type = std::max(0, std::min(6, chord_type));

            // Bar height: root 0 = short, root 11 = tall
            float bar_frac = (static_cast<float>(root) + 1.0f) / 12.0f;
            float bar_h = bar_frac * (plot_h - 2.0f);
            float bar_x = pad + s * col_w + 1.0f;
            float bar_w_px = col_w - 2.0f;
            float bar_y = pad + plot_h - bar_h;

            uint8_t cr = kTypeColors[chord_type][0];
            uint8_t cg = kTypeColors[chord_type][1];
            uint8_t cb = kTypeColors[chord_type][2];
            bool is_current = (s == current_step);

            // Brighten current step
            if (is_current) {
                cr = static_cast<uint8_t>(std::min(255, cr + 60));
                cg = static_cast<uint8_t>(std::min(255, cg + 60));
                cb = static_cast<uint8_t>(std::min(255, cb + 60));
            }

            uint32_t ix0 = static_cast<uint32_t>(bar_x);
            uint32_t ix1 = static_cast<uint32_t>(bar_x + bar_w_px);
            uint32_t iy0 = static_cast<uint32_t>(bar_y);
            uint32_t iy1 = static_cast<uint32_t>(pad + plot_h);
            ix1 = std::min(ix1, ctx->width);
            iy1 = std::min(iy1, ctx->height);

            // Current step: dim background highlight
            if (is_current) {
                uint32_t col_x0 = static_cast<uint32_t>(pad + s * col_w);
                uint32_t col_x1 = std::min(static_cast<uint32_t>(pad + (s + 1) * col_w), ctx->width);
                uint32_t col_y0 = static_cast<uint32_t>(pad);
                uint32_t col_y1 = std::min(static_cast<uint32_t>(pad + plot_h), ctx->height);
                for (uint32_t y = col_y0; y < col_y1; ++y) {
                    uint8_t* row = ctx->pixels + y * ctx->stride;
                    for (uint32_t x = col_x0; x < col_x1; ++x) {
                        uint8_t* px = row + x * 4;
                        px[0] = 35; px[1] = 38; px[2] = 45; px[3] = 230;
                    }
                }
            }

            // Draw bar
            for (uint32_t y = iy0; y < iy1; ++y) {
                uint8_t* row = ctx->pixels + y * ctx->stride;
                for (uint32_t x = ix0; x < ix1; ++x) {
                    uint8_t* px = row + x * 4;
                    px[0] = cr; px[1] = cg; px[2] = cb; px[3] = 220;
                }
            }
        }

        // Baseline
        uint32_t base_y = std::min(static_cast<uint32_t>(pad + plot_h), ctx->height - 1);
        uint8_t* base_row = ctx->pixels + base_y * ctx->stride;
        for (uint32_t x = static_cast<uint32_t>(pad);
             x < std::min(static_cast<uint32_t>(pad + plot_w), ctx->width); ++x) {
            uint8_t* px = base_row + x * 4;
            px[0] = 80; px[1] = 85; px[2] = 95; px[3] = 200;
        }
    }
};

VIVID_REGISTER(NotePattern)
VIVID_THUMBNAIL(NotePattern)
VIVID_INSPECTOR(NotePattern)
