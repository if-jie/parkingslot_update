#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include "raylib.h"

// --- 基础数学结构 ---
struct Vec2 {
    double x, y;
    Vec2 operator+(const Vec2& b) const { return {x + b.x, y + b.y}; }
    Vec2 operator-(const Vec2& b) const { return {x - b.x, y - b.y}; }
    Vec2 operator*(double s) const { return {x * s, y * s}; }
    double dot(const Vec2& b) const { return x * b.x + y * b.y; }
};

struct Segment {
    Vec2 start;
    Vec2 end;
    Vec2 heading; // 推离方向 (Normal)

    Vec2 getDir() const {
        Vec2 d = end - start;
        double len = std::sqrt(d.x * d.x + d.y * d.y);
        return (len > 1e-6) ? Vec2{d.x / len, d.y / len} : Vec2{0, 0};
    }
    double length() const {
        Vec2 d = end - start;
        return std::sqrt(d.x * d.x + d.y * d.y);
    }
};

// --- 生成复杂多边形 ---
// center: 中心点, sides: 边数, avgRadius: 平均半径
std::vector<Vec2> CreateComplexPoly(Vec2 center, int sides, double avgRadius) {
    std::vector<Vec2> poly;
    for (int i = 0; i < sides; ++i) {
        double angle = i * (2.0 * PI / sides);
        // 随机改变半径，产生凹凸感
        double r = avgRadius * (0.5 + (double)GetRandomValue(0, 100) / 100.0);
        poly.push_back({ center.x + r * cos(angle), center.y + r * sin(angle) });
    }
    return poly;
}

// --- 核心判定逻辑 ---
double calculateSegmentShift(const Segment& seg, const std::vector<std::vector<Vec2>>& allPolys, double margin) {
    double maxShift = 0.0;
    Vec2 dir = seg.getDir();
    double segLen = seg.length();
    
    for (const auto& poly : allPolys) {
        for (const auto& v : poly) {
            Vec2 vToStart = v - seg.start;
            double projLen = vToStart.dot(dir);

            // 检查顶点是否在线段的“垂直影响范围”内
            if (projLen >= 0 && projLen <= segLen) {
                double dist = vToStart.dot(seg.heading);
                // 核心公式：目标偏移 = 顶点位置 + 安全间距
                double currentPush = dist + margin;
                if (currentPush > maxShift) maxShift = currentPush;
            }
        }
    }
    return maxShift;
}

int main() {
    const int screenWidth = 1000;
    const int screenHeight = 700;
    InitWindow(screenWidth, screenHeight, "Complex Obstacles & Dynamic Segment");

    // 1. 初始化线段
    Vec2 idealBasePos = {250, 150};
    double segLength = 300.0;
    Vec2 heading = {1, 0}; // 向右推
    double margin = 25.0;
    double currentShift = 0.0;

    // 2. 生成一些复杂的障碍物
    std::vector<std::vector<Vec2>> staticObstacles;
    staticObstacles.push_back(CreateComplexPoly({220, 200}, 12, 40));
    staticObstacles.push_back(CreateComplexPoly({240, 450}, 8, 50));

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // --- 交互控制 ---
        // 调节线段长度: 键盘上下键
        if (IsKeyDown(KEY_UP)) segLength += 2.0;
        if (IsKeyDown(KEY_DOWN)) segLength = std::max(10.0, segLength - 2.0);
        
        // 实时更新线段端点
        Segment currentIdeal = { idealBasePos, {idealBasePos.x, idealBasePos.y + segLength}, heading };

        // 鼠标控制的动态障碍物
        Vector2 m = GetMousePosition();
        static std::vector<Vec2> mousePoly = CreateComplexPoly({(double)m.x, (double)m.y}, 15, 60);
        // 每一帧更新鼠标多边形位置（简单偏移其所有顶点）
        static Vector2 lastMouse = m;
        for(auto& v : mousePoly) {
            v.x += (m.x - lastMouse.x);
            v.y += (m.y - lastMouse.y);
        }
        lastMouse = m;

        // 合并所有障碍物
        std::vector<std::vector<Vec2>> allWorld = staticObstacles;
        allWorld.push_back(mousePoly);

        // --- 计算 ---
        double targetShift = calculateSegmentShift(currentIdeal, allWorld, margin);
        currentShift += (targetShift - currentShift) * 0.2f; // 平滑平移

        // --- 绘制 ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // 绘制 UI 说明
        DrawText("Use UP/DOWN arrows to change Segment Length", 10, 10, 20, DARKGRAY);
        DrawText(TextFormat("Current Length: %.1f", segLength), 10, 35, 20, BLUE);

        // 计算当前物理位置
        Vec2 offset = heading * currentShift;
        Vector2 p1 = {(float)(currentIdeal.start.x + offset.x), (float)(currentIdeal.start.y + offset.y)};
        Vector2 p2 = {(float)(currentIdeal.end.x + offset.x), (float)(currentIdeal.end.y + offset.y)};

        // 1. 绘制理想路径参考线
        DrawLineV({(float)currentIdeal.start.x, (float)currentIdeal.start.y}, 
                  {(float)currentIdeal.end.x, (float)currentIdeal.end.y}, Fade(GRAY, 0.3f));

        // 2. 绘制排斥感应区 (Margin Box)
        DrawRectangleRec({p1.x - (float)margin, p1.y, (float)margin, (float)segLength}, ColorAlpha(SKYBLUE, 0.15f));

        // 3. 绘制实际线段
        DrawLineEx(p1, p2, 5.0f, DARKBLUE);
        DrawCircleV(p1, 4, DARKBLUE);
        DrawCircleV(p2, 4, DARKBLUE);

        // 4. 绘制所有多边形
        for (const auto& poly : allWorld) {
            for (size_t i = 0; i < poly.size(); i++) {
                Vector2 v1 = {(float)poly[i].x, (float)poly[i].y};
                Vector2 v2 = {(float)poly[(i + 1) % poly.size()].x, (float)poly[(i + 1) % poly.size()].y};
                DrawLineEx(v1, v2, 2.0f, MAROON);
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}