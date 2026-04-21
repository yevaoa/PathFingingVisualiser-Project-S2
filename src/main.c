#include "graph.h"
#include <math.h>

float GetDist(Vector2 v1, Vector2 v2) { 
    return sqrtf(powf(v1.x - v2.x, 2) + powf(v1.y - v2.y, 2)); 
}

void DrawPersistentArrow(Vector2 start, Vector2 end, Color col, float thick) {
    float angle = atan2f(end.y - start.y, end.x - start.x);
    float dist = GetDist(start, end) - 22; 
    Vector2 tip = { start.x + cosf(angle) * dist, start.y + sinf(angle) * dist };
    DrawLineEx(start, tip, thick, col);
    DrawPoly(tip, 3, 8, angle * RAD2DEG, col);
}

int main() {
    InitWindow(1000, 700, "PathFinder");
    SetTargetFPS(60);

    Node nodes[MAX_NODES];
    Edge edges[MAX_NODES * 4];
    int nodeCount = 0, edgeCount = 0;
    int dragStartNode = -1;
    int startNodeIdx = 0, endNodeIdx = -1;

    MinHeap pq = { .size = 0 };
    bool running = false;
    float stepTimer = 0;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Vector2 mouse = GetMousePosition();

        if (!running) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                int clicked = -1;
                for (int i = 0; i < nodeCount; i++) 
                    if (CheckCollisionPointCircle(mouse, nodes[i].pos, 20)) { clicked = i; break; }

                // erase
                if (IsKeyDown(KEY_Z) && clicked != -1) {
                    // remove edges connected to this node
                    for (int i = 0; i < edgeCount; i++) {
                        if (edges[i].u == clicked || edges[i].v == clicked) {
                            edges[i] = edges[--edgeCount];
                            i--; // Re-check the new edge shifted into this spot
                        }
                    }
                    // shift nodes array to fill the gap
                    for (int i = clicked; i < nodeCount - 1; i++) nodes[i] = nodes[i + 1];
                    nodeCount--;
                    
                    // update edge references because indices changed
                    for (int i = 0; i < edgeCount; i++) {
                        if (edges[i].u > clicked) edges[i].u--;
                        if (edges[i].v > clicked) edges[i].v--;
                    }
                    if (startNodeIdx == clicked) startNodeIdx = 0;
                    if (endNodeIdx == clicked) endNodeIdx = -1;
                }
                // start point and end point
                else if (IsKeyDown(KEY_S) && clicked != -1) startNodeIdx = clicked;
                else if (IsKeyDown(KEY_E) && clicked != -1) endNodeIdx = clicked;
                else if (nodeCount < MAX_NODES && clicked == -1) {
                    nodes[nodeCount] = (Node){mouse, nodeCount, INF, -1, false, 0.0f};
                    nodeCount++;
                }
            }

            // edge creation
            if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                for (int i = 0; i < nodeCount; i++) 
                    if (CheckCollisionPointCircle(mouse, nodes[i].pos, 20)) { dragStartNode = i; break; }
            }
            if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON) && dragStartNode != -1) {
                for (int i = 0; i < nodeCount; i++) {
                    if (i != dragStartNode && CheckCollisionPointCircle(mouse, nodes[i].pos, 20)) {
                        int weight = (int)GetDist(nodes[dragStartNode].pos, nodes[i].pos) / 10;
                        edges[edgeCount++] = (Edge){dragStartNode, i, weight};
                        break;
                    }
                }
                dragStartNode = -1;
            }
        }

        // reset and clear
        if (IsKeyPressed(KEY_R)) { 
            running = false; pq.size = 0;
            for(int i=0; i<nodeCount; i++) { nodes[i].dist = INF; nodes[i].visited = false; nodes[i].parent = -1; nodes[i].flash = 0; }
        }
        if (IsKeyPressed(KEY_C)) { nodeCount = 0; edgeCount = 0; running = false; startNodeIdx = 0; endNodeIdx = -1; }
        if (IsKeyPressed(KEY_SPACE) && nodeCount > 0 && !running) {
            running = true; nodes[startNodeIdx].dist = 0; Push(&pq, 0, startNodeIdx);
        }

        // algorithm
        if (running && pq.size > 0) {
            stepTimer += dt;
            if (stepTimer >= 0.3f) {
                HeapNode top = Pop(&pq);
                if (!nodes[top.u].visited) {
                    nodes[top.u].visited = true;
                    if (top.u == endNodeIdx) pq.size = 0; 
                    else {
                        for (int i = 0; i < edgeCount; i++) {
                            if (edges[i].u == top.u) {
                                int v = edges[i].v;
                                if (nodes[top.u].dist + edges[i].weight < nodes[v].dist) {
                                    nodes[v].dist = nodes[top.u].dist + edges[i].weight;
                                    nodes[v].parent = top.u;
                                    nodes[v].flash = 1.0f;
                                    Push(&pq, nodes[v].dist, v);
                                }
                            }
                        }
                    }
                }
                stepTimer = 0;
            }
        }

        for(int i=0; i<nodeCount; i++) if(nodes[i].flash > 0) nodes[i].flash -= dt * 2.0f;

        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        for (int i = 0; i < edgeCount; i++) {
            Color col = (nodes[edges[i].v].parent == edges[i].u) ? SKYBLUE : LIGHTGRAY;
            DrawPersistentArrow(nodes[edges[i].u].pos, nodes[edges[i].v].pos, col, 2.0f);
            Vector2 mid = {(nodes[edges[i].u].pos.x + nodes[edges[i].v].pos.x)/2, (nodes[edges[i].u].pos.y + nodes[edges[i].v].pos.y)/2};
            DrawText(TextFormat("%d", edges[i].weight), mid.x + 5, mid.y + 5, 16, MAROON);
        }

        for (int i = 0; i < nodeCount; i++) {
            Color c = nodes[i].visited ? SKYBLUE : GOLD;
            if (i == startNodeIdx) c = GREEN;
            if (i == endNodeIdx) c = RED;
            if (nodes[i].flash > 0) c = ColorLerp(c, YELLOW, nodes[i].flash);

            DrawCircleV(nodes[i].pos, 20, c);
            DrawCircleLines(nodes[i].pos.x, nodes[i].pos.y, 20, BLACK);
            DrawText(TextFormat("%d", i), nodes[i].pos.x - 5, nodes[i].pos.y - 8, 18, BLACK);
            DrawText(nodes[i].dist == INF ? "inf" : TextFormat("%d", nodes[i].dist), nodes[i].pos.x - 15, nodes[i].pos.y + 25, 15, DARKGRAY);
        }

        DrawText("S+Click: Start Point | E+Click: End Point| Z+Click: Erase | Space: Run | R: Reset | C: Clear", 10, 10, 20, DARKGRAY);
        if (dragStartNode != -1) DrawLineV(nodes[dragStartNode].pos, mouse, GRAY);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}