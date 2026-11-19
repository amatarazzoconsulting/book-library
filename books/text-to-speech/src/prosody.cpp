// MIT License - (c) Anthony Matarazzo 2026
#include "prosody.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>

namespace tts {

namespace {

// ---------------------------------------------------------------------------
// ARPABET inventory. Formant values follow the vowel chart quoted in the book
// (/a/ 730/1090/2440, /i/ 270/2290/3010, /u/ 300/870/2240) with the remaining
// vowels and consonants filled in from the same source.
// ---------------------------------------------------------------------------
const std::vector<PhonemeDef>& inventory_storage() {
    static const std::vector<PhonemeDef> inv = {
        {"AA", 730.f, 1090.f, 2440.f, true, 120.f, 1.00f, 0.00f, "father"},
        {"AE", 660.f, 1720.f, 2410.f, true, 110.f, 0.95f, 0.00f, "cat"},
        {"AH", 640.f, 1190.f, 2390.f, true, 85.f, 0.80f, 0.00f, "up"},
        {"AO", 570.f, 840.f, 2410.f, true, 115.f, 0.95f, 0.00f, "off"},
        {"AW", 640.f, 1230.f, 2400.f, true, 140.f, 1.00f, 0.00f, "now"},
        {"AY", 660.f, 1200.f, 2550.f, true, 145.f, 1.00f, 0.00f, "ice"},
        {"B", 350.f, 1100.f, 2200.f, true, 65.f, 0.55f, 0.10f, "bed"},
        {"CH", 400.f, 1900.f, 2600.f, false, 85.f, 0.55f, 0.85f, "chair"},
        {"D", 350.f, 1700.f, 2600.f, true, 60.f, 0.55f, 0.10f, "do"},
        {"DH", 350.f, 1600.f, 2500.f, true, 55.f, 0.45f, 0.20f, "then"},
        {"EH", 580.f, 1970.f, 2500.f, true, 100.f, 0.90f, 0.00f, "egg"},
        {"ER", 490.f, 1350.f, 1690.f, true, 110.f, 0.90f, 0.00f, "her"},
        {"EY", 500.f, 2100.f, 2650.f, true, 145.f, 1.00f, 0.00f, "ate"},
        {"F", 400.f, 1400.f, 2400.f, false, 85.f, 0.40f, 0.90f, "food"},
        {"G", 300.f, 1990.f, 2600.f, true, 65.f, 0.55f, 0.15f, "go"},
        {"HH", 500.f, 1500.f, 2500.f, false, 60.f, 0.35f, 0.95f, "hello"},
        {"IH", 400.f, 1990.f, 2550.f, true, 75.f, 0.80f, 0.00f, "it"},
        {"IY", 270.f, 2290.f, 3010.f, true, 95.f, 0.90f, 0.00f, "eel"},
        {"JH", 400.f, 1900.f, 2600.f, true, 80.f, 0.55f, 0.70f, "jump"},
        {"K", 300.f, 1900.f, 2600.f, false, 75.f, 0.55f, 0.85f, "cat"},
        {"L", 380.f, 1300.f, 2700.f, true, 70.f, 0.60f, 0.05f, "leg"},
        {"M", 280.f, 1000.f, 2200.f, true, 80.f, 0.55f, 0.05f, "me"},
        {"N", 280.f, 1400.f, 2500.f, true, 70.f, 0.55f, 0.05f, "no"},
        {"NG", 280.f, 1900.f, 2500.f, true, 80.f, 0.55f, 0.05f, "sing"},
        {"OW", 480.f, 880.f, 2500.f, true, 150.f, 1.00f, 0.00f, "over"},
        {"OY", 560.f, 850.f, 2400.f, true, 155.f, 1.00f, 0.00f, "oil"},
        {"P", 350.f, 1100.f, 2200.f, false, 75.f, 0.50f, 0.90f, "pen"},
        {"R", 420.f, 1300.f, 1600.f, true, 70.f, 0.65f, 0.05f, "red"},
        {"S", 350.f, 4200.f, 5500.f, false, 95.f, 0.50f, 1.00f, "see"},
        {"SH", 350.f, 2500.f, 4000.f, false, 100.f, 0.50f, 1.00f, "she"},
        {"T", 350.f, 1800.f, 2700.f, false, 70.f, 0.50f, 0.90f, "two"},
        {"TH", 350.f, 1600.f, 2500.f, false, 85.f, 0.40f, 0.95f, "thin"},
        {"UH", 440.f, 1020.f, 2240.f, true, 75.f, 0.75f, 0.00f, "book"},
        {"UW", 300.f, 870.f, 2240.f, true, 110.f, 0.90f, 0.00f, "too"},
        {"V", 350.f, 1200.f, 2300.f, true, 60.f, 0.50f, 0.40f, "very"},
        {"W", 300.f, 700.f, 2200.f, true, 65.f, 0.60f, 0.05f, "we"},
        {"Y", 280.f, 2200.f, 3000.f, true, 60.f, 0.60f, 0.05f, "yes"},
        {"Z", 350.f, 4000.f, 5200.f, true, 80.f, 0.50f, 0.60f, "zoo"},
        {"ZH", 350.f, 2400.f, 3900.f, true, 85.f, 0.50f, 0.60f, "vision"},
        {"SIL", 500.f, 1500.f, 2500.f, false, 0.f, 0.f, 0.f, "silence"},
    };
    return inv;
}

bool is_vowel_symbol(const std::string& s) {
    static const char* vowels[] = {"AA", "AE", "AH", "AO", "AW", "AY", "EH",
                                   "ER", "EY", "IH", "IY", "OW", "OY", "UH", "UW"};
    for (const char* v : vowels) {
        if (s == v) return true;
    }
    return false;
}

bool is_letter(char c) { return std::isalpha(static_cast<unsigned char>(c)) != 0; }

char lower(char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }

bool starts_with(const std::string& w, size_t pos, const char* s) {
    const size_t n = std::char_traits<char>::length(s);
    return w.compare(pos, n, s) == 0;
}

}  // namespace

const std::vector<PhonemeDef>& phoneme_inventory() { return inventory_storage(); }

const PhonemeDef* find_phoneme(const std::string& symbol) {
    for (const PhonemeDef& p : inventory_storage()) {
        if (symbol == p.symbol) return &p;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Rule based G2P. Ordered context-sensitive rules, longest match first. This is
// deliberately a small hand written rule set, not a trained model: the book's
// point is that prosody and voice are what make it sound alive.
// ---------------------------------------------------------------------------
std::vector<std::string> word_to_phonemes(const std::string& word) {
    std::string w;
    for (char c : word) {
        if (is_letter(c)) w.push_back(lower(c));
    }
    std::vector<std::string> out;
    if (w.empty()) return out;

    // Irregular words that the letter rules would mangle.
    static const std::vector<std::pair<const char*, const char*>> irregulars = {
        {"one", "W AH N"},     {"two", "T UW"},        {"three", "TH R IY"},
        {"four", "F AO R"},    {"eight", "EY T"},      {"the", "DH AH"},
        {"a", "AH"},           {"of", "AH V"},         {"to", "T UW"},
        {"you", "Y UW"},       {"are", "AA R"},        {"was", "W AH Z"},
        {"have", "HH AE V"},   {"said", "S EH D"},     {"who", "HH UW"},
        {"what", "W AH T"},    {"where", "W EH R"},    {"there", "DH EH R"},
        {"here", "HH IY R"},   {"hello", "HH AH L OW"},{"world", "W ER L D"},
        {"voice", "V OY S"},   {"speech", "S P IY CH"},{"text", "T EH K S T"},
        {"sound", "S AW N D"}, {"please", "P L IY Z"}, {"thank", "TH AE NG K"},
        {"through", "TH R UW"},{"science", "S AY AH N S"},
    };
    for (const auto& [key, phones] : irregulars) {
        if (w == key) {
            std::string token;
            for (const char* p = phones; *p; ++p) {
                if (*p == ' ') {
                    if (!token.empty()) out.push_back(token);
                    token.clear();
                } else {
                    token.push_back(*p);
                }
            }
            if (!token.empty()) out.push_back(token);
            return out;
        }
    }

    const size_t n = w.size();
    for (size_t i = 0; i < n;) {
        const char c = w[i];
        const char next = (i + 1 < n) ? w[i + 1] : '\0';
        const char prev = (i > 0) ? w[i - 1] : '\0';
        const bool final = (i + 1 >= n);

        // Vowel digraphs and r-coloured vowels.
        if (starts_with(w, i, "ough")) {
            out.push_back("AO");
            i += 4;
            continue;
        }
        if (starts_with(w, i, "eigh")) {
            out.push_back("EY");
            i += 4;
            continue;
        }
        if (starts_with(w, i, "tion")) {
            out.push_back("SH");
            out.push_back("AH");
            out.push_back("N");
            i += 4;
            continue;
        }
        if (starts_with(w, i, "sion")) {
            out.push_back("ZH");
            out.push_back("AH");
            out.push_back("N");
            i += 4;
            continue;
        }
        if (starts_with(w, i, "tch") || starts_with(w, i, "dge")) {
            out.push_back(w[i] == 't' ? "CH" : "JH");
            i += 3;
            continue;
        }
        if (starts_with(w, i, "air") || starts_with(w, i, "ear") || starts_with(w, i, "ere")) {
            out.push_back("EH");
            out.push_back("R");
            i += 3;
            continue;
        }
        if (starts_with(w, i, "ing") && i > 0) {
            out.push_back("IH");
            out.push_back("NG");
            i += 3;
            continue;
        }
        if (starts_with(w, i, "ee") || starts_with(w, i, "ea")) {
            out.push_back("IY");
            i += 2;
            continue;
        }
        if (starts_with(w, i, "oo")) {
            out.push_back("UW");
            i += 2;
            continue;
        }
        if (starts_with(w, i, "ou") || starts_with(w, i, "ow")) {
            out.push_back("AW");
            i += 2;
            continue;
        }
        if (starts_with(w, i, "oi") || starts_with(w, i, "oy")) {
            out.push_back("OY");
            i += 2;
            continue;
        }
        if (starts_with(w, i, "ai") || starts_with(w, i, "ay")) {
            out.push_back("EY");
            i += 2;
            continue;
        }
        if (starts_with(w, i, "au") || starts_with(w, i, "aw")) {
            out.push_back("AO");
            i += 2;
            continue;
        }
        if (starts_with(w, i, "ie")) {
            out.push_back("IY");
            i += 2;
            continue;
        }
        if (starts_with(w, i, "ar")) {
            out.push_back("AA");
            out.push_back("R");
            i += 2;
            continue;
        }
        if (starts_with(w, i, "or")) {
            out.push_back("AO");
            out.push_back("R");
            i += 2;
            continue;
        }
        if (starts_with(w, i, "er") || starts_with(w, i, "ir") || starts_with(w, i, "ur")) {
            out.push_back("ER");
            i += 2;
            continue;
        }
        if (starts_with(w, i, "qu")) {
            out.push_back("K");
            out.push_back("W");
            i += 2;
            continue;
        }
        if (starts_with(w, i, "ch") || starts_with(w, i, "sh")) {
            out.push_back(w[i] == 'c' ? "CH" : "SH");
            i += 2;
            continue;
        }
        if (starts_with(w, i, "th")) {
            out.push_back(prev == '\0' && (next == 'e' || next == 'a' || next == 'o' ||
                                           next == 'i' || next == 'u')
                             ? "DH"
                             : "TH");
            i += 2;
            continue;
        }
        if (starts_with(w, i, "ph")) {
            out.push_back("F");
            i += 2;
            continue;
        }
        if (starts_with(w, i, "wh")) {
            out.push_back("W");
            i += 2;
            continue;
        }
        if (starts_with(w, i, "ck") || starts_with(w, i, "nk")) {
            if (w[i] == 'n') out.push_back("NG");
            out.push_back("K");
            i += 2;
            continue;
        }
        if (starts_with(w, i, "ng")) {
            out.push_back("NG");
            i += 2;
            continue;
        }
        if (next == c && c != 'e' && c != 'o') {
            // Doubled consonant collapses to one sound.
            ++i;
            continue;
        }

        switch (c) {
            case 'a':
                // "a_e" silent-e pattern gives the long vowel.
                out.push_back(final ? "AH" : "AE");
                break;
            case 'e':
                if (final && i > 1) { ++i; continue; }  // silent final e
                out.push_back("EH");
                break;
            case 'i':
                out.push_back(final ? "AY" : "IH");
                break;
            case 'o':
                out.push_back(final ? "OW" : "AA");
                break;
            case 'u':
                out.push_back("AH");
                break;
            case 'y':
                out.push_back(i == 0 ? "Y" : (final ? "IY" : "IH"));
                break;
            case 'b': out.push_back("B"); break;
            case 'c':
                out.push_back((next == 'e' || next == 'i' || next == 'y') ? "S" : "K");
                break;
            case 'd': out.push_back("D"); break;
            case 'f': out.push_back("F"); break;
            case 'g': out.push_back((next == 'e' || next == 'i' || next == 'y') ? "JH" : "G"); break;
            case 'h': out.push_back("HH"); break;
            case 'j': out.push_back("JH"); break;
            case 'k': out.push_back("K"); break;
            case 'l': out.push_back("L"); break;
            case 'm': out.push_back("M"); break;
            case 'n': out.push_back("N"); break;
            case 'p': out.push_back("P"); break;
            case 'q': out.push_back("K"); break;
            case 'r': out.push_back("R"); break;
            case 's':
                // 's' is voiced only when it sits between two vowels, as in
                // "vision" or "easy"; everywhere else it stays voiceless.
                out.push_back((prev != '\0' && std::string("aeiou").find(prev) != std::string::npos &&
                               std::string("aeiou").find(next) != std::string::npos)
                                  ? "Z"
                                  : "S");
                break;
            case 't': out.push_back("T"); break;
            case 'v': out.push_back("V"); break;
            case 'w': out.push_back("W"); break;
            case 'x': out.push_back("K"); out.push_back("S"); break;
            case 'z': out.push_back("Z"); break;
            default: break;
        }
        ++i;
    }
    return out;
}

// ---------------------------------------------------------------------------
// Prosody
// ---------------------------------------------------------------------------
double Prosody::total_duration(const std::vector<Phoneme>& seq) {
    double total = 0.0;
    for (const Phoneme& p : seq) total += p.duration_s;
    return total;
}

std::vector<Phoneme> Prosody::convert(const std::string& text) const {
    const ProsodyOptions& o = opt_;

    // ---- 1. Tokenize into words and punctuation marks -------------------
    struct Token {
        std::string text;
        bool is_punct;
        char punct;
    };
    std::vector<Token> tokens;
    std::string current;
    for (char c : text) {
        if (is_letter(c) || c == '\'') {
            current.push_back(c);
        } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            if (!current.empty()) {
                tokens.push_back({current, false, 0});
                current.clear();
            }
        } else if (c == ',' || c == ';' || c == ':' || c == '.' || c == '!' || c == '?') {
            if (!current.empty()) {
                tokens.push_back({current, false, 0});
                current.clear();
            }
            tokens.push_back({std::string(1, c), true, c});
        }
    }
    if (!current.empty()) tokens.push_back({current, false, 0});

    std::vector<Phoneme> seq;
    if (tokens.empty()) return seq;

    // ---- 2. Grapheme to phoneme, with a fallback symbol ------------------
    struct Raw {
        std::vector<std::string> phones;
        std::string word;
        char trailing_punct = 0;
        bool starts_sentence = false;
    };
    std::vector<Raw> words;
    bool sentence_start = true;
    char pending_punct = 0;
    for (const Token& t : tokens) {
        if (t.is_punct) {
            pending_punct = t.punct;
            continue;
        }
        Raw r;
        r.word = t.text;
        r.phones = word_to_phonemes(t.text);
        if (r.phones.empty()) continue;
        r.trailing_punct = pending_punct;
        r.starts_sentence = sentence_start;
        pending_punct = 0;
        sentence_start = false;
        words.push_back(std::move(r));
    }
    if (words.empty()) return seq;

    // ---- 3. Pitch contour: declination across the utterance -------------
    // f0 falls roughly one octave from the first stressed vowel to the last.
    // Arousal widens the range, sadness narrows it. Valence shifts the whole
    // contour: a happy voice sits above the neutral centre, a sad one below.
    const float range_st = o.pitch_range_semitones * (1.0f + 0.35f * o.emotion_arousal);
    const float valence_st = 1.5f * o.emotion_valence;
    const float top_st = range_st * 0.5f + valence_st;
    const float bottom_st = -range_st * 0.5f + valence_st;
    const float base = std::max(40.0f, o.base_f0);
    auto st_to_hz = [base](float st) { return base * std::pow(2.0f, st / 12.0f); };

    // The utterance ends in a question if the last token carried '?'.
    const bool is_question = o.question_rise && pending_punct == '?';

    size_t vowel_count = 0;
    for (const Raw& r : words) {
        for (const std::string& p : r.phones) {
            if (is_vowel_symbol(p)) ++vowel_count;
        }
    }
    if (vowel_count == 0) vowel_count = 1;

    // ---- 4. Duration and amplitude --------------------------------------
    const float rate = std::min(4.0f, std::max(0.25f, o.rate));
    double cursor = 0.0;
    size_t vowel_index = 0;
    size_t word_index = 0;

    for (const Raw& r : words) {
        const size_t phones_in_word = r.phones.size();
        // Stress: the first vowel of a word with two or more syllables takes
        // primary stress, later vowels take secondary stress.
        size_t vowel_ordinal = 0;
        for (size_t k = 0; k < phones_in_word; ++k) {
            const std::string& sym = r.phones[k];
            const PhonemeDef* def = find_phoneme(sym);
            if (!def) continue;

            Phoneme ph;
            ph.symbol = sym;
            ph.word = r.word;
            ph.voiced = def->voiced;
            ph.noise = def->noise;
            ph.f1 = def->f1;
            ph.f2 = def->f2;
            ph.f3 = def->f3;
            ph.amplitude = def->amplitude;
            ph.stress = 0;

            const bool vowel = is_vowel_symbol(sym);
            float dur_ms = def->duration_ms;

            if (vowel) {
                if (phones_in_word == 1 || vowel_ordinal == 0) {
                    ph.stress = 1;
                } else {
                    ph.stress = 2;
                }
                ++vowel_ordinal;
                if (ph.stress == 1) dur_ms *= 1.35f;
                else if (ph.stress == 2) dur_ms *= 1.12f;
            } else {
                // Low articulation precision smears consonants together.
                const float smear = 1.0f + (1.0f - std::min(1.0f, std::max(0.0f,
                                                  o.articulation_precision))) * 0.25f;
                dur_ms *= smear;
            }

            ph.duration_s = (dur_ms / 1000.0) / rate;

            // Pitch: linear position through the utterance plus accent.
            const float pos = (vowel_count > 1)
                                  ? static_cast<float>(vowel_index) /
                                        static_cast<float>(vowel_count - 1)
                                  : 0.0f;
            float start_st = top_st + (bottom_st - top_st) * pos;
            float end_st = start_st;

            if (vowel) {
                if (ph.stress == 1) start_st += o.accent_semitones;
                // A question lifts its final stressed vowel.
                if (is_question && vowel_index + 1 == vowel_count) {
                    start_st += 3.0f;
                    end_st = start_st + 5.0f;
                } else if (is_question) {
                    end_st = start_st - 0.15f;
                }
                ++vowel_index;
            }
            ph.f0_start = st_to_hz(start_st);
            ph.f0_end = st_to_hz(end_st);
            ph.start_s = cursor;
            cursor += ph.duration_s;
            seq.push_back(ph);
        }

        // ---- 5. Pauses and final lengthening ----------------------------
        const bool last_word = (word_index + 1 == words.size());
        if (last_word && o.final_lengthening) {
            // Stretch the final vowel by 40 percent, the way a speaker does
            // before handing the turn over.
            for (size_t k = seq.size(); k-- > 0;) {
                if (!seq[k].word.empty() && seq[k].word == r.word &&
                    is_vowel_symbol(seq[k].symbol)) {
                    const double extra = seq[k].duration_s * 0.4;
                    seq[k].duration_s += extra;
                    // Every later phoneme moves back by the same amount, so the
                    // timeline stays contiguous and nothing overlaps.
                    for (size_t m = k + 1; m < seq.size(); ++m) seq[m].start_s += extra;
                    cursor += extra;
                    break;
                }
            }
        }

        float pause_s = 0.0f;
        switch (r.trailing_punct) {
            case ',': pause_s = 0.18f; break;
            case ';': pause_s = 0.22f; break;
            case ':': pause_s = 0.24f; break;
            case '.': pause_s = 0.35f; break;
            case '!': pause_s = 0.35f; break;
            case '?': pause_s = 0.35f; break;
            default: pause_s = last_word ? 0.0f : 0.06f; break;
        }
        pause_s *= std::max(0.0f, o.pause_scale);
        if (pause_s > 0.0f) {
            Phoneme sil;
            sil.symbol = "SIL";
            sil.word = "";
            sil.voiced = false;
            sil.amplitude = 0.0f;
            sil.noise = 0.0f;
            sil.duration_s = pause_s / rate;
            sil.start_s = cursor;
            sil.f0_start = sil.f0_end = base;
            cursor += sil.duration_s;
            seq.push_back(sil);
        }
        ++word_index;
    }

    // Drop a trailing silence: the renderer already pads the file.
    while (!seq.empty() && seq.back().symbol == "SIL") seq.pop_back();
    return seq;
}

}  // namespace tts
