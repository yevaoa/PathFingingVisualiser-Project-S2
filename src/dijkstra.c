#include "graph.h"

void Push(MinHeap* hp, int dist, int u) {
    hp->data[hp->size] = (HeapNode){dist, u};
    int i = hp->size++;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (hp->data[p].dist <= hp->data[i].dist) break;
        HeapNode temp = hp->data[p];
        hp->data[p] = hp->data[i];
        hp->data[i] = temp;
        i = p;
    }
}

HeapNode Pop(MinHeap* hp) {
    HeapNode root = hp->data[0];
    hp->data[0] = hp->data[--hp->size];
    int i = 0;
    while (1) {
        int l = 2 * i + 1, r = 2 * i + 2, smallest = i;
        if (l < hp->size && hp->data[l].dist < hp->data[smallest].dist) smallest = l;
        if (r < hp->size && hp->data[r].dist < hp->data[smallest].dist) smallest = r;
        if (smallest == i) break;
        HeapNode temp = hp->data[i];
        hp->data[i] = hp->data[smallest];
        hp->data[smallest] = temp;
        i = smallest;
    }
    return root;
}
