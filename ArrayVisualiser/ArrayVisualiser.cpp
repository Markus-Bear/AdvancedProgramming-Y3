// Test.cpp
// Array Visualiser — 3x2 grid layout, TEMP swap, slowed animations
//
// Grid:
//   [ Pseudocode ] [    Array    ] [  Temp  ]
//   [  Message   ] [ Controls/UI ] [ (empty)]
//
// - Window is resizable.
// - Regions are separated by the grid; no overlap.
// - TEMP never overlaps the array.
// - Array starts empty.
// - Insert / Delete-by-value / Traverse / Search / Bubble Sort with animations.

#include "raylib.h"
#include <string>
#include <vector>
#include <queue>
#include <cmath>
#include <limits>
#include <climits>
#include <algorithm>

using std::string;
using std::vector;

static const int ARRAY_CAPACITY = 10;
const int INITIAL_WINDOW_WIDTH = 1700;
const int INITIAL_WINDOW_HEIGHT = 950;

// Base pseudocode line height
static const int PSEUDOCODE_LINE_HEIGHT = 26;

// Global sentinel for empty slots
const int EMPTY = std::numeric_limits<int>::min();

// Slot colours (for readability only)
const Color COLOR_SLOT_EMPTY = LIGHTGRAY;
const Color COLOR_SLOT_HIGHLIGHT = GOLD;
const Color COLOR_SLOT_SELECTED = SKYBLUE;
const Color COLOR_SLOT_SEARCH = GREEN;
const Color COLOR_SLOT_DELETE = RED;

// ---------- Action system ----------
enum ActionType {
    HIGHLIGHT_SLOT,
    UNHIGHLIGHT_SLOT,
    CLEAR_SELECTION,
    MARK_DELETE_SLOT,
    CLEAR_DELETE_SLOT,
    MARK_SEARCH_FOUND,
    CLEAR_SEARCH_FOUND,
    START_MOVE,
    START_MOVE_PAIR,
    WRITE_VALUE,
    CLEAR_VALUE,
    PSEUDOCODE_LINE,
    DISPLAY_MESSAGE,
    WAIT
};

struct Action {
    ActionType type;
    int slotIndex;
    int targetIndex;
    int value;
    int value2;
    int pseudocodeLine;
    float duration;
    std::string message;
    Action(ActionType t = DISPLAY_MESSAGE,
        int s = -1,
        int tgt = -1,
        int v = EMPTY,
        int v2 = EMPTY,
        int pc = 0,
        float d = 0.0f,
        const std::string& m = "")
        : type(t),
        slotIndex(s),
        targetIndex(tgt),
        value(v),
        value2(v2),
        pseudocodeLine(pc),
        duration(d),
        message(m) {
    }
};

std::queue<Action> actionQueue;

// ---------- Array + animation state ----------
std::vector<int> arrayData(ARRAY_CAPACITY, EMPTY);
std::vector<bool> slotHighlighted(ARRAY_CAPACITY, false);
std::vector<bool> slotSelected(ARRAY_CAPACITY, false);

int elementCount = 0;

std::vector<bool> slotSearchFound(ARRAY_CAPACITY, false);
std::vector<bool> slotDeleteMarked(ARRAY_CAPACITY, false);

struct AnimatedMover {
    bool active = false;
    int value = 0;
    Vector2 startPos = { 0,0 };
    Vector2 endPos = { 0,0 };
    float duration = 0.0f;
    float elapsed = 0.0f;
    int fromIndex = -1;
    int toIndex = -1;
};
AnimatedMover animatedMovers[2];

const int TEMP_ID = -999;

// TEMP storage
int  tempValue = EMPTY;
bool tempOccupied = false;

// WAIT state
bool  waitActive = false;
float waitRemaining = 0.0f;

// ---------- Which pseudocode to show ----------
enum AlgorithmKind {
    ALG_NONE,
    ALG_INSERT,
    ALG_DELETE,
    ALG_SEARCH,
    ALG_TRAVERSE,
    ALG_BUBBLE
};
AlgorithmKind currentAlgorithm = ALG_NONE;

// ---------- UI state ----------
std::string typedValue = "";
int  selectedSlotIndex = -1;
std::string messageLog = "";
bool valueBoxFocused = true;
int  currentPseudocodeLine = 0;
float caretTimer = 0.0f;

// ---------- Pseudocode text ----------
vector<string> pseudocodeInsert = {
    "1  // Insert at index",
    "2  i = lastFilled",
    "3  while i >= index:",
    "4      array[i+1] = array[i]  // shift right",
    "5      i = i - 1",
    "6  array[index] = newValue"
};
vector<string> pseudocodeDelete = {
    "1  // Delete by value (from textbox)",
    "2  find first index with value",
    "3  array[index] = empty",
    "4  i = index + 1",
    "5  while i <= lastFilled:",
    "6      array[i-1] = array[i]  // shift left",
    "7  i = i + 1"
};
vector<string> pseudocodeSearch = {
    "1  // Linear search",
    "2  for i from 0 to lastFilled:",
    "3      if array[i] == target: return i",
    "4  return not found"
};
vector<string> pseudocodeTraverse = {
    "1  // Traverse",
    "2  for i from 0 to lastFilled:",
    "3      visit array[i]"
};
vector<string> pseudocodeBubble = {
    "1  // Bubble sort (using TEMP)",
    "2  for pass = 0 to n-2:",
    "3      for i = 0 to n-2:",
    "4          if array[i] > array[i+1]:",
    "5              TEMP = array[i]",
    "6              array[i] = array[i+1]",
    "7              array[i+1] = TEMP"
};

// ---------- Layout (computed each frame) ----------
struct Layout {
    int winW, winH;

    // Grid cells
    Rectangle pseudoCell;
    Rectangle messageCell;
    Rectangle arrayCell;
    Rectangle controlsCell;
    Rectangle tempCell;

    // Derived geometry
    Rectangle pseudocodePanel;
    Rectangle inputRect;
    Rectangle btnInsert, btnDelete, btnTraverse, btnSearch, btnBubble, btnReset;
    Vector2   arrayOrigin;
    float     slotW, slotH, slotSpacing;
    Vector2   tempPos;

    // Fonts
    int fontHeader, fontSub, fontValue, fontIndex, fontPseudocode, fontMessage, fontLabel;
} layout;

// ---------- Utilities ----------
static float clampf(float v, float a, float b) {
    return (v < a) ? a : ((v > b) ? b : v);
}

bool parseInt(const char* s, int& out) {
    if (!s || *s == '\0') return false;
    char* endp = nullptr;
    long v = strtol(s, &endp, 10);
    if (endp == s || *endp != '\0') return false;
    out = (int)v;
    return true;
}

// Helper: fit text into a given width by reducing font size (no behaviour change)
int FitTextToWidth(const char* txt, int baseFontSize, float maxWidth) {
    int fontSize = baseFontSize;
    int textWidth = MeasureText(txt, fontSize);
    while (textWidth > maxWidth - 10 && fontSize > 10) {
        fontSize--;
        textWidth = MeasureText(txt, fontSize);
    }
    return fontSize;
}

// ---------- Layout computation (3x2 grid; array strictly inside middle column) ----------
void computeLayout() {
    layout.winW = GetScreenWidth();
    layout.winH = GetScreenHeight();

    const float margin = 24.0f;

    const float DESIGN_W = 1400.0f;
    float scale = layout.winW / DESIGN_W;
    const float MIN_SCALE = 0.9f;
    const float MAX_SCALE = 1.6f;
    scale = clampf(scale, MIN_SCALE, MAX_SCALE);

    layout.fontHeader = std::max(30, (int)roundf(26.0f * scale));
    layout.fontSub = std::max(18, (int)roundf(16.0f * scale));
    layout.fontValue = std::max(26, (int)roundf(26.0f * scale));
    layout.fontIndex = std::max(16, (int)roundf(14.0f * scale));
    layout.fontPseudocode = std::max(20, (int)roundf(19.0f * scale));
    layout.fontMessage = std::max(20, (int)roundf(18.0f * scale));
    layout.fontLabel = std::max(20, (int)roundf(18.0f * scale));

    float headerSpace = layout.fontHeader + layout.fontSub + 24.0f;

    float innerW = layout.winW - 2 * margin;
    float innerH = layout.winH - headerSpace - margin;

    float colW = innerW / 3.0f;
    float topH = innerH * 0.55f;
    float bottomH = innerH - topH;

    layout.pseudoCell = { margin + 0 * colW, headerSpace,        colW, topH };
    layout.arrayCell = { margin + 1 * colW, headerSpace,        colW, topH };
    layout.tempCell = { margin + 2 * colW, headerSpace,        colW, topH };

    // Make the message area shorter (only use ~35% of the bottom height)
    float messageHeight = bottomH * 0.35f;
    layout.messageCell = { margin + 0 * colW, headerSpace + topH, colW, messageHeight };

    // Controls keep the full bottom height (for buttons + input)
    layout.controlsCell = { margin + 1 * colW, headerSpace + topH, colW, bottomH };

    layout.pseudocodePanel = {
        layout.pseudoCell.x + 6.0f,
        layout.pseudoCell.y + 6.0f,
        layout.pseudoCell.width - 12.0f,
        layout.pseudoCell.height - 12.0f
    };

    // ----- Array slots: keep strictly within arrayCell -----
    float arrayPaddingX = 20.0f;
    float arrayPaddingY = 20.0f;
    float usableW = layout.arrayCell.width - 2 * arrayPaddingX;

    layout.slotSpacing = clampf(18.0f * scale, 8.0f, 30.0f);
    float maxSlotW = (usableW - (ARRAY_CAPACITY - 1) * layout.slotSpacing) / ARRAY_CAPACITY;

    if (maxSlotW < 60.0f * scale) {
        layout.slotSpacing = 6.0f;
        maxSlotW = (usableW - (ARRAY_CAPACITY - 1) * layout.slotSpacing) / ARRAY_CAPACITY;
    }

    layout.slotW = maxSlotW;
    if (layout.slotW < 60.0f) layout.slotW = 60.0f;

    layout.slotH = clampf(80.0f * scale, 65.0f, layout.arrayCell.height - 2 * arrayPaddingY);

    float totalSlotsWidth =
        layout.slotW * ARRAY_CAPACITY + layout.slotSpacing * (ARRAY_CAPACITY - 1);

    float arrayY = layout.arrayCell.y + (layout.arrayCell.height - layout.slotH) / 2.0f;
    float arrayX = layout.arrayCell.x + (layout.arrayCell.width - totalSlotsWidth) / 2.0f;
    if (arrayX < layout.arrayCell.x + arrayPaddingX)
        arrayX = layout.arrayCell.x + arrayPaddingX;

    layout.arrayOrigin = { arrayX, arrayY };

    // ----- TEMP box inside its own right-hand cell -----
    float tempBoxSize = layout.slotH;
    float maxTempSize = std::min(layout.tempCell.width, layout.tempCell.height) * 0.8f;
    if (tempBoxSize > maxTempSize) tempBoxSize = maxTempSize;
    layout.tempPos = {
        layout.tempCell.x + layout.tempCell.width * 0.5f,
        layout.tempCell.y + layout.tempCell.height * 0.4f
    };

    // ----- Controls -----
    float controlsCentreX = layout.controlsCell.x + layout.controlsCell.width * 0.5f;   // for input box
    float buttonsCentreX = layout.controlsCell.x + layout.controlsCell.width * 0.85f;  // shift buttons to the right
    float controlsUsableW = layout.controlsCell.width * 0.9f;
    float inputW = clampf(280.0f * scale, 220.0f, controlsUsableW);
    float inputH = clampf(50.0f * scale, 40.0f, layout.controlsCell.height * 0.35f);

    float selectedY = layout.controlsCell.y + 4.0f;
    float valueLabelY = selectedY + layout.fontLabel + 8.0f;
    float inputY = valueLabelY + layout.fontLabel + 4.0f;

    layout.inputRect = {
        controlsCentreX - inputW / 2.0f,
        inputY,
        inputW,
        inputH
    };

    float buttonsTopY = layout.inputRect.y + layout.inputRect.height + 22.0f;

    // Spacing and base size
    float buttonPadding = 14.0f * scale;
    float buttonHeight = clampf(60.0f * scale, 50.0f, 80.0f);
    float buttonRowUsableW = controlsUsableW;

    // Standard small button width
    float buttonWidth = (buttonRowUsableW - 3 * buttonPadding) / 4.0f;
    buttonWidth = clampf(buttonWidth, 130.0f, 220.0f);

    // Now define wide button width (used for BOTH Delete and Bubble sort)
    float wideButtonWidth = buttonRowUsableW * 0.60f;

    // -------- Row 1 --------
    // [ Insert ] [ Delete (wide) ] [ Traverse ] [ Search ]

    float totalRow1Width =
        buttonWidth +
        buttonPadding +
        wideButtonWidth +
        buttonPadding +
        buttonWidth +
        buttonPadding +
        buttonWidth;

    float startX = buttonsCentreX - totalRow1Width / 2.0f;

    layout.btnInsert = {
        startX,
        buttonsTopY,
        buttonWidth,
        buttonHeight
    };

    layout.btnDelete = {
        layout.btnInsert.x + buttonWidth + buttonPadding,
        buttonsTopY,
        wideButtonWidth,
        buttonHeight
    };

    layout.btnTraverse = {
        layout.btnDelete.x + wideButtonWidth + buttonPadding,
        buttonsTopY,
        buttonWidth,
        buttonHeight
    };

    layout.btnSearch = {
        layout.btnTraverse.x + buttonWidth + buttonPadding,
        buttonsTopY,
        buttonWidth,
        buttonHeight
    };

    // -------- Row 2 --------
    // [ Bubble Sort (wide) ] [ Reset ]

    float secondRowY = buttonsTopY + buttonHeight + 24.0f;

    float resetWidth = buttonRowUsableW - wideButtonWidth - buttonPadding;
    float totalRow2Width = wideButtonWidth + resetWidth + buttonPadding;
    float bubbleX = buttonsCentreX - totalRow2Width / 2.0f;

    layout.btnBubble = {
        bubbleX,
        secondRowY,
        wideButtonWidth,
        buttonHeight + 8.0f
    };

    layout.btnReset = {
        bubbleX + wideButtonWidth + buttonPadding,
        secondRowY,
        resetWidth,
        buttonHeight + 8.0f
    };
}

// slot geometry helpers
Rectangle slotRectForIndex(int index) {
    return Rectangle{
        layout.arrayOrigin.x + index * (layout.slotW + layout.slotSpacing),
        layout.arrayOrigin.y,
        layout.slotW,
        layout.slotH
    };
}

Vector2 slotCentre(int index) {
    Rectangle r = slotRectForIndex(index);
    return Vector2{ r.x + r.width * 0.5f, r.y + r.height * 0.5f };
}

void enqueueAction(const Action& a) { actionQueue.push(a); }

float easeOutCubic(float t) {
    float p = t - 1.0f;
    return 1.0f + p * p * p;
}

// ---------- Action starter ----------
void startAction(const Action& action) {
    switch (action.type) {
    case HIGHLIGHT_SLOT:
        if (action.slotIndex >= 0 && action.slotIndex < ARRAY_CAPACITY)
            slotHighlighted[action.slotIndex] = true;
        break;

    case UNHIGHLIGHT_SLOT:
        if (action.slotIndex >= 0 && action.slotIndex < ARRAY_CAPACITY)
            slotHighlighted[action.slotIndex] = false;
        break;

    case MARK_SEARCH_FOUND:
        if (action.slotIndex >= 0 && action.slotIndex < ARRAY_CAPACITY) {
            slotSearchFound[action.slotIndex] = true;
        }
        break;

    case CLEAR_SEARCH_FOUND:
        if (action.slotIndex < 0) {
            // slotIndex == -1 -> clear all
            for (int i = 0; i < ARRAY_CAPACITY; ++i) {
                slotSearchFound[i] = false;
            }
        }
        else if (action.slotIndex < ARRAY_CAPACITY) {
            slotSearchFound[action.slotIndex] = false;
        }
        break;

    case MARK_DELETE_SLOT:
        if (action.slotIndex >= 0 && action.slotIndex < ARRAY_CAPACITY) {
            slotDeleteMarked[action.slotIndex] = true;
        }
        break;

    case CLEAR_DELETE_SLOT:
        if (action.slotIndex < 0) {
            for (int i = 0; i < ARRAY_CAPACITY; ++i) {
                slotDeleteMarked[i] = false;
            }
        }
        else if (action.slotIndex < ARRAY_CAPACITY) {
            slotDeleteMarked[action.slotIndex] = false;
        }
        break;

    case CLEAR_SELECTION:
        for (int i = 0; i < ARRAY_CAPACITY; ++i) {
            slotSelected[i] = false;
        }
        selectedSlotIndex = -1;
        break;

    case WRITE_VALUE:
        if (action.slotIndex >= 0 && action.slotIndex < ARRAY_CAPACITY)
            arrayData[action.slotIndex] = action.value;
        break;

    case CLEAR_VALUE:
        if (action.slotIndex >= 0 && action.slotIndex < ARRAY_CAPACITY)
            arrayData[action.slotIndex] = EMPTY;
        break;

    case PSEUDOCODE_LINE:
        currentPseudocodeLine = action.pseudocodeLine;
        if (!action.message.empty()) messageLog = action.message;
        break;

    case DISPLAY_MESSAGE:
        messageLog = action.message;
        break;

    case WAIT:
        waitActive = true;
        waitRemaining = (action.duration > 0.0f ? action.duration : 0.5f);
        break;

    case START_MOVE: {
        int src = action.slotIndex;
        int dst = action.targetIndex;

        int val = action.value;
        if (val == EMPTY) {
            if (src == TEMP_ID) val = tempValue;
            else if (src >= 0 && src < ARRAY_CAPACITY) val = arrayData[src];
        }

        int freeMover = (animatedMovers[0].active ? (animatedMovers[1].active ? -1 : 1) : 0);
        if (freeMover == -1) {
            enqueueAction(action); // try again later
            return;
        }

        AnimatedMover& m = animatedMovers[freeMover];
        m.active = true;
        m.value = val;
        m.duration = std::max(0.05f, action.duration);
        m.elapsed = 0.0f;
        m.fromIndex = src;
        m.toIndex = dst;

        if (src == TEMP_ID) {
            m.startPos = layout.tempPos;
            tempOccupied = false; // TEMP is now "in flight"
        }
        else if (src >= 0 && src < ARRAY_CAPACITY) {
            m.startPos = slotCentre(src);
            arrayData[src] = EMPTY;
        }
        else {
            m.startPos = layout.tempPos;
        }

        if (dst == TEMP_ID) {
            m.endPos = layout.tempPos;
        }
        else if (dst >= 0 && dst < ARRAY_CAPACITY) {
            m.endPos = slotCentre(dst);
        }
        else {
            m.endPos = layout.tempPos;
        }
        break;
    }

    case START_MOVE_PAIR:
        // Not used in this version.
        break;

    default:
        break;
    }
}

// ---------- High-level builders ----------
// Insert with conditional shift: only shift when inserting into an occupied slot.
void buildInsertActions(int insertIndex, int valueToInsert) {
    currentAlgorithm = ALG_INSERT;
    currentPseudocodeLine = 0;

    // Basic range check for the UI
    if (insertIndex < 0 || insertIndex >= ARRAY_CAPACITY) {
        enqueueAction(Action(DISPLAY_MESSAGE, -1, -1, 0, 0, 0, 0.5f,
            "Insert index out of range"));
        return;
    }

    // True "full" check using elementCount
    if (elementCount >= ARRAY_CAPACITY) {
        enqueueAction(Action(DISPLAY_MESSAGE, -1, -1, 0, 0, 0, 0.8f,
            "Array is full; cannot insert"));
        return;
    }

    // CASE 1: target slot is EMPTY -> no shifting, just place the value
    if (arrayData[insertIndex] == EMPTY) {
        // Line 6 of the pseudocode: array[index] = newValue
        enqueueAction(Action(PSEUDOCODE_LINE, -1, -1, 0, 0, 6, 0.2f,
            "Write new value into empty slot"));
        enqueueAction(Action(WRITE_VALUE, insertIndex, -1, valueToInsert));
        elementCount++;
        enqueueAction(Action(DISPLAY_MESSAGE, -1, -1, 0, 0, 0, 0.8f,
            "Insert complete"));
        return;
    }

    // CASE 2: target slot is OCCUPIED -> need to shift right IF there is space
    // Find a free slot to the RIGHT of insertIndex (including the end)
    int freeIndex = -1;
    for (int i = ARRAY_CAPACITY - 1; i >= insertIndex; --i) {
        if (arrayData[i] == EMPTY) {
            freeIndex = i;
            break;
        }
    }

    // If there is no free slot to the right, we cannot shift
    if (freeIndex == -1) {
        enqueueAction(Action(DISPLAY_MESSAGE, -1, -1, 0, 0, 0, 0.8f,
            "No empty slot to the right; cannot insert here"));
        return;
    }

    // Pseudocode line 2: i = lastFilled (conceptually; here we treat freeIndex-1 as our start)
    enqueueAction(Action(PSEUDOCODE_LINE, -1, -1, 0, 0, 2, 0.2f,
        "Prepare to shift elements to the right"));

    // Shift values to the right from freeIndex-1 down to insertIndex
    for (int i = freeIndex - 1; i >= insertIndex; --i) {
        // Line 3: while i >= index
        enqueueAction(Action(PSEUDOCODE_LINE, -1, -1, 0, 0, 3, 0.15f,
            "Check i >= index"));
        enqueueAction(Action(HIGHLIGHT_SLOT, i));
        enqueueAction(Action(WAIT, -1, -1, 0, 0, 0, 0.4f));

        if (arrayData[i] != EMPTY) {
            // Line 4: array[i+1] = array[i]  // shift right
            enqueueAction(Action(PSEUDOCODE_LINE, -1, -1, 0, 0, 4, 0.15f,
                "Shift right"));
            int val = arrayData[i];
            enqueueAction(Action(START_MOVE, i, i + 1, val, 0, 0, 0.55f,
                "Shift right"));
            enqueueAction(Action(WAIT, -1, -1, 0, 0, 0, 0.2f));
        }

        // Line 5: i = i - 1 (conceptual)
        enqueueAction(Action(PSEUDOCODE_LINE, -1, -1, 0, 0, 5, 0.15f,
            "i = i - 1"));
        enqueueAction(Action(UNHIGHLIGHT_SLOT, i));
    }

    // Now slot insertIndex is free; write the new value there
    enqueueAction(Action(PSEUDOCODE_LINE, -1, -1, 0, 0, 6, 0.18f,
        "Write new value"));
    enqueueAction(Action(WRITE_VALUE, insertIndex, -1, valueToInsert));

    elementCount++;

    enqueueAction(Action(DISPLAY_MESSAGE, -1, -1, 0, 0, 0, 0.9f,
        "Insert complete (with right shift)"));
}

// Delete by value: animate search, highlight red, then delete and left-shift.
void buildDeleteByValueActions(int valueToDelete) {
    currentAlgorithm = ALG_DELETE;
    currentPseudocodeLine = 0;

    // Clear selection and any old visual flags
    enqueueAction(Action(CLEAR_SELECTION));
    enqueueAction(Action(CLEAR_DELETE_SLOT, -1));   // clear all red marks
    enqueueAction(Action(CLEAR_SEARCH_FOUND, -1));  // clear any green marks

    enqueueAction(Action(PSEUDOCODE_LINE, -1, -1, 0, 0, 2, 0.25f,
        "Search for value to delete"));

    // First, find the index (for logic) but we'll still animate the search
    int foundIndex = -1;
    for (int i = 0; i < ARRAY_CAPACITY; ++i) {
        if (arrayData[i] != EMPTY && arrayData[i] == valueToDelete) {
            foundIndex = i;
            break;
        }
    }

    if (foundIndex < 0) {
        enqueueAction(Action(DISPLAY_MESSAGE, -1, -1, 0, 0, 0, 0.9f,
            std::string(TextFormat("%d not found", valueToDelete))));
        return;
    }

    // Animate the search: walk from 0 to foundIndex, highlighting each
    for (int i = 0; i <= foundIndex; ++i) {
        enqueueAction(Action(HIGHLIGHT_SLOT, i));
        enqueueAction(Action(WAIT, -1, -1, 0, 0, 0, 0.6f));

        if (i == foundIndex) {
            // Found: mark red
            enqueueAction(Action(MARK_DELETE_SLOT, foundIndex));

            enqueueAction(Action(PSEUDOCODE_LINE, -1, -1, 0, 0, 3, 0.2f,
                "Found value; delete it"));
            enqueueAction(Action(DISPLAY_MESSAGE, -1, -1, 0, 0, 0, 0.5f,
                std::string(TextFormat("Deleting %d at index %d",
                    valueToDelete, foundIndex))));

            // Hold the red highlight on screen for ~1.5s BEFORE deletion
            enqueueAction(Action(WAIT, -1, -1, 0, 0, 0, 1.5f));

            // Now actually clear the slot
            enqueueAction(Action(CLEAR_VALUE, foundIndex));
            elementCount = std::max(0, elementCount - 1);

            // Small pause after deletion
            enqueueAction(Action(WAIT, -1, -1, 0, 0, 0, 0.4f));

            // Remove red marking and highlight after deletion
            enqueueAction(Action(CLEAR_DELETE_SLOT, foundIndex));
            enqueueAction(Action(UNHIGHLIGHT_SLOT, foundIndex));
        }
        else {
            // Not this one: unhighlight and keep going
            enqueueAction(Action(UNHIGHLIGHT_SLOT, i));
        }
    }

    // Now perform the left shift of elements after foundIndex
    int lastFilled = -1;
    for (int i = ARRAY_CAPACITY - 1; i >= 0; --i) {
        if (arrayData[i] != EMPTY) {
            lastFilled = i;
            break;
        }
    }

    if (lastFilled > foundIndex) {
        enqueueAction(Action(PSEUDOCODE_LINE, -1, -1, 0, 0, 5, 0.2f,
            "Shift elements left"));
    }

    for (int i = foundIndex + 1; i <= lastFilled; ++i) {
        int val = arrayData[i];
        enqueueAction(Action(HIGHLIGHT_SLOT, i));
        enqueueAction(Action(WAIT, -1, -1, 0, 0, 0, 0.35f));

        if (val != EMPTY) {
            enqueueAction(Action(START_MOVE, i, i - 1, val, 0, 0, 0.5f,
                "Shift left"));
            enqueueAction(Action(WAIT, -1, -1, 0, 0, 0, 0.2f));
        }

        enqueueAction(Action(UNHIGHLIGHT_SLOT, i));
    }

    enqueueAction(Action(DISPLAY_MESSAGE, -1, -1, 0, 0, 0, 0.9f,
        "Delete by value complete"));
}

// Traverse: highlight each element in order.
void buildTraverseActions() {
    currentAlgorithm = ALG_TRAVERSE;
    currentPseudocodeLine = 0;

    enqueueAction(Action(PSEUDOCODE_LINE, -1, -1, 0, 0, 2, 0.25f, "Traverse all elements"));
    for (int i = 0; i < ARRAY_CAPACITY; ++i) {
        if (arrayData[i] != EMPTY) {
            enqueueAction(Action(HIGHLIGHT_SLOT, i));
            enqueueAction(Action(WAIT, -1, -1, 0, 0, 0, 0.9f));
            enqueueAction(Action(UNHIGHLIGHT_SLOT, i));
        }
    }
    enqueueAction(Action(DISPLAY_MESSAGE, -1, -1, 0, 0, 0, 1.0f, "Traversal complete"));
}

// Search: animate scan, then mark found cell green.
void buildSearchActions(int searchValue) {
    currentAlgorithm = ALG_SEARCH;
    currentPseudocodeLine = 0;

    // Clear selection and old search highlights via actions
    enqueueAction(Action(CLEAR_SELECTION));
    enqueueAction(Action(CLEAR_SEARCH_FOUND, -1));  // -1 => clear all

    enqueueAction(Action(PSEUDOCODE_LINE, -1, -1, 0, 0, 2, 0.32f,
        std::string(TextFormat("Searching for %d", searchValue))));

    for (int i = 0; i < ARRAY_CAPACITY; ++i) {
        // Highlight current slot
        enqueueAction(Action(HIGHLIGHT_SLOT, i));
        enqueueAction(Action(WAIT, -1, -1, 0, 0, 0, 0.9f));

        // Check value
        if (arrayData[i] != EMPTY && arrayData[i] == searchValue) {
            // Mark as found (turn green)
            enqueueAction(Action(MARK_SEARCH_FOUND, i));

            enqueueAction(Action(PSEUDOCODE_LINE, -1, -1, 0, 0, 3, 0.2f,
                "Match found"));
            enqueueAction(Action(DISPLAY_MESSAGE, -1, -1, 0, 0, 0, 1.0f,
                std::string(TextFormat("Found %d at index %d",
                    searchValue, i))));
            enqueueAction(Action(UNHIGHLIGHT_SLOT, i));
            return; // Stop searching further
        }

        // Not a match: unhighlight and continue
        enqueueAction(Action(UNHIGHLIGHT_SLOT, i));
    }

    // Not found
    enqueueAction(Action(PSEUDOCODE_LINE, -1, -1, 0, 0, 4, 0.2f, "Not found"));
    enqueueAction(Action(DISPLAY_MESSAGE, -1, -1, 0, 0, 0, 1.0f,
        std::string(TextFormat("%d not found", searchValue))));
}

// Bubble sort swap using TEMP box
void enqueueSwapViaTemp(int i, int j, int valI, int valJ) {
    // i -> TEMP
    enqueueAction(Action(PSEUDOCODE_LINE, -1, -1, 0, 0, 5, 0.15f, "TEMP = array[i]"));
    enqueueAction(Action(START_MOVE, i, TEMP_ID, valI, 0, 0, 0.55f, "Move to TEMP"));
    enqueueAction(Action(WAIT, -1, -1, 0, 0, 0, 0.25f));

    // j -> i
    enqueueAction(Action(PSEUDOCODE_LINE, -1, -1, 0, 0, 6, 0.15f, "array[i] = array[i+1]"));
    enqueueAction(Action(START_MOVE, j, i, valJ, 0, 0, 0.55f, "Move right to left"));
    enqueueAction(Action(WAIT, -1, -1, 0, 0, 0, 0.25f));

    // TEMP -> j
    enqueueAction(Action(PSEUDOCODE_LINE, -1, -1, 0, 0, 7, 0.15f, "array[i+1] = TEMP"));
    enqueueAction(Action(START_MOVE, TEMP_ID, j, valI, 0, 0, 0.55f, "TEMP back"));
    enqueueAction(Action(WAIT, -1, -1, 0, 0, 0, 0.25f));
}

void buildBubbleSortActions() {
    currentAlgorithm = ALG_BUBBLE;
    currentPseudocodeLine = 0;

    enqueueAction(Action(DISPLAY_MESSAGE, -1, -1, 0, 0, 0, 0.6f, "Starting Bubble Sort"));

    vector<int> simArray = arrayData;
    auto valueOrLarge = [](int v) {
        return (v == EMPTY) ? INT_MAX : v;
        };
    int n = (int)simArray.size();

    for (int pass = 0; pass < n - 1; ++pass) {
        bool anySwap = false;
        enqueueAction(Action(PSEUDOCODE_LINE, -1, -1, 0, 0, 2, 0.2f,
            std::string(TextFormat("Pass %d", pass))));
        for (int i = 0; i < n - 1; ++i) {
            enqueueAction(Action(PSEUDOCODE_LINE, -1, -1, 0, 0, 3, 0.2f,
                std::string(TextFormat("Compare %d and %d", i, i + 1))));
            int left = valueOrLarge(simArray[i]);
            int right = valueOrLarge(simArray[i + 1]);
            enqueueAction(Action(HIGHLIGHT_SLOT, i));
            enqueueAction(Action(HIGHLIGHT_SLOT, i + 1));
            enqueueAction(Action(WAIT, -1, -1, 0, 0, 0, 0.7f));

            if (left > right) {
                int vI = simArray[i];
                int vJ = simArray[i + 1];
                enqueueSwapViaTemp(i, i + 1, vI, vJ);
                std::swap(simArray[i], simArray[i + 1]);
                anySwap = true;
                enqueueAction(Action(DISPLAY_MESSAGE, -1, -1, 0, 0, 0, 0.4f,
                    std::string(TextFormat("Swapped %d and %d", vI, vJ))));
            }
            else {
                enqueueAction(Action(DISPLAY_MESSAGE, -1, -1, 0, 0, 0, 0.35f, "No swap"));
            }

            enqueueAction(Action(UNHIGHLIGHT_SLOT, i));
            enqueueAction(Action(UNHIGHLIGHT_SLOT, i + 1));
        }
        if (!anySwap) break;
    }
    enqueueAction(Action(DISPLAY_MESSAGE, -1, -1, 0, 0, 0, 1.0f, "Bubble Sort complete"));
}

// ---------- Rendering helpers ----------
void drawPseudocodePanel(const vector<string>& lines, int highlightLine) {
    DrawRectangleRec(layout.pseudocodePanel, Fade(LIGHTGRAY, 0.98f));
    DrawRectangleLines((int)layout.pseudocodePanel.x, (int)layout.pseudocodePanel.y,
        (int)layout.pseudocodePanel.width, (int)layout.pseudocodePanel.height,
        BLACK);

    int baseY = (int)layout.pseudocodePanel.y + 14;
    int lineHeight = std::max(PSEUDOCODE_LINE_HEIGHT, layout.fontPseudocode + 6);

    for (int idx = 0; idx < (int)lines.size(); ++idx) {
        int lineNo = idx + 1;
        float y = (float)(baseY + idx * lineHeight);
        Rectangle lineRect = {
            layout.pseudocodePanel.x + 10.0f,
            y - 4.0f,
            layout.pseudocodePanel.width - 20.0f,
            (float)lineHeight
        };

        if (lineNo == highlightLine) {
            Rectangle hlRect = lineRect;
            hlRect.x -= 4.0f;
            hlRect.width += 8.0f;
            DrawRectangleRec(hlRect, Fade(SKYBLUE, 0.45f));
        }

        DrawText(lines[idx].c_str(),
            (int)lineRect.x + 4,
            (int)lineRect.y + 4,
            layout.fontPseudocode,
            BLACK);
    }
}

void drawSlot(int index) {
    Rectangle r = slotRectForIndex(index);
    Color background = COLOR_SLOT_EMPTY;

    if (slotDeleteMarked[index]) {
        background = COLOR_SLOT_DELETE;   // delete target (red)
    }
    else if (slotSearchFound[index]) {
        background = COLOR_SLOT_SEARCH;   // search found (green)
    }
    else if (slotSelected[index]) {
        background = COLOR_SLOT_SELECTED; // user-selected index (blue)
    }
    else if (slotHighlighted[index]) {
        background = COLOR_SLOT_HIGHLIGHT; // generic algorithm highlight (gold)
    }

    DrawRectangleRec(r, background);
    DrawRectangleLinesEx(r, 2, BLACK);

    DrawText(TextFormat("%d", index),
        (int)r.x + 6,
        (int)(r.y + r.height + 6),
        layout.fontIndex,
        DARKGRAY);

    if (arrayData[index] != EMPTY) {
        DrawText(TextFormat("%d", arrayData[index]),
            (int)r.x + 16,
            (int)(r.y + (r.height - layout.fontValue) / 2),
            layout.fontValue,
            BLACK);
    }
    else {
        DrawText("-",
            (int)(r.x + r.width / 2 - 6),
            (int)(r.y + (r.height - layout.fontValue) / 2),
            layout.fontValue,
            GRAY);
    }
}

void drawAnimatedMovers() {
    for (int mi = 0; mi < 2; ++mi) {
        AnimatedMover& m = animatedMovers[mi];
        if (!m.active) continue;
        float t = m.elapsed / std::max(0.0001f, m.duration);
        t = clampf(t, 0.0f, 1.0f);
        float e = easeOutCubic(t);
        Vector2 pos = {
            m.startPos.x + (m.endPos.x - m.startPos.x) * e,
            m.startPos.y + (m.endPos.y - m.startPos.y) * e
        };
        DrawCircleV(pos, std::max(18.0f, layout.slotH * 0.35f), PURPLE);
        DrawText(TextFormat("%d", m.value),
            (int)pos.x - (layout.fontValue / 2),
            (int)pos.y - (layout.fontValue / 2),
            layout.fontValue - 4,
            WHITE);
    }
}

// ---------- Input handling ----------
void handleTextInput() {
    // TAB always focuses the value box
    if (IsKeyPressed(KEY_TAB)) {
        valueBoxFocused = true;
    }
    // If the value box is not focused, ignore normal typing
    if (!valueBoxFocused) {
        return;
    }
    // Handle character input
    int c = GetCharPressed();
    while (c > 0) {
        if ((c >= '0' && c <= '9') || c == '-') {
            typedValue.push_back((char)c);
        }
        c = GetCharPressed();
    }
    // Handle backspace
    if (IsKeyPressed(KEY_BACKSPACE) && !typedValue.empty()) {
        typedValue.pop_back();
    }
}

void handleMouseClick(Vector2 mousePoint) {
    if (CheckCollisionPointRec(mousePoint, layout.inputRect)) {
        valueBoxFocused = true;
        return;
    }

    if (CheckCollisionPointRec(mousePoint, layout.btnInsert)) {
        if (selectedSlotIndex < 0) {
            messageLog = "Select an index before inserting";
            return;
        }

        int parsedValue;
        if (!parseInt(typedValue.c_str(), parsedValue)) {
            messageLog = "Invalid value entered";
            return;
        }

        // Queue the insert animation
        buildInsertActions(selectedSlotIndex, parsedValue);

        // Immediately clear the selection so the slot is no longer highlighted
        for (int i = 0; i < ARRAY_CAPACITY; ++i) {
            slotSelected[i] = false;
        }
        selectedSlotIndex = -1;

        typedValue.clear();
        messageLog = "Insert queued";
        return;
    }

    if (CheckCollisionPointRec(mousePoint, layout.btnDelete)) {
        int pv;
        if (!parseInt(typedValue.c_str(), pv)) {
            messageLog = "Type a value to delete";
            return;
        }
        buildDeleteByValueActions(pv);
        return;
    }

    if (CheckCollisionPointRec(mousePoint, layout.btnTraverse)) {
        buildTraverseActions();
        messageLog = "Traversal queued";
        return;
    }

    if (CheckCollisionPointRec(mousePoint, layout.btnSearch)) {
        int pv;
        if (!parseInt(typedValue.c_str(), pv)) {
            messageLog = "Type a value to search";
            return;
        }
        buildSearchActions(pv);
        messageLog = TextFormat("Search queued for %d", pv);
        return;
    }

    if (CheckCollisionPointRec(mousePoint, layout.btnBubble)) {
        buildBubbleSortActions();
        messageLog = "Bubble Sort queued";
        return;
    }

    if (CheckCollisionPointRec(mousePoint, layout.btnReset)) {
        while (!actionQueue.empty()) actionQueue.pop();
        for (int i = 0; i < ARRAY_CAPACITY; ++i) {
            arrayData[i] = EMPTY;
            slotHighlighted[i] = false;
            slotSelected[i] = false;
            slotSearchFound[i] = false;
            slotDeleteMarked[i] = false;
        }
        elementCount = 0;

        selectedSlotIndex = -1;
        typedValue.clear();
        currentAlgorithm = ALG_NONE;
        currentPseudocodeLine = 0;
        tempOccupied = false;
        messageLog = "Reset complete";
        return;
    }

    // Click slots to select index
    for (int i = 0; i < ARRAY_CAPACITY; ++i) {
        Rectangle r = slotRectForIndex(i);
        if (CheckCollisionPointRec(mousePoint, r)) {
            for (int k = 0; k < ARRAY_CAPACITY; ++k) slotSelected[k] = false;
            slotSelected[i] = true;
            selectedSlotIndex = i;
            if (arrayData[i] != EMPTY)
                typedValue = TextFormat("%d", arrayData[i]);
            else
                typedValue.clear();
            messageLog = TextFormat("Selected index %d", i);
            return;
        }
    }
}

// ---------- Main ----------
int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT,
        "Array Visualiser");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        computeLayout();
        float dt = GetFrameTime();
        caretTimer += dt;

        handleTextInput();
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mp = { (float)GetMouseX(), (float)GetMouseY() };
            handleMouseClick(mp);
        }

        // start next action if nothing is animating or waiting
        bool moversActive = (animatedMovers[0].active || animatedMovers[1].active || waitActive);
        if (!moversActive && !actionQueue.empty()) {
            Action a = actionQueue.front();
            actionQueue.pop();
            startAction(a);
        }

        // update movers
        for (int mi = 0; mi < 2; ++mi) {
            AnimatedMover& m = animatedMovers[mi];
            if (!m.active) continue;
            m.elapsed += dt;
            if (m.elapsed >= m.duration) {
                if (m.toIndex == TEMP_ID) {
                    tempValue = m.value;
                    tempOccupied = true;
                }
                else if (m.toIndex >= 0 && m.toIndex < ARRAY_CAPACITY) {
                    arrayData[m.toIndex] = m.value;
                }
                m.active = false;
            }
        }

        // update wait
        if (waitActive) {
            waitRemaining -= dt;
            if (waitRemaining <= 0.0f) {
                waitActive = false;
                waitRemaining = 0.0f;
            }
        }

        // ---------- Render ----------
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("Array Visualiser",
            18, 10, layout.fontHeader, BLACK);
        DrawText("Make sure to click a slot to select index.",
            18, 10 + layout.fontHeader + 4, layout.fontSub, DARKGRAY);

        // Choose pseudocode based on current algorithm
        const vector<string>* pcode = nullptr;
        switch (currentAlgorithm) {
        case ALG_INSERT:   pcode = &pseudocodeInsert;   break;
        case ALG_DELETE:   pcode = &pseudocodeDelete;   break;
        case ALG_SEARCH:   pcode = &pseudocodeSearch;   break;
        case ALG_TRAVERSE: pcode = &pseudocodeTraverse; break;
        case ALG_BUBBLE:   pcode = &pseudocodeBubble;   break;
        case ALG_NONE:
        default:           pcode = &pseudocodeInsert;   break;
        }

        drawPseudocodePanel(*pcode, currentPseudocodeLine);

        // array + animations
        for (int i = 0; i < ARRAY_CAPACITY; ++i) drawSlot(i);
        drawAnimatedMovers();

        // Show element count (visual only, no logic change)
        DrawText(TextFormat("Elements: %d / %d", elementCount, ARRAY_CAPACITY),
            (int)layout.arrayCell.x + 10,
            (int)layout.arrayCell.y + 10,
            layout.fontSub,
            DARKGRAY);

        // TEMP box
        float tempBoxSize = layout.slotH;
        float maxTempSize = std::min(layout.tempCell.width, layout.tempCell.height) * 0.8f;
        if (tempBoxSize > maxTempSize) tempBoxSize = maxTempSize;
        Rectangle tempBox = {
            layout.tempPos.x - tempBoxSize / 2.0f,
            layout.tempPos.y - tempBoxSize / 2.0f,
            tempBoxSize,
            tempBoxSize
        };
        DrawText("TEMP",
            (int)(tempBox.x + tempBox.width / 2.0f
                - MeasureText("TEMP", layout.fontIndex) / 2),
            (int)(tempBox.y - layout.fontIndex - 4),
            layout.fontIndex,
            DARKGRAY);

        // Slightly more distinctive TEMP fill colour
        DrawRectangleRec(tempBox, Fade(SKYBLUE, 0.25f));
        DrawRectangleLines((int)tempBox.x, (int)tempBox.y,
            (int)tempBox.width, (int)tempBox.height, DARKGRAY);

        if (tempOccupied && tempValue != EMPTY) {
            DrawText(TextFormat("%d", tempValue),
                (int)(tempBox.x + tempBox.width / 2
                    - MeasureText("000", layout.fontValue) / 2),
                (int)(tempBox.y + tempBox.height / 2 - layout.fontValue / 2),
                layout.fontValue,
                BLACK);
        }

        // Controls: Selected index + Value + textbox
        int selectedY = (int)(layout.controlsCell.y + 4.0f);
        int valueLabelY = selectedY + layout.fontLabel + 8;

        const char* selText =
            (selectedSlotIndex >= 0)
            ? TextFormat("Selected index: %d (click another slot to change)",
                selectedSlotIndex)
            : "Selected index: - (click a slot to choose index)";

        DrawText(selText,
            (int)(layout.controlsCell.x + layout.controlsCell.width * 0.5f
                - MeasureText(selText, layout.fontLabel) / 2),
            selectedY,
            layout.fontLabel,
            DARKBLUE);

        DrawText("Value:",
            (int)layout.inputRect.x,
            valueLabelY,
            layout.fontLabel,
            BLACK);

        DrawRectangleRec(layout.inputRect,
            valueBoxFocused ? RAYWHITE : LIGHTGRAY);
        DrawRectangleLines((int)layout.inputRect.x, (int)layout.inputRect.y,
            (int)layout.inputRect.width, (int)layout.inputRect.height,
            BLACK);

        DrawText(typedValue.c_str(),
            (int)layout.inputRect.x + 10,
            (int)(layout.inputRect.y
                + (layout.inputRect.height - layout.fontValue) / 2),
            layout.fontValue - 2,
            BLACK);

        if (valueBoxFocused && fmod(caretTimer, 1.0f) < 0.6f) {
            int tw = MeasureText(typedValue.c_str(), layout.fontValue - 2);
            DrawRectangle((int)layout.inputRect.x + 10 + tw + 3,
                (int)layout.inputRect.y + 8,
                3,
                layout.fontValue,
                BLACK);
        }

        // Buttons row 1
        DrawRectangleRec(layout.btnInsert, BLUE);
        {
            const char* txt = "Insert";
            int fontSize = FitTextToWidth(txt, layout.fontLabel, layout.btnInsert.width);
            int textWidth = MeasureText(txt, fontSize);

            DrawText(txt,
                (int)(layout.btnInsert.x + layout.btnInsert.width / 2 - textWidth / 2),
                (int)(layout.btnInsert.y + (layout.btnInsert.height - fontSize) / 2),
                fontSize,
                WHITE);
        }

        DrawRectangleRec(layout.btnDelete, RED);
        {
            const char* txt = "Delete (by value)";
            int fontSize = FitTextToWidth(txt, layout.fontLabel, layout.btnDelete.width);
            int textWidth = MeasureText(txt, fontSize);

            DrawText(txt,
                (int)(layout.btnDelete.x + layout.btnDelete.width / 2 - textWidth / 2),
                (int)(layout.btnDelete.y + (layout.btnDelete.height - fontSize) / 2),
                fontSize,
                WHITE);
        }

        DrawRectangleRec(layout.btnTraverse, ORANGE);
        {
            const char* txt = "Traverse";
            int fontSize = FitTextToWidth(txt, layout.fontLabel, layout.btnTraverse.width);
            int textWidth = MeasureText(txt, fontSize);

            DrawText(txt,
                (int)(layout.btnTraverse.x + layout.btnTraverse.width / 2 - textWidth / 2),
                (int)(layout.btnTraverse.y + (layout.btnTraverse.height - fontSize) / 2),
                fontSize,
                WHITE);
        }

        DrawRectangleRec(layout.btnSearch, PURPLE);
        {
            const char* txt = "Search";
            int fontSize = FitTextToWidth(txt, layout.fontLabel, layout.btnSearch.width);
            int textWidth = MeasureText(txt, fontSize);

            DrawText(txt,
                (int)(layout.btnSearch.x + layout.btnSearch.width / 2 - textWidth / 2),
                (int)(layout.btnSearch.y + (layout.btnSearch.height - fontSize) / 2),
                fontSize,
                WHITE);
        }

        // Buttons row 2
        DrawRectangleRec(layout.btnBubble, DARKGREEN);
        {
            const char* txt = "Bubble Sort";
            int fontSize = FitTextToWidth(txt, layout.fontHeader - 4, layout.btnBubble.width);
            int textWidth = MeasureText(txt, fontSize);

            DrawText(txt,
                (int)(layout.btnBubble.x + layout.btnBubble.width / 2 - textWidth / 2),
                (int)(layout.btnBubble.y + (layout.btnBubble.height - fontSize) / 2),
                fontSize,
                WHITE);
        }

        DrawRectangleRec(layout.btnReset, DARKGRAY);
        {
            const char* txt = "Reset";
            int fontSize = FitTextToWidth(txt, layout.fontLabel, layout.btnReset.width);
            int textWidth = MeasureText(txt, fontSize);

            DrawText(txt,
                (int)(layout.btnReset.x + layout.btnReset.width / 2 - textWidth / 2),
                (int)(layout.btnReset.y + (layout.btnReset.height - fontSize) / 2),
                fontSize,
                WHITE);
        }

        // Message area
        DrawText("Message:",
            (int)layout.messageCell.x + 10,
            (int)layout.messageCell.y + 8,
            layout.fontLabel,
            BLACK);

        float shortcutsReserve = layout.fontSub + 18.0f;

        Rectangle msgBox = {
            layout.messageCell.x + 8,
            layout.messageCell.y + 8 + layout.fontLabel + 4,
            layout.messageCell.width - 16,
            layout.messageCell.height - (layout.fontLabel + 24.0f + shortcutsReserve)
        };
        DrawRectangleLines((int)msgBox.x, (int)msgBox.y,
            (int)msgBox.width, (int)msgBox.height,
            LIGHTGRAY);

        DrawText(messageLog.c_str(),
            (int)msgBox.x + 8,
            (int)msgBox.y + 6,
            layout.fontMessage,
            DARKBLUE);

        int shortcutsY = (int)(msgBox.y + msgBox.height + 6);

        DrawText("Shortcuts: TAB focus, BACKSPACE delete",
            (int)layout.messageCell.x + 10,
            shortcutsY,
            layout.fontSub,
            DARKGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
