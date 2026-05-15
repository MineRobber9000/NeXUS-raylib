#include "raylib.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
    size_t len;
    uint8_t *data;
    char *error; // NULL if no error
} NeXUS_Save;

typedef struct {
    char *error; // NULL if no error
    FilePathList files;
} NeXUS_SaveList;

NeXUS_Save saves_load(char *savename);
bool saves_save(char *savename, NeXUS_Save save);
NeXUS_SaveList saves_list(char *pattern);
bool saves_exist(char *savename);
bool saves_check(char *savename);
bool saves_delete(char *savename);