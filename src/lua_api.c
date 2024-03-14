#include "raylib.h"
#include "eightbitcolor.h"
#include "rlgl.h"
#include "lua_api.h"

lua_State *L;

static const luaL_Reg loadedlibs[] = {
  {LUA_GNAME, luaopen_base},
  {LUA_LOADLIBNAME, luaopen_package},
  {LUA_COLIBNAME, luaopen_coroutine},
  {LUA_TABLIBNAME, luaopen_table},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_MATHLIBNAME, luaopen_math},
  {LUA_UTF8LIBNAME, luaopen_utf8},
  {LUA_DBLIBNAME, luaopen_debug},
  {NULL, NULL}
};

void nullify(const char * global)
{
    lua_pushnil(L);
    lua_setglobal(L, global);
}

// GRAPHICS

int api_circ(lua_State *L)
{
    double x = luaL_checknumber(L, 1);
    double y = luaL_checknumber(L, 2);
    double rad = luaL_checknumber(L, 3);
    uint8_t color = luaL_checkinteger(L, 4)&0xFF;
    DrawCircleV((Vector2){x, y}, rad, eightbitcolor_LUT[color]);
    vm.screen_dirty = 1; // is it worth it to duplicate on vm.screen?
    return 0;
}

int api_circb(lua_State *L)
{
    double x = luaL_checknumber(L, 1);
    double y = luaL_checknumber(L, 2);
    double rad = luaL_checknumber(L, 3);
    uint8_t color = luaL_checkinteger(L, 4)&0xFF;
    DrawCircleLinesV((Vector2){x, y}, rad, eightbitcolor_LUT[color]);
    vm.screen_dirty = 1; // is it worth it to duplicate on vm.screen?
    return 0;
}

int api_clip(lua_State *L)
{
    if (lua_isnoneornil(L,1)) {
        EndScissorMode();
    } else {
        int x = luaL_checkinteger(L,1);
        int y = luaL_checkinteger(L,2);
        int w = luaL_checkinteger(L,3);
        int h = luaL_checkinteger(L,4);
        BeginScissorMode(x,y,w,h);
    }
}

int api_cls(lua_State *L)
{
    uint8_t color = luaL_optinteger(L,1,0)&0xFF;
    ClearBackground(eightbitcolor_LUT[color]);
    if (HAS_SCREEN()) ImageClearBackground(&vm.screen, eightbitcolor_LUT[color]);
    return 0;
}

int api_define_spr(lua_State *L)
{
    uint32_t grph_id = luaL_checkinteger(L,1);
    uint32_t x = luaL_checkinteger(L,2);
    uint32_t y = luaL_checkinteger(L,3);
    uint32_t w = luaL_checkinteger(L,4);
    uint32_t h = luaL_checkinteger(L,5);
    Cart_GraphicsPage *page = vm.cart->graphics;
    while (page!=NULL && page->id!=grph_id) page = page->next;
    if (page==NULL) luaL_error(L,"no such graphics page %d",grph_id);
    if (x<0 || x>page->width) luaL_error(L, "out of bounds X position");
    if (y<0 || y>page->height) luaL_error(L, "out of bounds Y position");
    if (w<1) luaL_error(L, "must have at least 1 width");
    if (h<1) luaL_error(L, "must have at least 1 height");
    if ((x+w)>page->width) luaL_error(L, "cannot build sprite from X position %d with width %d",x,w);
    if ((y+h)>page->height) luaL_error(L, "cannot build sprite from Y position %d with height %d",y,h);
    uint32_t spr_id = 0;
    if (vm.cart->sprites!=NULL) spr_id = vm.cart->sprites->id + 1;
    Cart_Sprites *spr = MemAlloc(sizeof(Cart_Sprites));
    spr->id = spr_id;
    spr->img = ImageFromImage(page->img,(Rectangle){(float)x, (float)y, (float)w, (float)h});
    if (!lua_isnoneornil(L, 6)) {
        uint8_t colorkey = luaL_checkinteger(L, 6)&0xFF;
        for (int y = 0; y<spr->img.height; ++y) {
            for (int x = 0; x<spr->img.width; ++x) {
                uint8_t col = eightbitcolor_nearest(GetImageColor(spr->img, x, y));
                if (col==colorkey) ImageDrawPixel(&spr->img, x, y, (Color){0});
            }
        }
    }
    spr->texture = LoadTextureFromImage(spr->img);
    spr->next = vm.cart->sprites;
    vm.cart->sprites = spr;
    lua_pushinteger(L, spr_id);
    return 1;
}

int api_line(lua_State *L)
{
    uint32_t x1 = (uint32_t)luaL_checknumber(L, 1);
    uint32_t y1 = (uint32_t)luaL_checknumber(L, 2);
    uint32_t x2 = (uint32_t)luaL_checknumber(L, 3);
    uint32_t y2 = (uint32_t)luaL_checknumber(L, 4);
    uint8_t color = luaL_checkinteger(L, 5)&0xFF;
    DrawLine(x1, y1, x2, y2, eightbitcolor_LUT[color]);
    vm.screen_dirty = 1; // is it worth it to duplicate on vm.screen?
    return 0;
}

int api_rect(lua_State *L)
{
    uint32_t x = (uint32_t)luaL_checknumber(L, 1);
    uint32_t y = (uint32_t)luaL_checknumber(L, 2);
    uint32_t w = (uint32_t)luaL_checknumber(L, 3);
    uint32_t h = (uint32_t)luaL_checknumber(L, 4);
    uint8_t color = luaL_checkinteger(L, 5)&0xFF;
    DrawRectangle(x, y, w, h, eightbitcolor_LUT[color]);
    vm.screen_dirty = 1; // is it worth it to duplicate on vm.screen?
    return 0;
}

int api_rectb(lua_State *L)
{
    uint32_t x = (uint32_t)luaL_checknumber(L, 1);
    uint32_t y = (uint32_t)luaL_checknumber(L, 2);
    uint32_t w = (uint32_t)luaL_checknumber(L, 3);
    uint32_t h = (uint32_t)luaL_checknumber(L, 4);
    uint8_t color = luaL_checkinteger(L, 5)&0xFF;
    DrawRectangleLines(x, y, w, h, eightbitcolor_LUT[color]);
    vm.screen_dirty = 1; // is it worth it to duplicate on vm.screen?
    return 0;
}

int api_pix(lua_State *L)
{
    int64_t x = luaL_checkinteger(L,1);
    int64_t y = luaL_checkinteger(L,2);
    // THIS FUNCTION IS VERY HACKY
    // It may not be performant, you probably shouldn't be doing this
    // In the original NeXUS  (LOVE2D) we had an imagedata that we carried
    // until something caused it to become invalid
    // reads were chonky, writes were also chonky for some godforsaken reason
    // We should be better off here but who knows
    // At the very least some graphics operations can update the image like
    // they would the screen here
    // (we couldn't do that in LOVE2D)
    // still is probably a good idea to do all your reads in one go
    // though pixel writes shouldn't invalidate the screen
    // (they didn't in LOVE2D either but writes were still chonky)
    if (lua_isnoneornil(L,3)) {
        if (NO_SCREEN()) {
            if (vm.screen.data==NULL) { // no screen data - pull down image
                // LoadImageFromTexture sets the important bits
                vm.screen = LoadImageFromTexture(vm.framebuffer.texture);
            } else { // screen dirty - pull pixels
                MemFree(vm.screen.data);
                // yeah this one's a doozy
                // basically we're doing what LoadImageFromTexture does
                // which means we directly invoke rlgl
                vm.screen.data = rlReadTexturePixels(vm.framebuffer.texture.id, vm.framebuffer.texture.width, vm.framebuffer.texture.height, vm.framebuffer.texture.format);
            }
        }
        // we now have a representation of the screen which we can trust in vm.screen
        Color c = GetImageColor(vm.screen, x, y);
        lua_pushinteger(L,eightbitcolor_nearest(c));
        return 1;
    } else {
        uint8_t c = luaL_checkinteger(L,3)&0xFF;
        DrawPixel(x, y, eightbitcolor_LUT[c]);
        // if we have a screen and it isn't dirty, keep it up to date
        // if it's not there, we get null pointer references
        // if it's dirty, there's no point-- we'll pull it down next read anyways!
        if (HAS_SCREEN()) ImageDrawPixel(&vm.screen, x, y, eightbitcolor_LUT[c]);
    }
    return 0;
}

int api_print(lua_State *L)
{
    const char *str = luaL_checklstring(L,1,0);
    if (!str) return 0;
    int x = luaL_optinteger(L,2,0);
    int y = luaL_optinteger(L,3,0);
    uint8_t color = luaL_optinteger(L,4,0xFF)&0xFF; // default white text
    DrawTextEx(vm.font, str, (Vector2){x, y}, 15, 0, eightbitcolor_LUT[color]);
    vm.screen_dirty = 1; // is it worth it to duplicate on vm.screen?
    return 0;
}

int api_spr(lua_State *L)
{
    uint32_t id = luaL_checkinteger(L, 1);
    uint32_t x = (uint32_t)luaL_checknumber(L, 2);
    uint32_t y = (uint32_t)luaL_checknumber(L, 3);
    double scale = luaL_optnumber(L, 4, 1.0f);
    int flip = luaL_optinteger(L, 5, 0)&3;
    double rotate = luaL_optnumber(L, 6, 0.0f);
    Cart_Sprites *spr = vm.cart->sprites;
    while (spr!=NULL && spr->id!=id) spr = spr->next;
    if (spr==NULL) luaL_error(L, "invalid sprite %d", id);
    double sx = scale;
    double sy = scale;
    if (flip&1) sx*=-1;
    if (flip&2) sy*=-1;
    // TODO: should rotate around origin (requires rlgl because of course it does)
    DrawTexturePro(spr->texture,(Rectangle){0,0,spr->img.width,spr->img.height},(Rectangle){0,0,spr->img.width*sx,spr->img.height*sy},(Vector2){x,y},0,WHITE);
    vm.screen_dirty = 1; // is it worth it to duplicate on vm.screen? probably not in this case
    return 0;
}

int api_textwidth(lua_State *L)
{
    const char *str = luaL_checklstring(L,1,0);
    if (!str) return 0;
    lua_pushinteger(L, MeasureTextEx(vm.font, str, 15, 0).x);
    return 1;
}

int api_tri(lua_State *L)
{
    double x1 = luaL_checknumber(L, 1);
    double y1 = luaL_checknumber(L, 2);
    double x2 = luaL_checknumber(L, 3);
    double y2 = luaL_checknumber(L, 4);
    double x3 = luaL_checknumber(L, 5);
    double y3 = luaL_checknumber(L, 6);
    uint8_t color = luaL_checkinteger(L, 7)&0xFF;
    DrawTriangle((Vector2){x1, y1}, (Vector2){x2, y2}, (Vector2){x3, y3}, eightbitcolor_LUT[color]);
    vm.screen_dirty = 1; // is it worth it to duplicate on vm.screen?
    return 0;
}

int api_trib(lua_State *L)
{
    double x1 = luaL_checknumber(L, 1);
    double y1 = luaL_checknumber(L, 2);
    double x2 = luaL_checknumber(L, 3);
    double y2 = luaL_checknumber(L, 4);
    double x3 = luaL_checknumber(L, 5);
    double y3 = luaL_checknumber(L, 6);
    uint8_t color = luaL_checkinteger(L, 7)&0xFF;
    DrawTriangleLines((Vector2){x1, y1}, (Vector2){x2, y2}, (Vector2){x3, y3}, eightbitcolor_LUT[color]);
    vm.screen_dirty = 1; // is it worth it to duplicate on vm.screen?
    return 0;
}

// MISC

int api_trace(lua_State *L)
{
    char *message = luaL_checklstring(L,1,0);
    if (!message) return 0;
    TraceLog(LOG_INFO,"TRACE: %s",message);
    return 0;
}

int api_version(lua_State *L)
{
    lua_pushliteral(L, "Alpha-RAYLIB");
    return 1;
}

struct NeXUS_API api_funcs[] = {
    {api_circ, "circ"},
    {api_circb, "circb"},
    {api_clip, "clip"},
    {api_cls, "cls"},
    {api_define_spr, "define_spr"},
    {api_line, "line"},
    {api_pix, "pix"},
    {api_print, "print"},
    {api_rect, "rect"},
    {api_rectb, "rectb"},
    {api_spr, "spr"},
    {api_textwidth, "textwidth"},
    {api_trace, "trace"},
    {api_tri, "tri"},
    {api_trib, "trib"},
    {api_version, "version"},
    {0, 0}
};

void RegisterFunction(struct NeXUS_API *func)
{
    lua_pushcfunction(L,func->func);
    lua_setglobal(L,func->name);
}

void InitLua(void)
{
    // yes I am aware of the Lua Uppercase Accident
    // but all of the other subsystems render their names in allcaps
    // and it'd be awkward if we didn't
    TraceLog(LOG_INFO,"LUA: Initializing Lua runtime");
    L = luaL_newstate();
    TraceLog(LOG_INFO,"LUA: Loading libraries");
    // code yoinked from linit.c (note the missing io and os libs above)
    for (const luaL_Reg *lib = loadedlibs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);
    }
    // nullify dofile and loadfile
    // technically you can still access the filesystem via `require` but I can't be bothered
    // besides that should be read-only which isn't too bad
    nullify("dofile");
    nullify("loadfile");
    // load API functions
    // similar run to above
    for (struct NeXUS_API *func = api_funcs; func->func; func++) {
        RegisterFunction(func);
    }
    // also initialize GC (the Lua interpreter does it so we should too probably)
    lua_gc(L, LUA_GCRESTART);
    lua_gc(L, LUA_GCGEN, 0, 0);
    TraceLog(LOG_INFO,"LUA: Lua runtime initialized!");
}

void SetGlobalString(char *name, char *val)
{
    lua_pushstring(L,val);
    lua_setglobal(L,name);
}

int LoadString(char * code, size_t len)
{
    return luaL_loadbufferx(L, code, len, "=[ROM code]", "t");
}

// message handler
// adds traceback and such
// yoinked from lua.c
int MessageHandler(lua_State *L)
{
    const char *msg = lua_tostring(L, 1);
    if (msg == NULL) {  /* is error object not a string? */
        if (luaL_callmeta(L, 1, "__tostring") &&  /* does it have a metamethod */
            lua_type(L, -1) == LUA_TSTRING)  /* that produces a string? */
            return 1;  /* that is the message */
        else
            msg = lua_pushfstring(L, "(error object is a %s value)",
                                    luaL_typename(L, 1));
    }
    luaL_traceback(L, L, msg, 1);  /* append a standard traceback */
    return 1;  /* return the traceback */
}

char * CopyString(const char * from)
{
    size_t len = strlen(from)+1;
    char *ret = MemAlloc(len);
    memcpy(ret, from, len);
    return ret;
}

// does the call in protected mode
// yoinked from lua.c
int DoCall(int narg, int nres)
{
    int status;
    int base = lua_gettop(L) - narg;  /* function index */
    lua_pushcfunction(L, MessageHandler);  /* push message handler */
    lua_insert(L, base);  /* put it under function and args */
    status = lua_pcall(L, narg, nres, base);
    lua_remove(L, base);  /* remove message handler from the stack */
    return status;
}

int CallGlobal(char * global)
{
    if (lua_getglobal(L, global)==LUA_TFUNCTION) {
        if (DoCall(0,0)!=LUA_OK) {
            char *msg = CopyString(lua_tostring(L,-1));
            TraceLog(LOG_ERROR,msg); // TODO: this should take you into the error screen
            lua_pop(L,1);
            ErrorScreen(msg);
            MemFree(msg);
        }
        return 1;
    } else {
        lua_pop(L,1);
    }
    return 0;
}

void CloseLua(void)
{
    lua_close(L);
    TraceLog(LOG_INFO,"LUA: Deinitialized Lua runtime");
}