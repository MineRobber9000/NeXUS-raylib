#include "saves.h"
#include "pattern.h"
#include <string.h>

// ensure save directory exists (call before any code that assumes it does)
void EnsureSaveDirectory() {
    if (!DirectoryExists("saves")) {
        MakeDirectory("saves");
    }
}

// save header
const char *saveHeader = "NSV\0";
const int headerLength = 4;

// get a filename given a "savename"
char * savenameToFilename(char *savename) {
    return (char *)TextFormat("saves/%s.nxsv", savename);
}

// load a save given a savename
NeXUS_Save saves_load(char *savename) {
    NeXUS_Save save = { 0 };
    // ensure save directory exists
    EnsureSaveDirectory();
    // validate name
    if (!saves_check(savename)) {
        save.error = "Invalid savename";
        return save;
    }
    // load file
    int len;
    unsigned char *data = LoadFileData(savenameToFilename(savename), &len);
    // check header
    if (memcmp(data, saveHeader, headerLength)!=0) {
        UnloadFileData(data);
        save.error = "Invalid save header!";
        return save;
    }
    // copy in data
    save.len = (size_t)len-headerLength;
    save.data = (uint8_t *)MemAlloc(save.len);
    if (save.data==NULL) {
        save.error = "No memory!";
        return save;
    }
    memcpy(save.data, data+headerLength, save.len);
    // free file
    UnloadFileData(data);
    return save;
}

// save a given save to a given savename
bool saves_save(char *savename, NeXUS_Save save) {
    // ensure save directory exists
    EnsureSaveDirectory();
    // validate name
    if (!saves_check(savename)) return false;
    // allocate data to use in saving
    unsigned char *data = (unsigned char *)MemAlloc(save.len+headerLength);
    // header, then data
    memcpy(data, saveHeader, headerLength);
    memcpy(data+headerLength, save.data, save.len);
    // do the saving
    bool returnValue = SaveFileData(savenameToFilename(savename), data, (int)save.len+headerLength);
    // free the allocated data before returning the value
    MemFree(data);
    return returnValue;
}

// list all saves with savenames matching pattern
char * TextCopyAlloc(char *src) {
    char *dst = (char *)MemAlloc(TextLength(src)+1);
    TextCopy(dst, src);
    return dst;
}

NeXUS_SaveList saves_list(char *pattern) {
    // ensure save directory exists
    EnsureSaveDirectory();
    // if pattern is not given, use ".+" (should match all non-empty)
    if (pattern==NULL) pattern = ".+";
    // list all files
    FilePathList files = LoadDirectoryFilesEx("saves", "*.nxsv", "");
    NeXUS_SaveList savelist = { 0 };
    // if there are no files, return early
    if (files.count==0) {
        savelist.files = files;
        return savelist;
    }
    // allocate space for filtered savelist
    savelist.files.paths = (char **)MemAlloc(files.count * sizeof(char *));
    // for each save file in the save directory:
    for (int i=0;i<files.count;++i) {
        Pattern_State ps;
        // savename = filename without extension (copy it here)
        char *filename = TextCopyAlloc((char *)GetFileNameWithoutExt(files.paths[i]));
        // check if it's valid
        if (!saves_check(filename)) {
            MemFree(filename);
            continue;
        }
        // match against pattern
        switch (pattern_match_cstr(&ps, filename, pattern)) {
            case PATTERN_MATCH: // successful match, add to filtered list
                savelist.files.paths[savelist.files.count++] = filename;
                break;
            case PATTERN_ERROR: // error, get error string here and fallthrough
                savelist.error = (char *)pattern_strerror(ps.error);
                // [[fallthrough]];
            case PATTERN_NO_MATCH: // no match, free copied name
                MemFree(filename);
                break;
        }
        // if the pattern had an error, bail early
        if (savelist.error) {
            UnloadDirectoryFiles(files);
            return savelist;
        }
    }
    // free the original filelist
    UnloadDirectoryFiles(files);
    // return the new savelist
    return savelist;
}

// returns whether a save exists
bool saves_exist(char *savename) {
    // ensure save directory exists
    EnsureSaveDirectory();
    // if the savename is invalid, refuse to check
    if (!saves_check(savename)) return false;
    // return whether it exists
    return FileExists(savenameToFilename(savename));
}

// returns whether a save has a valid name (only A-Z, a-z, 0-9, _, -, and .)
const char *ALLOWED_CHARS = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-.";
bool saves_check(char *savename) {
    return strspn(savename, ALLOWED_CHARS) == strlen(savename);
}

// delete the save with the given savename
bool saves_delete(char *savename) {
    // ensure save directory exists
    EnsureSaveDirectory();
    // if the savename is invalid, refuse to remove it
    if (!saves_check(savename)) return false;
    // remove it
    return FileRemove(savenameToFilename(savename))==0;
}