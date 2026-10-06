# Advanced Programming

A third-year project for the Advanced Programming module at SETU Carlow, written in **C++** with the [raylib](https://www.raylib.com/) graphics library. The repository holds three interactive visualisers that animate classic data structures step by step. Each one shows the pseudocode alongside the animation and highlights the line being executed, so you can follow exactly what the algorithm is doing.

## Projects

| Project | Data structure | Operations | Tutorial |
| --- | --- | --- | --- |
| [ArrayVisualiser](ArrayVisualiser) | Fixed-size array of 10 integers | Insert, delete by value, search, traverse, bubble sort | [ArrayVisualiserTutorial.docx](ArrayVisualiserTutorial.docx) |
| [BinarySearchTree](BinarySearchTree) | Binary search tree | Insert, search, delete, in-order, pre-order and post-order traversal | [BinarySearchTreeTutorial.docx](BinarySearchTreeTutorial.docx) |
| [LinkedListVisualiser](LinkedListVisualiser) | Singly linked list with head and tail pointers | Insert at head, insert at tail, search, delete | [LinkedListVisualiserTutorial.docx](LinkedListVisualiserTutorial.docx) |

Each tutorial covers setting the project up in Visual Studio, using the controls, how each operation works and how the code is organised.

## Features

- Step-by-step animations of every operation, with a plain-English description of each step.
- Live pseudocode panel that highlights the current line.
- Colour-coded highlighting for the item being examined, found items and items about to be deleted.
- Array Visualiser: a TEMP box that shows how a bubble sort swaps two values, and a resizable window.
- Binary Search Tree Visualiser: play, pause and single-step controls, a random tree generator, hover tooltips showing depth and subtree size, and live node count and height.
- Linked List Visualiser: arrows that are redirected as pointers change, with head and tail labels.

## How this project was built

This project was built under a specific constraint: only free-tier AI tools could be used to write the code, and no code could be changed by hand. All debugging and every correction was done by telling the AI what was wrong and having it fix the code.

## Requirements

- Windows 10 or 11
- Visual Studio 2022 with the **Desktop development with C++** workload (MSVC v143 toolset)
- An internet connection on the first build so NuGet can restore raylib 5.5.0

## Getting started

1. Clone the repository:

   ```bash
   git clone https://github.com/Markus-Bear/AdvancedProgramming-Y2.git
   ```

2. Open the `.sln` file inside the project folder you want, for example `ArrayVisualiser/ArrayVisualiser.sln`.
3. Set the configuration to **Debug** and the platform to **x64**.
4. Build with **Ctrl + Shift + B**. The raylib NuGet package is restored automatically.
5. Run with **Ctrl + F5**.

The projects use the **Console** linker subsystem because they start from a standard `int main()`, so a console window opens behind the visualiser window. This is expected.

## Repository structure

```
AdvancedProgramming-Y2/
├── ArrayVisualiser/
│   ├── ArrayVisualiser.cpp
│   └── ArrayVisualiser.sln
├── BinarySearchTree/
│   ├── BinarySearchTree.cpp
│   └── BinarySearchTree.sln
├── LinkedListVisualiser/
│   ├── LinkedListVisualiser.cpp
│   └── LinkedListVisualiser.sln
├── ArrayVisualiserTutorial.docx
├── BinarySearchTreeTutorial.docx
├── LinkedListVisualiserTutorial.docx
└── README.md
```

## Built with

- C++ (Visual Studio 2022, MSVC v143)
- [raylib 5.5.0](https://www.raylib.com/), installed through NuGet

## Author

Mark Mukiiza, Software Development student at SETU Carlow. [LinkedIn](https://www.linkedin.com/in/mukiiza-mark)
