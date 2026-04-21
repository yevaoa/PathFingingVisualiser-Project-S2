#ifndef GRAPH_H
#define GRAPH_H

#include "raylib.h"
#include <stdbool.h>

#define MAX_NODES 30
#define INF 999999

typedef struct {
    Vector2 pos;
    int id;
    int dist;
    int parent;   
    bool visited;
    float flash;  
} Node;

typedef struct {
    int u, v;     
    int weight;
} Edge;

typedef struct {
    int dist;
    int u;
} HeapNode;

typedef struct {
    HeapNode data[200];
    int size;
} MinHeap;

void Push(MinHeap* hp, int dist, int u);
HeapNode Pop(MinHeap* hp);

#endif