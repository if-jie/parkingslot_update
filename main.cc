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

// --- 生成复杂多边形辅助函数 ---
std::vector<Vec2> CreateComplexPoly(Vec2 center, int sides, double avgRadius) {
    std::vector<Vec2> poly;
    for (int i = 0; i < sides; ++i) {
        double angle = i * (2.0 * PI / sides);
        // 随机改变半径，产生凹凸感
        double r = avgRadius * (0.6 + (double)GetRandomValue(0, 80) / 100.0);
        poly.push_back({ center.x + r * cos(angle), center.y + r * sin(angle) });
    }
    return poly;
}

// --- 核心判定逻辑：带探测范围限制 ---
double calculateSegmentShift(const Segment& seg, const std::vector<std::vector<Vec2>>& allPolys, double margin, double detectionRange) {
    double maxShift = 0.0;
    Vec2 dir = seg.getDir();
    double segLen = seg.length();
    
    for (const auto& poly : allPolys) {
        for (const auto& v : poly) {
            Vec2 vToStart = v - seg.start;
            double projLen = vToStart.dot(dir);

            // 1. 纵向范围判定（是否在线段长度内）
            if (projLen >= 0 && projLen <= segLen) {
                // 2. 横向投影距离（相对于理想位置）
                double dist = vToStart.dot(seg.heading);
                
                // 3. 有效范围过滤：
                // 只有当障碍物顶点在 [理想位置] 到 [理想位置 + detectionRange] 之间时才考虑
                // 我们允许 dist 稍微小于 0 (比如 -10)，以确保平滑处理已经在背后的物体
                if (dist < detectionRange && dist > -margin) {
                    double currentPush = dist + margin;
                    if (currentPush > maxShift) {
                        maxShift = currentPush;
                    }
                }
            }
        }
    }
    return maxShift;
}

int main() {
    // 1. 初始化窗口
    const int screenWidth = 2000;
    const int screenHeight = 700;
    InitWindow(screenWidth, screenHeight, "Segment Pushing - Bounded Range");

    // 2. 初始化线段属性
    Vec2 idealBasePos = {300, 150};
    double segLength = 300.0;
    Vec2 heading = {1, 0};      // 线段受压后向右移动
    double margin = 30.0;       // 必须保持的安全距离
    double detectionRange = 600.0; // 探测距离：只考虑线段右侧100像素内的物体
    double currentShift = 0.0;

    // 3. 创建静态障碍物
    std::vector<std::vector<Vec2>> staticObstacles;
    staticObstacles.push_back(CreateComplexPoly({250, 200}, 10, 40));
    staticObstacles.push_back(CreateComplexPoly({280, 500}, 8, 55));

    // 4. 初始化鼠标障碍物（复杂多边形）
    std::vector<Vec2> mousePolyTemplate = CreateComplexPoly({0, 0}, 15, 60);

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // --- A. 交互控制 ---
        // 调节线段长度: 键盘上下键
        if (IsKeyDown(KEY_UP)) segLength += 2.0;
        if (IsKeyDown(KEY_DOWN)) segLength = std::max(20.0, segLength - 2.0);
        
        // 更新理想线段状态
        Segment currentIdeal = { idealBasePos, {idealBasePos.x, idealBasePos.y + segLength}, heading };

        // 更新鼠标多边形位置
        Vector2 m = GetMousePosition();
        std::vector<Vec2> currentMousePoly;
        for(auto& v : mousePolyTemplate) {
            currentMousePoly.push_back({ v.x + m.x, v.y + m.y });
        }

        // 合并所有障碍物
        std::vector<std::vector<Vec2>> allWorld = staticObstacles;
        allWorld.push_back(currentMousePoly);

        // --- B. 核心计算 ---
        double targetShift = calculateSegmentShift(currentIdeal, allWorld, margin, detectionRange);
        
        // 平滑插值 (Lerp)
        currentShift += (targetShift - currentShift) * 0.15f;

        // --- C. 绘图 ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // 1. 绘制探测有效区 (可视化检测范围)
        DrawRectangleV({(float)currentIdeal.start.x, (float)currentIdeal.start.y}, 
                       {(float)detectionRange, (float)segLength}, ColorAlpha(LIME, 0.08f));
        DrawRectangleLinesEx({(float)currentIdeal.start.x, (float)currentIdeal.start.y, (float)detectionRange, (float)segLength}, 
                             1.0f, ColorAlpha(LIME, 0.3f));

        // 2. 绘制理想位置参考线 (灰)
        DrawLineV({(float)currentIdeal.start.x, (float)currentIdeal.start.y}, 
                  {(float)currentIdeal.end.x, (float)currentIdeal.end.y}, Fade(GRAY, 0.5f));

        // 3. 计算并绘制实际线段 (蓝)
        Vec2 offset = heading * currentShift;
        Vector2 p1 = {(float)(currentIdeal.start.x + offset.x), (float)(currentIdeal.start.y + offset.y)};
        Vector2 p2 = {(float)(currentIdeal.end.x + offset.x), (float)(currentIdeal.end.y + offset.y)};
        
        // 绘制排斥感应区 (Margin)
        DrawRectangleRec({p1.x - (float)margin, p1.y, (float)margin, (float)segLength}, ColorAlpha(SKYBLUE, 0.2f));
        // 绘制主线段
        DrawLineEx(p1, p2, 6.0f, DARKBLUE);
        DrawCircleV(p1, 5, DARKBLUE);
        DrawCircleV(p2, 5, DARKBLUE);

        // 4. 绘制所有多边形
        for (const auto& poly : allWorld) {
            for (size_t i = 0; i < poly.size(); i++) {
                DrawLineEx({(float)poly[i].x, (float)poly[i].y}, 
                           {(float)poly[(i+1)%poly.size()].x, (float)poly[(i+1)%poly.size()].y}, 
                           2.0f, MAROON);
            }
        }

        // 5. 状态文字
        DrawText("Controls:", 10, 10, 20, DARKGRAY);
        DrawText("- UP/DOWN: Resize Line", 10, 35, 18, GRAY);
        DrawText("- Mouse: Move Obstacle", 10, 55, 18, GRAY);
        DrawText(TextFormat("Detection Range: %.0f px", detectionRange), 10, 85, 20, DARKGREEN);
        DrawText(TextFormat("Current Shift: %.1f", currentShift), 10, 110, 20, DARKBLUE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}