#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Helper function to split path
static char** split_path(const char* path, int* count) {
    char* path_copy = strdup(path);
    char** parts = malloc(sizeof(char*) * MAX_PATH);
    *count = 0;
    
    char* token = strtok(path_copy, "/");
    while (token != NULL) {
        parts[*count] = strdup(token);
        (*count)++;
        token = strtok(NULL, "/");
    }
    
    free(path_copy);
    return parts;
}

// New function: computes size of a node.
// For files, returns file->size. For directories, recursively sums sizes.
size_t fs_get_size(FileNode* node) {
    if (!node) return 0;
    if (!node->is_directory)
        return node->size;
    size_t total = 0;
    for (int i = 0; i < node->child_count; i++) {
        total += fs_get_size(node->children[i]);
    }
    return total;
}

FileSystem* fs_init(void) {
    FileSystem* fs = malloc(sizeof(FileSystem));
    fs->root = malloc(sizeof(FileNode));
    strcpy(fs->root->name, "/");
    fs->root->is_directory = true;
    fs->root->created = time(NULL);
    fs->root->modified = time(NULL);
    fs->root->size = 0;
    fs->root->parent = NULL;
    fs->root->child_count = 0;
    fs->current_dir = fs->root;
    
    // Create some default directories
    fs_create_file(fs, "/home", true);
    fs_create_file(fs, "/system", true);
    fs_create_file(fs, "/apps", true);
    
    // Create some sample files
    fs_create_file(fs, "/home/welcome.txt", false);
    fs_write_file(fs, "/home/welcome.txt", "Welcome to MicroOS!\nType 'help' for commands.\n");
    
    return fs;
}

FileNode* fs_create_file(FileSystem* fs, const char* path, bool is_directory) {
    int count;
    char** parts = split_path(path, &count);
    
    FileNode* current = fs->root;
    FileNode* parent = NULL;
    
    // Navigate to parent directory
    for (int i = 0; i < count - 1; i++) {
        bool found = false;
        for (int j = 0; j < current->child_count; j++) {
            if (strcmp(current->children[j]->name, parts[i]) == 0) {
                parent = current;
                current = current->children[j];
                found = true;
                break;
            }
        }
        if (!found) return NULL;
    }
    
    // Create new node
    FileNode* new_node = malloc(sizeof(FileNode));
    strcpy(new_node->name, parts[count - 1]);
    new_node->is_directory = is_directory;
    new_node->created = time(NULL);
    new_node->modified = time(NULL);
    new_node->size = 0;
    new_node->parent = current;
    new_node->child_count = 0;
    
    // Add to parent
    if (current->child_count < MAX_CHILDREN) {
        current->children[current->child_count++] = new_node;
    }
    
    // Cleanup
    for (int i = 0; i < count; i++) {
        free(parts[i]);
    }
    free(parts);
    
    return new_node;
}

void fs_list_directory(FileSystem* fs, const char* path, FileNode*** files, int* count) {
    FileNode* dir = fs_get_file(fs, path);
    if (dir && dir->is_directory) {
        *files = dir->children;
        *count = dir->child_count;
    } else {
        *files = NULL;
        *count = 0;
    }
}

// Modified fs_get_file: Supports absolute paths (starting with '/'),
// relative paths (starting without '/'), and tokens "." and "..".
FileNode* fs_get_file(FileSystem* fs, const char* path) {
    int count;
    char** parts = split_path(path, &count);
    
    // Start from root if path begins with '/', otherwise from current directory.
    FileNode* current = (path[0]=='/') ? fs->root : fs->current_dir;
    
    for (int i = 0; i < count; i++) {
        if (strcmp(parts[i], ".") == 0) {
            // Do nothing, remain in current directory.
        } else if (strcmp(parts[i], "..") == 0) {
            if (current->parent != NULL)
                current = current->parent;
        } else {
            bool found = false;
            for (int j = 0; j < current->child_count; j++) {
                if (strcmp(current->children[j]->name, parts[i]) == 0) {
                    current = current->children[j];
                    found = true;
                    break;
                }
            }
            if (!found) {
                current = NULL;
                break;
            }
        }
    }
    
    for (int i = 0; i < count; i++) {
        free(parts[i]);
    }
    free(parts);
    
    return current;
}

bool fs_write_file(FileSystem* fs, const char* path, const char* content) {
    FileNode* file = fs_get_file(fs, path);
    if (file && !file->is_directory) {
        strncpy(file->content, content, MAX_CONTENT);
        file->size = strlen(content);
        file->modified = time(NULL);
        return true;
    }
    return false;
}

char* fs_read_file(FileSystem* fs, const char* path) {
    FileNode* file = fs_get_file(fs, path);
    if (file && !file->is_directory) {
        return file->content;
    }
    return NULL;
}

char* fs_format_size(size_t size) {
    static char buffer[20];
    if (size < 1024) {
        snprintf(buffer, sizeof(buffer), "%zu B", size);
    } else if (size < 1024 * 1024) {
        snprintf(buffer, sizeof(buffer), "%.2f KB", size / 1024.0);
    } else {
        snprintf(buffer, sizeof(buffer), "%.2f MB", size / (1024.0 * 1024.0));
    }
    return buffer;
}

char* fs_format_time(time_t time) {
    static char buffer[20];
    struct tm* tm_info = localtime(&time);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", tm_info);
    return buffer;
}

char* fs_get_current_path(FileSystem* fs) {
    static char path[MAX_PATH];
    FileNode* current = fs->current_dir;
    path[0] = '\0';
    
    while (current != NULL) {
        char temp[MAX_PATH];
        snprintf(temp, sizeof(temp), "/%s%s", current->name, path);
        strcpy(path, temp);
        current = current->parent;
    }
    
    return path;
}

bool fs_change_dir(FileSystem* fs, const char* path) {
    FileNode* dir = fs_get_file(fs, path);
    if (dir && dir->is_directory) {
        fs->current_dir = dir;
        return true;
    }
    return false;
}

// ... Add other filesystem function implementations ...
