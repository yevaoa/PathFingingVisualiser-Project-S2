#include "raylib.h"
#include "graph.h"
#include <math.h>
#include <stdlib.h>
 
//colors
#define C_BG          (Color){  8, 12, 28, 255}
#define C_GRID        (Color){ 30, 40, 70,  60}
#define C_EDGE        (Color){ 60, 90,140, 130}
#define C_EDGE_TREE   (Color){ 80,180,240, 200}
#define C_NODE_IDLE   (Color){ 30,160,180, 255}
#define C_NODE_VIS    (Color){ 60,160,255, 255}
#define C_NODE_START  (Color){ 50,230,100, 255}
#define C_NODE_END    (Color){240, 60,160, 255}
#define C_NODE_BLOCK  (Color){160, 30, 40, 255}
#define C_PATH_CORE   (Color){  0,240,220, 255}
#define C_PATH_GLOW   (Color){  0,200,180,  80}
#define C_PATH_MARCH  (Color){255,255,255, 180}
#define C_LABEL       (Color){200,220,255, 255}
#define C_HUD_BG      (Color){  8, 14, 32, 220}
#define C_SPARK       (Color){255,200, 80, 255}
#define C_WARN        (Color){255, 70, 70, 255}
 
#define NODE_R           22.0f
#define GLOW_R1          34.0f
#define GLOW_R2          26.0f
#define WIN_W            1100
#define WIN_H             720
#define HUD_H              44
#define EDGE_BLOCK_DIST   14.0f   //min distance from edge segment to place a node
 
//font
static Font  gFont;
static float gFontSize = 17.f;   //base size for UI text
static bool  gFontLoaded = false;
 
//raw text using custom font (or default fallback)
static void TDraw(const char* text, int x, int y, float size, Color col) {
    if (gFontLoaded)
        DrawTextEx(gFont, text, (Vector2){(float)x,(float)y}, size, 1.f, col);
    else
        DrawText(text, x, y, (int)size, col);
}
 
//measure text width
static int TMeasure(const char* text, float size) {
    if (gFontLoaded)
        return (int)MeasureTextEx(gFont, text, size, 1.f).x;
    return MeasureText(text, (int)size);
}
 
//ripples
#define MAX_RIPPLES 80
typedef struct { Vector2 pos; float t; Color col; bool active; } Ripple;
static Ripple ripples[MAX_RIPPLES];
 
static void SpawnRipple(Vector2 pos, Color col) {
    for (int i = 0; i < MAX_RIPPLES; i++)
        if (!ripples[i].active) { ripples[i] = (Ripple){pos, 0.f, col, true}; return; }
}
 
static void UpdateDrawRipples(float dt) {
    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (!ripples[i].active) continue;
        ripples[i].t += dt * 1.4f;
        if (ripples[i].t >= 1.f) { ripples[i].active = false; continue; }
        float ease = 1.f - ripples[i].t * ripples[i].t;
        DrawCircleLinesV(ripples[i].pos, NODE_R + 50.f * ripples[i].t, Fade(ripples[i].col, ease * 0.55f));
        DrawCircleLinesV(ripples[i].pos, NODE_R + 28.f * ripples[i].t, Fade(ripples[i].col, ease * 0.35f));
    }
}
 
//particles
#define MAX_PARTS 300
typedef struct { Vector2 pos, vel; float life, maxLife, size; bool active; } Particle;
static Particle parts[MAX_PARTS];
 
static void SpawnSparks(Vector2 origin, int n) {
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < MAX_PARTS; i++) {
            if (!parts[i].active) {
                float ang = (float)(rand() % 628) / 100.f;
                float spd = 40.f + (float)(rand() % 80);
                parts[i].pos     = origin;
                parts[i].vel     = (Vector2){cosf(ang)*spd, sinf(ang)*spd};
                parts[i].maxLife = 0.55f + (float)(rand()%30)/100.f;
                parts[i].life    = parts[i].maxLife;
                parts[i].size    = 2.5f + (float)(rand()%20)/10.f;
                parts[i].active  = true;
                break;
            }
        }
    }
}
 
static void UpdateDrawParticles(float dt) {
    for (int i = 0; i < MAX_PARTS; i++) {
        if (!parts[i].active) continue;
        parts[i].life -= dt;
        if (parts[i].life <= 0.f) { parts[i].active = false; continue; }
        parts[i].pos.x += parts[i].vel.x * dt;
        parts[i].pos.y += parts[i].vel.y * dt;
        parts[i].vel.x *= 0.92f;
        parts[i].vel.y *= 0.92f;
        float a = parts[i].life / parts[i].maxLife;
        DrawCircleV(parts[i].pos, parts[i].size * a,        Fade(C_SPARK, a * 0.9f));
        DrawCircleV(parts[i].pos, parts[i].size * a * 0.5f, Fade(WHITE,   a * 0.6f));
    }
}
 
//final path
#define MAX_PATH MAX_NODES
static int   pathBuf[MAX_PATH];
static int   pathLen     = 0;
static float pulseT      = 0.f;
static float marchOffset = 0.f;
 
static void BuildPath(Node* nodes, int endIdx) {
    pathLen = 0;
    if (endIdx == -1 || nodes[endIdx].dist == INF) return;
    int cur = endIdx;
    while (cur != -1 && pathLen < MAX_PATH) { pathBuf[pathLen++] = cur; cur = nodes[cur].parent; }
    for (int i = 0, j = pathLen-1; i < j; i++, j--)
        { int t = pathBuf[i]; pathBuf[i] = pathBuf[j]; pathBuf[j] = t; }
}
 
static void DrawDashedLine(Vector2 a, Vector2 b, float dashLen, float offset, Color col, float thick) {
    float dx = b.x-a.x, dy = b.y-a.y;
    float total = sqrtf(dx*dx+dy*dy); if (total < 1.f) return;
    float nx = dx/total, ny = dy/total, pos = -offset;
    while (pos < total) {
        float s = pos<0.f?0.f:pos, e = pos+dashLen;
        if (e > total) e = total;
        if (s < e) DrawLineEx((Vector2){a.x+nx*s,a.y+ny*s},(Vector2){a.x+nx*e,a.y+ny*e},thick,col);
        pos += dashLen * 2.f;
    }
}
 
static void DrawFinalPath(Node* nodes) {
    if (pathLen < 2) return;
    for (int i = 0; i < pathLen-1; i++) {
        Vector2 a = nodes[pathBuf[i]].pos, b = nodes[pathBuf[i+1]].pos;
        DrawLineEx(a, b, 14.f, Fade(C_PATH_GLOW, 0.5f));
        DrawLineEx(a, b,  8.f, Fade(C_PATH_GLOW, 0.8f));
        DrawLineEx(a, b,  3.f, C_PATH_CORE);
        DrawDashedLine(a, b, 14.f, marchOffset, C_PATH_MARCH, 2.f);
    }
    float frac = pulseT*(float)(pathLen-1);
    int   seg  = (int)frac; if (seg>=pathLen-1) seg=pathLen-2;
    float t    = frac-(float)seg;
    Vector2 a  = nodes[pathBuf[seg]].pos, b = nodes[pathBuf[seg+1]].pos;
    Vector2 p  = {a.x+(b.x-a.x)*t, a.y+(b.y-a.y)*t};
    DrawCircleV(p, 14.f, Fade(C_PATH_CORE,0.35f));
    DrawCircleV(p,  9.f, C_PATH_CORE);
    DrawCircleV(p,  5.f, WHITE);
}
 
//geometry helpers
static float GetDist(Vector2 a, Vector2 b) {
    return sqrtf(powf(a.x-b.x,2)+powf(a.y-b.y,2));
}
 
//point to segment distance (clamped)
static float PointSegDist(Vector2 p, Vector2 a, Vector2 b) {
    float dx=b.x-a.x, dy=b.y-a.y;
    float len2 = dx*dx+dy*dy;
    if (len2 < 0.0001f) return GetDist(p,a);
    float t = ((p.x-a.x)*dx + (p.y-a.y)*dy) / len2;
    if (t < 0.f) t = 0.f;
    if (t > 1.f) t = 1.f;
    Vector2 proj = {a.x+t*dx, a.y+t*dy};
    return GetDist(p, proj);
}
 
//returns true if point is too close to any existing edge
static bool IsOnEdge(Vector2 p, Node* nodes, Edge* edges, int edgeCount) {
    for (int i=0; i<edgeCount; i++) {
        Vector2 a = nodes[edges[i].u].pos, b = nodes[edges[i].v].pos;
        if (PointSegDist(p, a, b) < EDGE_BLOCK_DIST) return true;
    }
    return false;
}
 
//draw helpers
static void DrawGlowArrow(Vector2 start, Vector2 end, Color col, float thick, float alpha) {
    float angle = atan2f(end.y-start.y, end.x-start.x);
    float dist  = GetDist(start, end) - NODE_R - 2.f;
    if (dist < 4.f) return;
    Vector2 tip = {start.x+cosf(angle)*dist, start.y+sinf(angle)*dist};
    DrawLineEx(start, tip, thick+4.f, Fade(col, alpha*0.25f));
    DrawLineEx(start, tip, thick,     Fade(col, alpha));
    DrawPoly(tip, 3, 7, angle*RAD2DEG, Fade(col, alpha));
}
 
static void DrawGlowNode(Vector2 pos, Color core, float flash) {
    if (flash > 0.f) core = ColorLerp(core, YELLOW, flash);
    DrawCircleV(pos, GLOW_R1, Fade(core, 0.12f+flash*0.08f));
    DrawCircleV(pos, GLOW_R2, Fade(core, 0.22f+flash*0.08f));
    DrawCircleV(pos, NODE_R,  core);
    DrawCircleLinesV(pos, NODE_R,     Fade(WHITE, 0.50f));
    DrawCircleLinesV(pos, NODE_R+1.f, Fade(core,  0.40f));
}
 
static void DrawPill(int x, int y, int w, int h, Color bg) {
    DrawRectangleRounded((Rectangle){(float)x,(float)y,(float)w,(float)h}, 0.5f, 6, bg);
}
 
static void DrawDotGrid(float t) {
    int cols=WIN_W/36+1, rows=WIN_H/36+1;
    for (int r=0;r<rows;r++)
        for (int c=0;c<cols;c++) {
            float pulse = 0.35f+0.28f*sinf(t*0.55f+(float)(r+c)*0.38f);
            DrawCircleV((Vector2){(float)c*36+18,(float)r*36+18}, 1.4f, Fade(C_GRID,pulse));
        }
}
 
//HUD key hint
static void DrawHint(int* x, int y, const char* key, const char* desc) {
    //key badge
    int kw = TMeasure(key, gFontSize-3);
    DrawRectangleRounded((Rectangle){(float)*x,(float)y+2,(float)(kw+10),20.f},0.4f,4,
                         (Color){50,70,110,200});
    DrawRectangleRoundedLines((Rectangle){(float)*x,(float)y+2,(float)(kw+10),20.f},0.4f,4,
                              Fade(C_PATH_CORE,0.4f));
    TDraw(key, *x+5, y+4, gFontSize-3, (Color){200,230,255,240});
    *x += kw+14;
    //description
    int dw = TMeasure(desc, gFontSize-3);
    TDraw(desc, *x, y+4, gFontSize-3, (Color){130,160,200,180});
    *x += dw+18;
}
 

int main(void) {
    InitWindow(WIN_W, WIN_H, "PathFinder");
    SetTargetFPS(60);
 
    //load font in same directory as executable
    if (FileExists("Oxanium-Regular.ttf")) {
        gFont = LoadFontEx("Oxanium-Regular.ttf", 64, NULL, 0);
        SetTextureFilter(gFont.texture, TEXTURE_FILTER_BILINEAR);
        gFontLoaded = true;
    }
 
    Node  nodes[MAX_NODES];
    Edge  edges[MAX_NODES * 4];
    int   nodeCount=0, edgeCount=0;
    int   dragStart=-1;
    int   startIdx=0, endIdx=-1;
    bool  blocked[MAX_NODES];
    for (int i=0;i<MAX_NODES;i++) blocked[i]=false;
 
    MinHeap pq       = {.size=0};
    bool running     = false;
    bool finished    = false;
    float stepTimer  = 0.f;
    float globalT    = 0.f;
    const float STEP = 0.6f;
 
    //edge block feedback
    float warnTimer  = 0.f;   //>0 → show red cursor + tooltip
 
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        globalT += dt;
        if (warnTimer > 0.f) warnTimer -= dt;
        Vector2 mouse = GetMousePosition();
 
        //hover edge: only true while warnTimer is active (triggered by a blocked click)
        bool hoverEdge = (warnTimer > 0.f);
 
        if (!running) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                int hit=-1;
                for (int i=0;i<nodeCount;i++)
                    if (CheckCollisionPointCircle(mouse, nodes[i].pos, NODE_R)) { hit=i; break; }
 
                if (IsKeyDown(KEY_Z) && hit != -1) {
                    for (int i=0;i<edgeCount;i++)
                        if (edges[i].u==hit||edges[i].v==hit) { edges[i]=edges[--edgeCount]; i--; }
                    for (int i=hit;i<nodeCount-1;i++) { nodes[i]=nodes[i+1]; blocked[i]=blocked[i+1]; }
                    nodeCount--;
                    for (int i=0;i<edgeCount;i++) {
                        if (edges[i].u>hit) edges[i].u--;
                        if (edges[i].v>hit) edges[i].v--;
                    }
                    if (startIdx==hit) startIdx=0;
                    if (endIdx  ==hit) endIdx  =-1;
                } else if (IsKeyDown(KEY_B) && hit != -1) {
                    blocked[hit]=!blocked[hit];
                    if (blocked[hit]&&startIdx==hit) startIdx=-1;
                    if (blocked[hit]&&endIdx  ==hit) endIdx  =-1;
                } else if (IsKeyDown(KEY_S) && hit != -1 && !blocked[hit]) {
                    startIdx=hit;
                } else if (IsKeyDown(KEY_E) && hit != -1 && !blocked[hit]) {
                    endIdx=hit;
                } else if (nodeCount < MAX_NODES && hit == -1) {

                    //edge collision guard
                    if (IsOnEdge(mouse, nodes, edges, edgeCount)) {
                        warnTimer = 1.2f;   //show warning for 1.2 seconds
                    } else {
                        nodes[nodeCount]=(Node){mouse,nodeCount,INF,-1,false,0.f};
                        blocked[nodeCount]=false;
                        nodeCount++;
                        SpawnSparks(mouse, 8);
                    }
                }
            }
            if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                for (int i=0;i<nodeCount;i++)
                    if (CheckCollisionPointCircle(mouse,nodes[i].pos,NODE_R)) { dragStart=i; break; }
            }
            if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON) && dragStart != -1) {
                for (int i=0;i<nodeCount;i++) {
                    if (i!=dragStart && CheckCollisionPointCircle(mouse,nodes[i].pos,NODE_R)) {
                        int w=(int)GetDist(nodes[dragStart].pos,nodes[i].pos)/10;
                        edges[edgeCount++]=(Edge){dragStart,i,w};
                        SpawnSparks(nodes[i].pos,6);
                        break;
                    }
                }
                dragStart=-1;
            }
        }
 
        if (IsKeyPressed(KEY_R)) {
            running=false; finished=false; pq.size=0; pathLen=0; pulseT=0.f; marchOffset=0.f; warnTimer=0.f;
            for (int i=0;i<MAX_RIPPLES;i++) ripples[i].active=false;
            for (int i=0;i<MAX_PARTS;i++)   parts[i].active=false;
            for (int i=0;i<nodeCount;i++) { nodes[i].dist=INF; nodes[i].visited=false; nodes[i].parent=-1; nodes[i].flash=0.f; }
        }
        if (IsKeyPressed(KEY_C)) {
            nodeCount=0; edgeCount=0; running=false; finished=false;
            startIdx=0; endIdx=-1; pathLen=0; pulseT=0.f; marchOffset=0.f; warnTimer=0.f;
            for (int i=0;i<MAX_RIPPLES;i++) ripples[i].active=false;
            for (int i=0;i<MAX_PARTS;i++)   parts[i].active=false;
        }
        if (IsKeyPressed(KEY_SPACE)&&nodeCount>0&&!running&&!finished&&startIdx>=0&&!blocked[startIdx]) {
            running=true; nodes[startIdx].dist=0; Push(&pq,0,startIdx);
            SpawnSparks(nodes[startIdx].pos,15);
        }
 
        if (running && pq.size > 0) {
            stepTimer += dt;
            if (stepTimer >= STEP) {
                HeapNode top = Pop(&pq);
                if (blocked[top.u]) { stepTimer=0.f; }
                else if (!nodes[top.u].visited) {
                    nodes[top.u].visited=true;
                    SpawnRipple(nodes[top.u].pos, C_NODE_VIS);
                    SpawnSparks(nodes[top.u].pos, 10);
                    if (top.u==endIdx) {
                        pq.size=0; running=false; finished=true;
                        BuildPath(nodes,endIdx);
                        SpawnSparks(nodes[top.u].pos,30);
                    } else {
                        for (int i=0;i<edgeCount;i++) {
                            if (edges[i].u==top.u) {
                                int v=edges[i].v;
                                if (blocked[v]) continue;
                                if (nodes[top.u].dist+edges[i].weight < nodes[v].dist) {
                                    nodes[v].dist  =nodes[top.u].dist+edges[i].weight;
                                    nodes[v].parent=top.u;
                                    nodes[v].flash =1.0f;
                                    SpawnRipple(nodes[v].pos, C_PATH_CORE);
                                    SpawnSparks(nodes[v].pos, 7);
                                    Push(&pq,nodes[v].dist,v);
                                }
                            }
                        }
                    }
                    stepTimer=0.f;
                }
            }
        } else if (running && pq.size==0) {
            running=false; finished=true;
            BuildPath(nodes,endIdx);
        }
 
        for (int i=0;i<nodeCount;i++) if(nodes[i].flash>0.f) nodes[i].flash-=dt*1.4f;
        if (finished && pathLen>=2) {
            pulseT     +=dt*0.42f; if(pulseT>1.f) pulseT=0.f;
            marchOffset+=dt*28.f;  if(marchOffset>28.f) marchOffset=0.f;
        }
 
        //drawing
        BeginDrawing();
        ClearBackground(C_BG);
        DrawDotGrid(globalT);
 
        //base edges
        for (int i=0;i<edgeCount;i++) {
            Vector2 a=nodes[edges[i].u].pos, b=nodes[edges[i].v].pos;
            DrawGlowArrow(a, b, C_EDGE, 1.5f, 1.0f);
            Vector2 mid={(a.x+b.x)/2.f+6.f,(a.y+b.y)/2.f+4.f};
            const char* wt=TextFormat("%d",edges[i].weight);
            int tw=TMeasure(wt, gFontSize-3);
            DrawPill((int)mid.x-4,(int)mid.y-2,tw+8,18,(Color){8,12,28,185});
            TDraw(wt,(int)mid.x,(int)mid.y, gFontSize-3, (Color){180,210,255,200});
        }
        //tree edges
        for (int i=0;i<edgeCount;i++)
            if (nodes[edges[i].v].parent==edges[i].u)
                DrawGlowArrow(nodes[edges[i].u].pos,nodes[edges[i].v].pos,C_EDGE_TREE,2.f,1.0f);

        //final path
        if (finished) DrawFinalPath(nodes);

        //fx
        UpdateDrawRipples(dt);
        UpdateDrawParticles(dt);

        //drag preview
        if (dragStart != -1) {
            DrawLineEx(nodes[dragStart].pos, mouse, 1.5f, Fade(C_EDGE,0.5f));
            DrawCircleLinesV(mouse, 6.f, Fade(C_EDGE,0.6f));
        }
 
        //nodes
        for (int i=0;i<nodeCount;i++) {
            Color core;
            if      (blocked[i])       core=C_NODE_BLOCK;
            else if (i==startIdx)      core=C_NODE_START;
            else if (i==endIdx)        core=C_NODE_END;
            else if (nodes[i].visited) core=C_NODE_VIS;
            else                       core=C_NODE_IDLE;
 
            DrawGlowNode(nodes[i].pos, core, nodes[i].flash>0.f?nodes[i].flash:0.f);
 
            if (blocked[i]) {
                float px=nodes[i].pos.x, py=nodes[i].pos.y, r=11.f;
                DrawLineEx((Vector2){px-r,py-r},(Vector2){px+r,py+r},2.5f,Fade(WHITE,0.8f));
                DrawLineEx((Vector2){px+r,py-r},(Vector2){px-r,py+r},2.5f,Fade(WHITE,0.8f));
            }
 
            const char* idStr=TextFormat("%d",i);
            int iw=TMeasure(idStr, gFontSize);
            TDraw(idStr,(int)nodes[i].pos.x-iw/2,(int)nodes[i].pos.y-9, gFontSize, WHITE);
 
            if (!blocked[i]) {
                const char* dStr=nodes[i].dist==INF?"inf":TextFormat("%d",nodes[i].dist);
                int dw=TMeasure(dStr, gFontSize-4);
                DrawPill((int)nodes[i].pos.x-dw/2-4,(int)nodes[i].pos.y+24,dw+8,17,(Color){8,12,28,185});
                TDraw(dStr,(int)nodes[i].pos.x-dw/2,(int)nodes[i].pos.y+25, gFontSize-4, C_LABEL);
            }
        }
 
        //edge hover highlight
        if (hoverEdge || warnTimer > 0.f) {
            for (int i=0;i<edgeCount;i++) {
                Vector2 a=nodes[edges[i].u].pos, b=nodes[edges[i].v].pos;
                if (PointSegDist(mouse,a,b) < EDGE_BLOCK_DIST+8.f) {
                    float a2 = warnTimer>0.f ? fminf(warnTimer,0.5f)/0.5f : 0.5f;
                    DrawLineEx(a, b, 6.f, Fade(C_WARN, a2*0.55f));
                }
            }
        }
 
        //custom cursor
        if (!running) {
            Color curCol = (hoverEdge||warnTimer>0.f) ? C_WARN : Fade(C_PATH_CORE,0.85f);
            DrawCircleLinesV(mouse, 7.f,  curCol);
            DrawCircleV     (mouse, 2.5f, curCol);
            if (hoverEdge || warnTimer > 0.f) {
                //pulsing X over cursor
                float px=mouse.x, py=mouse.y, r=5.f;
                DrawLineEx((Vector2){px-r,py-r},(Vector2){px+r,py+r},2.f,C_WARN);
                DrawLineEx((Vector2){px+r,py-r},(Vector2){px-r,py+r},2.f,C_WARN);

                //tooltip
                const char* tip="Can't place a node here";
                int tw=TMeasure(tip, gFontSize-2);
                DrawPill((int)mouse.x+12,(int)mouse.y-22,tw+14,22,(Color){20,8,8,220});
                DrawRectangleRounded((Rectangle){(float)(int)mouse.x+12,(float)(int)mouse.y-22,(float)(tw+14),22.f},
                                     0.4f,4,Fade(C_WARN,0.25f));
                TDraw(tip,(int)mouse.x+19,(int)mouse.y-19, gFontSize-2, C_WARN);
            }
        }
 
        //HUD
        DrawRectangle(0, WIN_H-HUD_H, WIN_W, HUD_H, C_HUD_BG);
        DrawRectangle(0, WIN_H-HUD_H, WIN_W, 1, Fade(C_PATH_CORE,0.45f));
        int hx=10, hy=WIN_H-HUD_H+11;
        DrawHint(&hx, hy, "LClick", "Node");
        DrawHint(&hx, hy, "RDrag", "Edge");
        DrawHint(&hx, hy, "S",     "+Click Start");
        DrawHint(&hx, hy, "E",     "+Click End");
        DrawHint(&hx, hy, "B",     "+Click Block");
        DrawHint(&hx, hy, "Z",     "+Click Erase");
        DrawHint(&hx, hy, "Space", " Run");
        DrawHint(&hx, hy, "R",     "Reset");
        DrawHint(&hx, hy, "C",     "Clear");
 
        //result banner
        if (finished) {
            const char* msg = pathLen>=2 ? "Shortest path found!" : "No path exists!";
            Color mc = pathLen>=2 ? C_PATH_CORE : C_NODE_END;
            int mw=TMeasure(msg, gFontSize+2);
            DrawPill(WIN_W/2-mw/2-14, 10, mw+28, 30, (Color){8,12,28,220});
            DrawRectangle(WIN_W/2-mw/2-14,10,3,30,mc);
            TDraw(msg, WIN_W/2-mw/2, 15, gFontSize+2, mc);
        }
 
        //font missing notice (shown for first 4 seconds)
        if (!gFontLoaded && globalT < 4.f) {
            const char* fn = "Tip: place Oxanium-Regular.ttf next to the exe for a better font";
            int fw=MeasureText(fn,13);
            DrawPill(WIN_W/2-fw/2-8,WIN_H-HUD_H-28,fw+16,22,(Color){8,12,28,200});
            DrawText(fn, WIN_W/2-fw/2, WIN_H-HUD_H-24, 13, (Color){140,160,200,180});
        }
 
        EndDrawing();
    }
 
    if (gFontLoaded) UnloadFont(gFont);
    CloseWindow();
    return 0;
}