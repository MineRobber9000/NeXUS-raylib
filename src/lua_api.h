#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
#include "nexus.h"

extern lua_State *L;

void InitLua(void);
void CloseLua(void);
int LoadString(char * code, size_t len);
int DoCall(int nargs, int nres);
int CallGlobal(char * global);

char * CopyString(char * from);
struct NeXUS_API {
    lua_CFunction func;
    const char * name;
};
void RegisterFunction(struct NeXUS_API *func);