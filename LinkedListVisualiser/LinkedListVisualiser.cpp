#include "raylib.h"
#include <string>
#include <vector>
#include <functional>
#include <cstdlib>
#include <cmath>

// -----------------------------
// Visual theme colours
// -----------------------------

const Color COLOUR_BACKGROUND = { 245, 246, 250, 255 }; // soft light
const Color COLOUR_PANEL_BACKGROUND = { 230, 233, 240, 255 };
const Color COLOUR_PANEL_BORDER = { 180, 185, 195, 255 };

const Color COLOUR_BUTTON_NORMAL = { 60, 63, 81, 255 };
const Color COLOUR_BUTTON_HOVER = { 90, 94, 115, 255 };
const Color COLOUR_BUTTON_DISABLED = { 120, 123, 140, 255 };

const Color COLOUR_NODE_NORMAL = { 255, 255, 255, 255 };
const Color COLOUR_NODE_CURRENT = { 255, 245, 157, 255 }; // yellow-ish
const Color COLOUR_NODE_FOUND = { 129, 199, 132, 255 }; // green-ish
const Color COLOUR_NODE_DELETE = { 239, 154, 154, 255 }; // red-ish

const Color COLOUR_TEXT_MAIN = BLACK;
const Color COLOUR_TEXT_MUTED = DARKGRAY;

// -----------------------------
// Layout constants
// -----------------------------

const int   SCREEN_WIDTH = 1500;
const int   SCREEN_HEIGHT = 900;

const float NODE_WIDTH = 100.0f;
const float NODE_HEIGHT = 60.0f;
const float HORIZONTAL_SPACING = 60.0f;

const float LIST_START_X = 80.0f;
const float LIST_START_Y = 450.0f;

const float LIFT_HEIGHT = 120.0f;

const Rectangle VALUE_TEXTBOX_RECT = { 80.0f, 120.0f, 140.0f, 40.0f };

// -----------------------------
// Data structures and enums
// -----------------------------

enum class NodeVisualState
{
    Normal,
    Current,
    Found,
    ToDelete
};

struct ListNode
{
    int value;
    ListNode* nextPointer;
    Vector2 position;
    NodeVisualState visualState;

    explicit ListNode(int nodeValue)
        : value(nodeValue),
        nextPointer(nullptr),
        position{ 0.0f, 0.0f },
        visualState(NodeVisualState::Normal)
    {
    }
};

class LinkedList
{
public:
    LinkedList() : headPointer(nullptr), tailPointer(nullptr) {}

    ~LinkedList()
    {
        Clear();
    }

    void InsertAtHeadImmediate(int value)
    {
        ListNode* newNodePointer = new ListNode(value);

        if (headPointer == nullptr)
        {
            headPointer = newNodePointer;
            tailPointer = newNodePointer;
        }
        else
        {
            newNodePointer->nextPointer = headPointer;
            headPointer = newNodePointer;
        }
    }

    void InsertAtTailImmediate(int value)
    {
        ListNode* newNodePointer = new ListNode(value);

        if (headPointer == nullptr)
        {
            headPointer = newNodePointer;
            tailPointer = newNodePointer;
        }
        else
        {
            tailPointer->nextPointer = newNodePointer;
            tailPointer = newNodePointer;
        }
    }

    // Delete a specific node, given its previous node (can be nullptr if deleting head)
    void DeleteNodeWithPreviousImmediate(ListNode* previousNodePointer, ListNode* currentNodePointer)
    {
        if (currentNodePointer == nullptr)
        {
            return;
        }

        ListNode* nextNodePointer = currentNodePointer->nextPointer;

        if (previousNodePointer == nullptr)
        {
            // Deleting head
            if (headPointer == currentNodePointer)
            {
                headPointer = nextNodePointer;
            }
        }
        else if (previousNodePointer->nextPointer == currentNodePointer)
        {
            previousNodePointer->nextPointer = nextNodePointer;
        }

        // Update tailPointer if we removed the last node
        if (nextNodePointer == nullptr)
        {
            if (previousNodePointer == nullptr)
            {
                // Deleted the only node in the list
                tailPointer = headPointer; // which is now nullptr
            }
            else
            {
                tailPointer = previousNodePointer;
            }
        }

        if (headPointer == nullptr)
        {
            tailPointer = nullptr;
        }

        delete currentNodePointer;
    }

    void Clear()
    {
        ListNode* currentNodePointer = headPointer;
        while (currentNodePointer != nullptr)
        {
            ListNode* nextNodePointer = currentNodePointer->nextPointer;
            delete currentNodePointer;
            currentNodePointer = nextNodePointer;
        }
        headPointer = nullptr;
        tailPointer = nullptr;
    }

    ListNode* GetHeadPointer()
    {
        return headPointer;
    }

    ListNode* GetTailPointer()
    {
        return tailPointer;
    }

    void ResetVisualStates()
    {
        ListNode* currentNodePointer = headPointer;
        while (currentNodePointer != nullptr)
        {
            currentNodePointer->visualState = NodeVisualState::Normal;
            currentNodePointer = currentNodePointer->nextPointer;
        }
    }

    void UpdateLayout(float startX, float startY, float nodeWidth, float nodeHeight, float horizontalSpacing)
    {
        (void)nodeHeight; // unused, but kept for future flexibility

        ListNode* currentNodePointer = headPointer;
        int nodeIndex = 0;
        while (currentNodePointer != nullptr)
        {
            currentNodePointer->position = {
                startX + static_cast<float>(nodeIndex) * (nodeWidth + horizontalSpacing),
                startY
            };
            currentNodePointer = currentNodePointer->nextPointer;
            nodeIndex++;
        }
    }

private:
    ListNode* headPointer;
    ListNode* tailPointer;
};

enum class OperationType
{
    None,
    InsertHead,
    InsertTail,
    Search,
    Delete
};

struct AlgorithmStep
{
    std::string descriptionText;
    int highlightedPseudocodeLineIndex = -1;
    std::function<void()> applyStepFunction;
};

// -----------------------------
// Global / shared state
// -----------------------------

LinkedList linkedList;

OperationType currentOperationType = OperationType::None;
std::vector<AlgorithmStep> currentSteps;
int currentStepIndex = -1;
bool isOperationInProgress = false;

// For timed step playback
float stepTimerSeconds = 0.0f;
const float STEP_INTERVAL_SECONDS = 0.8f; // delay between steps (seconds)

// For insert operations
ListNode* newNodeForInsertPointer = nullptr;
int pendingInsertValue = 0;

// For tail connection animation
ListNode* tailNodeBeforeInsertPointer = nullptr;
bool shouldHideTailNullArrow = false;       // hide tail -> NULL arrow
bool shouldDrawTailToNewArrow = false;      // draw tail -> newNode arrow
bool shouldDrawNewNodeToNullArrow = false;  // draw newNode -> NULL arrow

// For delete
ListNode* deleteCurrentNodePointer = nullptr;
ListNode* deletePreviousNodePointer = nullptr;
bool shouldHidePrevToCurrentArrow = false;      // hide previous -> current arrow
bool shouldDrawPrevToNextArrow = false;         // draw previous -> current->next arrow
bool shouldHideDeletedNodeInList = false;       // hide deleted node in row
bool shouldDrawLiftedDeletedNode = false;       // show deleted node above list
Vector2 liftedDeletedNodePosition = { 0.0f, 0.0f };

// -----------------------------
// Small helpers
// -----------------------------

void HighlightNode(ListNode* nodePointer, NodeVisualState state)
{
    linkedList.ResetVisualStates();
    if (nodePointer != nullptr)
    {
        nodePointer->visualState = state;
    }
}

AlgorithmStep MakeStep(const std::string& descriptionText,
    int highlightedLineIndex,
    std::function<void()> applyStepFunction)
{
    return AlgorithmStep{ descriptionText, highlightedLineIndex, std::move(applyStepFunction) };
}

// -----------------------------
// UI helpers
// -----------------------------

bool IsPointInsideRectangle(Vector2 point, Rectangle rectangle)
{
    return CheckCollisionPointRec(point, rectangle);
}

bool IsButtonClicked(Rectangle buttonRectangle)
{
    Vector2 mousePosition = GetMousePosition();
    bool isHovered = IsPointInsideRectangle(mousePosition, buttonRectangle);
    bool isClicked = isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    return isClicked;
}

void DrawButton(Rectangle buttonRectangle, const std::string& buttonText, bool isDisabled = false)
{
    Vector2 mousePosition = GetMousePosition();
    bool isHovered = IsPointInsideRectangle(mousePosition, buttonRectangle);

    Color buttonColour = COLOUR_BUTTON_NORMAL;
    if (isDisabled)
    {
        buttonColour = COLOUR_BUTTON_DISABLED;
    }
    else if (isHovered)
    {
        buttonColour = COLOUR_BUTTON_HOVER;
    }

    // Rounded button
    DrawRectangleRounded(buttonRectangle, 0.25f, 8, buttonColour);
    DrawRectangleRoundedLinesEx(buttonRectangle, 0.25f, 8, 2.0f, Fade(BLACK, 0.6f));

    int fontSize = 18;
    int textWidth = MeasureText(buttonText.c_str(), fontSize);
    float textX = buttonRectangle.x + (buttonRectangle.width - static_cast<float>(textWidth)) / 2.0f;
    float textY = buttonRectangle.y + (buttonRectangle.height - static_cast<float>(fontSize)) / 2.0f;

    DrawText(buttonText.c_str(), static_cast<int>(textX), static_cast<int>(textY), fontSize, RAYWHITE);
}

// Simple integer text box (only allows digits and optional minus sign at start)
// This now *only* handles input; drawing is done once in main.
void HandleTextBoxInput(const Rectangle& textBoxRectangle,
    bool& isActive,
    std::string& textValue,
    bool& shouldClearOnClick)
{
    Vector2 mousePosition = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        bool isInside = IsPointInsideRectangle(mousePosition, textBoxRectangle);

        if (isInside)
        {
            // We’re (re)focusing the box
            if (!isActive || shouldClearOnClick)
            {
                textValue.clear();          // clear old value
                shouldClearOnClick = false; // only clear once
            }
            isActive = true;
        }
        else
        {
            isActive = false;
        }
    }

    if (isActive)
    {
        int key = GetCharPressed();
        while (key > 0)
        {
            if ((key >= '0' && key <= '9') || (key == '-' && textValue.empty()))
            {
                textValue.push_back(static_cast<char>(key));
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) && !textValue.empty())
        {
            textValue.pop_back();
        }
    }
}

// -----------------------------
// Drawing helpers
// -----------------------------

void DrawArrow(Vector2 startPosition, Vector2 endPosition)
{
    DrawLineEx(startPosition, endPosition, 3.0f, BLACK);

    Vector2 directionVector = { endPosition.x - startPosition.x, endPosition.y - startPosition.y };
    float length = std::sqrt(directionVector.x * directionVector.x + directionVector.y * directionVector.y);
    if (length == 0.0f)
    {
        return;
    }

    directionVector.x /= length;
    directionVector.y /= length;

    float arrowSize = 10.0f;

    Vector2 leftWing = {
        endPosition.x - directionVector.x * arrowSize - directionVector.y * arrowSize * 0.5f,
        endPosition.y - directionVector.y * arrowSize + directionVector.x * arrowSize * 0.5f
    };

    Vector2 rightWing = {
        endPosition.x - directionVector.x * arrowSize + directionVector.y * arrowSize * 0.5f,
        endPosition.y - directionVector.y * arrowSize - directionVector.x * arrowSize * 0.5f
    };

    DrawTriangle(endPosition, leftWing, rightWing, BLACK);
}

void DrawNodeRectangle(ListNode* nodePointer, float nodeWidth, float nodeHeight)
{
    if (nodePointer == nullptr)
    {
        return;
    }

    Color fillColour = COLOUR_NODE_NORMAL;

    switch (nodePointer->visualState)
    {
    case NodeVisualState::Normal:   fillColour = COLOUR_NODE_NORMAL;  break;
    case NodeVisualState::Current:  fillColour = COLOUR_NODE_CURRENT; break;
    case NodeVisualState::Found:    fillColour = COLOUR_NODE_FOUND;   break;
    case NodeVisualState::ToDelete: fillColour = COLOUR_NODE_DELETE;  break;
    }

    Rectangle nodeRectangle = {
        nodePointer->position.x,
        nodePointer->position.y,
        nodeWidth,
        nodeHeight
    };

    // Rounded node
    DrawRectangleRounded(nodeRectangle, 0.15f, 6, fillColour);
    DrawRectangleRoundedLinesEx(nodeRectangle, 0.15f, 6, 2.0f, BLACK);

    // Split into value and pointer sections
    float valueWidth = nodeRectangle.width * 0.6f;
    DrawLineEx(
        { nodeRectangle.x + valueWidth, nodeRectangle.y + 4.0f },
        { nodeRectangle.x + valueWidth, nodeRectangle.y + nodeRectangle.height - 4.0f },
        2.0f,
        BLACK
    );

    // Draw value text
    std::string valueText = std::to_string(nodePointer->value);
    int fontSize = 20;
    int textWidth = MeasureText(valueText.c_str(), fontSize);
    float textX = nodeRectangle.x + (valueWidth - static_cast<float>(textWidth)) / 2.0f;
    float textY = nodeRectangle.y + (nodeRectangle.height - static_cast<float>(fontSize)) / 2.0f;
    DrawText(valueText.c_str(), static_cast<int>(textX), static_cast<int>(textY), fontSize, COLOUR_TEXT_MAIN);
}

// New helper: draw arrow from a node's right edge to a NULL box,
// matching the original (pre-refactor) geometry.
void DrawNullArrowForNode(ListNode* nodePointer,
    float nodeWidth,
    float nodeHeight,
    float horizontalSpacing)
{
    if (nodePointer == nullptr)
    {
        return;
    }

    float nullBoxWidth = 50.0f;
    float nullBoxHeight = 30.0f;

    Vector2 startPosition = {
        nodePointer->position.x + nodeWidth,
        nodePointer->position.y + nodeHeight / 2.0f
    };

    Vector2 nullBoxPosition = {
        nodePointer->position.x + nodeWidth + horizontalSpacing * 0.5f,
        nodePointer->position.y + (nodeHeight - nullBoxHeight) / 2.0f
    };

    Vector2 endPosition = {
        nullBoxPosition.x,
        nullBoxPosition.y + nullBoxHeight / 2.0f
    };

    DrawArrow(startPosition, endPosition);

    Rectangle nullRectangle = {
        nullBoxPosition.x,
        nullBoxPosition.y,
        nullBoxWidth,
        nullBoxHeight
    };

    DrawRectangleLinesEx(nullRectangle, 2.0f, BLACK);
    DrawText("NULL",
        static_cast<int>(nullRectangle.x + 5),
        static_cast<int>(nullRectangle.y + 7),
        18,
        COLOUR_TEXT_MAIN);
}

// Forward-declare so we can call after DrawLinkedList
void DrawHeadTailPointers(float nodeWidth, float nodeHeight);

void DrawLinkedList(float nodeWidth, float nodeHeight, float horizontalSpacing)
{
    ListNode* currentNodePointer = linkedList.GetHeadPointer();

    // Draw list nodes and their normal arrows
    while (currentNodePointer != nullptr)
    {
        // Optionally hide the node in the main row if it is being lifted during deletion
        if (!(currentOperationType == OperationType::Delete &&
            shouldHideDeletedNodeInList &&
            currentNodePointer == deleteCurrentNodePointer))
        {
            DrawNodeRectangle(currentNodePointer, nodeWidth, nodeHeight);
        }

        if (currentNodePointer->nextPointer != nullptr)
        {
            bool isPrevToDeleteEdgeHidden =
                (currentOperationType == OperationType::Delete &&
                    shouldHidePrevToCurrentArrow &&
                    deletePreviousNodePointer == currentNodePointer &&
                    currentNodePointer->nextPointer == deleteCurrentNodePointer);

            if (!isPrevToDeleteEdgeHidden)
            {
                Vector2 startPosition = {
                    currentNodePointer->position.x + nodeWidth,
                    currentNodePointer->position.y + nodeHeight / 2.0f
                };

                Vector2 endPosition = {
                    currentNodePointer->nextPointer->position.x,
                    currentNodePointer->nextPointer->position.y + nodeHeight / 2.0f
                };

                DrawArrow(startPosition, endPosition);
            }
        }
        else
        {
            // For tail insertion animation, we may temporarily hide tail -> NULL
            bool isTailBeingRewired =
                (currentOperationType == OperationType::InsertTail &&
                    tailNodeBeforeInsertPointer == currentNodePointer &&
                    shouldHideTailNullArrow);

            if (!isTailBeingRewired)
            {
                // Draw tail -> NULL exactly as in the original version
                DrawNullArrowForNode(currentNodePointer, nodeWidth, nodeHeight, horizontalSpacing);
            }
        }

        currentNodePointer = currentNodePointer->nextPointer;
    }

    // Draw the temporary new node for current insert operation (if any)
    if (newNodeForInsertPointer != nullptr &&
        (currentOperationType == OperationType::InsertHead ||
            currentOperationType == OperationType::InsertTail))
    {
        DrawNodeRectangle(newNodeForInsertPointer, nodeWidth, nodeHeight);

        // For InsertHead we visually connect the temporary new node to the old head
        if (currentOperationType == OperationType::InsertHead &&
            newNodeForInsertPointer->nextPointer != nullptr)
        {
            Vector2 startPosition = {
                newNodeForInsertPointer->position.x + nodeWidth,
                newNodeForInsertPointer->position.y + nodeHeight / 2.0f
            };

            Vector2 endPosition = {
                newNodeForInsertPointer->nextPointer->position.x,
                newNodeForInsertPointer->nextPointer->position.y + nodeHeight / 2.0f
            };

            DrawArrow(startPosition, endPosition);
        }

        // For InsertTail, draw the animated pointer rewiring
        if (currentOperationType == OperationType::InsertTail &&
            tailNodeBeforeInsertPointer != nullptr)
        {
            // Step: tail.next = newNode (visual arrow)
            if (shouldDrawTailToNewArrow && newNodeForInsertPointer != nullptr)
            {
                Vector2 startPosition = {
                    tailNodeBeforeInsertPointer->position.x + nodeWidth,
                    tailNodeBeforeInsertPointer->position.y + nodeHeight / 2.0f
                };

                Vector2 endPosition = {
                    newNodeForInsertPointer->position.x,
                    newNodeForInsertPointer->position.y + nodeHeight / 2.0f
                };

                DrawArrow(startPosition, endPosition);
            }

            // Step: newNode.next = NULL (visual arrow + NULL box)
            if (shouldDrawNewNodeToNullArrow && newNodeForInsertPointer != nullptr)
            {
                // Arrow from newNode to NULL, same style as tail -> NULL
                DrawNullArrowForNode(newNodeForInsertPointer, nodeWidth, nodeHeight, horizontalSpacing);
            }
        }
    }

    // For Delete operation: draw the lifted deleted node (if any)
    if (currentOperationType == OperationType::Delete &&
        shouldDrawLiftedDeletedNode &&
        deleteCurrentNodePointer != nullptr)
    {
        Vector2 originalPosition = deleteCurrentNodePointer->position;
        deleteCurrentNodePointer->position = liftedDeletedNodePosition;
        DrawNodeRectangle(deleteCurrentNodePointer, nodeWidth, nodeHeight);
        deleteCurrentNodePointer->position = originalPosition;
    }

    // For Delete operation: draw the new previous -> next arrow (if any)
    if (currentOperationType == OperationType::Delete &&
        shouldDrawPrevToNextArrow &&
        deletePreviousNodePointer != nullptr &&
        deleteCurrentNodePointer != nullptr &&
        deleteCurrentNodePointer->nextPointer != nullptr)
    {
        Vector2 startPosition = {
            deletePreviousNodePointer->position.x + nodeWidth,
            deletePreviousNodePointer->position.y + nodeHeight / 2.0f
        };

        Vector2 endPosition = {
            deleteCurrentNodePointer->nextPointer->position.x,
            deleteCurrentNodePointer->nextPointer->position.y + nodeHeight / 2.0f
        };

        DrawArrow(startPosition, endPosition);
    }

    // After drawing nodes and arrows, draw head/tail pointers
    DrawHeadTailPointers(nodeWidth, nodeHeight);
}

// Draw "head" and "tail" pointers as labels + arrows
void DrawHeadTailPointers(float nodeWidth, float nodeHeight)
{
    ListNode* headPointer = linkedList.GetHeadPointer();

    // ----- EMPTY LIST: just show head = NULL / tail = NULL text -----
    if (headPointer == nullptr)
    {
        DrawText("head = NULL", 80, 260, 20, COLOUR_TEXT_MUTED);
        DrawText("tail = NULL", 80, 290, 20, COLOUR_TEXT_MUTED);
        return;
    }

    ListNode* tailPointer = linkedList.GetTailPointer();
    if (tailPointer == nullptr)
    {
        // Fallback (should not normally happen)
        tailPointer = headPointer;
        while (tailPointer->nextPointer != nullptr)
        {
            tailPointer = tailPointer->nextPointer;
        }
    }

    int fontSize = 20;

    auto getBasePosition = [&](ListNode* nodePointer) -> Vector2
        {
            if (currentOperationType == OperationType::Delete &&
                shouldDrawLiftedDeletedNode &&
                deleteCurrentNodePointer == nodePointer)
            {
                return liftedDeletedNodePosition;
            }
            return nodePointer->position;
        };

    Vector2 headBasePosition = getBasePosition(headPointer);
    Vector2 tailBasePosition = getBasePosition(tailPointer);

    Vector2 headTargetPosition = {
        headBasePosition.x + nodeWidth / 2.0f,
        headBasePosition.y
    };

    Vector2 tailTargetPosition = {
        tailBasePosition.x + nodeWidth / 2.0f,
        tailBasePosition.y + nodeHeight
    };

    // HEAD above first node
    const char* headLabel = "head";
    int headLabelWidth = MeasureText(headLabel, fontSize);
    Vector2 headLabelPosition = {
        headTargetPosition.x - headLabelWidth / 2.0f,
        headTargetPosition.y - 50.0f
    };

    DrawText(headLabel,
        static_cast<int>(headLabelPosition.x),
        static_cast<int>(headLabelPosition.y),
        fontSize,
        COLOUR_TEXT_MAIN);

    Vector2 headArrowStart = {
        headTargetPosition.x,
        headLabelPosition.y + static_cast<float>(fontSize) + 4.0f
    };
    DrawArrow(headArrowStart, headTargetPosition);

    // TAIL below last node
    const char* tailLabel = "tail";
    int tailLabelWidth = MeasureText(tailLabel, fontSize);
    Vector2 tailLabelPosition = {
        tailTargetPosition.x - tailLabelWidth / 2.0f,
        tailTargetPosition.y + 30.0f
    };

    DrawText(tailLabel,
        static_cast<int>(tailLabelPosition.x),
        static_cast<int>(tailLabelPosition.y),
        fontSize,
        COLOUR_TEXT_MAIN);

    Vector2 tailArrowStart = {
        tailTargetPosition.x,
        tailLabelPosition.y
    };
    DrawArrow(tailArrowStart, tailTargetPosition);
}

// -----------------------------
// Pseudocode management
// -----------------------------

std::vector<std::string> GetPseudocodeLinesForOperation(OperationType operationType)
{
    if (operationType == OperationType::InsertHead)
    {
        return {
            "1. newNode = createNode(value)",
            "2. newNode.next = head",
            "3. head = newNode"
        };
    }
    else if (operationType == OperationType::InsertTail)
    {
        // Proper tail-pointer version
        return {
            "1. newNode = createNode(value)",
            "2. newNode.next = NULL",
            "3. if head == NULL then",
            "4.     head = newNode",
            "5.     tail = newNode",
            "6. else",
            "7.     tail.next = newNode",
            "8.     tail = newNode"
        };
    }
    else if (operationType == OperationType::Search)
    {
        return {
            "1. current = head",
            "2. while current != NULL",
            "3.     if current.value == target",
            "4.         return FOUND",
            "5.     current = current.next",
            "6. return NOT_FOUND"
        };
    }
    else if (operationType == OperationType::Delete)
    {
        return {
            "1. current = head",
            "2. previous = NULL",
            "3. while current != NULL and current.value != target",
            "4.     previous = current",
            "5.     current = current.next",
            "6. if current == NULL",
            "7.     return NOT_FOUND",
            "8. if previous == NULL",
            "9.     head = current.next",
            "10. else",
            "11.     previous.next = current.next",
            "12. delete current"
        };
    }

    return {};
}

void DrawPseudocodePanel(int panelX, int panelY, int panelWidth, int panelHeight,
    OperationType operationType, int highlightedLineIndex)
{
    Rectangle panelRectangle = { static_cast<float>(panelX), static_cast<float>(panelY),
                                 static_cast<float>(panelWidth), static_cast<float>(panelHeight) };

    // Panel background + border
    DrawRectangleRounded(panelRectangle, 0.03f, 4, COLOUR_PANEL_BACKGROUND);
    DrawRectangleRoundedLinesEx(panelRectangle, 0.03f, 4, 2.0f, COLOUR_PANEL_BORDER);

    int fontSize = 18;
    int margin = 14;
    int lineSpacing = 6;

    // Title
    const char* titleText = "Pseudocode";

    DrawText(titleText,
        panelX + margin,
        panelY + margin - 4,
        fontSize,
        COLOUR_TEXT_MAIN);

    // Horizontal line under title
    DrawLine(panelX + margin,
        panelY + margin + fontSize,
        panelX + panelWidth - margin,
        panelY + margin + fontSize,
        Fade(COLOUR_PANEL_BORDER, 0.7f));

    std::vector<std::string> pseudocodeLines = GetPseudocodeLinesForOperation(operationType);

    int lineY = panelY + margin + fontSize + 10;

    for (int lineIndex = 0; lineIndex < static_cast<int>(pseudocodeLines.size()); lineIndex++)
    {
        const std::string& lineText = pseudocodeLines[lineIndex];

        if (lineIndex == highlightedLineIndex)
        {
            int textWidth = MeasureText(lineText.c_str(), fontSize);
            DrawRectangle(panelX + margin - 4, lineY - 2, textWidth + 8, fontSize + 4,
                Color{ 255, 249, 196, 255 }); // soft yellow highlight
        }

        DrawText(lineText.c_str(), panelX + margin, lineY, fontSize, COLOUR_TEXT_MAIN);
        lineY += fontSize + lineSpacing;
    }

    if (pseudocodeLines.empty())
    {
        DrawText("No operation selected.",
            panelX + margin,
            lineY,
            fontSize,
            COLOUR_TEXT_MUTED);
    }
}

// -----------------------------
// Algorithm step management
// -----------------------------

void ResetDeleteFlags()
{
    deleteCurrentNodePointer = nullptr;
    deletePreviousNodePointer = nullptr;
    shouldHidePrevToCurrentArrow = false;
    shouldDrawPrevToNextArrow = false;
    shouldHideDeletedNodeInList = false;
    shouldDrawLiftedDeletedNode = false;
    liftedDeletedNodePosition = { 0.0f, 0.0f };
}

void ResetTailInsertFlags()
{
    tailNodeBeforeInsertPointer = nullptr;
    shouldHideTailNullArrow = false;
    shouldDrawTailToNewArrow = false;
    shouldDrawNewNodeToNullArrow = false;
}

void ClearCurrentOperation()
{
    currentSteps.clear();
    currentStepIndex = -1;
    isOperationInProgress = false;
    currentOperationType = OperationType::None;

    if (newNodeForInsertPointer != nullptr)
    {
        delete newNodeForInsertPointer;
        newNodeForInsertPointer = nullptr;
    }

    ResetTailInsertFlags();
    ResetDeleteFlags();

    stepTimerSeconds = 0.0f;
    linkedList.ResetVisualStates();
}

void AdvanceToNextStep()
{
    if (!isOperationInProgress)
    {
        return;
    }

    currentStepIndex++;

    if (currentStepIndex >= 0 && currentStepIndex < static_cast<int>(currentSteps.size()))
    {
        if (currentSteps[currentStepIndex].applyStepFunction)
        {
            currentSteps[currentStepIndex].applyStepFunction();
        }
    }
    else
    {
        isOperationInProgress = false;
        linkedList.ResetVisualStates();
        currentOperationType = OperationType::None;

        if (newNodeForInsertPointer != nullptr)
        {
            delete newNodeForInsertPointer;
            newNodeForInsertPointer = nullptr;
        }

        ResetTailInsertFlags();
        ResetDeleteFlags();
        stepTimerSeconds = 0.0f;
    }
}

// Helper: start a new operation and immediately show the first step
void StartOperationPlayback()
{
    currentStepIndex = -1;
    isOperationInProgress = true;
    stepTimerSeconds = 0.0f;
    AdvanceToNextStep(); // show step 0 immediately
}

// Insert at head: build steps but do not change the real list yet
void BuildInsertHeadSteps(int value)
{
    ClearCurrentOperation();
    currentOperationType = OperationType::InsertHead;
    pendingInsertValue = value;

    newNodeForInsertPointer = new ListNode(value);
    newNodeForInsertPointer->visualState = NodeVisualState::Current;

    newNodeForInsertPointer->position = {
        LIST_START_X,
        LIST_START_Y - LIFT_HEIGHT
    };

    ListNode* headPointerAtStart = linkedList.GetHeadPointer();

    currentSteps.clear();

    // Step 0: newNode = createNode(value)
    currentSteps.push_back(MakeStep(
        "Create a new node and store the value.",
        0,
        []()
        {
            if (newNodeForInsertPointer != nullptr)
            {
                newNodeForInsertPointer->visualState = NodeVisualState::Current;
            }
        }));

    // Step 1: newNode.next = head
    currentSteps.push_back(MakeStep(
        "Set newNode.next to point to the current head node.",
        1,
        [headPointerAtStart]()
        {
            if (newNodeForInsertPointer != nullptr)
            {
                newNodeForInsertPointer->nextPointer = headPointerAtStart;
            }
        }));

    // Step 2: head = newNode (apply logical change)
    currentSteps.push_back(MakeStep(
        "Update head so that it points to the new node.",
        2,
        []()
        {
            linkedList.InsertAtHeadImmediate(pendingInsertValue);
            linkedList.UpdateLayout(LIST_START_X, LIST_START_Y, NODE_WIDTH, 0.0f, HORIZONTAL_SPACING);
            linkedList.ResetVisualStates();

            ListNode* headPointer = linkedList.GetHeadPointer();
            HighlightNode(headPointer, NodeVisualState::Current);

            if (newNodeForInsertPointer != nullptr)
            {
                delete newNodeForInsertPointer;
                newNodeForInsertPointer = nullptr;
            }
        }));

    StartOperationPlayback();
}

// Insert at tail: proper tail-pointer algorithm
void BuildInsertTailSteps(int value)
{
    ClearCurrentOperation();
    currentOperationType = OperationType::InsertTail;
    pendingInsertValue = value;

    ListNode* headPointerAtStart = linkedList.GetHeadPointer();
    bool isListEmpty = (headPointerAtStart == nullptr);
    ListNode* tailPointerAtStart = linkedList.GetTailPointer();

    newNodeForInsertPointer = new ListNode(value);
    newNodeForInsertPointer->visualState = NodeVisualState::Current;

    // Position the new node below the list (near tail if exists)
    float newNodeX = isListEmpty
        ? LIST_START_X
        : (tailPointerAtStart != nullptr ? tailPointerAtStart->position.x : LIST_START_X);

    newNodeForInsertPointer->position = {
        newNodeX,
        LIST_START_Y + LIFT_HEIGHT
    };

    currentSteps.clear();
    ResetTailInsertFlags();

    // Step 0: newNode = createNode(value)
    currentSteps.push_back(MakeStep(
        "Create a new node and store the value.",
        0,
        []()
        {
            if (newNodeForInsertPointer != nullptr)
            {
                newNodeForInsertPointer->visualState = NodeVisualState::Current;
            }
        }));

    // Step 1: newNode.next = NULL
    currentSteps.push_back(MakeStep(
        "Set newNode.next to NULL (it will be the last node).",
        1,
        [isListEmpty]()
        {
            linkedList.ResetVisualStates();

            if (!isListEmpty)
            {
                // For non-empty list, we will draw newNode -> NULL later
                shouldDrawNewNodeToNullArrow = true;
            }

            if (newNodeForInsertPointer != nullptr)
            {
                newNodeForInsertPointer->visualState = NodeVisualState::Current;
            }
        }));

    if (isListEmpty)
    {
        // ----- Empty list branch -----
        currentSteps.push_back(MakeStep(
            "Since head is NULL, the new node will become both head and tail.",
            2,
            []()
            {
                HighlightNode(newNodeForInsertPointer, NodeVisualState::Current);
            }));

        currentSteps.push_back(MakeStep(
            "Set head = newNode and tail = newNode so the list has one element.",
            4,
            []()
            {
                linkedList.InsertAtTailImmediate(pendingInsertValue); // also sets tail
                linkedList.UpdateLayout(LIST_START_X, LIST_START_Y, NODE_WIDTH, 0.0f, HORIZONTAL_SPACING);
                linkedList.ResetVisualStates();

                ListNode* headPointer = linkedList.GetHeadPointer();
                HighlightNode(headPointer, NodeVisualState::Current);

                if (newNodeForInsertPointer != nullptr)
                {
                    delete newNodeForInsertPointer;
                    newNodeForInsertPointer = nullptr;
                }

                ResetTailInsertFlags();
            }));
    }
    else
    {
        // ----- Non-empty list branch -----
        tailNodeBeforeInsertPointer = tailPointerAtStart;

        currentSteps.push_back(MakeStep(
            "head is not NULL, so we go to the 'else' branch.",
            5,
            [tailPointerAtStart]()
            {
                HighlightNode(tailPointerAtStart, NodeVisualState::Current);
            }));

        currentSteps.push_back(MakeStep(
            "Set tail.next = newNode (connect the old tail to the new node).",
            6,
            []()
            {
                linkedList.ResetVisualStates();

                shouldHideTailNullArrow = true;
                shouldDrawTailToNewArrow = true;
                shouldDrawNewNodeToNullArrow = true;

                HighlightNode(tailNodeBeforeInsertPointer, NodeVisualState::Current);
                HighlightNode(newNodeForInsertPointer, NodeVisualState::Current);
            }));

        currentSteps.push_back(MakeStep(
            "Update tail so it points to the new node. The list now ends at newNode.",
            7,
            []()
            {
                linkedList.InsertAtTailImmediate(pendingInsertValue);
                linkedList.UpdateLayout(LIST_START_X, LIST_START_Y, NODE_WIDTH, 0.0f, HORIZONTAL_SPACING);
                linkedList.ResetVisualStates();

                ListNode* tailPointer = linkedList.GetTailPointer();
                HighlightNode(tailPointer, NodeVisualState::Current);

                if (newNodeForInsertPointer != nullptr)
                {
                    delete newNodeForInsertPointer;
                    newNodeForInsertPointer = nullptr;
                }

                ResetTailInsertFlags();
            }));
    }

    StartOperationPlayback();
}

// Search: build steps for linear search
void BuildSearchSteps(int targetValue)
{
    ClearCurrentOperation();
    currentOperationType = OperationType::Search;

    currentSteps.clear();

    ListNode* headPointer = linkedList.GetHeadPointer();

    // Step 0: current = head
    currentSteps.push_back(MakeStep(
        "Set current to the head of the list.",
        0,
        [headPointer]()
        {
            HighlightNode(headPointer, NodeVisualState::Current);
        }));

    // If list is empty, immediately show NOT_FOUND
    if (headPointer == nullptr)
    {
        currentSteps.push_back(MakeStep(
            "The list is empty. current is NULL, value not found.",
            5,
            []()
            {
                linkedList.ResetVisualStates();
            }));
        StartOperationPlayback();
        return;
    }

    // Traverse each node
    ListNode* traversalNodePointer = headPointer;
    bool isValueFound = false;

    while (traversalNodePointer != nullptr)
    {
        ListNode* nodeToCheckPointer = traversalNodePointer;

        // Step: while current != NULL (enter loop with this current)
        currentSteps.push_back(MakeStep(
            "Check that current is not NULL (loop continues).",
            1,
            [nodeToCheckPointer]()
            {
                HighlightNode(nodeToCheckPointer, NodeVisualState::Current);
            }));

        if (nodeToCheckPointer->value == targetValue)
        {
            currentSteps.push_back(MakeStep(
                "Compare current.value with target (" + std::to_string(targetValue) +
                "). They are equal, value found.",
                2,
                [nodeToCheckPointer]()
                {
                    HighlightNode(nodeToCheckPointer, NodeVisualState::Found);
                }));

            currentSteps.push_back(MakeStep(
                "Return FOUND. The search is complete.",
                3,
                []()
                {
                    // Node remains green.
                }));

            isValueFound = true;
            break;
        }
        else
        {
            currentSteps.push_back(MakeStep(
                "Compare current.value with target (" + std::to_string(targetValue) +
                "). They are not equal.",
                2,
                [nodeToCheckPointer]()
                {
                    HighlightNode(nodeToCheckPointer, NodeVisualState::Current);
                }));

            currentSteps.push_back(MakeStep(
                "Move current to the next node (current = current.next).",
                4,
                []()
                {
                    linkedList.ResetVisualStates();
                }));
        }

        traversalNodePointer = traversalNodePointer->nextPointer;
    }

    if (!isValueFound)
    {
        currentSteps.push_back(MakeStep(
            "current is NULL, value not found in the list.",
            5,
            []()
            {
                linkedList.ResetVisualStates();
            }));
    }

    StartOperationPlayback();
}

// Delete: build steps for delete-by-value
void BuildDeleteSteps(int targetValue)
{
    ClearCurrentOperation();
    currentOperationType = OperationType::Delete;

    currentSteps.clear();
    ResetDeleteFlags();

    ListNode* headPointer = linkedList.GetHeadPointer();
    ListNode* traversalCurrentPointer = headPointer;
    ListNode* traversalPreviousPointer = nullptr;

    // Step 0: current = head
    currentSteps.push_back(MakeStep(
        "Set current to head and previous to NULL.",
        0,
        [headPointer]()
        {
            HighlightNode(headPointer, NodeVisualState::Current);
        }));

    // If list is empty
    if (headPointer == nullptr)
    {
        currentSteps.push_back(MakeStep(
            "The list is empty. current is NULL, nothing to delete.",
            6,
            []()
            {
                linkedList.ResetVisualStates();
            }));

        StartOperationPlayback();
        return;
    }

    // Explicitly set previous = NULL (matches pseudocode line 2)
    currentSteps.push_back(MakeStep(
        "Set previous to NULL (we are at the head).",
        1,
        [headPointer]()
        {
            HighlightNode(headPointer, NodeVisualState::Current);
        }));

    // Build traversal steps
    while (traversalCurrentPointer != nullptr && traversalCurrentPointer->value != targetValue)
    {
        ListNode* nodeBeforeUpdatePointer = traversalCurrentPointer;
        ListNode* previousBeforeUpdatePointer = traversalPreviousPointer;

        currentSteps.push_back(MakeStep(
            "Check current != NULL and current.value != target (loop continues).",
            2,
            [nodeBeforeUpdatePointer]()
            {
                HighlightNode(nodeBeforeUpdatePointer, NodeVisualState::Current);
            }));

        currentSteps.push_back(MakeStep(
            "Move previous to current and current to current.next.",
            4,
            [nodeBeforeUpdatePointer, previousBeforeUpdatePointer]()
            {
                HighlightNode(nodeBeforeUpdatePointer, NodeVisualState::Current);
                if (previousBeforeUpdatePointer != nullptr)
                {
                    previousBeforeUpdatePointer->visualState = NodeVisualState::Normal;
                }
            }));

        traversalPreviousPointer = traversalCurrentPointer;
        traversalCurrentPointer = traversalCurrentPointer->nextPointer;
    }

    // After loop: either current is NULL (not found) or current.value == target
    if (traversalCurrentPointer == nullptr)
    {
        currentSteps.push_back(MakeStep(
            "current is NULL, so the value was not found.",
            6,
            []()
            {
                linkedList.ResetVisualStates();
            }));

        currentSteps.push_back(MakeStep(
            "Return NOT_FOUND (no node with the target value exists).",
            7,
            []()
            {
                linkedList.ResetVisualStates();
            }));
    }
    else
    {
        // We found the node to delete
        deleteCurrentNodePointer = traversalCurrentPointer;
        deletePreviousNodePointer = traversalPreviousPointer;

        currentSteps.push_back(MakeStep(
            "current is the node to delete (current.value == target). Mark it to delete (red).",
            6,
            []()
            {
                linkedList.ResetVisualStates();
                if (deleteCurrentNodePointer != nullptr)
                {
                    deleteCurrentNodePointer->visualState = NodeVisualState::ToDelete;
                }
                if (deletePreviousNodePointer != nullptr)
                {
                    deletePreviousNodePointer->visualState = NodeVisualState::Current;
                }
            }));

        if (deletePreviousNodePointer == nullptr)
        {
            // Deleting the head node
            currentSteps.push_back(MakeStep(
                "previous is NULL, so we are deleting the head node (head = current.next).",
                8,
                []()
                {
                    linkedList.ResetVisualStates();
                    if (deleteCurrentNodePointer != nullptr)
                    {
                        deleteCurrentNodePointer->visualState = NodeVisualState::ToDelete;
                    }
                }));

            currentSteps.push_back(MakeStep(
                "Lift the head node out of the list before deleting it.",
                9,
                []()
                {
                    linkedList.ResetVisualStates();
                    if (deleteCurrentNodePointer != nullptr)
                    {
                        deleteCurrentNodePointer->visualState = NodeVisualState::ToDelete;
                        shouldHideDeletedNodeInList = true;
                        shouldDrawLiftedDeletedNode = true;
                        liftedDeletedNodePosition = {
                            deleteCurrentNodePointer->position.x,
                            deleteCurrentNodePointer->position.y - LIFT_HEIGHT
                        };
                    }
                }));

            currentSteps.push_back(MakeStep(
                "Set head = current.next and delete the node from the list.",
                9,
                []()
                {
                    linkedList.DeleteNodeWithPreviousImmediate(nullptr, deleteCurrentNodePointer);
                    deleteCurrentNodePointer = nullptr;

                    linkedList.UpdateLayout(LIST_START_X, LIST_START_Y, NODE_WIDTH, 0.0f, HORIZONTAL_SPACING);
                    linkedList.ResetVisualStates();

                    ResetDeleteFlags();
                }));
        }
        else
        {
            // Deleting a middle or tail node
            currentSteps.push_back(MakeStep(
                "Disconnect previous.next from current (remove the arrow to the node being deleted).",
                11,
                []()
                {
                    linkedList.ResetVisualStates();

                    shouldHidePrevToCurrentArrow = true;
                    shouldDrawPrevToNextArrow = false;
                    shouldHideDeletedNodeInList = false;
                    shouldDrawLiftedDeletedNode = false;

                    if (deletePreviousNodePointer != nullptr)
                    {
                        deletePreviousNodePointer->visualState = NodeVisualState::Current;
                    }
                    if (deleteCurrentNodePointer != nullptr)
                    {
                        deleteCurrentNodePointer->visualState = NodeVisualState::ToDelete;
                    }
                }));

            currentSteps.push_back(MakeStep(
                "Lift the node being deleted out of the list.",
                11,
                []()
                {
                    linkedList.ResetVisualStates();

                    shouldHidePrevToCurrentArrow = true;
                    shouldHideDeletedNodeInList = true;
                    shouldDrawLiftedDeletedNode = true;

                    if (deleteCurrentNodePointer != nullptr)
                    {
                        deleteCurrentNodePointer->visualState = NodeVisualState::ToDelete;
                        liftedDeletedNodePosition = {
                            deleteCurrentNodePointer->position.x,
                            deleteCurrentNodePointer->position.y - LIFT_HEIGHT
                        };
                    }
                    if (deletePreviousNodePointer != nullptr)
                    {
                        deletePreviousNodePointer->visualState = NodeVisualState::Current;
                    }
                }));

            currentSteps.push_back(MakeStep(
                "Connect previous.next to current.next so the list bypasses the deleted node.",
                11,
                []()
                {
                    linkedList.ResetVisualStates();

                    shouldHidePrevToCurrentArrow = true;
                    shouldHideDeletedNodeInList = true;
                    shouldDrawLiftedDeletedNode = true;

                    if (deleteCurrentNodePointer != nullptr &&
                        deleteCurrentNodePointer->nextPointer != nullptr)
                    {
                        shouldDrawPrevToNextArrow = true;
                    }
                    else
                    {
                        // tail deletion: new tail will later show arrow to NULL
                        shouldDrawPrevToNextArrow = false;
                    }

                    if (deletePreviousNodePointer != nullptr)
                    {
                        deletePreviousNodePointer->visualState = NodeVisualState::Current;
                    }
                    if (deleteCurrentNodePointer != nullptr)
                    {
                        deleteCurrentNodePointer->visualState = NodeVisualState::ToDelete;
                    }
                }));

            currentSteps.push_back(MakeStep(
                "Update the list pointers and delete the node from memory.",
                12,
                []()
                {
                    linkedList.DeleteNodeWithPreviousImmediate(deletePreviousNodePointer, deleteCurrentNodePointer);
                    deleteCurrentNodePointer = nullptr;

                    linkedList.UpdateLayout(LIST_START_X, LIST_START_Y, NODE_WIDTH, 0.0f, HORIZONTAL_SPACING);
                    linkedList.ResetVisualStates();

                    ResetDeleteFlags();
                }));
        }
    }

    StartOperationPlayback();
}

// -----------------------------
// Main
// -----------------------------

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Singly Linked List Visualiser - Insert, Search, Delete");
    SetTargetFPS(60);

    std::string valueTextBoxValue;
    bool isValueTextBoxActive = false;
    bool shouldClearTextOnNextClick = false;

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        // -------------------------
        // Update
        // -------------------------

        HandleTextBoxInput(VALUE_TEXTBOX_RECT,
            isValueTextBoxActive,
            valueTextBoxValue,
            shouldClearTextOnNextClick);

        bool canStartNewOperation = !valueTextBoxValue.empty() && !isOperationInProgress;

        // Buttons
        Rectangle insertHeadButtonRectangle = { 240.0f, 120.0f, 160.0f, 40.0f };
        Rectangle insertTailButtonRectangle = { 420.0f, 120.0f, 160.0f, 40.0f };
        Rectangle searchButtonRectangle = { 600.0f, 120.0f, 160.0f, 40.0f };

        Rectangle deleteButtonRectangle = { 240.0f, 180.0f, 160.0f, 40.0f };
        Rectangle resetButtonRectangle = { 420.0f, 180.0f, 160.0f, 40.0f };

        if (IsButtonClicked(insertHeadButtonRectangle) && canStartNewOperation)
        {
            int parsedValue = std::atoi(valueTextBoxValue.c_str());
            linkedList.UpdateLayout(LIST_START_X, LIST_START_Y, NODE_WIDTH, NODE_HEIGHT, HORIZONTAL_SPACING);
            BuildInsertHeadSteps(parsedValue);
            shouldClearTextOnNextClick = true;
        }

        if (IsButtonClicked(insertTailButtonRectangle) && canStartNewOperation)
        {
            int parsedValue = std::atoi(valueTextBoxValue.c_str());
            linkedList.UpdateLayout(LIST_START_X, LIST_START_Y, NODE_WIDTH, NODE_HEIGHT, HORIZONTAL_SPACING);
            BuildInsertTailSteps(parsedValue);
            shouldClearTextOnNextClick = true;
        }

        if (IsButtonClicked(searchButtonRectangle) && canStartNewOperation)
        {
            int parsedValue = std::atoi(valueTextBoxValue.c_str());
            linkedList.UpdateLayout(LIST_START_X, LIST_START_Y, NODE_WIDTH, NODE_HEIGHT, HORIZONTAL_SPACING);
            BuildSearchSteps(parsedValue);
            shouldClearTextOnNextClick = true;
        }

        if (IsButtonClicked(deleteButtonRectangle) && canStartNewOperation)
        {
            int parsedValue = std::atoi(valueTextBoxValue.c_str());
            linkedList.UpdateLayout(LIST_START_X, LIST_START_Y, NODE_WIDTH, NODE_HEIGHT, HORIZONTAL_SPACING);
            BuildDeleteSteps(parsedValue);
            shouldClearTextOnNextClick = true;
        }

        if (IsButtonClicked(resetButtonRectangle))
        {
            linkedList.Clear();
            ClearCurrentOperation();
            valueTextBoxValue.clear();
            shouldClearTextOnNextClick = false;
        }

        // Auto-advance steps while an operation is in progress
        if (isOperationInProgress)
        {
            stepTimerSeconds += deltaTime;
            if (stepTimerSeconds >= STEP_INTERVAL_SECONDS)
            {
                stepTimerSeconds = 0.0f;
                AdvanceToNextStep();
            }
        }

        // Keep list layout updated (for non-animated positions)
        linkedList.UpdateLayout(LIST_START_X, LIST_START_Y, NODE_WIDTH, NODE_HEIGHT, HORIZONTAL_SPACING);

        // -------------------------
        // Draw
        // -------------------------

        BeginDrawing();
        ClearBackground(COLOUR_BACKGROUND);

        // Title + subtitle
        DrawText("Singly Linked List Visualiser", 40, 18, 30, COLOUR_TEXT_MAIN);
        DrawText("Insert at Head / Tail, Search, Delete", 40, 52, 20, COLOUR_TEXT_MUTED);

        DrawText("Value / Target:",
            static_cast<int>(VALUE_TEXTBOX_RECT.x),
            static_cast<int>(VALUE_TEXTBOX_RECT.y - 24),
            20,
            COLOUR_TEXT_MAIN);

        // Text box
        DrawRectangleRec(VALUE_TEXTBOX_RECT, WHITE);
        DrawRectangleLinesEx(VALUE_TEXTBOX_RECT, 2.0f, BLACK);
        DrawText(valueTextBoxValue.c_str(),
            static_cast<int>(VALUE_TEXTBOX_RECT.x + 8),
            static_cast<int>(VALUE_TEXTBOX_RECT.y + 10),
            20,
            COLOUR_TEXT_MAIN);

        DrawButton(insertHeadButtonRectangle, "Insert at Head", !canStartNewOperation);
        DrawButton(insertTailButtonRectangle, "Insert at Tail", !canStartNewOperation);
        DrawButton(searchButtonRectangle, "Search", !canStartNewOperation);

        DrawButton(deleteButtonRectangle, "Delete", !canStartNewOperation);
        DrawButton(resetButtonRectangle, "Reset List", false);

        DrawLinkedList(NODE_WIDTH, NODE_HEIGHT, HORIZONTAL_SPACING);

        int pseudocodePanelWidth = 420;
        int pseudocodePanelHeight = 340;
        int pseudocodePanelX = SCREEN_WIDTH - pseudocodePanelWidth - 40;
        int pseudocodePanelY = 80;

        int highlightedLineIndex = -1;
        std::string currentStepDescriptionText;

        if (isOperationInProgress &&
            currentStepIndex >= 0 &&
            currentStepIndex < static_cast<int>(currentSteps.size()))
        {
            highlightedLineIndex = currentSteps[currentStepIndex].highlightedPseudocodeLineIndex;
            currentStepDescriptionText = currentSteps[currentStepIndex].descriptionText;
        }

        DrawPseudocodePanel(pseudocodePanelX,
            pseudocodePanelY,
            pseudocodePanelWidth,
            pseudocodePanelHeight,
            currentOperationType,
            highlightedLineIndex);

        int descriptionFontSize = 18;
        int descriptionX = 40;
        int descriptionY = SCREEN_HEIGHT - 100;

        Rectangle descRect = { descriptionX - 10.0f, descriptionY - 10.0f, 1100.0f, 70.0f };
        DrawRectangleRounded(descRect, 0.05f, 4, COLOUR_PANEL_BACKGROUND);
        DrawRectangleRoundedLinesEx(descRect, 0.05f, 4, 2.0f, COLOUR_PANEL_BORDER);

        if (!currentStepDescriptionText.empty())
        {
            DrawText(currentStepDescriptionText.c_str(),
                descriptionX,
                descriptionY,
                descriptionFontSize,
                COLOUR_TEXT_MAIN);
        }
        else
        {
            DrawText("Enter a value, choose an operation, and watch the steps animate automatically.",
                descriptionX,
                descriptionY,
                descriptionFontSize,
                COLOUR_TEXT_MUTED);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
