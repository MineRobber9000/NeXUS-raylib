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

struct NeXUS_API error_screen_funcs[];

//----------------------------------------------------------------------------------
// Local Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);          // Update and draw one frame
static void _DrawFPS(void);                 // Draw FPS
static void DrawTextBoxed(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint);

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

    // Keyboard controls
    vm.controls.keyboard[0] = KEY_UP;
    vm.controls.keyboard[1] = KEY_DOWN;
    vm.controls.keyboard[2] = KEY_LEFT;
    vm.controls.keyboard[3] = KEY_RIGHT;
    vm.controls.keyboard[4] = KEY_Z;
    vm.controls.keyboard[5] = KEY_X;
    vm.controls.keyboard[6] = KEY_LEFT_SHIFT;
    vm.controls.keyboard[7] = KEY_ENTER;

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
    while (!(WindowShouldClose() || vm.should_close))    // Detect window close button or ESC key
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
        if (vm.cart->sprites) {
            FreeSprites(vm.cart->sprites); // free sprites on reset
            vm.cart->sprites = NULL;
        }
        InitLua();
        LoadString(vm.cart->code,vm.cart->code_size);
        if (DoCall(0,0)!=LUA_OK) {
            char *msg = CopyString(lua_tostring(L,-1));
            TraceLog(LOG_INFO, "RESET: Lua error: %s",msg); // TODO: this should take you into the error screen
            lua_pop(L,1);
            ErrorScreen(msg);
            MemFree(msg);
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

//----------------------------------------------------------------------------------
// Error screen
//----------------------------------------------------------------------------------

const char * error_screen =
    "local sanitizedmsg = {}\n"
    "for char in msg:gmatch(utf8.charpattern) do table.insert(sanitizedmsg,char) end\n"
    "sanitizedmsg = table.concat(sanitizedmsg)\n"
    "sanitizedmsg = sanitizedmsg:gsub('\\r\\n','\\n')\n"
    "local err = {}\n"
    "table.insert(err,\"*** SOFTWARE FAILURE ***\")\n"
    "table.insert(err,\"GURU MEDITATION: \") table.insert(err,\"\")\n"
    "table.insert(err,sanitizedmsg)\n"
    "if #sanitizedmsg ~= #msg then table.insert(err, \"* Invalid UTF-8 string in error message. *\") end\n"
    "local p = table.concat(err,'\\n')\n"
    "p = p:gsub(\"\\t\", (\" \"):rep(8))\n" // tabs 8 spaces, fight me
    "p = p:gsub(\"stack traceback:\",\"\\n\\nTraceback:\")\n"
    "p = p .. \"\\nPress Ctrl+C to copy error, Ctrl+R to restart NeXUS\"\n"
    "function doframe()\n"
    "   if ctrlCPressed() then copyMsg(sanitizedmsg) p = p .. \"\\nCopied!\" end\n"
    "   cls(7)\n"
    "   printscreenbox(p)\n"
    "end";

static int in_error_screen = 0;

void ErrorScreen(char *msg)
{
    if (in_error_screen) return;
    in_error_screen = 1;
    EndScissorMode();
    // Essentially just a custom `doframe()` with some custom API
    // When you reset the ROM it clears out state anyways
    SetGlobalString("msg",msg);
    for (struct NeXUS_API *func = error_screen_funcs; func->func; ++func) {
        RegisterFunction(func);
    }
    if (LoadString(error_screen,strlen(error_screen))>0) {
        TraceLog(LOG_ERROR,"ERROR: Meta error: %s",lua_tostring(L,-1));
        lua_pop(L,1);
        vm.should_close = 1;
        in_error_screen = 0;
        return;
    }
    if (DoCall(0,0)>0) {
        TraceLog(LOG_ERROR,"ERROR: Meta error: %s",lua_tostring(L,-1));
        lua_pop(L,1);
        vm.should_close = 1;
        in_error_screen = 0;
        return;
    }
    in_error_screen = 0;
    return;
}

int api_print_screenbox(lua_State *L)
{
    const char *str = luaL_checklstring(L,1,0);
    if (!str) return 0;
    DrawTextBoxed(vm.font, str, (Rectangle){0, 0, 320, 240}, 15, 0, true, eightbitcolor_LUT[255]);
    vm.screen_dirty = 1; // is it worth it to duplicate on vm.screen?
    return 0;
}

int api_ctrlCPressed(lua_State *L)
{
    int ctrlDown = IsKeyDown(KEY_LEFT_CONTROL)||IsKeyDown(KEY_RIGHT_CONTROL);
    lua_pushboolean(L,ctrlDown && IsKeyPressed(KEY_C));
    return 1;
}

int api_copyMsg(lua_State *L)
{
    SetClipboardText(lua_tostring(L, 1));
    return 0;
}

struct NeXUS_API error_screen_funcs[] = {
    {api_print_screenbox, "printscreenbox"},
    {api_ctrlCPressed, "ctrlCPressed"},
    {api_copyMsg, "copyMsg"},
    {0, 0}
};

// Draw text using font inside rectangle limits
// stole from the text_rectangle_bounds example
static void DrawTextBoxed(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint)
{
    int length = TextLength(text);  // Total length in bytes of the text, scanned by codepoints in loop

    float textOffsetY = 0;          // Offset between lines (on line break '\n')
    float textOffsetX = 0.0f;       // Offset X to next character to draw

    float scaleFactor = fontSize/(float)font.baseSize;     // Character rectangle scaling factor

    // Word/character wrapping mechanism variables
    enum { MEASURE_STATE = 0, DRAW_STATE = 1 };
    int state = wordWrap? MEASURE_STATE : DRAW_STATE;

    int startLine = -1;         // Index where to begin drawing (where a line begins)
    int endLine = -1;           // Index where to stop drawing (where a line ends)
    int lastk = -1;             // Holds last value of the character position

    for (int i = 0, k = 0; i < length; i++, k++)
    {
        // Get next codepoint from byte string and glyph index in font
        int codepointByteCount = 0;
        int codepoint = GetCodepoint(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
        // but we need to draw all of the bad bytes using the '?' symbol moving one byte
        if (codepoint == 0x3f) codepointByteCount = 1;
        i += (codepointByteCount - 1);

        float glyphWidth = 0;
        if (codepoint != '\n')
        {
            glyphWidth = (font.glyphs[index].advanceX == 0) ? font.recs[index].width*scaleFactor : font.glyphs[index].advanceX*scaleFactor;

            if (i + 1 < length) glyphWidth = glyphWidth + spacing;
        }

        // NOTE: When wordWrap is ON we first measure how much of the text we can draw before going outside of the rec container
        // We store this info in startLine and endLine, then we change states, draw the text between those two variables
        // and change states again and again recursively until the end of the text (or until we get outside of the container).
        // When wordWrap is OFF we don't need the measure state so we go to the drawing state immediately
        // and begin drawing on the next line before we can get outside the container.
        if (state == MEASURE_STATE)
        {
            // TODO: There are multiple types of spaces in UNICODE, maybe it's a good idea to add support for more
            // Ref: http://jkorpela.fi/chars/spaces.html
            if ((codepoint == ' ') || (codepoint == '\t') || (codepoint == '\n')) endLine = i;

            if ((textOffsetX + glyphWidth) > rec.width)
            {
                endLine = (endLine < 1)? i : endLine;
                if (i == endLine) endLine -= codepointByteCount;
                if ((startLine + codepointByteCount) == endLine) endLine = (i - codepointByteCount);

                state = !state;
            }
            else if ((i + 1) == length)
            {
                endLine = i;
                state = !state;
            }
            else if (codepoint == '\n') state = !state;

            if (state == DRAW_STATE)
            {
                textOffsetX = 0;
                i = startLine;
                glyphWidth = 0;

                // Save character position when we switch states
                int tmp = lastk;
                lastk = k - 1;
                k = tmp;
            }
        }
        else
        {
            if (codepoint == '\n')
            {
                if (!wordWrap)
                {
                    textOffsetY += 16; //(font.baseSize + font.baseSize/2)*scaleFactor;
                    textOffsetX = 0;
                }
            }
            else
            {
                if (!wordWrap && ((textOffsetX + glyphWidth) > rec.width))
                {
                    textOffsetY += 16; //(font.baseSize + font.baseSize/2)*scaleFactor;
                    textOffsetX = 0;
                }

                // When text overflows rectangle height limit, just stop drawing
                if ((textOffsetY + font.baseSize*scaleFactor) > rec.height) break;

                // Draw current character glyph
                if ((codepoint != ' ') && (codepoint != '\t'))
                {
                    DrawTextCodepoint(font, codepoint, (Vector2){ rec.x + textOffsetX, rec.y + textOffsetY }, fontSize, tint);
                }
            }

            if (wordWrap && (i == endLine))
            {
                textOffsetY += 16; //(font.baseSize + font.baseSize/2)*scaleFactor;
                textOffsetX = 0;
                startLine = endLine;
                endLine = -1;
                glyphWidth = 0;
                k = lastk;

                state = !state;
            }
        }

        // if ((textOffsetX != 0) || (codepoint != ' ')) textOffsetX += glyphWidth;  // avoid leading spaces
        textOffsetX += glyphWidth;
    }
}