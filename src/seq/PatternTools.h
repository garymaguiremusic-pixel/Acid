#pragma once

#include "Step.h"
#include <cstdint>
#include <deque>
#include <vector>

namespace acid
{
namespace PatternTools
{
    enum class RandomMode { All = 0, Notes, Accents, Slides, Octaves };

    struct Scale { const char* name; std::vector<int> intervals; };
    const std::vector<Scale>& scales();          // index 0 = Chromatic
    const char* rootName (int root);              // "C", "C#" ...

    void randomise (Pattern&, RandomMode, int root, int scaleIndex, uint32_t seed);
    void mutate (Pattern&, int root, int scaleIndex, uint32_t seed, float amount = 0.15f);
    void rotate (Pattern&, int direction);         // +1 = right, -1 = left (within the length)
    void clear (Pattern&);
}

/** Undo / redo of pattern edits (snapshots of the edited pattern slot). */
class PatternUndoStack
{
public:
    struct Snapshot { int slot; Pattern pattern; };

    void push (int slot, const Pattern& before)
    {
        undo.push_back ({ slot, before });
        if (undo.size() > 100) undo.pop_front();
        redo.clear();
    }

    bool canUndo() const noexcept { return ! undo.empty(); }
    bool canRedo() const noexcept { return ! redo.empty(); }

    /** current = the slot's pattern right now (it goes onto the other stack). */
    bool undoInto (PatternBank& bank) { return move (undo, redo, bank); }
    bool redoInto (PatternBank& bank) { return move (redo, undo, bank); }
    int lastSlot() const noexcept { return lastMovedSlot; }

private:
    bool move (std::deque<Snapshot>& from, std::deque<Snapshot>& to, PatternBank& bank)
    {
        if (from.empty()) return false;
        auto s = from.back();
        from.pop_back();
        to.push_back ({ s.slot, bank[(size_t) s.slot] });
        bank[(size_t) s.slot] = s.pattern;
        lastMovedSlot = s.slot;
        return true;
    }

    std::deque<Snapshot> undo, redo;
    int lastMovedSlot = 0;
};
} // namespace acid
