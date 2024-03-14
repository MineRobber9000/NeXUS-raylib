#include "raylib.h"
#include "eightbitcolor.h"
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

struct NeXUS_API {
    lua_CFunction func;
    const char * name;
};

// GRAPHICS

int api_cls(lua_State *L)
{
    uint8_t color = luaL_optinteger(L,1,0)&0xFF;
    ClearBackground(eightbitcolor_LUT[color]);
    return 0;
}

int api_pix(lua_State *L)
{
    int64_t x = luaL_checkinteger(L,1);
    int64_t y = luaL_checkinteger(L,2);
    uint8_t c = luaL_checkinteger(L,3)&0xFF;
    DrawPixel(x, y, eightbitcolor_LUT[c]);
    return 0;
}

int api_print(lua_State *L)
{
    const char *str = luaL_checklstring(L,1,0);
    if (!str) return 0;
    int x = luaL_optinteger(L,2,0);
    int y = luaL_optinteger(L,3,0);
    uint8_t color = luaL_optinteger(L,4,0xFF)&0xFF; // default white text
    DrawTextEx(font, str, (Vector2){x, y}, 15, 0, eightbitcolor_LUT[color]);
    return 0;
}

int api_textwidth(lua_State *L)
{
    const char *str = luaL_checklstring(L,1,0);
    if (!str) return 0;
    lua_pushinteger(L, MeasureTextEx(font, str, 15, 0).x);
    return 1;
}

// MISC

int api_version(lua_State *L)
{
    lua_pushliteral(L, "Alpha-RAYLIB");
    return 1;
}

struct NeXUS_API api_funcs[] = {
    {api_cls, "cls"},
    {api_pix, "pix"},
    {api_print, "print"},
    {api_textwidth, "textwidth"},
    {api_version, "version"},
    {0, 0}
};

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
        lua_pushcfunction(L,func->func);
        lua_setglobal(L,func->name);
    }
    // also initialize GC (the Lua interpreter does it so we should too probably)
    lua_gc(L, LUA_GCRESTART);
    lua_gc(L, LUA_GCGEN, 0, 0);
    TraceLog(LOG_INFO,"LUA: Lua runtime initialized!");
}

void LoadString(char * code, size_t len)
{
    luaL_loadbufferx(L, code, len, "[ROM code]", "t");
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
            const char *msg = lua_tostring(L,-1);
            TraceLog(LOG_ERROR,msg); // TODO: this should take you into the error screen
            lua_pop(L,1);
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