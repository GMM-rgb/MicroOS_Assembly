#ifndef MICROOS_FILESYSTEM_H
#define MICROOS_FILESYSTEM_H

#include <time.h>
#include <stdbool.h>

#define MAX_FILENAME 256
#define MAX_PATH 1024
#define MAX_CONTENT 4096
#define MAX_FILES 100
#define MAX_CHILDREN 20

typedef struct FileNode {
    char name[MAX_FILENAME];
    bool is_directory;
    time_t created;
    time_t modified;
    size_t size;
    char content[MAX_CONTENT];
    struct FileNode* parent;
    struct FileNode* children[MAX_CHILDREN];
    int child_count;
} FileNode;

typedef struct {
    FileNode* root;
    FileNode* current_dir;
} FileSystem;

// Filesystem operations
FileSystem* fs_init(void);
void fs_destroy(FileSystem* fs);
FileNode* fs_create_file(FileSystem* fs, const char* path, bool is_directory);
bool fs_delete_file(FileSystem* fs, const char* path);
FileNode* fs_get_file(FileSystem* fs, const char* path);
bool fs_write_file(FileSystem* fs, const char* path, const char* content);
char* fs_read_file(FileSystem* fs, const char* path);
bool fs_rename(FileSystem* fs, const char* old_path, const char* new_path);
char* fs_get_current_path(FileSystem* fs);
bool fs_change_dir(FileSystem* fs, const char* path);

// Utility functions
void fs_list_directory(FileSystem* fs, const char* path, FileNode*** files, int* count);
char* fs_format_size(size_t size);
char* fs_format_time(time_t time);

#endif // MICROOS_FILESYSTEM_H
