#pragma once

#include <stdint.h>

struct config_entry_parent_t
{
    struct config_entry_parent_t *prev;
    struct config_entry_parent_t *next;
    int value_indent;
    char parent_path[1024];
};

struct config_entry_t
{
    struct config_entry_parent_t *parent;
    uint8_t location[1024];
    uint8_t value[1024];
};

struct config_t
{
    uint8_t location[1024];
    uint8_t in_group;
    struct config_entry_parent_t **parents;
    struct config_entry_t **entries;
    int entries_len;
    int parents_len;
};

void free_config(struct config_t **cfg);
struct config_t *load_config(char *path);
uint8_t config_entry_exists(struct config_t *cfg, char *path, char *entry);
uint8_t *config_entry_get(struct config_t *cfg, char *path, char *entry);
uint8_t **config_entry_get_section(struct config_t *cfg, char *path, int *_ret_len);
struct config_t *config_entry_set(struct config_t *cfg, char *path, char *entry_str, uint8_t *value);
