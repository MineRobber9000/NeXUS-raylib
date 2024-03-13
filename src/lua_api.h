#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
#include "nexus.h"

extern lua_State *L;

void InitLua(void);
void CloseLua(void);
void LoadString(char * code, size_t len);
int DoCall(int nargs, int nres);
int CallGlobal(char * global);