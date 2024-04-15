// Copyright (c) 2024 Wildan R Wijanarko (@wildan9)

// This software is provided "as-is", without any express or implied warranty. In no event 
// will the authors be held liable for any damages arising from the use of this software.

// Permission is granted to anyone to use this software for any purpose, including commercial 
// applications, and to alter it and redistribute it freely, subject to the following restrictions:

//   1. The origin of this software must not be misrepresented; you must not claim that you 
//   wrote the original software. If you use this software in a product, an acknowledgment 
//   in the product documentation would be appreciated but is not required.

//   2. Altered source versions must be plainly marked as such, and must not be misrepresented
//   as being the original software.

//   3. This notice may not be removed or altered from any source distribution.

#include "raylib.h"
#include "matrix.hpp"
#include "rlgl.h"
#include "raymath.h"
#include <string>

using namespace std;

#define RAYGUI_IMPLEMENTATION
#include "extras/raygui.h"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.hpp"

#define GLSL_VERSION 330

static void DrawModelPro(
    const Model& model, 
    Vector3 position, 
    Quaternion rotation, 
    float scale, 
    Color color, 
    bool drawFacesAndTexture, 
    string mode)
{
    scale = scale * 0.1f; // Personal custom

    Matrix rotationMatrix = {};
    if (mode == "ZYX Euler")
    {
        rotationMatrix = MatrixRotateZYX(QuaternionToEuler(QuaternionNormalize(rotation)));
    }
    else if (mode == "Quaternion")
    {
        rotationMatrix = QuaternionToMatrix(QuaternionNormalize(rotation));
    }
    else if (mode == "Axis Angle")
    {
        Vector3 vec = {};
        float angle = {};
        QuaternionToAxisAngle(rotation, &vec, &angle);
        rotationMatrix = QuaternionToMatrix(QuaternionFromAxisAngle(vec, angle));
    }

    Matrix transformMatrix = MatrixTranslate(position.x, position.y, position.z) * rotationMatrix * MatrixScale(scale, scale, scale);

    // Push the current matrix and multiply it with the transformation matrix
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(transformMatrix));

    if (drawFacesAndTexture) DrawModel(model, Vector3Zero(), scale, WHITE);
    else DrawModelWires(model, Vector3Zero(), scale, WHITE);

    // Pop the matrix back to the previous state
    rlPopMatrix();
}

static inline void UpdateLightPos(Light& light, float speed, float radius)
{
    // Update time within the function
    static float time = 0.0f;
    time += GetFrameTime();

    // Calculate the new position based on time
    float dx = radius * cos(speed * time);
    float dz = radius * sin(speed * time);

    light.position = (Vector3){ dx, light.position.y, dz };
}

int main(void)
{
    const int screenWidth  = 1080;
    const int screenHeight = 720;

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "");

    Camera3D camera     = { 0 };
    camera.position     = (Vector3){ 10.0f, 10.0f, 10.0f };     // Camera position
    camera.target       = (Vector3){ 0.0f, 0.0f, 0.0f };        // Camera looking at point
    camera.up           = (Vector3){ 0.0f, 1.0f, 0.0f };        // Camera up vector (rotation towards target)
    camera.fovy         = 45.0f;                                // Camera field-of-view Y
    camera.projection   = CAMERA_PERSPECTIVE;                   // Camera projection type

    SetTargetFPS(120);

    Vector3 boxPos          = { 0.0f, 0.0f, 0.0f };
    Quaternion boxRotation  = { 0.0f, 0.0f, 0.0f, 0.0f };
    float   boxScale        = 1.0f;

    Model boxModel = LoadModel("resources/models/box/wooden_box.obj");
    
    // Load basic lighting shader
    Shader shader = LoadShader(TextFormat("resources/shaders/lighting.vs", GLSL_VERSION), TextFormat("resources/shaders/lighting.fs", GLSL_VERSION));

    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    
    // Ambient light level (some basic lighting)
    int ambientLoc = GetShaderLocation(shader, "ambient");
    float ambientValues[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    SetShaderValue(shader, ambientLoc, ambientValues, SHADER_UNIFORM_VEC4);

    // Assign out lighting shader to model
    boxModel.materials->shader = shader;

    // Create light
    Light light = {};
    light = CreateLight(LIGHT_POINT, (Vector3){ 4.0f, 4.0f, -2.0f }, Vector3Zero(), RAYWHITE, shader);

    float lightSpeed = 2.0f;

    bool drawFacesAndTexture = false;

    int selectedMode = 0; // Default selected mode index

    // Items for the dropdown
    string strRotationMode[3] = { "ZYX Euler", "Quaternion", "Axis Angle" };

    bool dropdownActive = false;

    while (!WindowShouldClose())
    {
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            UpdateCamera(&camera, CAMERA_PERSPECTIVE);
        }

        if (IsKeyPressed('Z')) camera.target = (Vector3){ 0.0f, 0.5f, 0.0f };

        UpdateLightPos(light, lightSpeed, 6.0f);
        UpdateLightValues(shader, light);
        
        BeginDrawing();

        ClearBackground(BLACK);

        BeginMode3D(camera);
            // Draw spheres to show where the lights are
            DrawSphereEx(light.position, 0.2f, 8, 8, light.color);
  
            DrawModelPro(boxModel, boxPos, boxRotation, boxScale, BLUE, drawFacesAndTexture, strRotationMode[selectedMode]);

            DrawGrid(10, 1.0f);
        EndMode3D();

        DrawFPS(0, 0);

        float uiSettingsLeft = screenWidth - 150;
        GuiGroupBox((Rectangle){ uiSettingsLeft - 10, 20, 150, 310 }, "Settings");
        drawFacesAndTexture = GuiCheckBox((Rectangle){ uiSettingsLeft, 40, 15, 15 }, "Draw Faces & Texture", drawFacesAndTexture);
        
        DrawText("Rotation Mode", 20, 40, 19, RAYWHITE);
        if (GuiDropdownBox(((Rectangle){ 20, 60, 140, 30 }), "ZYX Euler;Quaternion;Axis Angle", &selectedMode, dropdownActive))
        {
            dropdownActive = !dropdownActive;
        }

        if (strRotationMode[selectedMode] == "ZYX Euler")
        {
            boxPos.x = GuiSliderBar(
                (Rectangle){ uiSettingsLeft + 40, 80 + 20 * 0, 50, 15 }, 
                "PosX", 
                TextFormat("%3.2f", boxPos.x),
                boxPos.x,
                -10.0f, 10.0f);
        
            boxPos.y = GuiSliderBar(
                (Rectangle){ uiSettingsLeft + 40, 80 + 20 * 1, 50, 15 }, 
                "PosY", 
                TextFormat("%3.2f", boxPos.y),
                boxPos.y,
                -10.0f, 10.0f);
            
            boxPos.z = GuiSliderBar(
                (Rectangle){ uiSettingsLeft + 40, 80 + 20 * 2, 50, 15 }, 
                "PosZ", 
                TextFormat("%3.2f", boxPos.z),
                boxPos.z,
                -10.0f, 10.0f);
            
            boxRotation.x = GuiSliderBar(
                (Rectangle){ uiSettingsLeft + 40, 80 + 20 * 3, 50, 15 }, 
                "RotX", 
                TextFormat("%3.2f", boxRotation.x),
                boxRotation.x,
                -180.0f, 180.0f);
            
            boxRotation.y = GuiSliderBar(
                (Rectangle){ uiSettingsLeft + 40, 80 + 20 * 4, 50, 15 }, 
                "RotY", 
                TextFormat("%3.2f", boxRotation.y),
                boxRotation.y,
                -180.0f, 180.0f);

            boxRotation.z = GuiSliderBar(
                (Rectangle){ uiSettingsLeft + 40, 80 + 20 * 5, 50, 15 }, 
                "RotZ", 
                TextFormat("%3.2f", boxRotation.z),
                boxRotation.z,
                -180.0f, 180.0f);

            boxScale = GuiSliderBar(
                (Rectangle){ uiSettingsLeft + 40, 80 + 20 * 6, 50, 15 }, 
                "Scale", 
                TextFormat("%3.2f", boxScale),
                boxScale,
                1.0f, 8.0f);

            lightSpeed = GuiSliderBar(
                (Rectangle){ uiSettingsLeft + 40, 80 + 20 * 7, 50, 15 }, 
                "LSpeed", 
                TextFormat("%3.2f", lightSpeed),
                lightSpeed,
                0.2f, 2.0f);

            light.position.y = GuiSliderBar(
                (Rectangle){ uiSettingsLeft + 40, 80 + 20 * 8, 50, 15 }, 
                "LHeight", 
                TextFormat("%3.2f", light.position.y),
                light.position.y,
                -2.0f, 5.0f);
        }
        else if (strRotationMode[selectedMode] == "Quaternion" || strRotationMode[selectedMode] == "Axis Angle")
        {
            boxPos.x = GuiSliderBar(
                (Rectangle){ uiSettingsLeft + 40, 80 + 20 * 0, 50, 15 }, 
                "PosX", 
                TextFormat("%3.2f", boxPos.x),
                boxPos.x,
                -10.0f, 10.0f);
        
            boxPos.y = GuiSliderBar(
                (Rectangle){ uiSettingsLeft + 40, 80 + 20 * 1, 50, 15 }, 
                "PosY", 
                TextFormat("%3.2f", boxPos.y),
                boxPos.y,
                -10.0f, 10.0f);
            
            boxPos.z = GuiSliderBar(
                (Rectangle){ uiSettingsLeft + 40, 80 + 20 * 2, 50, 15 }, 
                "PosZ", 
                TextFormat("%3.2f", boxPos.z),
                boxPos.z,
                -10.0f, 10.0f);

            boxRotation.w = GuiSliderBar(
                (Rectangle){ uiSettingsLeft + 40, 80 + 20 * 3, 50, 15 }, 
                "RotW", 
                TextFormat("%3.2f", boxRotation.w),
                boxRotation.w,
                -180.0f, 180.0f);
            
            boxRotation.x = GuiSliderBar(
                (Rectangle){ uiSettingsLeft + 40, 80 + 20 * 4, 50, 15 }, 
                "RotX", 
                TextFormat("%3.2f", boxRotation.x),
                boxRotation.x,
                -180.0f, 180.0f);
            
            boxRotation.y = GuiSliderBar(
                (Rectangle){ uiSettingsLeft + 40, 80 + 20 * 5, 50, 15 }, 
                "RotY", 
                TextFormat("%3.2f", boxRotation.y),
                boxRotation.y,
                -180.0f, 180.0f);

            boxRotation.z = GuiSliderBar(
                (Rectangle){ uiSettingsLeft + 40, 80 + 20 * 6, 50, 15 }, 
                "RotZ", 
                TextFormat("%3.2f", boxRotation.z),
                boxRotation.z,
                -180.0f, 180.0f);

            boxScale = GuiSliderBar(
                (Rectangle){ uiSettingsLeft + 40, 80 + 20 * 7, 50, 15 }, 
                "Scale", 
                TextFormat("%3.2f", boxScale),
                boxScale,
                1.0f, 8.0f);

            lightSpeed = GuiSliderBar(
                (Rectangle){ uiSettingsLeft + 40, 80 + 20 * 8, 50, 15 }, 
                "LSpeed", 
                TextFormat("%3.2f", lightSpeed),
                lightSpeed,
                0.2f, 2.0f);

            light.position.y = GuiSliderBar(
                (Rectangle){ uiSettingsLeft + 40, 80 + 20 * 9, 50, 15 }, 
                "LHeight", 
                TextFormat("%3.2f", light.position.y),
                light.position.y,
                -2.0f, 5.0f);
        }

        if (GuiButton((Rectangle){ uiSettingsLeft, 300, 50, 20}, "RESET"))
        {
            boxPos            = { 0.0f, 0.0f, 0.0f };
            boxRotation       = { 0.0f, 0.0f, 0.0f, 0.0f };
            boxScale          = 1.0f;
            lightSpeed        = 2.0f;
            light.position.y  = 4.0f;
        }

        EndDrawing();
    }

    UnloadModel(boxModel); // Unload box model
    UnloadShader(shader);  // Unload shader

    CloseWindow();

    return 0;
}
