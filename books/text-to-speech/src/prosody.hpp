// MIT License - (c) Anthony Matarazzo 2026
//
// prosody.hpp - text to phoneme conversion with prosody.
//
// Covers the phoneme processing layer of the book (chapter 2.1): grapheme to
// phoneme rules, an ARPABET inventory with formant targets (chapter 12.1),
// lexical stress, a declination pitch contour with question rise, and
// duration rules including final lengthening and punctuation pauses.
//
// The output is a flat, sample-independent timeline. Nothing here touches
// audio, so it can be tested on its own.
#pragma once

#include <string>
#include <vector>

namespace tts {

// One entry of the ARPABET inventory plus the acoustic data the synthesizer
// needs to render it.
struct PhonemeDef {
    const char* symbol;   // ARPABET, upper case
    float f1, f2, f3;     // formant targets in Hz
    bool voiced;          // periodic excitation present
    float duration_ms;    // neutral duration at rate 1.0
    float amplitude;      // relative loudness 0..1
    float noise;          // turbulence mixed into the source 0..1
    const char* example;  // word illustrating the sound
};

// A scheduled phoneme: when it sounds, how long, at what pitch.
struct Phoneme {
    std::string symbol = "SIL";
    std::string word;      // source word, empty for pauses
    double start_s = 0.0;
    double duration_s = 0.0;
    float f0_start = 165.0f;
    float f0_end = 165.0f;
    float amplitude = 1.0f;
    float f1 = 500.0f, f2 = 1500.0f, f3 = 2500.0f;
    bool voiced = true;
    float noise = 0.0f;
    int stress = 0;        // 0 none, 1 primary, 2 secondary

    double end_s() const { return start_s + duration_s; }
};

// Knobs for the whole conversion. Defaults match the Neutral preset.
struct ProsodyOptions {
    float base_f0 = 165.0f;          // Hz, centre of the pitch range
    float rate = 1.0f;               // 1.0 normal, 2.0 twice as fast
    float pitch_range_semitones = 12.0f;  // total declination span
    float accent_semitones = 3.0f;   // extra pitch on stressed vowels
    float articulation_precision = 0.95f;  // >0.5 reduces consonant smearing
    float emotion_arousal = 0.0f;    // -1 calm .. +1 excited
    float emotion_valence = 0.0f;    // -1 sad .. +1 happy
    float pause_scale = 1.0f;        // multiplies punctuation pauses
    bool question_rise = true;       // rising contour when text ends with '?'
    bool final_lengthening = true;   // stretch the last syllable
};

// The full inventory, in the order used by the appendix.
const std::vector<PhonemeDef>& phoneme_inventory();
// Null when the symbol is unknown.
const PhonemeDef* find_phoneme(const std::string& symbol);

// Rule based grapheme to phoneme for one word, lower or upper case, letters
// only. Diphthongs are expanded into two targets. An unknown character is
// skipped; a word with no letters yields an empty vector.
std::vector<std::string> word_to_phonemes(const std::string& word);

class Prosody {
public:
    Prosody() = default;
    explicit Prosody(const ProsodyOptions& options) : opt_(options) {}

    ProsodyOptions& options() { return opt_; }
    const ProsodyOptions& options() const { return opt_; }

    // Convert a sentence or paragraph into a scheduled phoneme timeline.
    // Words are separated by spaces; ',' ';' ':' '.' '!' '?' produce pauses.
    std::vector<Phoneme> convert(const std::string& text) const;

    // Convenience: sum of durations.
    static double total_duration(const std::vector<Phoneme>& seq);

private:
    ProsodyOptions opt_{};
};

}  // namespace tts
