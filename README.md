# PathFindingVisualiser-Project-S2

An interactive educational tool designed to visualise Dijkstra's shortest path algorithm. This application allows users to create directed graphs manually and observe the process used to calculate optimal paths.

## Development and Tools

Written in **C** and developed in **Visual Studio Code**, using **Raylib 5.0** library for the graphical user interface and animation.

## Core Features

* Build nodes and directed edges via mouse input.
* Nodes flash yellow when a shorter path is identified and distances are updated.
* Explicitly define Starting (Green) and Ending (Red) points using keyboard modifiers given below.
* Display real-time edge weights and node distances from the source.
* Built using a custom Min-Heap for efficient pathfinding.

## How to Use
* Download the Zip file then extract it
* Navigate to main.c in src file and double click it
* Press F5 on keyboard to run

## Important Note

Ensure Raylib is installed on your system before compiling or running the code. 

Source: https://www.raylib.com/

## User Controls

| Action | Command |
| :--- | :--- |
| Place Node | Left Click |
| Create Directed Edge | Right Click Drag (Node to Node) |
| Set Start Node | S + Left Click |
| Set Finish Node | E + Left Click |
| Erase Node/Edge | Z + Left Click |
| Execute Algorithm | Spacebar |
| Reset Logic | R Key |
| Clear Canvas | C Key |

## An Example
<img width="800" height="576" alt="example (2)" src="https://github.com/user-attachments/assets/2d625f1e-65c1-405a-86d4-aadcbd8e2d85" />


## Known Issues

The main one is probably the not-so pleasant UI, which I will be working on later! :) 
If you find any other issues please let me know.

## Credits

This project was built using the **Raylib-CPP-Starter-Template-for-VSCODE-V2** by **educ8s** which provided essential materials to get started with Raylib easily.
Later on I've changed some configurations to be able to write the codes in C.

Source: [github.com/educ8s/Raylib-CPP-Starter-Template-for-VSCODE-V2](https://github.com/educ8s/Raylib-CPP-Starter-Template-for-VSCODE-V2)
