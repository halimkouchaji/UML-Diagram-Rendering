# UML Diagram Rendering

A C++-based UML diagram processing and rendering system that converts UML diagrams from **XMI** into structured **JSON** data and renders them as **SVG** visualizations.

## Supported Diagrams

* Activity Diagrams
* Class Diagrams

## Workflow

```text
XMI Input
   ↓
XMI Parser
   ↓
Diagram Processing
   ↓
Layout & Rendering
   ↓
JSON + SVG Output
```

## Features

* Parse UML diagrams from XMI files
* Extract diagram elements and relationships
* Process and structure diagram data
* Calculate element positions and layout
* Generate JSON representation
* Render diagrams as SVG
* Support for Activity and Class Diagrams

## Technologies

* **C++**
* **XMI** — Input format
* **JSON** — Structured output
* **SVG** — Diagram visualization

## Input & Output

### Input

```text
.xmi
```

The system reads UML models stored in XMI format.

### Output

```text
.json
.svg
```

* **JSON:** Contains the processed diagram structure, elements, relationships, and layout information.
* **SVG:** Contains the final rendered diagram for visualization.



## Purpose

The project aims to provide an automated pipeline for transforming UML models into structured data and visually rendered diagrams, making it easier to analyze, process, and visualize UML diagrams programmatically.
