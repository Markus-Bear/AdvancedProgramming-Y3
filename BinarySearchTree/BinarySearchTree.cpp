#include "raylib.h"
#include <string>
#include <vector>
#include <functional>
#include <cstdlib>
#include <cmath>
#include <queue>

// ============================================================================
// Visual theme colours
// ============================================================================

const Color COLOUR_BACKGROUND = { 245, 246, 250, 255 };
const Color COLOUR_PANEL_BACKGROUND = { 230, 233, 240, 255 };
const Color COLOUR_PANEL_BORDER = { 180, 185, 195, 255 };

const Color COLOUR_BUTTON_NORMAL = { 60, 63, 81, 255 };
const Color COLOUR_BUTTON_HOVER = { 90, 94, 115, 255 };
const Color COLOUR_BUTTON_DISABLED = { 120, 123, 140, 255 };

const Color COLOUR_NODE_NORMAL = { 255, 255, 255, 255 };
const Color COLOUR_NODE_CURRENT = { 255, 245, 157, 255 };
const Color COLOUR_NODE_FOUND = { 129, 199, 132, 255 };
const Color COLOUR_NODE_DELETE = { 239, 154, 154, 255 };

const Color COLOUR_TEXT_MAIN = BLACK;
const Color COLOUR_TEXT_MUTED = DARKGRAY;

// ============================================================================
// Layout constants
// ============================================================================

// Bigger window so controls don't overlap
const int   SCREEN_WIDTH = 1800;
const int   SCREEN_HEIGHT = 950;

const float NODE_RADIUS = 26.0f;
const float VERTICAL_SPACING = 90.0f;
const float INITIAL_OFFSET_RATIO = 0.35f; // kept in case needed later

const Rectangle VALUE_TEXTBOX_RECT = { 80.0f, 120.0f, 140.0f, 40.0f };

// Pseudocode panel
const int PSEUDOCODE_PANEL_WIDTH = 420;
const int PSEUDOCODE_PANEL_HEIGHT = 480;
const int PSEUDOCODE_PANEL_X = SCREEN_WIDTH - PSEUDOCODE_PANEL_WIDTH - 40;
const int PSEUDOCODE_PANEL_Y = 80;

// Tree drawing area (the grey panel)
// *** moved down so it no longer overlaps traversal buttons ***
const float TREE_AREA_X = 30.0f;
const float TREE_AREA_Y = 220.0f;
const float TREE_AREA_WIDTH = PSEUDOCODE_PANEL_X - TREE_AREA_X - 20.0f;
const float TREE_AREA_HEIGHT = SCREEN_HEIGHT - TREE_AREA_Y - 130.0f;

// Animation timing
const float STEP_INTERVAL_SECONDS = 0.5f;   // slightly faster, smoother with tweens

// ============================================================================
// Data structures
// ============================================================================

enum class NodeVisualState
{
    Normal,
    Current,
    Found,
    ToDelete
};

enum class OperationType
{
    None,
    Insert,
    Search,
    Delete
};

enum class TraversalType
{
    InOrder,
    PreOrder,
    PostOrder
};

struct BSTNode
{
    int value;
    BSTNode* leftPointer;
    BSTNode* rightPointer;
    Vector2 position;        // current render position
    Vector2 targetPosition;  // desired layout position
    NodeVisualState visualState;

    // Layout helpers
    int depth;
    int columnIndex;

    // Visual effect helpers
    float highlightAmount;   // 0..1 for smooth highlight fade

    explicit BSTNode(int nodeValue)
        : value(nodeValue),
        leftPointer(nullptr),
        rightPointer(nullptr),
        position{ 0.0f, 0.0f },
        targetPosition{ 0.0f, 0.0f },
        visualState(NodeVisualState::Normal),
        depth(0),
        columnIndex(0),
        highlightAmount(0.0f)
    {
    }
};

struct AlgorithmStep
{
    std::string descriptionText;
    int highlightedPseudocodeLineIndex = -1;
    std::function<void()> applyStepFunction;
};

// ============================================================================
// BST implementation
// ============================================================================

class BinarySearchTree
{
public:
    BinarySearchTree() : rootPointer(nullptr) {}

    ~BinarySearchTree()
    {
        Clear();
    }

    void Clear()
    {
        ClearRecursive(rootPointer);
        rootPointer = nullptr;
    }

    BSTNode* GetRootPointer() const
    {
        return rootPointer;
    }

    void ResetVisualStates()
    {
        ResetVisualStatesRecursive(rootPointer);
    }

    // Immediate insertion (no animation)
    void InsertImmediate(int value)
    {
        rootPointer = InsertRecursive(rootPointer, value);
    }

    // Immediate deletion (no animation)
    void DeleteImmediate(int value)
    {
        rootPointer = DeleteRecursive(rootPointer, value);
    }

    // Traversal getters
    std::vector<int> GetInOrderValues() const
    {
        std::vector<int> values;
        CollectInOrderValuesRecursive(rootPointer, values);
        return values;
    }

    std::vector<int> GetPreOrderValues() const
    {
        std::vector<int> values;
        CollectPreOrderValuesRecursive(rootPointer, values);
        return values;
    }

    std::vector<int> GetPostOrderValues() const
    {
        std::vector<int> values;
        CollectPostOrderValuesRecursive(rootPointer, values);
        return values;
    }

    int GetNodeCount() const
    {
        return CountNodesRecursive(rootPointer);
    }

    int GetHeight() const
    {
        return HeightRecursive(rootPointer);
    }

    // Layout: compact in-order layout that keeps all nodes inside TREE_AREA
    void UpdateLayout()
    {
        int currentColumnIndex = 0;
        AssignColumnsRecursive(rootPointer, 0, currentColumnIndex);
        int totalColumns = currentColumnIndex;

        if (totalColumns == 0 || rootPointer == nullptr)
        {
            return; // empty tree
        }

        const float horizontalMargin = 40.0f; // padding inside tree panel (left/right)
        const float topPadding = 40.0f;       // padding from top of tree panel

        float usableWidth = TREE_AREA_WIDTH - 2.0f * horizontalMargin;
        if (usableWidth < NODE_RADIUS * 2.0f)
        {
            usableWidth = NODE_RADIUS * 2.0f;
        }

        float columnStep = usableWidth / static_cast<float>(totalColumns);

        ApplyScreenPositionsRecursive(rootPointer, horizontalMargin, topPadding, columnStep);
    }

private:
    BSTNode* rootPointer;

    static void ClearRecursive(BSTNode* nodePointer)
    {
        if (nodePointer == nullptr)
        {
            return;
        }
        ClearRecursive(nodePointer->leftPointer);
        ClearRecursive(nodePointer->rightPointer);
        delete nodePointer;
    }

    static void ResetVisualStatesRecursive(BSTNode* nodePointer)
    {
        if (nodePointer == nullptr)
        {
            return;
        }
        nodePointer->visualState = NodeVisualState::Normal;
        ResetVisualStatesRecursive(nodePointer->leftPointer);
        ResetVisualStatesRecursive(nodePointer->rightPointer);
    }

    static BSTNode* InsertRecursive(BSTNode* nodePointer, int value)
    {
        if (nodePointer == nullptr)
        {
            return new BSTNode(value);
        }

        if (value < nodePointer->value)
        {
            nodePointer->leftPointer = InsertRecursive(nodePointer->leftPointer, value);
        }
        else if (value > nodePointer->value)
        {
            nodePointer->rightPointer = InsertRecursive(nodePointer->rightPointer, value);
        }
        return nodePointer;
    }

    static BSTNode* FindMinNode(BSTNode* nodePointer)
    {
        while (nodePointer != nullptr && nodePointer->leftPointer != nullptr)
        {
            nodePointer = nodePointer->leftPointer;
        }
        return nodePointer;
    }

    static BSTNode* DeleteRecursive(BSTNode* nodePointer, int value)
    {
        if (nodePointer == nullptr)
        {
            return nullptr;
        }

        if (value < nodePointer->value)
        {
            nodePointer->leftPointer = DeleteRecursive(nodePointer->leftPointer, value);
        }
        else if (value > nodePointer->value)
        {
            nodePointer->rightPointer = DeleteRecursive(nodePointer->rightPointer, value);
        }
        else
        {
            if (nodePointer->leftPointer == nullptr)
            {
                BSTNode* rightChildPointer = nodePointer->rightPointer;
                delete nodePointer;
                return rightChildPointer;
            }
            else if (nodePointer->rightPointer == nullptr)
            {
                BSTNode* leftChildPointer = nodePointer->leftPointer;
                delete nodePointer;
                return leftChildPointer;
            }
            else
            {
                BSTNode* successorPointer = FindMinNode(nodePointer->rightPointer);
                nodePointer->value = successorPointer->value;
                nodePointer->rightPointer = DeleteRecursive(nodePointer->rightPointer, successorPointer->value);
            }
        }

        return nodePointer;
    }

    void AssignColumnsRecursive(BSTNode* nodePointer, int depth, int& currentColumnIndex)
    {
        if (nodePointer == nullptr)
        {
            return;
        }

        AssignColumnsRecursive(nodePointer->leftPointer, depth + 1, currentColumnIndex);

        nodePointer->depth = depth;
        nodePointer->columnIndex = currentColumnIndex;
        currentColumnIndex++;

        AssignColumnsRecursive(nodePointer->rightPointer, depth + 1, currentColumnIndex);
    }

    void ApplyScreenPositionsRecursive(BSTNode* nodePointer,
        float horizontalMargin,
        float topPadding,
        float columnStep)
    {
        if (nodePointer == nullptr)
        {
            return;
        }

        float x = TREE_AREA_X + horizontalMargin
            + (nodePointer->columnIndex + 0.5f) * columnStep;

        float y = TREE_AREA_Y + topPadding
            + static_cast<float>(nodePointer->depth) * VERTICAL_SPACING;

        nodePointer->targetPosition = { x, y };

        if (nodePointer->position.x == 0.0f && nodePointer->position.y == 0.0f)
        {
            nodePointer->position = nodePointer->targetPosition;
        }

        ApplyScreenPositionsRecursive(nodePointer->leftPointer,
            horizontalMargin, topPadding, columnStep);
        ApplyScreenPositionsRecursive(nodePointer->rightPointer,
            horizontalMargin, topPadding, columnStep);
    }

    static void CollectInOrderValuesRecursive(BSTNode* nodePointer, std::vector<int>& values)
    {
        if (nodePointer == nullptr)
        {
            return;
        }

        CollectInOrderValuesRecursive(nodePointer->leftPointer, values);
        values.push_back(nodePointer->value);
        CollectInOrderValuesRecursive(nodePointer->rightPointer, values);
    }

    static void CollectPreOrderValuesRecursive(BSTNode* nodePointer, std::vector<int>& values)
    {
        if (nodePointer == nullptr)
        {
            return;
        }

        values.push_back(nodePointer->value);
        CollectPreOrderValuesRecursive(nodePointer->leftPointer, values);
        CollectPreOrderValuesRecursive(nodePointer->rightPointer, values);
    }

    static void CollectPostOrderValuesRecursive(BSTNode* nodePointer, std::vector<int>& values)
    {
        if (nodePointer == nullptr)
        {
            return;
        }

        CollectPostOrderValuesRecursive(nodePointer->leftPointer, values);
        CollectPostOrderValuesRecursive(nodePointer->rightPointer, values);
        values.push_back(nodePointer->value);
    }

    static int CountNodesRecursive(BSTNode* nodePointer)
    {
        if (nodePointer == nullptr)
        {
            return 0;
        }
        return 1 + CountNodesRecursive(nodePointer->leftPointer) + CountNodesRecursive(nodePointer->rightPointer);
    }

    static int HeightRecursive(BSTNode* nodePointer)
    {
        if (nodePointer == nullptr)
        {
            return 0;
        }
        int leftHeight = HeightRecursive(nodePointer->leftPointer);
        int rightHeight = HeightRecursive(nodePointer->rightPointer);
        return 1 + ((leftHeight > rightHeight) ? leftHeight : rightHeight);
    }
};

// ============================================================================
// Global state
// ============================================================================

BinarySearchTree bst;

OperationType currentOperationType = OperationType::None;
TraversalType currentTraversalType = TraversalType::InOrder;

std::vector<AlgorithmStep> currentSteps;
int currentStepIndex = -1;
bool isOperationInProgress = false;
bool isAutoPlayEnabled = true;

float stepTimerSeconds = 0.0f;

int pendingValue = 0;

BSTNode* deleteCurrentPointer = nullptr;
BSTNode* animatedEdgeParentPointer = nullptr;
BSTNode* animatedEdgeChildPointer = nullptr;
float animatedEdgeProgress = 0.0f;

struct DeletedNodeEffect
{
    bool isActive = false;
    Vector2 position{ 0.0f, 0.0f };
    float radius = 0.0f;
    float alpha = 0.0f;
    int value = 0;
};
DeletedNodeEffect deletedNodeEffect;

BSTNode* hoveredNodePointer = nullptr;
std::string lastOperationLabel = "None";
std::vector<int> currentPathValues;

void ClearAnimatedEdge();

// ============================================================================
// Small helpers
// ============================================================================

void HighlightNode(BSTNode* nodePointer, NodeVisualState state)
{
    bst.ResetVisualStates();
    if (nodePointer != nullptr)
    {
        nodePointer->visualState = state;
    }
}

AlgorithmStep MakeStep(const std::string& descriptionText,
    int highlightedPseudocodeLineIndex,
    std::function<void()> applyStepFunction)
{
    return AlgorithmStep{ descriptionText, highlightedPseudocodeLineIndex, std::move(applyStepFunction) };
}

void CollectInOrderNodesRecursive(BSTNode* nodePointer, std::vector<BSTNode*>& nodes)
{
    if (nodePointer == nullptr)
    {
        return;
    }
    CollectInOrderNodesRecursive(nodePointer->leftPointer, nodes);
    nodes.push_back(nodePointer);
    CollectInOrderNodesRecursive(nodePointer->rightPointer, nodes);
}

void CollectInOrderNodes(BSTNode* rootPointer, std::vector<BSTNode*>& nodes)
{
    CollectInOrderNodesRecursive(rootPointer, nodes);
}

// ============================================================================
// UI helpers
// ============================================================================

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

    DrawRectangleRounded(buttonRectangle, 0.25f, 8, buttonColour);
    DrawRectangleRoundedLinesEx(buttonRectangle, 0.25f, 8, 2.0f, Fade(BLACK, 0.6f));

    int fontSize = 18;
    int textWidth = MeasureText(buttonText.c_str(), fontSize);
    float textX = buttonRectangle.x + (buttonRectangle.width - static_cast<float>(textWidth)) / 2.0f;
    float textY = buttonRectangle.y + (buttonRectangle.height - static_cast<float>(fontSize)) / 2.0f;

    DrawText(buttonText.c_str(), static_cast<int>(textX), static_cast<int>(textY), fontSize, RAYWHITE);
}

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
            if (!isActive || shouldClearOnClick)
            {
                textValue.clear();
                shouldClearOnClick = false;
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

// ============================================================================
// Drawing helpers (tree)
// ============================================================================

void DrawArrow(Vector2 startPosition, Vector2 endPosition)
{
    DrawLineEx(startPosition, endPosition, 3.0f, BLACK);

    Vector2 directionVector = { endPosition.x - startPosition.x, endPosition.y - startPosition.y };
    float length = std::sqrt(directionVector.x * directionVector.x + directionVector.y * directionVector.y);
    if (length <= 0.0001f)
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

void DrawNodeCircle(BSTNode* nodePointer)
{
    if (nodePointer == nullptr)
    {
        return;
    }

    Color baseColour = COLOUR_NODE_NORMAL;
    Color targetColour = COLOUR_NODE_NORMAL;
    switch (nodePointer->visualState)
    {
    case NodeVisualState::Current:  targetColour = COLOUR_NODE_CURRENT; break;
    case NodeVisualState::Found:    targetColour = COLOUR_NODE_FOUND;   break;
    case NodeVisualState::ToDelete: targetColour = COLOUR_NODE_DELETE;  break;
    case NodeVisualState::Normal:
    default:                        targetColour = COLOUR_NODE_NORMAL;  break;
    }

    float t = nodePointer->highlightAmount;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    Color fillColour = {
        (unsigned char)(baseColour.r + (targetColour.r - baseColour.r) * t),
        (unsigned char)(baseColour.g + (targetColour.g - baseColour.g) * t),
        (unsigned char)(baseColour.b + (targetColour.b - baseColour.b) * t),
        255
    };

    float pulseScale = 1.0f;
    if (nodePointer->visualState == NodeVisualState::Current)
    {
        pulseScale = 1.0f + 0.05f * std::sin(GetTime() * 6.0f);
    }

    DrawCircleV(nodePointer->position, NODE_RADIUS * pulseScale, fillColour);
    DrawCircleLines(static_cast<int>(nodePointer->position.x),
        static_cast<int>(nodePointer->position.y),
        NODE_RADIUS * pulseScale,
        BLACK);

    std::string text = std::to_string(nodePointer->value);
    int fontSize = 20;
    int textWidth = MeasureText(text.c_str(), fontSize);
    DrawText(text.c_str(),
        static_cast<int>(nodePointer->position.x - textWidth / 2),
        static_cast<int>(nodePointer->position.y - fontSize / 2),
        fontSize,
        COLOUR_TEXT_MAIN);
}

void DrawEdgeBetween(BSTNode* parentPointer, BSTNode* childPointer)
{
    if (parentPointer == nullptr || childPointer == nullptr)
    {
        return;
    }

    Vector2 startCentre = parentPointer->position;
    Vector2 endCentre = childPointer->position;

    Vector2 directionVector = { endCentre.x - startCentre.x, endCentre.y - startCentre.y };
    float length = std::sqrt(directionVector.x * directionVector.x + directionVector.y * directionVector.y);
    if (length <= 0.0001f)
    {
        return;
    }

    directionVector.x /= length;
    directionVector.y /= length;

    Vector2 startPosition = {
        startCentre.x + directionVector.x * NODE_RADIUS,
        startCentre.y + directionVector.y * NODE_RADIUS
    };
    Vector2 endPosition = {
        endCentre.x - directionVector.x * NODE_RADIUS,
        endCentre.y - directionVector.y * NODE_RADIUS
    };

    DrawArrow(startPosition, endPosition);

    bool isHighlightedEdge =
        (parentPointer == animatedEdgeParentPointer &&
            childPointer == animatedEdgeChildPointer);

    if (isHighlightedEdge)
    {
        float clampedProgress = animatedEdgeProgress;
        if (clampedProgress < 0.0f) clampedProgress = 0.0f;
        if (clampedProgress > 1.0f) clampedProgress = 1.0f;

        float fullEdgeLength = std::sqrt(
            (endPosition.x - startPosition.x) * (endPosition.x - startPosition.x) +
            (endPosition.y - startPosition.y) * (endPosition.y - startPosition.y));

        float highlightedLength = fullEdgeLength * clampedProgress;

        Vector2 highlightedEnd = {
            startPosition.x + directionVector.x * highlightedLength,
            startPosition.y + directionVector.y * highlightedLength
        };

        DrawLineEx(startPosition, highlightedEnd, 5.0f, Fade(RED, 0.7f));
    }
}

int ComputeSubtreeSize(BSTNode* nodePointer)
{
    if (nodePointer == nullptr)
    {
        return 0;
    }
    return 1 + ComputeSubtreeSize(nodePointer->leftPointer) + ComputeSubtreeSize(nodePointer->rightPointer);
}

void DrawTreeRecursive(BSTNode* nodePointer)
{
    if (nodePointer == nullptr)
    {
        return;
    }

    if (nodePointer->leftPointer != nullptr)
    {
        DrawEdgeBetween(nodePointer, nodePointer->leftPointer);
        DrawTreeRecursive(nodePointer->leftPointer);
    }
    if (nodePointer->rightPointer != nullptr)
    {
        DrawEdgeBetween(nodePointer, nodePointer->rightPointer);
        DrawTreeRecursive(nodePointer->rightPointer);
    }

    DrawNodeCircle(nodePointer);

    Vector2 mousePosition = GetMousePosition();
    float dx = mousePosition.x - nodePointer->position.x;
    float dy = mousePosition.y - nodePointer->position.y;
    if (dx * dx + dy * dy <= NODE_RADIUS * NODE_RADIUS)
    {
        hoveredNodePointer = nodePointer;
    }
}

void DrawNodeTooltip()
{
    if (hoveredNodePointer == nullptr)
    {
        return;
    }

    BSTNode* nodePointer = hoveredNodePointer;

    int subtreeSize = ComputeSubtreeSize(nodePointer);
    int depth = nodePointer->depth;

    std::string line1 = "Value: " + std::to_string(nodePointer->value);
    std::string line2 = "Depth: " + std::to_string(depth);
    std::string line3 = "Subtree size: " + std::to_string(subtreeSize);

    int fontSize = 16;
    int padding = 6;

    int width1 = MeasureText(line1.c_str(), fontSize);
    int width2 = MeasureText(line2.c_str(), fontSize);
    int width3 = MeasureText(line3.c_str(), fontSize);

    int maxWidth = width1;
    if (width2 > maxWidth) maxWidth = width2;
    if (width3 > maxWidth) maxWidth = width3;

    float boxWidth = static_cast<float>(maxWidth + padding * 2);
    float boxHeight = static_cast<float>((fontSize + 4) * 3 + padding * 2);

    float boxX = nodePointer->position.x - boxWidth / 2.0f;
    float boxY = nodePointer->position.y - NODE_RADIUS - boxHeight - 8.0f;

    if (boxX < TREE_AREA_X + 4.0f)
    {
        boxX = TREE_AREA_X + 4.0f;
    }
    if (boxX + boxWidth > TREE_AREA_X + TREE_AREA_WIDTH - 4.0f)
    {
        boxX = TREE_AREA_X + TREE_AREA_WIDTH - boxWidth - 4.0f;
    }
    if (boxY < TREE_AREA_Y + 4.0f)
    {
        boxY = TREE_AREA_Y + 4.0f;
    }

    Rectangle tooltipRect = { boxX, boxY, boxWidth, boxHeight };
    DrawRectangleRounded(tooltipRect, 0.1f, 4, Fade(WHITE, 0.98f));
    DrawRectangleRoundedLinesEx(tooltipRect, 0.1f, 4, 1.0f, COLOUR_PANEL_BORDER);

    float textX = boxX + static_cast<float>(padding);
    float textY = boxY + static_cast<float>(padding);

    DrawText(line1.c_str(), static_cast<int>(textX), static_cast<int>(textY), fontSize, COLOUR_TEXT_MAIN);
    DrawText(line2.c_str(), static_cast<int>(textX), static_cast<int>(textY + fontSize + 4), fontSize, COLOUR_TEXT_MAIN);
    DrawText(line3.c_str(), static_cast<int>(textX), static_cast<int>(textY + (fontSize + 4) * 2), fontSize, COLOUR_TEXT_MAIN);
}

void DrawTreePanel()
{
    Rectangle panelRectangle = {
        TREE_AREA_X,
        TREE_AREA_Y,
        TREE_AREA_WIDTH,
        TREE_AREA_HEIGHT
    };

    DrawRectangleRounded(panelRectangle, 0.03f, 4, COLOUR_PANEL_BACKGROUND);
    DrawRectangleRoundedLinesEx(panelRectangle, 0.03f, 4, 2.0f, COLOUR_PANEL_BORDER);

    hoveredNodePointer = nullptr;
    DrawTreeRecursive(bst.GetRootPointer());

    int nodeCount = bst.GetNodeCount();
    int treeHeight = bst.GetHeight();

    Rectangle statsRect = {
        TREE_AREA_X + 10.0f,
        TREE_AREA_Y + 10.0f,
        260.0f,
        72.0f
    };
    DrawRectangleRounded(statsRect, 0.1f, 4, Fade(WHITE, 0.85f));
    DrawRectangleRoundedLinesEx(statsRect, 0.1f, 4, 1.0f, COLOUR_PANEL_BORDER);

    int statsFontSize = 16;
    int statsX = static_cast<int>(statsRect.x) + 8;
    int statsY = static_cast<int>(statsRect.y) + 6;

    std::string nodesText = "Nodes: " + std::to_string(nodeCount);
    std::string heightText = "Height: " + std::to_string(treeHeight);
    std::string lastOpText = "Last op: " + lastOperationLabel;

    DrawText(nodesText.c_str(), statsX, statsY, statsFontSize, COLOUR_TEXT_MAIN);
    DrawText(heightText.c_str(), statsX, statsY + statsFontSize + 2, statsFontSize, COLOUR_TEXT_MAIN);
    DrawText(lastOpText.c_str(), statsX, statsY + (statsFontSize + 2) * 2, statsFontSize, COLOUR_TEXT_MAIN);

    std::vector<int> traversalValues;
    std::string traversalLabel;

    switch (currentTraversalType)
    {
    case TraversalType::InOrder:
        traversalValues = bst.GetInOrderValues();
        traversalLabel = "In-order: ";
        break;
    case TraversalType::PreOrder:
        traversalValues = bst.GetPreOrderValues();
        traversalLabel = "Pre-order: ";
        break;
    case TraversalType::PostOrder:
        traversalValues = bst.GetPostOrderValues();
        traversalLabel = "Post-order: ";
        break;
    }

    if (!traversalValues.empty())
    {
        const int fontSize = 18;
        const float bottomMargin = 24.0f;
        float traversalTextY = TREE_AREA_Y + TREE_AREA_HEIGHT - bottomMargin;
        float traversalTextX = TREE_AREA_X + 16.0f;

        DrawText(traversalLabel.c_str(),
            static_cast<int>(traversalTextX),
            static_cast<int>(traversalTextY),
            fontSize,
            COLOUR_TEXT_MAIN);

        int labelWidth = MeasureText(traversalLabel.c_str(), fontSize);
        float currentX = traversalTextX + static_cast<float>(labelWidth) + 6.0f;

        for (int valueIndex = 0; valueIndex < static_cast<int>(traversalValues.size()); valueIndex++)
        {
            std::string valueText = std::to_string(traversalValues[valueIndex]);
            if (valueIndex < static_cast<int>(traversalValues.size()) - 1)
            {
                valueText += " ";
            }

            DrawText(valueText.c_str(),
                static_cast<int>(currentX),
                static_cast<int>(traversalTextY),
                fontSize,
                COLOUR_TEXT_MAIN);

            int valueWidth = MeasureText(valueText.c_str(), fontSize);
            currentX += static_cast<float>(valueWidth);
        }
    }
}

void DrawDeletedNodeEffect()
{
    if (!deletedNodeEffect.isActive)
    {
        return;
    }

    Color fillColour = COLOUR_NODE_DELETE;
    fillColour.a = static_cast<unsigned char>(255 * deletedNodeEffect.alpha);
    Color outlineColour = BLACK;
    outlineColour.a = fillColour.a;

    DrawCircleV(deletedNodeEffect.position, deletedNodeEffect.radius, fillColour);
    DrawCircleLines(static_cast<int>(deletedNodeEffect.position.x),
        static_cast<int>(deletedNodeEffect.position.y),
        deletedNodeEffect.radius,
        outlineColour);

    std::string text = std::to_string(deletedNodeEffect.value);
    int fontSize = 20;
    int textWidth = MeasureText(text.c_str(), fontSize);
    DrawText(text.c_str(),
        static_cast<int>(deletedNodeEffect.position.x - textWidth / 2),
        static_cast<int>(deletedNodeEffect.position.y - fontSize / 2),
        fontSize,
        COLOUR_TEXT_MAIN);
}

// ============================================================================
// Pseudocode management
// ============================================================================

std::vector<std::string> GetPseudocodeLinesForOperation(OperationType operationType)
{
    if (operationType == OperationType::Insert)
    {
        return {
            "1. newNode = createNode(value)",
            "2. if root == NULL then",
            "3.     root = newNode",
            "4. else",
            "5.     current = root",
            "6.     while true",
            "7.         if value < current.value then",
            "8.             if current.left == NULL then",
            "9.                 current.left = newNode; break",
            "10.            else current = current.left",
            "11.        else if value > current.value then",
            "12.            if current.right == NULL then",
            "13.                current.right = newNode; break",
            "14.            else current = current.right",
            "15.        else  // duplicate",
            "16.            break"
        };
    }
    else if (operationType == OperationType::Search)
    {
        return {
            "1. current = root",
            "2. while current != NULL",
            "3.     if value == current.value",
            "4.         return FOUND",
            "5.     else if value < current.value",
            "6.         current = current.left",
            "7.     else",
            "8.         current = current.right",
            "9. return NOT_FOUND"
        };
    }
    else if (operationType == OperationType::Delete)
    {
        return {
            "1. current = root, parent = NULL",
            "2. while current != NULL and current.value != target",
            "3.     parent = current",
            "4.     if target < current.value",
            "5.         current = current.left",
            "6.     else",
            "7.         current = current.right",
            "8. if current == NULL",
            "9.     return NOT_FOUND",
            "10. // case analysis for delete",
            "11. if node has 0 or 1 child",
            "12.     replace node with its child (if any)",
            "13. else // two children",
            "14.     find in-order successor and copy its value",
            "15.     delete successor node"
        };
    }

    return {};
}

void DrawPseudocodePanel(OperationType operationType, int highlightedLineIndex)
{
    Rectangle panelRectangle = {
        static_cast<float>(PSEUDOCODE_PANEL_X),
        static_cast<float>(PSEUDOCODE_PANEL_Y),
        static_cast<float>(PSEUDOCODE_PANEL_WIDTH),
        static_cast<float>(PSEUDOCODE_PANEL_HEIGHT)
    };

    DrawRectangleRounded(panelRectangle, 0.03f, 4, COLOUR_PANEL_BACKGROUND);
    DrawRectangleRoundedLinesEx(panelRectangle, 0.03f, 4, 2.0f, COLOUR_PANEL_BORDER);

    int fontSize = 18;
    int margin = 14;
    int lineSpacing = 6;

    DrawText("Pseudocode",
        PSEUDOCODE_PANEL_X + margin,
        PSEUDOCODE_PANEL_Y + margin - 4,
        fontSize,
        COLOUR_TEXT_MAIN);

    DrawLine(PSEUDOCODE_PANEL_X + margin,
        PSEUDOCODE_PANEL_Y + margin + fontSize,
        PSEUDOCODE_PANEL_X + PSEUDOCODE_PANEL_WIDTH - margin,
        PSEUDOCODE_PANEL_Y + margin + fontSize,
        Fade(COLOUR_PANEL_BORDER, 0.7f));

    std::vector<std::string> pseudocodeLines = GetPseudocodeLinesForOperation(operationType);
    int lineY = PSEUDOCODE_PANEL_Y + margin + fontSize + 10;

    for (int lineIndex = 0; lineIndex < static_cast<int>(pseudocodeLines.size()); lineIndex++)
    {
        const std::string& lineText = pseudocodeLines[lineIndex];

        if (lineIndex == highlightedLineIndex)
        {
            int textWidth = MeasureText(lineText.c_str(), fontSize);
            DrawRectangle(PSEUDOCODE_PANEL_X + margin - 4, lineY - 2, textWidth + 8, fontSize + 4,
                Color{ 255, 249, 196, 255 });
        }

        DrawText(lineText.c_str(),
            PSEUDOCODE_PANEL_X + margin,
            lineY,
            fontSize,
            COLOUR_TEXT_MAIN);

        lineY += fontSize + lineSpacing;
    }

    if (pseudocodeLines.empty())
    {
        DrawText("No operation selected.",
            PSEUDOCODE_PANEL_X + margin,
            lineY,
            fontSize,
            COLOUR_TEXT_MUTED);
    }
}

// ============================================================================
// Visual updates (tweens, highlights, edges, delete effect)
// ============================================================================

void UpdateTreeVisualEffectsRecursive(BSTNode* nodePointer, float deltaTime)
{
    if (nodePointer == nullptr)
    {
        return;
    }

    const float moveSpeed = 10.0f;
    float moveFactor = moveSpeed * deltaTime;

    nodePointer->position.x += (nodePointer->targetPosition.x - nodePointer->position.x) * moveFactor;
    nodePointer->position.y += (nodePointer->targetPosition.y - nodePointer->position.y) * moveFactor;

    float targetHighlight = 0.0f;
    if (nodePointer->visualState != NodeVisualState::Normal)
    {
        targetHighlight = 1.0f;
    }
    const float highlightSpeed = 6.0f;
    nodePointer->highlightAmount += (targetHighlight - nodePointer->highlightAmount) * highlightSpeed * deltaTime;

    if (nodePointer->highlightAmount < 0.0f) nodePointer->highlightAmount = 0.0f;
    if (nodePointer->highlightAmount > 1.0f) nodePointer->highlightAmount = 1.0f;

    UpdateTreeVisualEffectsRecursive(nodePointer->leftPointer, deltaTime);
    UpdateTreeVisualEffectsRecursive(nodePointer->rightPointer, deltaTime);
}

void UpdateAnimatedEdge(float deltaTime)
{
    if (animatedEdgeParentPointer == nullptr || animatedEdgeChildPointer == nullptr)
    {
        return;
    }

    const float edgeSpeed = 2.5f;
    animatedEdgeProgress += edgeSpeed * deltaTime;
    if (animatedEdgeProgress > 1.0f)
    {
        animatedEdgeProgress = 1.0f;
    }
}

void ClearAnimatedEdge()
{
    animatedEdgeParentPointer = nullptr;
    animatedEdgeChildPointer = nullptr;
    animatedEdgeProgress = 0.0f;
}

void UpdateDeletedNodeEffect(float deltaTime)
{
    if (!deletedNodeEffect.isActive)
    {
        return;
    }

    const float shrinkSpeed = 60.0f;
    const float fadeSpeed = 2.5f;

    deletedNodeEffect.radius -= shrinkSpeed * deltaTime;
    if (deletedNodeEffect.radius < 0.0f)
    {
        deletedNodeEffect.radius = 0.0f;
    }

    deletedNodeEffect.alpha -= fadeSpeed * deltaTime;
    if (deletedNodeEffect.alpha <= 0.0f)
    {
        deletedNodeEffect.alpha = 0.0f;
        deletedNodeEffect.isActive = false;
    }
}

// ============================================================================
// Algorithm step management
// ============================================================================

void ClearCurrentOperation()
{
    currentSteps.clear();
    currentStepIndex = -1;
    isOperationInProgress = false;
    currentOperationType = OperationType::None;
    deleteCurrentPointer = nullptr;
    stepTimerSeconds = 0.0f;
    bst.ResetVisualStates();
    ClearAnimatedEdge();
    currentPathValues.clear();
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
        bst.ResetVisualStates();
        currentOperationType = OperationType::None;
        deleteCurrentPointer = nullptr;
        stepTimerSeconds = 0.0f;
        ClearAnimatedEdge();
        currentPathValues.clear();
    }
}

void StartOperationPlayback()
{
    currentStepIndex = -1;
    isOperationInProgress = true;
    isAutoPlayEnabled = true;
    stepTimerSeconds = 0.0f;
    AdvanceToNextStep();
}

// ============================================================================
// Build operations & traversal animation
// ============================================================================

// (All the BuildInsertSteps, BuildSearchSteps, BuildDeleteSteps and
// BuildInOrderTraversalAnimationSteps functions stay exactly the same
// as in your previous version. For brevity they’re unchanged here;
// keep them as in the last code I sent.)

// -------------
// Insert steps
// -------------
void BuildInsertSteps(int value)
{
    ClearCurrentOperation();
    currentOperationType = OperationType::Insert;
    pendingValue = value;
    lastOperationLabel = "Insert(" + std::to_string(value) + ")";

    BSTNode* rootPointer = bst.GetRootPointer();
    currentSteps.clear();

    currentSteps.push_back(MakeStep(
        "Create a new node with the given value.",
        0,
        []()
        {
            bst.ResetVisualStates();
        }));

    if (rootPointer == nullptr)
    {
        currentSteps.push_back(MakeStep(
            "The tree is empty, so the new node becomes the root.",
            2,
            []()
            {
                bst.InsertImmediate(pendingValue);
                bst.UpdateLayout();

                BSTNode* newNodePointer = bst.GetRootPointer();
                if (newNodePointer != nullptr)
                {
                    newNodePointer->position = {
                        newNodePointer->targetPosition.x,
                        newNodePointer->targetPosition.y - 60.0f
                    };
                    newNodePointer->visualState = NodeVisualState::Current;
                    currentPathValues.clear();
                    currentPathValues.push_back(newNodePointer->value);
                }
            }));
    }
    else
    {
        currentSteps.push_back(MakeStep(
            "Set current to the root node.",
            5,
            [rootPointer]()
            {
                HighlightNode(rootPointer, NodeVisualState::Current);
                currentPathValues.clear();
                if (rootPointer != nullptr)
                {
                    currentPathValues.push_back(rootPointer->value);
                }
            }));

        BSTNode* traversalPointer = rootPointer;

        while (traversalPointer != nullptr)
        {
            BSTNode* nodeThisStepPointer = traversalPointer;

            if (pendingValue == traversalPointer->value)
            {
                currentSteps.push_back(MakeStep(
                    "Value already exists in the tree (duplicate). Do nothing.",
                    15,
                    [nodeThisStepPointer]()
                    {
                        HighlightNode(nodeThisStepPointer, NodeVisualState::Found);
                    }));
                break;
            }
            else if (pendingValue < traversalPointer->value)
            {
                BSTNode* childPointer = traversalPointer->leftPointer;

                currentSteps.push_back(MakeStep(
                    "Compare value with current.value: go left.",
                    7,
                    [nodeThisStepPointer, childPointer]()
                    {
                        bst.ResetVisualStates();
                        if (nodeThisStepPointer != nullptr)
                        {
                            nodeThisStepPointer->visualState = NodeVisualState::Current;
                        }
                        animatedEdgeParentPointer = nodeThisStepPointer;
                        animatedEdgeChildPointer = childPointer;
                        animatedEdgeProgress = 0.0f;
                    }));

                if (traversalPointer->leftPointer == nullptr)
                {
                    currentSteps.push_back(MakeStep(
                        "Left child is NULL, insert new node here as left child.",
                        8,
                        [nodeThisStepPointer]()
                        {
                            bst.InsertImmediate(pendingValue);
                            bst.UpdateLayout();
                            bst.ResetVisualStates();
                            ClearAnimatedEdge();

                            std::vector<BSTNode*> stack;
                            if (bst.GetRootPointer() != nullptr)
                            {
                                stack.push_back(bst.GetRootPointer());
                            }
                            while (!stack.empty())
                            {
                                BSTNode* nodePointer = stack.back();
                                stack.pop_back();
                                if (nodePointer->value == pendingValue)
                                {
                                    nodePointer->position = {
                                        nodePointer->targetPosition.x,
                                        nodePointer->targetPosition.y - 60.0f
                                    };
                                    nodePointer->visualState = NodeVisualState::Current;
                                    currentPathValues.push_back(nodePointer->value);
                                    break;
                                }
                                if (nodePointer->leftPointer != nullptr)
                                {
                                    stack.push_back(nodePointer->leftPointer);
                                }
                                if (nodePointer->rightPointer != nullptr)
                                {
                                    stack.push_back(nodePointer->rightPointer);
                                }
                            }
                        }));
                    break;
                }
                else
                {
                    currentSteps.push_back(MakeStep(
                        "Left child is not NULL, move current to current.left.",
                        10,
                        [childPointer]()
                        {
                            bst.ResetVisualStates();
                            if (childPointer != nullptr)
                            {
                                currentPathValues.push_back(childPointer->value);
                            }
                        }));
                    traversalPointer = traversalPointer->leftPointer;
                }
            }
            else
            {
                BSTNode* childPointer = traversalPointer->rightPointer;

                currentSteps.push_back(MakeStep(
                    "Compare value with current.value: go right.",
                    11,
                    [nodeThisStepPointer, childPointer]()
                    {
                        bst.ResetVisualStates();
                        if (nodeThisStepPointer != nullptr)
                        {
                            nodeThisStepPointer->visualState = NodeVisualState::Current;
                        }
                        animatedEdgeParentPointer = nodeThisStepPointer;
                        animatedEdgeChildPointer = childPointer;
                        animatedEdgeProgress = 0.0f;
                    }));

                if (traversalPointer->rightPointer == nullptr)
                {
                    currentSteps.push_back(MakeStep(
                        "Right child is NULL, insert new node here as right child.",
                        12,
                        [nodeThisStepPointer]()
                        {
                            bst.InsertImmediate(pendingValue);
                            bst.UpdateLayout();
                            bst.ResetVisualStates();
                            ClearAnimatedEdge();

                            std::vector<BSTNode*> stack;
                            if (bst.GetRootPointer() != nullptr)
                            {
                                stack.push_back(bst.GetRootPointer());
                            }
                            while (!stack.empty())
                            {
                                BSTNode* nodePointer = stack.back();
                                stack.pop_back();
                                if (nodePointer->value == pendingValue)
                                {
                                    nodePointer->position = {
                                        nodePointer->targetPosition.x,
                                        nodePointer->targetPosition.y - 60.0f
                                    };
                                    nodePointer->visualState = NodeVisualState::Current;
                                    currentPathValues.push_back(nodePointer->value);
                                    break;
                                }
                                if (nodePointer->leftPointer != nullptr)
                                {
                                    stack.push_back(nodePointer->leftPointer);
                                }
                                if (nodePointer->rightPointer != nullptr)
                                {
                                    stack.push_back(nodePointer->rightPointer);
                                }
                            }
                        }));
                    break;
                }
                else
                {
                    currentSteps.push_back(MakeStep(
                        "Right child is not NULL, move current to current.right.",
                        14,
                        [childPointer]()
                        {
                            bst.ResetVisualStates();
                            if (childPointer != nullptr)
                            {
                                currentPathValues.push_back(childPointer->value);
                            }
                        }));
                    traversalPointer = traversalPointer->rightPointer;
                }
            }
        }
    }

    StartOperationPlayback();
}

// -----------------------------
// Build Search steps
// -----------------------------

void BuildSearchSteps(int targetValue)
{
    ClearCurrentOperation();
    currentOperationType = OperationType::Search;
    lastOperationLabel = "Search(" + std::to_string(targetValue) + ")";

    currentSteps.clear();

    BSTNode* rootPointer = bst.GetRootPointer();

    currentSteps.push_back(MakeStep(
        "Set current to the root node.",
        1,
        [rootPointer]()
        {
            HighlightNode(rootPointer, NodeVisualState::Current);
            currentPathValues.clear();
            if (rootPointer != nullptr)
            {
                currentPathValues.push_back(rootPointer->value);
            }
        }));

    if (rootPointer == nullptr)
    {
        currentSteps.push_back(MakeStep(
            "The tree is empty. current is NULL, value not found.",
            8,
            []()
            {
                bst.ResetVisualStates();
            }));
        StartOperationPlayback();
        return;
    }

    BSTNode* traversalPointer = rootPointer;
    bool isFound = false;

    while (traversalPointer != nullptr)
    {
        BSTNode* nodeThisStepPointer = traversalPointer;

        currentSteps.push_back(MakeStep(
            "Check current != NULL (loop continues).",
            2,
            [nodeThisStepPointer]()
            {
                HighlightNode(nodeThisStepPointer, NodeVisualState::Current);
                ClearAnimatedEdge();
            }));

        if (targetValue == traversalPointer->value)
        {
            currentSteps.push_back(MakeStep(
                "current.value equals target, value found.",
                3,
                [nodeThisStepPointer]()
                {
                    HighlightNode(nodeThisStepPointer, NodeVisualState::Found);
                    ClearAnimatedEdge();
                }));

            currentSteps.push_back(MakeStep(
                "Return FOUND.",
                4,
                []()
                {
                }));

            isFound = true;
            break;
        }
        else if (targetValue < traversalPointer->value)
        {
            BSTNode* childPointer = traversalPointer->leftPointer;

            currentSteps.push_back(MakeStep(
                "target < current.value, move to current.left.",
                5,
                [nodeThisStepPointer, childPointer]()
                {
                    bst.ResetVisualStates();
                    if (nodeThisStepPointer != nullptr)
                    {
                        nodeThisStepPointer->visualState = NodeVisualState::Current;
                    }
                    animatedEdgeParentPointer = nodeThisStepPointer;
                    animatedEdgeChildPointer = childPointer;
                    animatedEdgeProgress = 0.0f;

                    if (childPointer != nullptr)
                    {
                        currentPathValues.push_back(childPointer->value);
                    }
                }));
            traversalPointer = traversalPointer->leftPointer;
        }
        else
        {
            BSTNode* childPointer = traversalPointer->rightPointer;

            currentSteps.push_back(MakeStep(
                "target > current.value, move to current.right.",
                7,
                [nodeThisStepPointer, childPointer]()
                {
                    bst.ResetVisualStates();
                    if (nodeThisStepPointer != nullptr)
                    {
                        nodeThisStepPointer->visualState = NodeVisualState::Current;
                    }
                    animatedEdgeParentPointer = nodeThisStepPointer;
                    animatedEdgeChildPointer = childPointer;
                    animatedEdgeProgress = 0.0f;

                    if (childPointer != nullptr)
                    {
                        currentPathValues.push_back(childPointer->value);
                    }
                }));
            traversalPointer = traversalPointer->rightPointer;
        }
    }

    if (!isFound)
    {
        currentSteps.push_back(MakeStep(
            "current is NULL, target not found.",
            8,
            []()
            {
                bst.ResetVisualStates();
                ClearAnimatedEdge();
            }));
    }

    StartOperationPlayback();
}

// -----------------------------
// Build Delete steps
// -----------------------------

void BuildDeleteSteps(int targetValue)
{
    ClearCurrentOperation();
    currentOperationType = OperationType::Delete;
    lastOperationLabel = "Delete(" + std::to_string(targetValue) + ")";

    currentSteps.clear();

    BSTNode* rootPointer = bst.GetRootPointer();
    BSTNode* traversalPointer = rootPointer;
    BSTNode* parentPointer = nullptr;

    currentSteps.push_back(MakeStep(
        "Set current to root and parent to NULL.",
        1,
        [rootPointer]()
        {
            HighlightNode(rootPointer, NodeVisualState::Current);
            currentPathValues.clear();
            if (rootPointer != nullptr)
            {
                currentPathValues.push_back(rootPointer->value);
            }
        }));

    if (rootPointer == nullptr)
    {
        currentSteps.push_back(MakeStep(
            "The tree is empty, nothing to delete.",
            8,
            []()
            {
                bst.ResetVisualStates();
            }));
        StartOperationPlayback();
        return;
    }

    while (traversalPointer != nullptr && traversalPointer->value != targetValue)
    {
        BSTNode* nodeThisStepPointer = traversalPointer;
        BSTNode* parentThisStepPointer = parentPointer;
        BSTNode* childPointer = (targetValue < traversalPointer->value)
            ? traversalPointer->leftPointer
            : traversalPointer->rightPointer;

        currentSteps.push_back(MakeStep(
            "Check current != NULL and current.value != target (loop continues).",
            2,
            [nodeThisStepPointer]()
            {
                HighlightNode(nodeThisStepPointer, NodeVisualState::Current);
            }));

        currentSteps.push_back(MakeStep(
            "Update parent and move current left/right depending on target.",
            (targetValue < traversalPointer->value) ? 4 : 6,
            [nodeThisStepPointer, parentThisStepPointer, childPointer]()
            {
                bst.ResetVisualStates();
                if (parentThisStepPointer != nullptr)
                {
                    parentThisStepPointer->visualState = NodeVisualState::Normal;
                }
                if (nodeThisStepPointer != nullptr)
                {
                    nodeThisStepPointer->visualState = NodeVisualState::Current;
                }

                animatedEdgeParentPointer = nodeThisStepPointer;
                animatedEdgeChildPointer = childPointer;
                animatedEdgeProgress = 0.0f;

                if (childPointer != nullptr)
                {
                    currentPathValues.push_back(childPointer->value);
                }
            }));

        parentPointer = traversalPointer;
        traversalPointer = childPointer;
    }

    if (traversalPointer == nullptr)
    {
        currentSteps.push_back(MakeStep(
            "current is NULL, the target value does not exist in the tree.",
            8,
            []()
            {
                bst.ResetVisualStates();
                ClearAnimatedEdge();
            }));
        currentSteps.push_back(MakeStep(
            "Return NOT_FOUND.",
            9,
            []()
            {
                bst.ResetVisualStates();
                ClearAnimatedEdge();
            }));
    }
    else
    {
        deleteCurrentPointer = traversalPointer;

        currentSteps.push_back(MakeStep(
            "current points to the node to delete. Highlight it in red.",
            10,
            []()
            {
                bst.ResetVisualStates();
                ClearAnimatedEdge();
                if (deleteCurrentPointer != nullptr)
                {
                    deleteCurrentPointer->visualState = NodeVisualState::ToDelete;
                }
            }));

        currentSteps.push_back(MakeStep(
            "Apply BST delete (handling 0, 1, or 2 children).",
            11,
            [targetValue]()
            {
                if (deleteCurrentPointer != nullptr)
                {
                    deletedNodeEffect.isActive = true;
                    deletedNodeEffect.position = deleteCurrentPointer->position;
                    deletedNodeEffect.radius = NODE_RADIUS;
                    deletedNodeEffect.alpha = 1.0f;
                    deletedNodeEffect.value = deleteCurrentPointer->value;
                }

                bst.DeleteImmediate(targetValue);
                bst.UpdateLayout();
                bst.ResetVisualStates();
                ClearAnimatedEdge();
                deleteCurrentPointer = nullptr;
            }));
    }

    StartOperationPlayback();
}

// -----------------------------
// In-order traversal animation
// -----------------------------

void BuildInOrderTraversalAnimationSteps()
{
    ClearCurrentOperation();
    currentOperationType = OperationType::None;
    lastOperationLabel = "Animate in-order";

    currentSteps.clear();

    std::vector<BSTNode*> nodeOrder;
    CollectInOrderNodes(bst.GetRootPointer(), nodeOrder);

    if (nodeOrder.empty())
    {
        currentSteps.push_back(MakeStep(
            "The tree is empty. Nothing to traverse.",
            -1,
            []()
            {
                bst.ResetVisualStates();
            }));
    }
    else
    {
        for (BSTNode* nodePointer : nodeOrder)
        {
            currentSteps.push_back(MakeStep(
                "Visit node " + std::to_string(nodePointer->value) + " in-order.",
                -1,
                [nodePointer]()
                {
                    bst.ResetVisualStates();
                    ClearAnimatedEdge();
                    if (nodePointer != nullptr)
                    {
                        nodePointer->visualState = NodeVisualState::Found;
                    }
                }));
        }
    }

    currentPathValues.clear();
    StartOperationPlayback();
}

// ============================================================================
// Main
// ============================================================================

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT,
        "Binary Search Tree Visualiser - Insert, Search, Delete");
    SetTargetFPS(60);

    std::string valueTextBoxValue;
    bool isValueTextBoxActive = false;
    bool shouldClearTextOnNextClick = false;

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        HandleTextBoxInput(VALUE_TEXTBOX_RECT,
            isValueTextBoxActive,
            valueTextBoxValue,
            shouldClearTextOnNextClick);

        bool canStartNewOperation = !valueTextBoxValue.empty() && !isOperationInProgress;

        Rectangle insertButtonRectangle = { 240.0f, 120.0f, 160.0f, 40.0f };
        Rectangle searchButtonRectangle = { 420.0f, 120.0f, 160.0f, 40.0f };
        Rectangle deleteButtonRectangle = { 600.0f, 120.0f, 160.0f, 40.0f };
        Rectangle clearButtonRectangle = { 780.0f, 120.0f, 160.0f, 40.0f };
        Rectangle randomTreeButtonRectangle = { 960.0f, 120.0f, 160.0f, 40.0f };

        float traversalButtonsY = VALUE_TEXTBOX_RECT.y + VALUE_TEXTBOX_RECT.height + 10.0f; // 170
        Rectangle inOrderButtonRectangle = { TREE_AREA_X,           traversalButtonsY, 120.0f, 30.0f };
        Rectangle preOrderButtonRectangle = { TREE_AREA_X + 130.0f, traversalButtonsY, 120.0f, 30.0f };
        Rectangle postOrderButtonRectangle = { TREE_AREA_X + 260.0f, traversalButtonsY, 120.0f, 30.0f };
        Rectangle animateInOrderButtonRectangle = { TREE_AREA_X + 390.0f, traversalButtonsY, 160.0f, 30.0f };

        float controlsY = static_cast<float>(SCREEN_HEIGHT - 90);
        Rectangle playPauseButtonRectangle = { static_cast<float>(SCREEN_WIDTH - 320), controlsY, 100.0f, 32.0f };
        Rectangle stepButtonRectangle = { static_cast<float>(SCREEN_WIDTH - 210), controlsY,  80.0f, 32.0f };
        Rectangle resetOpButtonRectangle = { static_cast<float>(SCREEN_WIDTH - 110), controlsY,  90.0f, 32.0f };

        if (IsButtonClicked(inOrderButtonRectangle) && currentTraversalType != TraversalType::InOrder)
        {
            currentTraversalType = TraversalType::InOrder;
        }
        if (IsButtonClicked(preOrderButtonRectangle) && currentTraversalType != TraversalType::PreOrder)
        {
            currentTraversalType = TraversalType::PreOrder;
        }
        if (IsButtonClicked(postOrderButtonRectangle) && currentTraversalType != TraversalType::PostOrder)
        {
            currentTraversalType = TraversalType::PostOrder;
        }

        if (IsButtonClicked(animateInOrderButtonRectangle) && !isOperationInProgress)
        {
            BuildInOrderTraversalAnimationSteps();
            shouldClearTextOnNextClick = true;
        }

        if (IsButtonClicked(insertButtonRectangle) && canStartNewOperation)
        {
            int parsedValue = std::atoi(valueTextBoxValue.c_str());

            bst.UpdateLayout();
            BuildInsertSteps(parsedValue);
            shouldClearTextOnNextClick = true;
        }

        if (IsButtonClicked(searchButtonRectangle) && canStartNewOperation)
        {
            int parsedValue = std::atoi(valueTextBoxValue.c_str());
            bst.UpdateLayout();
            BuildSearchSteps(parsedValue);
            shouldClearTextOnNextClick = true;
        }

        if (IsButtonClicked(deleteButtonRectangle) && canStartNewOperation)
        {
            int parsedValue = std::atoi(valueTextBoxValue.c_str());
            bst.UpdateLayout();
            BuildDeleteSteps(parsedValue);
            shouldClearTextOnNextClick = true;
        }

        if (IsButtonClicked(clearButtonRectangle))
        {
            bst.Clear();
            ClearCurrentOperation();
            valueTextBoxValue.clear();
            shouldClearTextOnNextClick = false;
            deletedNodeEffect.isActive = false;
            lastOperationLabel = "Clear tree";
        }

        if (IsButtonClicked(randomTreeButtonRectangle) && !isOperationInProgress)
        {
            bst.Clear();
            ClearCurrentOperation();
            valueTextBoxValue.clear();
            shouldClearTextOnNextClick = false;
            deletedNodeEffect.isActive = false;

            const int randomNodeCount = 10;
            for (int index = 0; index < randomNodeCount; index++)
            {
                int randomValue = GetRandomValue(0, 99);
                bst.InsertImmediate(randomValue);
            }
            bst.UpdateLayout();
            lastOperationLabel = "Random tree (10 nodes)";
        }

        bool hasSteps = !currentSteps.empty();
        std::string playPauseLabel = isAutoPlayEnabled ? "Pause" : "Play";

        if (IsButtonClicked(playPauseButtonRectangle) && hasSteps)
        {
            isAutoPlayEnabled = !isAutoPlayEnabled;
            if (!isOperationInProgress)
            {
                isOperationInProgress = true;
            }
        }

        if (IsButtonClicked(stepButtonRectangle) && hasSteps)
        {
            isAutoPlayEnabled = false;
            if (!isOperationInProgress)
            {
                isOperationInProgress = true;
            }
            stepTimerSeconds = 0.0f;
            AdvanceToNextStep();
        }

        if (IsButtonClicked(resetOpButtonRectangle) && hasSteps)
        {
            ClearCurrentOperation();
        }

        if (isAutoPlayEnabled && isOperationInProgress)
        {
            stepTimerSeconds += deltaTime;
            if (stepTimerSeconds >= STEP_INTERVAL_SECONDS)
            {
                stepTimerSeconds = 0.0f;
                AdvanceToNextStep();
            }
        }

        bst.UpdateLayout();
        UpdateTreeVisualEffectsRecursive(bst.GetRootPointer(), deltaTime);
        UpdateAnimatedEdge(deltaTime);
        UpdateDeletedNodeEffect(deltaTime);

        // -------------------------
        // Draw
        // -------------------------

        BeginDrawing();
        ClearBackground(COLOUR_BACKGROUND);

        DrawText("Binary Search Tree Visualiser", 30, 18, 30, COLOUR_TEXT_MAIN);
        DrawText("Insert, Search, Delete with step-by-step pseudocode", 30, 52, 20, COLOUR_TEXT_MUTED);

        DrawText("Value / Target:",
            static_cast<int>(VALUE_TEXTBOX_RECT.x),
            static_cast<int>(VALUE_TEXTBOX_RECT.y - 24),
            20,
            COLOUR_TEXT_MAIN);

        DrawRectangleRec(VALUE_TEXTBOX_RECT, WHITE);
        DrawRectangleLinesEx(VALUE_TEXTBOX_RECT, 2.0f, BLACK);
        DrawText(valueTextBoxValue.c_str(),
            static_cast<int>(VALUE_TEXTBOX_RECT.x + 8),
            static_cast<int>(VALUE_TEXTBOX_RECT.y + 10),
            20,
            COLOUR_TEXT_MAIN);

        DrawButton(insertButtonRectangle, "Insert", !canStartNewOperation);
        DrawButton(searchButtonRectangle, "Search", !canStartNewOperation);
        DrawButton(deleteButtonRectangle, "Delete", !canStartNewOperation);
        DrawButton(clearButtonRectangle, "Clear Tree", false);
        DrawButton(randomTreeButtonRectangle, "Random Tree", isOperationInProgress);

        // Tree panel, then overlays
        DrawTreePanel();
        DrawDeletedNodeEffect();
        DrawNodeTooltip();

        // Traversal selector + traversal animation button
        DrawButton(inOrderButtonRectangle, "In-order", currentTraversalType == TraversalType::InOrder);
        DrawButton(preOrderButtonRectangle, "Pre-order", currentTraversalType == TraversalType::PreOrder);
        DrawButton(postOrderButtonRectangle, "Post-order", currentTraversalType == TraversalType::PostOrder);
        DrawButton(animateInOrderButtonRectangle, "Animate In-order", isOperationInProgress);

        int highlightedLineIndex = -1;
        std::string currentStepDescriptionText;

        if (isOperationInProgress &&
            currentStepIndex >= 0 &&
            currentStepIndex < static_cast<int>(currentSteps.size()))
        {
            highlightedLineIndex = currentSteps[currentStepIndex].highlightedPseudocodeLineIndex;
            currentStepDescriptionText = currentSteps[currentStepIndex].descriptionText;
        }

        DrawPseudocodePanel(currentOperationType, highlightedLineIndex);

        int descriptionFontSize = 18;
        int descriptionX = 30;
        int descriptionY = SCREEN_HEIGHT - 90;

        Rectangle descRect = { descriptionX - 10.0f, descriptionY - 10.0f, 1120.0f, 70.0f };
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
            DrawText("Enter a value, choose an operation, and watch the BST algorithm animate step by step.",
                descriptionX,
                descriptionY,
                descriptionFontSize,
                COLOUR_TEXT_MUTED);
        }

        if (!currentPathValues.empty())
        {
            std::string pathText = "Path: ";
            for (int index = 0; index < static_cast<int>(currentPathValues.size()); index++)
            {
                pathText += std::to_string(currentPathValues[index]);
                if (index < static_cast<int>(currentPathValues.size()) - 1)
                {
                    pathText += " -> ";
                }
            }

            DrawText(pathText.c_str(),
                descriptionX,
                descriptionY + 24,
                descriptionFontSize - 2,
                COLOUR_TEXT_MUTED);
        }

        DrawButton(playPauseButtonRectangle, playPauseLabel, !hasSteps);
        DrawButton(stepButtonRectangle, "Step", !hasSteps);
        DrawButton(resetOpButtonRectangle, "Reset", !hasSteps);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
