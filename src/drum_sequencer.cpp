#include "operator_api/operator.h"
#include "operator_api/midi_types.h"
#include "operator_api/type_id.h"
#include <cmath>
#include <algorithm>
#include <cstring>

struct DrumSequencer : vivid::ControlOperatorBase {
    static constexpr const char* kName   = "DrumSequencer";
    static constexpr bool kTimeDependent = false;

    // Param index layout:
    // [0]=steps  [1]=swing
    // [2..7]=kick_note, snare_note, hat_note, oh_note, clap_note, tom_note
    // [8..23]=kick_0..15  [24..39]=snare_0..15  [40..55]=hat_0..15
    // [56..71]=oh_0..15   [72..87]=clap_0..15   [88..103]=tom_0..15
    // Mod A: [104..119]=kick_ma_0..15  [120..135]=snare_ma_0..15  [136..151]=hat_ma_0..15
    //        [152..167]=oh_ma_0..15    [168..183]=clap_ma_0..15   [184..199]=tom_ma_0..15
    // Mod B: [200..215]=kick_mb_0..15  [216..231]=snare_mb_0..15  [232..247]=hat_mb_0..15
    //        [248..263]=oh_mb_0..15    [264..279]=clap_mb_0..15   [280..295]=tom_mb_0..15

    vivid::Param<int>   steps {"steps",  16, 1, 16};
    vivid::Param<float> swing {"swing",  0.0f, 0.0f, 0.5f};

    // MIDI note number per drum track (indices 2..7)
    vivid::Param<int> kick_note  {"kick_note",  36, 0, 127};
    vivid::Param<int> snare_note {"snare_note", 38, 0, 127};
    vivid::Param<int> hat_note   {"hat_note",   42, 0, 127};
    vivid::Param<int> oh_note    {"oh_note",    46, 0, 127};
    vivid::Param<int> clap_note  {"clap_note",  39, 0, 127};
    vivid::Param<int> tom_note   {"tom_note",   45, 0, 127};

    // 6 drums x 16 steps = 96 bool params
    vivid::Param<float> kick_0 {"kick_0", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> kick_1 {"kick_1", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> kick_2 {"kick_2", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> kick_3 {"kick_3", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> kick_4 {"kick_4", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> kick_5 {"kick_5", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> kick_6 {"kick_6", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> kick_7 {"kick_7", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> kick_8 {"kick_8", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> kick_9 {"kick_9", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> kick_10{"kick_10",0.0f, 0.0f, 1.0f};
    vivid::Param<float> kick_11{"kick_11",0.0f, 0.0f, 1.0f};
    vivid::Param<float> kick_12{"kick_12",0.0f, 0.0f, 1.0f};
    vivid::Param<float> kick_13{"kick_13",0.0f, 0.0f, 1.0f};
    vivid::Param<float> kick_14{"kick_14",0.0f, 0.0f, 1.0f};
    vivid::Param<float> kick_15{"kick_15",0.0f, 0.0f, 1.0f};

    vivid::Param<float> snare_0 {"snare_0", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> snare_1 {"snare_1", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> snare_2 {"snare_2", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> snare_3 {"snare_3", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> snare_4 {"snare_4", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> snare_5 {"snare_5", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> snare_6 {"snare_6", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> snare_7 {"snare_7", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> snare_8 {"snare_8", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> snare_9 {"snare_9", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> snare_10{"snare_10",0.0f, 0.0f, 1.0f};
    vivid::Param<float> snare_11{"snare_11",0.0f, 0.0f, 1.0f};
    vivid::Param<float> snare_12{"snare_12",0.0f, 0.0f, 1.0f};
    vivid::Param<float> snare_13{"snare_13",0.0f, 0.0f, 1.0f};
    vivid::Param<float> snare_14{"snare_14",0.0f, 0.0f, 1.0f};
    vivid::Param<float> snare_15{"snare_15",0.0f, 0.0f, 1.0f};

    vivid::Param<float> hat_0 {"hat_0", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> hat_1 {"hat_1", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> hat_2 {"hat_2", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> hat_3 {"hat_3", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> hat_4 {"hat_4", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> hat_5 {"hat_5", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> hat_6 {"hat_6", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> hat_7 {"hat_7", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> hat_8 {"hat_8", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> hat_9 {"hat_9", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> hat_10{"hat_10",0.0f, 0.0f, 1.0f};
    vivid::Param<float> hat_11{"hat_11",0.0f, 0.0f, 1.0f};
    vivid::Param<float> hat_12{"hat_12",0.0f, 0.0f, 1.0f};
    vivid::Param<float> hat_13{"hat_13",0.0f, 0.0f, 1.0f};
    vivid::Param<float> hat_14{"hat_14",0.0f, 0.0f, 1.0f};
    vivid::Param<float> hat_15{"hat_15",0.0f, 0.0f, 1.0f};

    vivid::Param<float> oh_0 {"oh_0", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> oh_1 {"oh_1", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> oh_2 {"oh_2", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> oh_3 {"oh_3", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> oh_4 {"oh_4", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> oh_5 {"oh_5", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> oh_6 {"oh_6", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> oh_7 {"oh_7", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> oh_8 {"oh_8", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> oh_9 {"oh_9", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> oh_10{"oh_10",0.0f, 0.0f, 1.0f};
    vivid::Param<float> oh_11{"oh_11",0.0f, 0.0f, 1.0f};
    vivid::Param<float> oh_12{"oh_12",0.0f, 0.0f, 1.0f};
    vivid::Param<float> oh_13{"oh_13",0.0f, 0.0f, 1.0f};
    vivid::Param<float> oh_14{"oh_14",0.0f, 0.0f, 1.0f};
    vivid::Param<float> oh_15{"oh_15",0.0f, 0.0f, 1.0f};

    vivid::Param<float> clap_0 {"clap_0", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> clap_1 {"clap_1", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> clap_2 {"clap_2", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> clap_3 {"clap_3", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> clap_4 {"clap_4", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> clap_5 {"clap_5", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> clap_6 {"clap_6", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> clap_7 {"clap_7", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> clap_8 {"clap_8", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> clap_9 {"clap_9", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> clap_10{"clap_10",0.0f, 0.0f, 1.0f};
    vivid::Param<float> clap_11{"clap_11",0.0f, 0.0f, 1.0f};
    vivid::Param<float> clap_12{"clap_12",0.0f, 0.0f, 1.0f};
    vivid::Param<float> clap_13{"clap_13",0.0f, 0.0f, 1.0f};
    vivid::Param<float> clap_14{"clap_14",0.0f, 0.0f, 1.0f};
    vivid::Param<float> clap_15{"clap_15",0.0f, 0.0f, 1.0f};

    vivid::Param<float> tom_0 {"tom_0", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> tom_1 {"tom_1", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> tom_2 {"tom_2", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> tom_3 {"tom_3", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> tom_4 {"tom_4", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> tom_5 {"tom_5", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> tom_6 {"tom_6", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> tom_7 {"tom_7", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> tom_8 {"tom_8", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> tom_9 {"tom_9", 0.0f, 0.0f, 1.0f};
    vivid::Param<float> tom_10{"tom_10",0.0f, 0.0f, 1.0f};
    vivid::Param<float> tom_11{"tom_11",0.0f, 0.0f, 1.0f};
    vivid::Param<float> tom_12{"tom_12",0.0f, 0.0f, 1.0f};
    vivid::Param<float> tom_13{"tom_13",0.0f, 0.0f, 1.0f};
    vivid::Param<float> tom_14{"tom_14",0.0f, 0.0f, 1.0f};
    vivid::Param<float> tom_15{"tom_15",0.0f, 0.0f, 1.0f};

    // Mod A: 6 drums x 16 steps = 96 params (indices 98..193), default 0.5
    vivid::Param<float> kick_ma_0 {"kick_ma_0", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_ma_1 {"kick_ma_1", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_ma_2 {"kick_ma_2", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_ma_3 {"kick_ma_3", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_ma_4 {"kick_ma_4", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_ma_5 {"kick_ma_5", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_ma_6 {"kick_ma_6", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_ma_7 {"kick_ma_7", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_ma_8 {"kick_ma_8", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_ma_9 {"kick_ma_9", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_ma_10{"kick_ma_10",0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_ma_11{"kick_ma_11",0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_ma_12{"kick_ma_12",0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_ma_13{"kick_ma_13",0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_ma_14{"kick_ma_14",0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_ma_15{"kick_ma_15",0.5f, 0.0f, 1.0f};

    vivid::Param<float> snare_ma_0 {"snare_ma_0", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_ma_1 {"snare_ma_1", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_ma_2 {"snare_ma_2", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_ma_3 {"snare_ma_3", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_ma_4 {"snare_ma_4", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_ma_5 {"snare_ma_5", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_ma_6 {"snare_ma_6", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_ma_7 {"snare_ma_7", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_ma_8 {"snare_ma_8", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_ma_9 {"snare_ma_9", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_ma_10{"snare_ma_10",0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_ma_11{"snare_ma_11",0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_ma_12{"snare_ma_12",0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_ma_13{"snare_ma_13",0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_ma_14{"snare_ma_14",0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_ma_15{"snare_ma_15",0.5f, 0.0f, 1.0f};

    vivid::Param<float> hat_ma_0 {"hat_ma_0", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_ma_1 {"hat_ma_1", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_ma_2 {"hat_ma_2", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_ma_3 {"hat_ma_3", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_ma_4 {"hat_ma_4", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_ma_5 {"hat_ma_5", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_ma_6 {"hat_ma_6", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_ma_7 {"hat_ma_7", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_ma_8 {"hat_ma_8", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_ma_9 {"hat_ma_9", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_ma_10{"hat_ma_10",0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_ma_11{"hat_ma_11",0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_ma_12{"hat_ma_12",0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_ma_13{"hat_ma_13",0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_ma_14{"hat_ma_14",0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_ma_15{"hat_ma_15",0.5f, 0.0f, 1.0f};

    vivid::Param<float> oh_ma_0 {"oh_ma_0", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_ma_1 {"oh_ma_1", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_ma_2 {"oh_ma_2", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_ma_3 {"oh_ma_3", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_ma_4 {"oh_ma_4", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_ma_5 {"oh_ma_5", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_ma_6 {"oh_ma_6", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_ma_7 {"oh_ma_7", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_ma_8 {"oh_ma_8", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_ma_9 {"oh_ma_9", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_ma_10{"oh_ma_10",0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_ma_11{"oh_ma_11",0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_ma_12{"oh_ma_12",0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_ma_13{"oh_ma_13",0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_ma_14{"oh_ma_14",0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_ma_15{"oh_ma_15",0.5f, 0.0f, 1.0f};

    vivid::Param<float> clap_ma_0 {"clap_ma_0", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_ma_1 {"clap_ma_1", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_ma_2 {"clap_ma_2", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_ma_3 {"clap_ma_3", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_ma_4 {"clap_ma_4", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_ma_5 {"clap_ma_5", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_ma_6 {"clap_ma_6", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_ma_7 {"clap_ma_7", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_ma_8 {"clap_ma_8", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_ma_9 {"clap_ma_9", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_ma_10{"clap_ma_10",0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_ma_11{"clap_ma_11",0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_ma_12{"clap_ma_12",0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_ma_13{"clap_ma_13",0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_ma_14{"clap_ma_14",0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_ma_15{"clap_ma_15",0.5f, 0.0f, 1.0f};

    vivid::Param<float> tom_ma_0 {"tom_ma_0", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_ma_1 {"tom_ma_1", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_ma_2 {"tom_ma_2", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_ma_3 {"tom_ma_3", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_ma_4 {"tom_ma_4", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_ma_5 {"tom_ma_5", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_ma_6 {"tom_ma_6", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_ma_7 {"tom_ma_7", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_ma_8 {"tom_ma_8", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_ma_9 {"tom_ma_9", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_ma_10{"tom_ma_10",0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_ma_11{"tom_ma_11",0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_ma_12{"tom_ma_12",0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_ma_13{"tom_ma_13",0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_ma_14{"tom_ma_14",0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_ma_15{"tom_ma_15",0.5f, 0.0f, 1.0f};

    // Mod B: 6 drums x 16 steps = 96 params (indices 194..289), default 0.5
    vivid::Param<float> kick_mb_0 {"kick_mb_0", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_mb_1 {"kick_mb_1", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_mb_2 {"kick_mb_2", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_mb_3 {"kick_mb_3", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_mb_4 {"kick_mb_4", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_mb_5 {"kick_mb_5", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_mb_6 {"kick_mb_6", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_mb_7 {"kick_mb_7", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_mb_8 {"kick_mb_8", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_mb_9 {"kick_mb_9", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_mb_10{"kick_mb_10",0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_mb_11{"kick_mb_11",0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_mb_12{"kick_mb_12",0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_mb_13{"kick_mb_13",0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_mb_14{"kick_mb_14",0.5f, 0.0f, 1.0f};
    vivid::Param<float> kick_mb_15{"kick_mb_15",0.5f, 0.0f, 1.0f};

    vivid::Param<float> snare_mb_0 {"snare_mb_0", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_mb_1 {"snare_mb_1", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_mb_2 {"snare_mb_2", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_mb_3 {"snare_mb_3", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_mb_4 {"snare_mb_4", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_mb_5 {"snare_mb_5", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_mb_6 {"snare_mb_6", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_mb_7 {"snare_mb_7", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_mb_8 {"snare_mb_8", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_mb_9 {"snare_mb_9", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_mb_10{"snare_mb_10",0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_mb_11{"snare_mb_11",0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_mb_12{"snare_mb_12",0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_mb_13{"snare_mb_13",0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_mb_14{"snare_mb_14",0.5f, 0.0f, 1.0f};
    vivid::Param<float> snare_mb_15{"snare_mb_15",0.5f, 0.0f, 1.0f};

    vivid::Param<float> hat_mb_0 {"hat_mb_0", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_mb_1 {"hat_mb_1", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_mb_2 {"hat_mb_2", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_mb_3 {"hat_mb_3", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_mb_4 {"hat_mb_4", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_mb_5 {"hat_mb_5", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_mb_6 {"hat_mb_6", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_mb_7 {"hat_mb_7", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_mb_8 {"hat_mb_8", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_mb_9 {"hat_mb_9", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_mb_10{"hat_mb_10",0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_mb_11{"hat_mb_11",0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_mb_12{"hat_mb_12",0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_mb_13{"hat_mb_13",0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_mb_14{"hat_mb_14",0.5f, 0.0f, 1.0f};
    vivid::Param<float> hat_mb_15{"hat_mb_15",0.5f, 0.0f, 1.0f};

    vivid::Param<float> oh_mb_0 {"oh_mb_0", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_mb_1 {"oh_mb_1", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_mb_2 {"oh_mb_2", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_mb_3 {"oh_mb_3", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_mb_4 {"oh_mb_4", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_mb_5 {"oh_mb_5", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_mb_6 {"oh_mb_6", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_mb_7 {"oh_mb_7", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_mb_8 {"oh_mb_8", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_mb_9 {"oh_mb_9", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_mb_10{"oh_mb_10",0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_mb_11{"oh_mb_11",0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_mb_12{"oh_mb_12",0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_mb_13{"oh_mb_13",0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_mb_14{"oh_mb_14",0.5f, 0.0f, 1.0f};
    vivid::Param<float> oh_mb_15{"oh_mb_15",0.5f, 0.0f, 1.0f};

    vivid::Param<float> clap_mb_0 {"clap_mb_0", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_mb_1 {"clap_mb_1", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_mb_2 {"clap_mb_2", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_mb_3 {"clap_mb_3", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_mb_4 {"clap_mb_4", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_mb_5 {"clap_mb_5", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_mb_6 {"clap_mb_6", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_mb_7 {"clap_mb_7", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_mb_8 {"clap_mb_8", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_mb_9 {"clap_mb_9", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_mb_10{"clap_mb_10",0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_mb_11{"clap_mb_11",0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_mb_12{"clap_mb_12",0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_mb_13{"clap_mb_13",0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_mb_14{"clap_mb_14",0.5f, 0.0f, 1.0f};
    vivid::Param<float> clap_mb_15{"clap_mb_15",0.5f, 0.0f, 1.0f};

    vivid::Param<float> tom_mb_0 {"tom_mb_0", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_mb_1 {"tom_mb_1", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_mb_2 {"tom_mb_2", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_mb_3 {"tom_mb_3", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_mb_4 {"tom_mb_4", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_mb_5 {"tom_mb_5", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_mb_6 {"tom_mb_6", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_mb_7 {"tom_mb_7", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_mb_8 {"tom_mb_8", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_mb_9 {"tom_mb_9", 0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_mb_10{"tom_mb_10",0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_mb_11{"tom_mb_11",0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_mb_12{"tom_mb_12",0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_mb_13{"tom_mb_13",0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_mb_14{"tom_mb_14",0.5f, 0.0f, 1.0f};
    vivid::Param<float> tom_mb_15{"tom_mb_15",0.5f, 0.0f, 1.0f};

    void collect_params(std::vector<vivid::ParamBase*>& out) override {
        out.push_back(&steps);   // 0
        out.push_back(&swing);   // 1

        // Note params: 2..7
        out.push_back(&kick_note);
        out.push_back(&snare_note);
        out.push_back(&hat_note);
        out.push_back(&oh_note);
        out.push_back(&clap_note);
        out.push_back(&tom_note);

        // kick: 8..23
        out.push_back(&kick_0);  out.push_back(&kick_1);
        out.push_back(&kick_2);  out.push_back(&kick_3);
        out.push_back(&kick_4);  out.push_back(&kick_5);
        out.push_back(&kick_6);  out.push_back(&kick_7);
        out.push_back(&kick_8);  out.push_back(&kick_9);
        out.push_back(&kick_10); out.push_back(&kick_11);
        out.push_back(&kick_12); out.push_back(&kick_13);
        out.push_back(&kick_14); out.push_back(&kick_15);

        // snare: 24..39
        out.push_back(&snare_0);  out.push_back(&snare_1);
        out.push_back(&snare_2);  out.push_back(&snare_3);
        out.push_back(&snare_4);  out.push_back(&snare_5);
        out.push_back(&snare_6);  out.push_back(&snare_7);
        out.push_back(&snare_8);  out.push_back(&snare_9);
        out.push_back(&snare_10); out.push_back(&snare_11);
        out.push_back(&snare_12); out.push_back(&snare_13);
        out.push_back(&snare_14); out.push_back(&snare_15);

        // hat: 40..55
        out.push_back(&hat_0);  out.push_back(&hat_1);
        out.push_back(&hat_2);  out.push_back(&hat_3);
        out.push_back(&hat_4);  out.push_back(&hat_5);
        out.push_back(&hat_6);  out.push_back(&hat_7);
        out.push_back(&hat_8);  out.push_back(&hat_9);
        out.push_back(&hat_10); out.push_back(&hat_11);
        out.push_back(&hat_12); out.push_back(&hat_13);
        out.push_back(&hat_14); out.push_back(&hat_15);

        // oh: 56..71
        out.push_back(&oh_0);  out.push_back(&oh_1);
        out.push_back(&oh_2);  out.push_back(&oh_3);
        out.push_back(&oh_4);  out.push_back(&oh_5);
        out.push_back(&oh_6);  out.push_back(&oh_7);
        out.push_back(&oh_8);  out.push_back(&oh_9);
        out.push_back(&oh_10); out.push_back(&oh_11);
        out.push_back(&oh_12); out.push_back(&oh_13);
        out.push_back(&oh_14); out.push_back(&oh_15);

        // clap: 72..87
        out.push_back(&clap_0);  out.push_back(&clap_1);
        out.push_back(&clap_2);  out.push_back(&clap_3);
        out.push_back(&clap_4);  out.push_back(&clap_5);
        out.push_back(&clap_6);  out.push_back(&clap_7);
        out.push_back(&clap_8);  out.push_back(&clap_9);
        out.push_back(&clap_10); out.push_back(&clap_11);
        out.push_back(&clap_12); out.push_back(&clap_13);
        out.push_back(&clap_14); out.push_back(&clap_15);

        // tom: 88..103
        out.push_back(&tom_0);  out.push_back(&tom_1);
        out.push_back(&tom_2);  out.push_back(&tom_3);
        out.push_back(&tom_4);  out.push_back(&tom_5);
        out.push_back(&tom_6);  out.push_back(&tom_7);
        out.push_back(&tom_8);  out.push_back(&tom_9);
        out.push_back(&tom_10); out.push_back(&tom_11);
        out.push_back(&tom_12); out.push_back(&tom_13);
        out.push_back(&tom_14); out.push_back(&tom_15);

        // Mod A kick: 104..119
        out.push_back(&kick_ma_0);  out.push_back(&kick_ma_1);
        out.push_back(&kick_ma_2);  out.push_back(&kick_ma_3);
        out.push_back(&kick_ma_4);  out.push_back(&kick_ma_5);
        out.push_back(&kick_ma_6);  out.push_back(&kick_ma_7);
        out.push_back(&kick_ma_8);  out.push_back(&kick_ma_9);
        out.push_back(&kick_ma_10); out.push_back(&kick_ma_11);
        out.push_back(&kick_ma_12); out.push_back(&kick_ma_13);
        out.push_back(&kick_ma_14); out.push_back(&kick_ma_15);

        // Mod A snare: 120..135
        out.push_back(&snare_ma_0);  out.push_back(&snare_ma_1);
        out.push_back(&snare_ma_2);  out.push_back(&snare_ma_3);
        out.push_back(&snare_ma_4);  out.push_back(&snare_ma_5);
        out.push_back(&snare_ma_6);  out.push_back(&snare_ma_7);
        out.push_back(&snare_ma_8);  out.push_back(&snare_ma_9);
        out.push_back(&snare_ma_10); out.push_back(&snare_ma_11);
        out.push_back(&snare_ma_12); out.push_back(&snare_ma_13);
        out.push_back(&snare_ma_14); out.push_back(&snare_ma_15);

        // Mod A hat: 136..151
        out.push_back(&hat_ma_0);  out.push_back(&hat_ma_1);
        out.push_back(&hat_ma_2);  out.push_back(&hat_ma_3);
        out.push_back(&hat_ma_4);  out.push_back(&hat_ma_5);
        out.push_back(&hat_ma_6);  out.push_back(&hat_ma_7);
        out.push_back(&hat_ma_8);  out.push_back(&hat_ma_9);
        out.push_back(&hat_ma_10); out.push_back(&hat_ma_11);
        out.push_back(&hat_ma_12); out.push_back(&hat_ma_13);
        out.push_back(&hat_ma_14); out.push_back(&hat_ma_15);

        // Mod A oh: 152..167
        out.push_back(&oh_ma_0);  out.push_back(&oh_ma_1);
        out.push_back(&oh_ma_2);  out.push_back(&oh_ma_3);
        out.push_back(&oh_ma_4);  out.push_back(&oh_ma_5);
        out.push_back(&oh_ma_6);  out.push_back(&oh_ma_7);
        out.push_back(&oh_ma_8);  out.push_back(&oh_ma_9);
        out.push_back(&oh_ma_10); out.push_back(&oh_ma_11);
        out.push_back(&oh_ma_12); out.push_back(&oh_ma_13);
        out.push_back(&oh_ma_14); out.push_back(&oh_ma_15);

        // Mod A clap: 168..183
        out.push_back(&clap_ma_0);  out.push_back(&clap_ma_1);
        out.push_back(&clap_ma_2);  out.push_back(&clap_ma_3);
        out.push_back(&clap_ma_4);  out.push_back(&clap_ma_5);
        out.push_back(&clap_ma_6);  out.push_back(&clap_ma_7);
        out.push_back(&clap_ma_8);  out.push_back(&clap_ma_9);
        out.push_back(&clap_ma_10); out.push_back(&clap_ma_11);
        out.push_back(&clap_ma_12); out.push_back(&clap_ma_13);
        out.push_back(&clap_ma_14); out.push_back(&clap_ma_15);

        // Mod A tom: 184..199
        out.push_back(&tom_ma_0);  out.push_back(&tom_ma_1);
        out.push_back(&tom_ma_2);  out.push_back(&tom_ma_3);
        out.push_back(&tom_ma_4);  out.push_back(&tom_ma_5);
        out.push_back(&tom_ma_6);  out.push_back(&tom_ma_7);
        out.push_back(&tom_ma_8);  out.push_back(&tom_ma_9);
        out.push_back(&tom_ma_10); out.push_back(&tom_ma_11);
        out.push_back(&tom_ma_12); out.push_back(&tom_ma_13);
        out.push_back(&tom_ma_14); out.push_back(&tom_ma_15);

        // Mod B kick: 200..215
        out.push_back(&kick_mb_0);  out.push_back(&kick_mb_1);
        out.push_back(&kick_mb_2);  out.push_back(&kick_mb_3);
        out.push_back(&kick_mb_4);  out.push_back(&kick_mb_5);
        out.push_back(&kick_mb_6);  out.push_back(&kick_mb_7);
        out.push_back(&kick_mb_8);  out.push_back(&kick_mb_9);
        out.push_back(&kick_mb_10); out.push_back(&kick_mb_11);
        out.push_back(&kick_mb_12); out.push_back(&kick_mb_13);
        out.push_back(&kick_mb_14); out.push_back(&kick_mb_15);

        // Mod B snare: 216..231
        out.push_back(&snare_mb_0);  out.push_back(&snare_mb_1);
        out.push_back(&snare_mb_2);  out.push_back(&snare_mb_3);
        out.push_back(&snare_mb_4);  out.push_back(&snare_mb_5);
        out.push_back(&snare_mb_6);  out.push_back(&snare_mb_7);
        out.push_back(&snare_mb_8);  out.push_back(&snare_mb_9);
        out.push_back(&snare_mb_10); out.push_back(&snare_mb_11);
        out.push_back(&snare_mb_12); out.push_back(&snare_mb_13);
        out.push_back(&snare_mb_14); out.push_back(&snare_mb_15);

        // Mod B hat: 232..247
        out.push_back(&hat_mb_0);  out.push_back(&hat_mb_1);
        out.push_back(&hat_mb_2);  out.push_back(&hat_mb_3);
        out.push_back(&hat_mb_4);  out.push_back(&hat_mb_5);
        out.push_back(&hat_mb_6);  out.push_back(&hat_mb_7);
        out.push_back(&hat_mb_8);  out.push_back(&hat_mb_9);
        out.push_back(&hat_mb_10); out.push_back(&hat_mb_11);
        out.push_back(&hat_mb_12); out.push_back(&hat_mb_13);
        out.push_back(&hat_mb_14); out.push_back(&hat_mb_15);

        // Mod B oh: 248..263
        out.push_back(&oh_mb_0);  out.push_back(&oh_mb_1);
        out.push_back(&oh_mb_2);  out.push_back(&oh_mb_3);
        out.push_back(&oh_mb_4);  out.push_back(&oh_mb_5);
        out.push_back(&oh_mb_6);  out.push_back(&oh_mb_7);
        out.push_back(&oh_mb_8);  out.push_back(&oh_mb_9);
        out.push_back(&oh_mb_10); out.push_back(&oh_mb_11);
        out.push_back(&oh_mb_12); out.push_back(&oh_mb_13);
        out.push_back(&oh_mb_14); out.push_back(&oh_mb_15);

        // Mod B clap: 264..279
        out.push_back(&clap_mb_0);  out.push_back(&clap_mb_1);
        out.push_back(&clap_mb_2);  out.push_back(&clap_mb_3);
        out.push_back(&clap_mb_4);  out.push_back(&clap_mb_5);
        out.push_back(&clap_mb_6);  out.push_back(&clap_mb_7);
        out.push_back(&clap_mb_8);  out.push_back(&clap_mb_9);
        out.push_back(&clap_mb_10); out.push_back(&clap_mb_11);
        out.push_back(&clap_mb_12); out.push_back(&clap_mb_13);
        out.push_back(&clap_mb_14); out.push_back(&clap_mb_15);

        // Mod B tom: 280..295
        out.push_back(&tom_mb_0);  out.push_back(&tom_mb_1);
        out.push_back(&tom_mb_2);  out.push_back(&tom_mb_3);
        out.push_back(&tom_mb_4);  out.push_back(&tom_mb_5);
        out.push_back(&tom_mb_6);  out.push_back(&tom_mb_7);
        out.push_back(&tom_mb_8);  out.push_back(&tom_mb_9);
        out.push_back(&tom_mb_10); out.push_back(&tom_mb_11);
        out.push_back(&tom_mb_12); out.push_back(&tom_mb_13);
        out.push_back(&tom_mb_14); out.push_back(&tom_mb_15);
    }

    void collect_ports(std::vector<VividPortDescriptor>& out) override {
        out.push_back({"phase",   VIVID_PORT_FLOAT, VIVID_PORT_INPUT});
        out.push_back({"reset",   VIVID_PORT_FLOAT, VIVID_PORT_INPUT});
        out.push_back({"kick",    VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        out.push_back({"snare",   VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        out.push_back({"hat",     VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        out.push_back({"oh",      VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        out.push_back({"clap",    VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        out.push_back({"tom",     VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        out.push_back({"step",    VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        // Mod A outputs (ports 7..12 after 2 inputs)
        out.push_back({"kick_mod_a",  VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        out.push_back({"snare_mod_a", VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        out.push_back({"hat_mod_a",   VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        out.push_back({"oh_mod_a",    VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        out.push_back({"clap_mod_a",  VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        out.push_back({"tom_mod_a",   VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        // Mod B outputs (ports 13..18)
        out.push_back({"kick_mod_b",  VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        out.push_back({"snare_mod_b", VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        out.push_back({"hat_mod_b",   VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        out.push_back({"oh_mod_b",    VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        out.push_back({"clap_mod_b",  VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        out.push_back({"tom_mod_b",   VIVID_PORT_FLOAT, VIVID_PORT_OUTPUT});
        // Spread outputs for SP404 integration
        out.push_back({"gates",      VIVID_PORT_SPREAD, VIVID_PORT_OUTPUT});
        out.push_back({"notes",      VIVID_PORT_SPREAD, VIVID_PORT_OUTPUT});
        out.push_back({"velocities", VIVID_PORT_SPREAD, VIVID_PORT_OUTPUT});
        // MIDI handle output
        out.push_back(VIVID_HANDLE_PORT("midi_out", VIVID_PORT_OUTPUT, VividMidiBuffer));
    }

    void process(const VividProcessContext* ctx) override {
        float phase = ctx->input_values[0];
        bool reset = ctx->input_values[1] > 0.5f;

        // Rising-edge reset: capture current phase as offset
        if (reset && !prev_reset_)
            phase_offset_ = phase;
        prev_reset_ = reset;

        float adj_phase = std::fmod(phase - phase_offset_ + 1.0f, 1.0f);
        int n = std::max(steps.int_value(), 1);
        int step = static_cast<int>(adj_phase * n);
        step = std::clamp(step, 0, n - 1);

        bool step_changed = (step != prev_step_);
        prev_step_ = step;

        // Drum base param indices (shifted +6 for note params)
        static constexpr int kDrumBase[6] = { 8, 24, 40, 56, 72, 88 };
        static constexpr int kNoteBase = 2; // 6 note params at indices 2..7

        for (int d = 0; d < 6; ++d) {
            bool active = (step_changed && ctx->param_values[kDrumBase[d] + step] > 0.5f);
            ctx->output_values[d] = active ? 1.0f : 0.0f;
        }
        ctx->output_values[6] = static_cast<float>(step);

        // Per-step modulation outputs (continuous — emitted every frame)
        static constexpr int kModABase[6] = { 104, 120, 136, 152, 168, 184 };
        static constexpr int kModBBase[6] = { 200, 216, 232, 248, 264, 280 };

        for (int d = 0; d < 6; ++d) {
            ctx->output_values[7 + d]  = ctx->param_values[kModABase[d] + step];
            ctx->output_values[13 + d] = ctx->param_values[kModBBase[d] + step];
        }

        // Pack all 6 drum tracks into fixed-length spread outputs.
        // Fixed 6-element spreads keep slot-to-drum mapping stable so
        // the SP404's GateTracker can detect per-drum rising/falling edges.
        // Spread port indices = 19,20,21 (after 19 float output ports).
        if (ctx->output_spreads) {
            auto& gates_sp = ctx->output_spreads[19];
            auto& notes_sp = ctx->output_spreads[20];
            auto& vels_sp  = ctx->output_spreads[21];

            constexpr int kNumDrums = 6;
            if (gates_sp.capacity >= kNumDrums) {
                gates_sp.length = kNumDrums;
                notes_sp.length = kNumDrums;
                vels_sp.length  = kNumDrums;
                for (int d = 0; d < kNumDrums; ++d) {
                    gates_sp.data[d] = ctx->output_values[d];
                    notes_sp.data[d] = static_cast<float>(ctx->param_values[kNoteBase + d]);
                    vels_sp.data[d]  = ctx->param_values[kModABase[d] + step];
                }
            }
        }

        // Populate MIDI handle output with note-on messages for active drums
        midi_buf_.count = 0;
        for (int d = 0; d < 6; ++d) {
            bool active = (step_changed && ctx->param_values[kDrumBase[d] + step] > 0.5f);
            if (active && midi_buf_.count < VIVID_MIDI_BUFFER_CAPACITY) {
                VividMidiMessage& msg = midi_buf_.messages[midi_buf_.count++];
                msg.status = 0x90;  // note-on, channel 1
                msg.data1 = static_cast<uint8_t>(ctx->param_values[kNoteBase + d]);
                msg.data2 = 127;    // full velocity
                msg.reserved = 0;
                msg.frame_offset_samples = 0;
            }
        }
        if (ctx->output_handles && ctx->output_handle_count > 0) {
            ctx->output_handles[0] = &midi_buf_;
        }
    }

    void draw_thumbnail(const VividThumbnailContext* ctx) override {
        float w = static_cast<float>(ctx->width);
        float h = static_cast<float>(ctx->height);

        int n = 16;
        if (ctx->param_count > 0)
            n = std::max(1, std::min(16, static_cast<int>(ctx->param_values[0])));

        // Current step from output[6]
        int cur_step = -1;
        if (ctx->output_count > 6)
            cur_step = static_cast<int>(ctx->output_values[6]);

        // Colors per drum row (RGBA 0-255)
        static constexpr uint8_t kDrumColors[6][3] = {
            {220, 80, 80},    // kick — red
            {220, 190, 60},   // snare — gold
            {60, 200, 180},   // hat — teal
            {80, 130, 220},   // oh — blue
            {160, 90, 200},   // clap — purple
            {80, 200, 100},   // tom — green
        };

        static constexpr int kDrumBase[6] = { 8, 24, 40, 56, 72, 88 };

        float pad_x = 2.0f, pad_y = 2.0f;
        float grid_w = w - 2.0f * pad_x;
        float grid_h = h - 2.0f * pad_y;
        float cell_w = grid_w / 16.0f;
        float cell_h = grid_h / 6.0f;
        float dot_inset = 1.5f;

        // Clear to dark background
        for (uint32_t y = 0; y < ctx->height; ++y) {
            uint8_t* row = ctx->pixels + y * ctx->stride;
            for (uint32_t x = 0; x < ctx->width; ++x) {
                uint8_t* px = row + x * 4;
                px[0] = 18; px[1] = 20; px[2] = 23; px[3] = 230;
            }
        }

        // Draw active cells and current step highlight
        for (int drum = 0; drum < 6; ++drum) {
            for (int s = 0; s < 16; ++s) {
                float cx = pad_x + s * cell_w;
                float cy = pad_y + drum * cell_h;

                // Current step column highlight
                if (s == cur_step && s < n) {
                    int x0 = static_cast<int>(cx);
                    int x1 = static_cast<int>(cx + cell_w);
                    int y0 = static_cast<int>(cy);
                    int y1 = static_cast<int>(cy + cell_h);
                    x0 = std::max(0, x0); x1 = std::min(static_cast<int>(ctx->width), x1);
                    y0 = std::max(0, y0); y1 = std::min(static_cast<int>(ctx->height), y1);
                    for (int py2 = y0; py2 < y1; ++py2) {
                        uint8_t* row = ctx->pixels + py2 * ctx->stride;
                        for (int px2 = x0; px2 < x1; ++px2) {
                            uint8_t* p = row + px2 * 4;
                            // Brighten existing pixel
                            p[0] = std::min(255, p[0] + 25);
                            p[1] = std::min(255, p[1] + 30);
                            p[2] = std::min(255, p[2] + 35);
                        }
                    }
                }

                // Active cell dot
                bool active = false;
                if (ctx->param_count > static_cast<uint32_t>(kDrumBase[drum] + s))
                    active = ctx->param_values[kDrumBase[drum] + s] > 0.5f;

                if (active) {
                    int x0 = static_cast<int>(cx + dot_inset);
                    int x1 = static_cast<int>(cx + cell_w - dot_inset);
                    int y0 = static_cast<int>(cy + dot_inset);
                    int y1 = static_cast<int>(cy + cell_h - dot_inset);
                    x0 = std::max(0, x0); x1 = std::min(static_cast<int>(ctx->width), x1);
                    y0 = std::max(0, y0); y1 = std::min(static_cast<int>(ctx->height), y1);

                    uint8_t alpha = (s < n) ? 230 : 60;
                    for (int py2 = y0; py2 < y1; ++py2) {
                        uint8_t* row = ctx->pixels + py2 * ctx->stride;
                        for (int px2 = x0; px2 < x1; ++px2) {
                            uint8_t* p = row + px2 * 4;
                            p[0] = kDrumColors[drum][0];
                            p[1] = kDrumColors[drum][1];
                            p[2] = kDrumColors[drum][2];
                            p[3] = alpha;
                        }
                    }
                }
            }
        }
    }

private:
    int prev_step_ = -1;
    float phase_offset_ = 0.0f;
    bool prev_reset_ = false;
    VividMidiBuffer midi_buf_ = {};
};

VIVID_REGISTER(DrumSequencer)
VIVID_THUMBNAIL(DrumSequencer)
