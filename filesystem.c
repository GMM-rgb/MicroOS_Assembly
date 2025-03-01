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

// ... Add other filesystem function implementations ...
