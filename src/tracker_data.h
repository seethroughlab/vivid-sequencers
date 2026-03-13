#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <algorithm>

namespace tracker {

// Effect type IDs (MOD-style)
static constexpr uint8_t FX_NONE         = 0x00;
static constexpr uint8_t FX_ARPEGGIO     = 0x00;  // disambiguated: only when param != 0
static constexpr uint8_t FX_PORTA_UP     = 0x01;
static constexpr uint8_t FX_PORTA_DOWN   = 0x02;
static constexpr uint8_t FX_TONE_PORTA   = 0x03;
static constexpr uint8_t FX_VIBRATO      = 0x04;
static constexpr uint8_t FX_VOL_SLIDE    = 0x0A;
static constexpr uint8_t FX_SET_VOLUME   = 0x0C;
static constexpr uint8_t FX_PATTERN_BREAK= 0x0D;
static constexpr uint8_t FX_EXTENDED     = 0x0E;  // sub-commands in high nibble of param
static constexpr uint8_t FX_SET_SPEED    = 0x0F;

// Extended effect sub-commands (stored as effect_type = 0xE0 | sub)
static constexpr uint8_t FX_EXT_RETRIGGER = 0x09;
static constexpr uint8_t FX_EXT_NOTE_CUT  = 0x0C;
static constexpr uint8_t FX_EXT_NOTE_DELAY= 0x0D;

static constexpr uint8_t NOTE_EMPTY   = 0;
static constexpr uint8_t NOTE_OFF     = 255;
static constexpr int     MAX_CHANNELS = 8;
static constexpr int     MAX_ROWS     = 64;
static constexpr int     MAX_PATTERNS = 64;
static constexpr int     MAX_ARRANGEMENT = 128;

struct TrackerCell {
    uint8_t note;         // 0=empty, 1-127=MIDI note, 255=note-off
    uint8_t velocity;     // 0-127 (0 = use previous)
    uint8_t effect_type;  // see FX_ constants
    uint8_t effect_param; // XY nibbles
};

struct TrackerPattern {
    uint8_t num_rows;
    TrackerCell cells[MAX_CHANNELS][MAX_ROWS];
};

struct TrackerSong {
    uint8_t num_patterns;
    TrackerPattern patterns[MAX_PATTERNS];
    uint8_t arrangement[MAX_ARRANGEMENT];
    uint8_t arrangement_length;

    TrackerSong() {
        num_patterns = 1;
        arrangement_length = 1;
        arrangement[0] = 0;
        std::memset(arrangement + 1, 0, MAX_ARRANGEMENT - 1);
        patterns[0].num_rows = 16;
        std::memset(patterns[0].cells, 0, sizeof(patterns[0].cells));
        for (int i = 1; i < MAX_PATTERNS; ++i) {
            patterns[i].num_rows = 16;
            std::memset(patterns[i].cells, 0, sizeof(patterns[i].cells));
        }
    }
};

struct ChannelState {
    int8_t  prev_midi_note = -1;
    float   current_pitch = 0;
    float   target_pitch = 0;
    float   porta_speed = 0;
    float   vibrato_phase = 0;
    float   vibrato_depth = 0;
    float   vibrato_speed = 0;
    float   volume = 1.0f;
    float   volume_slide_delta = 0;
    int     retrigger_period = 0;
    int     retrigger_counter = 0;
    int     arpeggio_x = 0;
    int     arpeggio_y = 0;
    int     arpeggio_tick = 0;
    int     note_delay_ticks = 0;
    int     note_cut_ticks = 0;
    bool    gate_active = false;
    uint8_t current_velocity = 100;
    uint8_t last_note = 0;
};

// Base64 encoding/decoding for text param serialization
static constexpr char kBase64Chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

inline std::string base64_encode(const uint8_t* data, size_t len) {
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    for (size_t i = 0; i < len; i += 3) {
        uint32_t n = static_cast<uint32_t>(data[i]) << 16;
        if (i + 1 < len) n |= static_cast<uint32_t>(data[i + 1]) << 8;
        if (i + 2 < len) n |= static_cast<uint32_t>(data[i + 2]);
        out.push_back(kBase64Chars[(n >> 18) & 0x3F]);
        out.push_back(kBase64Chars[(n >> 12) & 0x3F]);
        out.push_back((i + 1 < len) ? kBase64Chars[(n >> 6) & 0x3F] : '=');
        out.push_back((i + 2 < len) ? kBase64Chars[n & 0x3F] : '=');
    }
    return out;
}

inline int base64_decode_char(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

inline std::string base64_decode(const std::string& encoded) {
    std::string out;
    out.reserve(encoded.size() * 3 / 4);
    uint32_t buf = 0;
    int bits = 0;
    for (char c : encoded) {
        if (c == '=' || c == '\n' || c == '\r') continue;
        int val = base64_decode_char(c);
        if (val < 0) continue;
        buf = (buf << 6) | static_cast<uint32_t>(val);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<char>((buf >> bits) & 0xFF));
        }
    }
    return out;
}

// Binary format:
// Header: [version(1), num_patterns(1), arrangement_length(1), reserved(1)]
// For each pattern (num_patterns): [num_rows(1), cells[8][num_rows] (4 bytes each)]
// Arrangement table: [arrangement_length bytes]

inline std::string serialize_song(const TrackerSong& song) {
    std::string bin;
    // Header
    bin.push_back(1);  // version
    bin.push_back(static_cast<char>(song.num_patterns));
    bin.push_back(static_cast<char>(song.arrangement_length));
    bin.push_back(0);  // reserved

    // Patterns
    for (int p = 0; p < song.num_patterns; ++p) {
        const auto& pat = song.patterns[p];
        bin.push_back(static_cast<char>(pat.num_rows));
        for (int ch = 0; ch < MAX_CHANNELS; ++ch) {
            for (int r = 0; r < pat.num_rows; ++r) {
                const auto& cell = pat.cells[ch][r];
                bin.push_back(static_cast<char>(cell.note));
                bin.push_back(static_cast<char>(cell.velocity));
                bin.push_back(static_cast<char>(cell.effect_type));
                bin.push_back(static_cast<char>(cell.effect_param));
            }
        }
    }

    // Arrangement
    for (int i = 0; i < song.arrangement_length; ++i)
        bin.push_back(static_cast<char>(song.arrangement[i]));

    return base64_encode(reinterpret_cast<const uint8_t*>(bin.data()), bin.size());
}

inline bool deserialize_song(const std::string& b64, TrackerSong& song) {
    if (b64.empty()) return false;

    std::string bin = base64_decode(b64);
    if (bin.size() < 4) return false;

    const uint8_t* d = reinterpret_cast<const uint8_t*>(bin.data());
    size_t pos = 0;

    uint8_t version = d[pos++];
    if (version != 1) return false;

    song.num_patterns = std::min(d[pos++], static_cast<uint8_t>(MAX_PATTERNS));
    song.arrangement_length = std::min(d[pos++], static_cast<uint8_t>(MAX_ARRANGEMENT));
    pos++;  // reserved

    for (int p = 0; p < song.num_patterns; ++p) {
        if (pos >= bin.size()) return false;
        song.patterns[p].num_rows = std::min(d[pos++], static_cast<uint8_t>(MAX_ROWS));
        int nr = song.patterns[p].num_rows;
        for (int ch = 0; ch < MAX_CHANNELS; ++ch) {
            for (int r = 0; r < nr; ++r) {
                if (pos + 3 >= bin.size()) return false;
                song.patterns[p].cells[ch][r].note         = d[pos++];
                song.patterns[p].cells[ch][r].velocity      = d[pos++];
                song.patterns[p].cells[ch][r].effect_type   = d[pos++];
                song.patterns[p].cells[ch][r].effect_param  = d[pos++];
            }
        }
    }

    for (int i = 0; i < song.arrangement_length; ++i) {
        if (pos >= bin.size()) return false;
        song.arrangement[i] = d[pos++];
    }

    return true;
}

// Note name helpers
inline const char* note_name(uint8_t note) {
    static const char* names[] = {"C-","C#","D-","D#","E-","F-","F#","G-","G#","A-","A#","B-"};
    if (note == NOTE_EMPTY) return "---";
    if (note == NOTE_OFF) return "===";
    if (note > 127) return "???";
    return names[note % 12];
}

inline int note_octave(uint8_t note) {
    if (note == NOTE_EMPTY || note == NOTE_OFF || note > 127) return -1;
    return (note / 12) - 1;
}

} // namespace tracker
