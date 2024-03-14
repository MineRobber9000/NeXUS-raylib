/*******************************************************************************************
*
*   raylib game template
*
*   <Game title>
*   <Game description>
*
*   This game has been created using raylib (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2021 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"
#include "eightbitcolor.h"
#include "lua_api.h"
#include "nexus.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

NeXUS_VM vm = { 0 };

//----------------------------------------------------------------------------------
// Local Variables Definition (local to this module)
//----------------------------------------------------------------------------------
static const int screenWidth = 320;
static const int screenHeight = 240;
static const int scale = 3;

static int ShouldDrawFPS = 0;

//----------------------------------------------------------------------------------
// Local Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);          // Update and draw one frame
static void _DrawFPS(void);                 // Draw FPS

//----------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //---------------------------------------------------------
    InitWindow(screenWidth*scale, screenHeight*scale, "NeXUS");

    // Load global data (assets that must be available in all screens, i.e. font)
    vm.font = LoadFont("resources/matchup_pro.png");
    SetTextureFilter(vm.font.texture, TEXTURE_FILTER_POINT);
    SetTextLineSpacing(16);

    // Framebuffer
    vm.framebuffer = LoadRenderTexture(screenWidth, screenHeight);
    SetTextureFilter(vm.framebuffer.texture, TEXTURE_FILTER_POINT);

    // Eight bit color
    eightbitcolor_init();

    // Lua
    InitLua();

    // Load nogameloaded.rom and load the code into the VM
    vm.cart = LoadCart("resources/nogameloaded.rom");
    LoadString(vm.cart->code,vm.cart->code_size);
    DoCall(0,0);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);       // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        UpdateDrawFrame();
    }
#endif

    // Unload global data loaded
    UnloadFont(vm.font);
    UnloadRenderTexture(vm.framebuffer);
    FreeCart(vm.cart);
    CloseLua();
    if (vm.screen.data!=NULL) UnloadImage(vm.screen);

    CloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

// Update and draw game frame
static void UpdateDrawFrame(void)
{
    // Update
    //----------------------------------------------------------------------------------
    int loaderWantsAReset = 0;
    if (IsFileDropped()) {
        FilePathList files = LoadDroppedFiles();
        if (files.count==1) {
            TraceLog(LOG_INFO, "LOADER: Loading %s, deinitialize previous cart",files.paths[0]);
            FreeCart(vm.cart);
            TraceLog(LOG_INFO, "LOADER: Initialize new cart");
            vm.cart = LoadCart(files.paths[0]);
            TraceLog(LOG_INFO, "LOADER: Set reset flag so the resetter can do the loading thing");
            loaderWantsAReset = 1; // set reset flag
            TraceLog(LOG_INFO,"LOADER: Exit loader (all crashes past this point are NOT our fault)");
        } else {
            TraceLog(LOG_INFO, "LOADER: Cowardly refusing to figure out which of %d files to load", files.count);
        }
        UnloadDroppedFiles(files);
    }
    int ctrlDown = IsKeyDown(KEY_LEFT_CONTROL)||IsKeyDown(KEY_RIGHT_CONTROL);
    if (ctrlDown && IsKeyPressed(KEY_F)) { // toggle FPS counter (^F)
        if (ShouldDrawFPS) ShouldDrawFPS = 0;
        else ShouldDrawFPS = 1;
    }
    if ((ctrlDown && IsKeyPressed(KEY_R)) // reset ROM (^R)
        || loaderWantsAReset) {
        BeginTextureMode(vm.framebuffer);
            ClearBackground(eightbitcolor_LUT[0]);
            if (HAS_SCREEN()) {
                ImageClearBackground(&vm.screen, eightbitcolor_LUT[0]);
            }
        EndTextureMode();
        CloseLua();
        InitLua();
        LoadString(vm.cart->code,vm.cart->code_size);
        if (DoCall(0,0)!=LUA_OK) {
            const char *msg = lua_tostring(L,-1);
            TraceLog(LOG_INFO, "RESET: Lua error: %s",msg); // TODO: this should take you into the error screen
            lua_pop(L,1);
        }
    }
    BeginTextureMode(vm.framebuffer);

        // ClearBackground(eightbitcolor_LUT[160]);

        // DrawTextEx(font,"THIS IS TEXT ON THE SCREEN",(Vector2){80,120-8},15,0,eightbitcolor_LUT[255]);

        CallGlobal("doframe");

    EndTextureMode();
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

        ClearBackground((Color){255,0,255,255});

        DrawTexturePro(vm.framebuffer.texture,(Rectangle){0,0,(float)screenWidth,(float)-screenHeight},(Rectangle){0,0,(float)screenWidth*scale,(float)screenHeight*scale},(Vector2){0,0},0,WHITE);

        if (ShouldDrawFPS) _DrawFPS();

    EndDrawing();
    //----------------------------------------------------------------------------------
}

//----------------------------------------------------------------------------------
// Draw FPS using the Correct(tm) font
//----------------------------------------------------------------------------------
static void _DrawFPS(void)
{
    Color color = WHITE;                         // Good FPS
    int fps = GetFPS();

    if ((fps < 30) && (fps >= 15)) color = ORANGE;  // Warning FPS
    else if (fps < 15) color = RED;             // Low FPS

    DrawTextEx(vm.font, TextFormat("FPS: %2i", fps), (Vector2){1, 1}, 45, 0, color);
}