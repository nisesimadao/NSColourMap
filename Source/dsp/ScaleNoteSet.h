#pragma once

#include <array>
#include <cstdint>

namespace nscm
{
// Scale types supported by Scale Resonance Mode (spec §15.1).
enum class Scale
{
    major = 0,
    naturalMinor,
    harmonicMinor,
    dorian,
    phrygian,
    lydian,
    mixolydian,
    minorPentatonic,
    majorPentatonic,
    pentatonicBlues,
    wholeTone,
    chromatic,
    count
};

inline const char* getScaleName (Scale scale) noexcept
{
    switch (scale)
    {
        case Scale::major:           return "Major";
        case Scale::naturalMinor:    return "Natural Minor";
        case Scale::harmonicMinor:   return "Harmonic Minor";
        case Scale::dorian:          return "Dorian";
        case Scale::phrygian:        return "Phrygian";
        case Scale::lydian:          return "Lydian";
        case Scale::mixolydian:      return "Mixolydian";
        case Scale::minorPentatonic: return "Minor Pentatonic";
        case Scale::majorPentatonic: return "Major Pentatonic";
        case Scale::pentatonicBlues: return "Pentatonic Blues";
        case Scale::wholeTone:       return "Whole Tone";
        case Scale::chromatic:       return "Chromatic";
        case Scale::count:
        default:                     return "Major";
    }
}

// 12-bit pitch-class mask. Bit i set => pitch class i is part of the scale.
struct ScaleNoteSet
{
    std::uint16_t mask = 0;

    bool contains (int pitchClass) const noexcept
    {
        return (mask & (1u << (((pitchClass % 12) + 12) % 12))) != 0;
    }

    int count() const noexcept
    {
        int n = 0;
        for (int i = 0; i < 12; ++i)
            if (mask & (1u << i))
                ++n;
        return n;
    }
};

// Build a pitch-class set from key (0=C .. 11=B) and scale type.
inline ScaleNoteSet makeScaleNoteSet (int key, Scale scale) noexcept
{
    // Semitone offsets from the root for each scale.
    static constexpr std::array<int, 7> major           { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr std::array<int, 7> naturalMinor     { 0, 2, 3, 5, 7, 8, 10 };
    static constexpr std::array<int, 7> harmonicMinor    { 0, 2, 3, 5, 7, 8, 11 };
    static constexpr std::array<int, 7> dorian           { 0, 2, 3, 5, 7, 9, 10 };
    static constexpr std::array<int, 7> phrygian         { 0, 1, 3, 5, 7, 8, 10 };
    static constexpr std::array<int, 7> lydian           { 0, 2, 4, 6, 7, 9, 11 };
    static constexpr std::array<int, 7> mixolydian       { 0, 2, 4, 5, 7, 9, 10 };
    static constexpr std::array<int, 5> minorPentatonic  { 0, 3, 5, 7, 10 };
    static constexpr std::array<int, 5> majorPentatonic  { 0, 2, 4, 7, 9 };
    static constexpr std::array<int, 6> pentatonicBlues  { 0, 3, 5, 6, 7, 10 };
    static constexpr std::array<int, 6> wholeTone        { 0, 2, 4, 6, 8, 10 };
    static constexpr std::array<int, 12> chromatic       { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

    const int root = ((key % 12) + 12) % 12;
    ScaleNoteSet set;

    const auto add = [&] (auto const& degrees)
    {
        for (int d : degrees)
            set.mask |= static_cast<std::uint16_t> (1u << (((root + d) % 12 + 12) % 12));
    };

    switch (scale)
    {
        case Scale::major:           add (major);           break;
        case Scale::naturalMinor:    add (naturalMinor);    break;
        case Scale::harmonicMinor:   add (harmonicMinor);   break;
        case Scale::dorian:          add (dorian);          break;
        case Scale::phrygian:        add (phrygian);        break;
        case Scale::lydian:          add (lydian);          break;
        case Scale::mixolydian:      add (mixolydian);      break;
        case Scale::minorPentatonic: add (minorPentatonic); break;
        case Scale::majorPentatonic: add (majorPentatonic); break;
        case Scale::pentatonicBlues: add (pentatonicBlues); break;
        case Scale::wholeTone:       add (wholeTone);       break;
        case Scale::chromatic:       add (chromatic);       break;
        case Scale::count:
        default:                     add (major);           break;
    }

    return set;
}
} // namespace nscm
