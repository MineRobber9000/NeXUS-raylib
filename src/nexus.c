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
#include "riff.h"
#include <string.h>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

Font font = { 0 };

//----------------------------------------------------------------------------------
// Local Variables Definition (local to this module)
//----------------------------------------------------------------------------------
static const int screenWidth = 320;
static const int screenHeight = 240;
static const int scale = 3;

static int ShouldDrawFPS = 0;

static RenderTexture2D framebuffer;

static FourCC _CODE = {'C','O','D','E'};

//----------------------------------------------------------------------------------
// Local Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);          // Update and draw one frame
static void _DrawFPS(void);                 // Draw FPS
static void LoadCart(RIFF_Chunk *chunk);    // Load cart from RIFF chunks

//----------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //---------------------------------------------------------
    InitWindow(screenWidth*scale, screenHeight*scale, "NeXUS");

    // Load global data (assets that must be available in all screens, i.e. font)
    font = LoadFont("resources/matchup_pro.png");
    SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);
    SetTextLineSpacing(16);

    // Framebuffer
    framebuffer = LoadRenderTexture(screenWidth, screenHeight);
    SetTextureFilter(framebuffer.texture, TEXTURE_FILTER_POINT);

    // Eight bit color
    eightbitcolor_init();

    // Lua
    InitLua();

    // Load nogameloaded.rom and load the code into the VM
    int len;
    unsigned char *data = LoadFileData("resources/nogameloaded.rom",&len);
    TraceLog(LOG_INFO,"CART: Loaded cart file, %d bytes",len);
    size_t offset = 0;
    RIFF_Chunk *cart = riff_parse_chunk_from_data(data,(size_t)len,&offset);
    if (cart==NULL) {
        TraceLog(LOG_ERROR, "CART: Error loading cart: %s", riff_get_error());
        char *error = "function doframe() cls(7) print('error loading cart') end";
        LoadString(error,strlen(error));
        DoCall(0,0);
    } else {
        LoadCart(cart);
    }

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
    UnloadFont(font);
    UnloadRenderTexture(framebuffer);
    CloseLua();

    CloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

// Update and draw game frame
static void UpdateDrawFrame(void)
{
    // Update
    //----------------------------------------------------------------------------------
    if (IsFileDropped()) {
        FilePathList files = LoadDroppedFiles();
        for (int i=0;i<files.count;++i) {
            TraceLog(LOG_INFO, "Dropped file: %s", files.paths[i]);
        }
        UnloadDroppedFiles(files);
    }
    int ctrlDown = IsKeyDown(KEY_LEFT_CONTROL)||IsKeyDown(KEY_RIGHT_CONTROL);
    if (ctrlDown && IsKeyPressed(KEY_F)) {
        if (ShouldDrawFPS) ShouldDrawFPS = 0;
        else ShouldDrawFPS = 1;
    }
    BeginTextureMode(framebuffer);

        // ClearBackground(eightbitcolor_LUT[160]);

        // DrawTextEx(font,"THIS IS TEXT ON THE SCREEN",(Vector2){80,120-8},15,0,eightbitcolor_LUT[255]);

        CallGlobal("doframe");

    EndTextureMode();
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

        ClearBackground((Color){255,0,255,255});

        DrawTexturePro(framebuffer.texture,(Rectangle){0,0,(float)screenWidth,(float)-screenHeight},(Rectangle){0,0,(float)screenWidth*scale,(float)screenHeight*scale},(Vector2){0,0},0,WHITE);

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

    DrawTextEx(font, TextFormat("FPS: %2i", fps), (Vector2){1, 1}, 45, 0, color);
}

//----------------------------------------------------------------------------------
// Load cart from RIFF chunks
//----------------------------------------------------------------------------------
static void LoadCart(RIFF_Chunk *chunk)
{
    if (riff_is_container(chunk->type)) {
        RIFF_ChunkListItem *walker = chunk->contains.chunks;
        while (walker!=NULL) {
            LoadCart(walker->chunk);
            walker=walker->next;
        }
    } else {
        if (riff_fourcc_equals(chunk->type,_CODE)) {
            LoadString(chunk->contains.data,chunk->size);
            DoCall(0,0);
        }
    }
}
