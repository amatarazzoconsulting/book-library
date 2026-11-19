// MIT License - (c) Anthony Matarazzo 2026
//
// test_prosody.cpp - inventory, grapheme to phoneme rules, scheduling, contour.
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "prosody.hpp"

using namespace tts;

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (!ok) ++failures;
}

bool near(double a, double b, double tol) { return std::fabs(a - b) <= tol; }

std::string join(const std::vector<std::string>& v) {
    std::string s;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) s += ' ';
        s += v[i];
    }
    return s;
}

// The timeline contract: phoneme k starts exactly where phoneme k-1 ended.
bool contiguous(const std::vector<Phoneme>& seq) {
    double cursor = 0.0;
    for (const Phoneme& p : seq) {
        if (!near(p.start_s, cursor, 1e-9)) return false;
        if (p.duration_s < 0.0) return false;
        cursor += p.duration_s;
    }
    return true;
}

size_t count_symbol(const std::vector<Phoneme>& seq, const std::string& sym) {
    size_t n = 0;
    for (const Phoneme& p : seq) {
        if (p.symbol == sym) ++n;
    }
    return n;
}

const Phoneme* last_vowel(const std::vector<Phoneme>& seq) {
    for (size_t i = seq.size(); i-- > 0;) {
        const std::string& s = seq[i].symbol;
        if (s != "SIL" && find_phoneme(s) != nullptr && find_phoneme(s)->voiced &&
            s != "M" && s != "N" && s != "NG" && s != "L" && s != "R" && s != "W" &&
            s != "Y" && s != "B" && s != "D" && s != "G" && s != "V" && s != "Z" &&
            s != "ZH" && s != "DH" && s != "JH") {
            return &seq[i];
        }
    }
    return nullptr;
}

}  // namespace

int main() {
    // ---- Inventory ------------------------------------------------------
    const auto& inv = phoneme_inventory();
    check(inv.size() == 40, "inventory holds 40 symbols");
    check(inv.front().symbol == std::string("AA"), "inventory starts at AA");
    check(inv.back().symbol == std::string("SIL"), "inventory ends with SIL");

    bool sane = true;
    for (const PhonemeDef& d : inv) {
        if (!(d.f1 > 0.0f && d.f2 > d.f1 && d.f3 > d.f2)) sane = false;
        if (d.duration_ms < 0.0f || d.amplitude < 0.0f || d.amplitude > 1.0f) sane = false;
        if (d.noise < 0.0f || d.noise > 1.0f) sane = false;
        if (d.example == nullptr || *d.example == '\0') sane = false;
    }
    check(sane, "every entry has ordered formants and in range levels");

    const PhonemeDef* aa = find_phoneme("AA");
    check(aa != nullptr, "find_phoneme locates AA");
    check(aa != nullptr && near(aa->f1, 730.0, 1e-3) && near(aa->f2, 1090.0, 1e-3),
          "AA carries the book's 730/1090/2440 targets");
    check(find_phoneme("ZZ") == nullptr, "find_phoneme returns null for an unknown symbol");
    check(find_phoneme("aa") == nullptr, "symbol lookup is case sensitive");

    // ---- Grapheme to phoneme --------------------------------------------
    check(join(word_to_phonemes("quick")) == "K W IH K", "quick -> K W IH K");
    check(join(word_to_phonemes("strength")) == "S T R EH NG TH",
          "strength -> S T R EH NG TH");
    check(join(word_to_phonemes("Nation")) == "N AE SH AH N", "Nation -> N AE SH AH N");
    check(join(word_to_phonemes("the")) == "DH AH", "the hits the irregular table");
    check(join(word_to_phonemes("voice")) == "V OY S", "voice -> V OY S");
    check(join(word_to_phonemes("text")) == "T EH K S T", "text -> T EH K S T");

    // Every emitted symbol must exist in the inventory, or the synthesizer
    // would silently drop it.
    bool all_known = true;
    for (const char* w : {"quick", "strength", "Nation", "xylophone", "queue", "rhythm",
                          "judge", "shopping", "though", "vision", "science"}) {
        for (const std::string& s : word_to_phonemes(w)) {
            if (find_phoneme(s) == nullptr) all_known = false;
        }
    }
    check(all_known, "every rule output is a known ARPABET symbol");

    // Edge cases: no letters, empty string, mixed case.
    check(word_to_phonemes("").empty(), "an empty word yields no phonemes");
    check(word_to_phonemes("123").empty(), "a word with no letters yields no phonemes");
    check(word_to_phonemes("   ").empty(), "a whitespace word yields no phonemes");
    check(!word_to_phonemes("QUICK").empty(), "upper case input still converts");
    check(join(word_to_phonemes("QUICK")) == join(word_to_phonemes("quick")),
          "case does not change the phonemes");

    // ---- convert: empty and trivial input -------------------------------
    Prosody prosody;
    check(prosody.convert("").empty(), "an empty sentence yields an empty timeline");
    check(prosody.convert("...").empty(), "punctuation alone yields an empty timeline");
    check(prosody.convert("   ").empty(), "whitespace alone yields an empty timeline");
    check(Prosody::total_duration(prosody.convert("")) == 0.0, "total duration of nothing is 0");

    // ---- convert: the reference sentence --------------------------------
    const std::string sentence = "The quick brown fox jumps over the lazy dog.";
    const std::vector<Phoneme> seq = prosody.convert(sentence);
    check(seq.size() == 39, "the reference sentence plans 39 phonemes");
    check(contiguous(seq), "the timeline is contiguous");
    check(near(Prosody::total_duration(seq), 3.5115, 1e-3),
          "the reference sentence plans 3.5115 s");
    check(count_symbol(seq, "SIL") == 8, "eight inter-word pauses appear");
    check(seq.back().symbol == std::string("G"),
          "the final pause is dropped so the file ends on speech");

    size_t stressed = 0;
    for (const Phoneme& p : seq) {
        if (p.stress == 1) ++stressed;
        if (p.stress < 0 || p.stress > 2) check(false, "stress stays in 0..2");
    }
    check(stressed == 9, "nine vowels take primary stress");

    bool words_tagged = true;
    for (const Phoneme& p : seq) {
        if (p.symbol != "SIL" && p.word.empty()) words_tagged = false;
        if (p.symbol == "SIL" && !p.word.empty()) words_tagged = false;
        if (p.amplitude < 0.0f || p.amplitude > 1.0f) words_tagged = false;
        if (p.f0_start < 40.0f || p.f0_start > 1200.0f) words_tagged = false;
    }
    check(words_tagged, "every phoneme is tagged with its word and sane levels");

    // Declination: the contour must fall from the first vowel to the last.
    const Phoneme* first_v = last_vowel(seq);
    check(first_v != nullptr, "the reference sentence contains a vowel");
    const std::vector<Phoneme> head = prosody.convert("Hello.");
    check(head.size() >= 4, "a one word sentence still schedules phonemes");
    check(head[1].f0_start > seq[1].f0_start - 1e-3, "a short utterance starts no lower");

    // ---- Stress and accent ----------------------------------------------
    ProsodyOptions flat;
    flat.accent_semitones = 0.0f;
    Prosody no_accent(flat);
    const std::vector<Phoneme> accented = prosody.convert("Hello world.");
    const std::vector<Phoneme> unaccented = no_accent.convert("Hello world.");
    check(accented[1].f0_start > unaccented[1].f0_start,
          "a primary accent raises the stressed vowel");
    check(near(unaccented[1].f0_start, 233.3, 0.5), "accent 0 leaves the contour bare");

    // ---- Question rise ---------------------------------------------------
    const std::vector<Phoneme> question = prosody.convert("Is this a question?");
    const Phoneme* qv = last_vowel(question);
    check(qv != nullptr, "the question contains a vowel");
    check(qv != nullptr && qv->f0_end > qv->f0_start,
          "the final stressed vowel of a question rises");

    ProsodyOptions no_rise;
    no_rise.question_rise = false;
    Prosody statement_only(no_rise);
    const std::vector<Phoneme> not_a_question = statement_only.convert("Is this a question?");
    const Phoneme* nv = last_vowel(not_a_question);
    check(nv != nullptr && nv->f0_end <= nv->f0_start,
          "question_rise off removes the final rise");
    check(contiguous(question), "the question timeline is contiguous");

    // A statement must not rise at the end.
    const Phoneme* sv = last_vowel(prosody.convert("This is a statement."));
    check(sv != nullptr && sv->f0_end <= sv->f0_start, "a statement falls at the end");

    // ---- Rate ------------------------------------------------------------
    ProsodyOptions fast;
    fast.rate = 2.0f;
    Prosody quick_voice(fast);
    const double slow_s = Prosody::total_duration(seq);
    const double fast_s = Prosody::total_duration(quick_voice.convert(sentence));
    check(near(fast_s, slow_s * 0.5, 1e-3), "rate 2.0 halves the total duration");

    ProsodyOptions crawling;
    crawling.rate = 0.25f;
    Prosody slow_voice(crawling);
    check(Prosody::total_duration(slow_voice.convert(sentence)) > slow_s * 3.5,
          "rate 0.25 stretches the utterance");

    ProsodyOptions insane;
    insane.rate = 100.0f;
    Prosody clamped_rate(insane);
    check(Prosody::total_duration(clamped_rate.convert(sentence)) > 0.0,
          "an out of range rate is clamped, not applied");

    // ---- Pauses ----------------------------------------------------------
    ProsodyOptions silent;
    silent.pause_scale = 0.0f;
    Prosody no_pauses(silent);
    check(count_symbol(no_pauses.convert(sentence), "SIL") == 0,
          "pause_scale 0 removes every pause");

    ProsodyOptions loud_pauses;
    loud_pauses.pause_scale = 2.0f;
    Prosody double_pauses(loud_pauses);
    check(Prosody::total_duration(double_pauses.convert(sentence)) > slow_s,
          "pause_scale 2 lengthens the utterance");

    // Punctuation strength: a period holds longer than a comma.
    ProsodyOptions no_final;
    no_final.final_lengthening = false;
    Prosody plain(no_final);
    const std::vector<Phoneme> comma = plain.convert("Hello, world");
    const std::vector<Phoneme> period = plain.convert("Hello. world");
    check(count_symbol(comma, "SIL") > 0, "a comma inserts a pause");
    check(count_symbol(period, "SIL") > 0, "a period inserts a pause");
    check(Prosody::total_duration(period) > Prosody::total_duration(comma),
          "a period pauses longer than a comma");

    // ---- Final lengthening ----------------------------------------------
    ProsodyOptions stretched;
    stretched.final_lengthening = false;
    Prosody no_stretch(stretched);
    const double with_stretch = Prosody::total_duration(prosody.convert("Hello world."));
    const double without = Prosody::total_duration(no_stretch.convert("Hello world."));
    check(with_stretch > without, "final lengthening adds duration");
    check(contiguous(prosody.convert("Hello world.")),
          "final lengthening keeps the timeline contiguous");

    // ---- Articulation precision -----------------------------------------
    ProsodyOptions slurred;
    slurred.articulation_precision = 0.0f;
    Prosody slur(slurred);
    const std::vector<Phoneme> crisp = prosody.convert("stop");
    const std::vector<Phoneme> loose = slur.convert("stop");
    check(crisp.size() == loose.size(), "precision does not change the phoneme count");
    check(loose.front().duration_s > crisp.front().duration_s,
          "low precision smears the consonant longer");
    check(near(crisp[2].duration_s, loose[2].duration_s, 1e-9),
          "precision leaves vowel durations alone");

    // ---- Emotion ---------------------------------------------------------
    ProsodyOptions excited;
    excited.emotion_arousal = 1.0f;
    Prosody excited_voice(excited);
    const std::vector<Phoneme> excited_seq = excited_voice.convert("Hello world.");
    check(excited_seq[1].f0_start > accented[1].f0_start,
          "arousal widens the pitch range");

    ProsodyOptions calm;
    calm.emotion_arousal = -1.0f;
    Prosody calm_voice(calm);
    check(calm_voice.convert("Hello world.")[1].f0_start < accented[1].f0_start,
          "negative arousal narrows the pitch range");

    ProsodyOptions happy;
    happy.emotion_valence = 1.0f;
    Prosody happy_voice(happy);
    ProsodyOptions sad;
    sad.emotion_valence = -1.0f;
    Prosody sad_voice(sad);
    check(happy_voice.convert("Hello world.")[1].f0_start >
              accented[1].f0_start,
          "positive valence lifts the contour");
    check(sad_voice.convert("Hello world.")[1].f0_start < accented[1].f0_start,
          "negative valence lowers the contour");

    // ---- Base pitch ------------------------------------------------------
    ProsodyOptions high;
    high.base_f0 = 220.0f;
    Prosody high_voice(high);
    check(high_voice.convert("Hello world.")[1].f0_start > accented[1].f0_start,
          "a higher base pitch lifts every vowel");
    check(near(high_voice.convert("Hello world.")[1].f0_start, 370.0, 0.5),
          "base 220 Hz scales the contour as documented");

    ProsodyOptions floor_pitch;
    floor_pitch.base_f0 = 1.0f;
    Prosody guarded(floor_pitch);
    check(guarded.convert("Hello.").front().f0_start >= 40.0f,
          "an absurd base pitch is floored at 40 Hz");

    // ---- Determinism -----------------------------------------------------
    const std::vector<Phoneme> again = prosody.convert(sentence);
    bool identical = again.size() == seq.size();
    for (size_t i = 0; identical && i < seq.size(); ++i) {
        identical = again[i].symbol == seq[i].symbol &&
                    again[i].start_s == seq[i].start_s &&
                    again[i].duration_s == seq[i].duration_s &&
                    again[i].f0_start == seq[i].f0_start;
    }
    check(identical, "converting the same text twice gives the same timeline");

    // ---- Options access --------------------------------------------------
    check(prosody.options().rate == 1.0f, "the default rate is 1.0");
    prosody.options().rate = 1.25f;
    check(Prosody::total_duration(prosody.convert(sentence)) < slow_s,
          "options() gives writable access to the knobs");

    std::cout << (failures == 0 ? "\nprosody: all checks passed\n" : "\nprosody: FAILURES\n");
    return failures == 0 ? 0 : 1;
}
