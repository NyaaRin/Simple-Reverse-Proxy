#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "configutils.h"

void free_config(struct config_t **cfg)
{
    int a = 0;
    
    for(a = 0; a < (*cfg)->entries_len; a++)
    {
        free((*cfg)->entries[a]);
        (*cfg)->entries[a] = NULL;
    }
    free((*cfg)->entries);
    (*cfg)->entries = NULL;

    for(a = 0; a < (*cfg)->parents_len; a++)
    {
        free((*cfg)->parents[a]);
        (*cfg)->parents[a] = NULL;
    }
    free((*cfg)->parents);
    (*cfg)->parents = NULL;

    (*cfg)->entries_len = 0;
    (*cfg)->parents_len = 0;

    (*cfg)->in_group = 0;

    configutil_zero((*cfg)->location, 1024);

    free(*cfg);
    *cfg = NULL;
}

struct config_t *load_config(char *path)
{
    struct config_t *config = malloc(sizeof(struct config_t));

    struct config_entry_parent_t *curr = NULL;

    configutil_zero(config->location, 1024);
    configutil_cpy(config->location, path, 0, configutil_len(path));
    config->in_group = 0;
    config->entries = NULL;
    config->entries_len = 0;
    config->parents = NULL;
    config->parents_len = 0;

    FILE *fp = fopen(path, "r");
    if(!fp)
    {
        free(config);
        config = NULL;
        return config;
    }

    unsigned char *buffer = malloc(1024);
    configutil_zero(buffer, 1024);
    while(fgets(buffer, 1024, fp) != NULL)
    {
        int pos, first_parent = 0;

        for(pos = 0; pos < 1024; pos++)
        {
            if(buffer[pos] == '\r')
            {
                buffer[pos] = 0;
                continue;
            }
            if(buffer[pos] == '\n')
            {
                buffer[pos] = 0;
                continue;
            }
        }

        //check for larger group
        for(pos = 0; pos < 1024; pos++)
        {
            if(buffer[pos] == '[')
            {
                config->in_group++;
                first_parent = 1;
                break;
            }
        }

        // get parent of group
        if(config->in_group > 0 && first_parent == 1)
        {
            config->parents = realloc(config->parents, (config->parents_len + 1) * sizeof(struct config_entry_parent_t *));
            config->parents[config->parents_len] = malloc(sizeof(struct config_entry_parent_t));
            struct config_entry_parent_t *parent = config->parents[config->parents_len];
            config->parents_len++;

            parent->next = NULL;
            parent->prev = NULL;
            configutil_zero(parent->parent_path, 1024);

            for(pos = 0; pos < 1024; pos++)
            {
                if(buffer[pos] == ':')
                {
                    if(curr == NULL)
                    {
                        configutil_cpy(parent->parent_path, buffer+configutil_get_white_end(buffer, 1024), 0, pos-configutil_get_white_end(buffer, 1024));
                        parent->prev = NULL;
                        curr = parent;
                    }
                    else
                    {
                        int parent_path_len = 0;
                        configutil_cpy(parent->parent_path, curr->parent_path, 0, configutil_len(curr->parent_path));
                        parent_path_len = configutil_len(parent->parent_path);
                        configutil_cpy(parent->parent_path+parent_path_len, ".", 0, 1);
                        parent_path_len = configutil_len(parent->parent_path);
                        configutil_cpy(parent->parent_path+parent_path_len, buffer+configutil_get_white_end(buffer, 1024), 0, pos-configutil_get_white_end(buffer, 1024));
                        parent->prev = curr;
                        curr = parent;
                    }
                    first_parent = 0;
                    break;
                }
            }
        }
        else if (config->in_group > 0)// process children of a parent
        {
            for(pos = 0; pos < 1024; pos++)
            {
                if(buffer[pos] == ']')
                {
                    config->in_group--;
                    if(curr != NULL && curr->prev != NULL)
                    {
                        curr = curr->prev;
                    }
                    else
                    {
                        curr = NULL;
                    }
                    break;
                }
                if(buffer[pos] == ':')
                {
                    config->entries = realloc(config->entries, (config->entries_len+1) * sizeof(struct config_entry_t *));
                    config->entries[config->entries_len] = malloc(sizeof(struct config_entry_t));
                    struct config_entry_t *entry = config->entries[config->entries_len];
                    int start = configutil_get_white_end(buffer, 1024);
                    config->entries_len++;
                    entry->parent = curr;
                    configutil_zero(entry->location, 1024);
                    configutil_zero(entry->value, 1024);
                    configutil_cpy(entry->location, buffer+start, 0, pos-start);
                    configutil_cpy(entry->value, buffer+pos+2, 0, configutil_len(buffer)-pos-2);
                    break;
                }
            }
        }
        else
        {
            for(pos = 0; pos < 1024; pos++)
            {
                if(buffer[pos] == ':')
                {
                    config->entries = realloc(config->entries, (config->entries_len+1) * sizeof(struct config_entry_t *));
                    config->entries[config->entries_len] = malloc(sizeof(struct config_entry_t));
                    struct config_entry_t *entry = config->entries[config->entries_len];
                    int start = configutil_get_white_end(buffer, 1024);
                    config->entries_len++;
                    entry->parent = NULL;
                    configutil_zero(entry->location, 1024);
                    configutil_zero(entry->value, 1024);
                    configutil_cpy(entry->location, buffer+start, 0, pos-start);
                    configutil_cpy(entry->value, buffer+pos+2, 0, configutil_len(buffer)-pos-2);
                    break;
                }
            }
        }

        configutil_zero(buffer, 1024);
    }

    fclose(fp);
    fp = NULL;

    #ifdef DEBUG
    int x = 0;
    for(x = 0; x < config->parents_len; x++)
    {
        printf("have group %d-(%s)\r\n", x, config->parents[x]->parent_path);
    }

    for(x = 0; x < config->entries_len; x++)
    {
        if(config->entries[x]->parent == NULL)
        {
            printf("have root entry %d-(%s)-(%s)\r\n", x, config->entries[x]->location, config->entries[x]->value);
        }
        else 
        {
            printf("have entry in group %d-(%s:%s)-(%s)\r\n", x, config->entries[x]->parent->parent_path, config->entries[x]->location, config->entries[x]->value);
        }
    }
    #endif

    free(buffer);
    buffer = NULL;

    return config;
}

uint8_t config_entry_exists(struct config_t *cfg, char *path, char *entry)
{
    int x;
    if(path != NULL)
    {
        for(x = 0; x < cfg->parents_len; x++)
        {
            if(strcmp(cfg->parents[x]->parent_path, path) == 0)
            {
                if(entry == NULL) return 1;
                struct config_entry_parent_t *parent;
                parent = cfg->parents[x];
                for(x = 0; x < cfg->entries_len; x++)
                {
                    if(parent == cfg->entries[x]->parent)
                    {
                        if(strcmp(cfg->entries[x]->location, entry) == 0)
                        {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    else if(entry != NULL)
    {
        for(x = 0; x < cfg->entries_len; x++)
        {
            if(NULL == cfg->entries[x]->parent)
            {
                if(strcmp(cfg->entries[x]->location, entry) == 0)
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}

uint8_t *config_entry_get(struct config_t *cfg, char *path, char *entry)
{
    int x;
    if(path != NULL)
    {
        for(x = 0; x < cfg->parents_len; x++)
        {
            if(strcmp(cfg->parents[x]->parent_path, path) == 0)
            {
                struct config_entry_parent_t *parent;
                parent = cfg->parents[x];
                for(x = 0; x < cfg->entries_len; x++)
                {
                    if(parent == cfg->entries[x]->parent)
                    {
                        if(strcmp(cfg->entries[x]->location, entry) == 0)
                        {
                            uint8_t *ret = malloc(configutil_len(cfg->entries[x]->value)+2);
                            configutil_zero(ret, configutil_len(cfg->entries[x]->value)+2);
                            configutil_cpy(ret, cfg->entries[x]->value, 0, configutil_len(cfg->entries[x]->value));
                            configutil_cpy(ret+configutil_len(ret), "\0", 0, 1);
                            return ret;
                        }
                    }
                }
            }
        }
    }
    else
    {
        for(x = 0; x < cfg->entries_len; x++)
        {
            if(NULL == cfg->entries[x]->parent)
            {
                if(strcmp(cfg->entries[x]->location, entry) == 0)
                {
                    uint8_t *ret = malloc(configutil_len(cfg->entries[x]->value)+2);
                    configutil_zero(ret, configutil_len(cfg->entries[x]->value)+2);
                    configutil_cpy(ret, cfg->entries[x]->value, 0, configutil_len(cfg->entries[x]->value));
                    configutil_cpy(ret+configutil_len(ret), "\0", 0, 1);
                    return ret;
                }
            }
        }
    }
    uint8_t *ret = malloc(5);
    configutil_cpy(ret, "NULL\0", 0, 5);
    return ret;
}

uint8_t **config_entry_get_section(struct config_t *cfg, char *path, int *_ret_len)
{
    uint8_t **ret = NULL;
    int x, ret_len = 0;
    if(path != NULL)
    {
        for(x = 0; x < cfg->parents_len; x++)
        {
            if(strcmp(cfg->parents[x]->parent_path, path) == 0)
            {
                int j = 0;
                
                for(j = 0; j < cfg->entries_len; j++)
                {
                    if(cfg->entries[j]->parent != NULL && cfg->entries[j]->parent->prev != NULL)
                    {
                        if(!strcmp(path, cfg->entries[j]->parent->prev->parent_path))
                        {
                             int i, found = 0;
                             for(i = 0; i < ret_len; i++)
                             {
                                  if(strcmp(ret[i], cfg->entries[j]->parent->parent_path) == 0)
                                  {
                                      found = 1;
                                      break;
                                  }
                             }
                             
                            if(found == 1) continue;
                            
                            printf("Found entry section %s at %d\r\n", cfg->entries[j]->parent->parent_path, j);
                            ret = realloc(ret, (ret_len+1)*sizeof(uint8_t *));
                            ret[ret_len] = malloc(configutil_len(cfg->entries[j]->parent->parent_path)+1);
                            uint8_t *str = ret[ret_len];
                            strcpy(str,cfg->entries[j]->parent->parent_path);
                            str[configutil_len(cfg->entries[j]->parent->parent_path)] = 0;
                            ret_len++;
                            continue;
                        }
                    }
                    else if(path == NULL)
                    {
                        int i, found = 0;
                        for(i = 0; i < ret_len; i++)
                        {
                          if(strcmp(ret[i],cfg->entries[j]->parent->parent_path) == 0)
                          {
                              found = 1;
                              break;
                          }
                        }

                        if(found == 1) continue;

                        ret = realloc(ret, (ret_len+1)*sizeof(uint8_t *));
                        ret[ret_len] = malloc(configutil_len(cfg->entries[j]->parent->parent_path)+1);
                        uint8_t *str = ret[ret_len];
                        strcpy(str,cfg->entries[j]->parent->parent_path);
                        str[configutil_len(cfg->entries[j]->parent->parent_path)] = 0;
                        ret_len++;
                        continue;
                    }
                }
                break;
            }
        }
    }
    *_ret_len = ret_len;
    return ret;
}

struct config_t *config_entry_set(struct config_t *cfg, char *path, char *entry_str, uint8_t *value)
{
    struct config_entry_parent_t *parent_find = {NULL};
    int parent_find_len = 0;

    struct config_entry_parent_t *curr = NULL;

    FILE *fp = fopen(cfg->location, "r");
    uint8_t path_exists = 1;
    
    if(!fp)
    {
        return cfg;
    }

    char newfile[1024];
    configutil_zero(newfile, 1024);
    configutil_cpy(newfile, cfg->location, 0, configutil_len(cfg->location));
    configutil_cpy(newfile+configutil_len(newfile), ".tmp", 0, 4);
    FILE *newfp = fopen(newfile, "w+");
    if(!newfp)
    {
        return cfg;
    }

    if(path != NULL)
    {
        path_exists = 0;
        if(config_entry_exists(cfg, path, NULL) != 1)
        {
            int path_parts_count = 0;
            uint8_t **path_parts = configutil_tokenize(path, configutil_len(path), &path_parts_count, '.');

            #ifdef DEBUG
            printf("got %d path_parts from path %s\r\n", path_parts_count, path);
            #endif
            if(path_parts != NULL)
            {
                if(path_parts_count > 0)
                {
                    struct config_entry_parent_t *last_parent = NULL;
                    char *tmp_path = malloc(512);//start at 0th part and check exists
                    int tmp_path_len = 0;
                    int x; 

                    configutil_zero(tmp_path, 512);

                    for(x = 0; x < path_parts_count; x++)
                    {
                        #ifdef DEBUG
                        printf("%d:(%s)\r\n", configutil_len(path_parts[x]), path_parts[x]);
                        #endif
                        if(x > 0)
                        {
                            tmp_path_len = configutil_cpy(tmp_path, ".", tmp_path_len, tmp_path_len+1);
                            tmp_path_len = configutil_cpy(tmp_path, path_parts[x], tmp_path_len, tmp_path_len+configutil_len(path_parts[x]));
                        }
                        else
                        {
                            tmp_path_len = configutil_cpy(tmp_path, path_parts[x], tmp_path_len, tmp_path_len+configutil_len(path_parts[x]));
                        }

                        #ifdef DEBUG
                        printf("tmp_path = (%s)\r\n", tmp_path);
                        #endif

                        if(config_entry_exists(cfg, tmp_path, NULL) == 1)
                        {
                            path_exists = 1;
                            if(last_parent != NULL)
                            {
                                free(last_parent);
                                last_parent = NULL;
                            }
                            last_parent = malloc(sizeof(struct config_entry_parent_t));
                            configutil_zero(last_parent, sizeof(struct config_entry_parent_t));
                            configutil_cpy(last_parent->parent_path, tmp_path, 0, tmp_path_len);
                        }
                        else
                        {
                            if(last_parent != NULL)
                            {
                                parent_find = realloc(parent_find, (parent_find_len+1)*sizeof(struct config_entry_parent_t));
                                configutil_zero(&(parent_find[parent_find_len]), sizeof(struct config_entry_parent_t));
                                configutil_cpy(parent_find[parent_find_len].parent_path, last_parent->parent_path, 0, configutil_len(last_parent->parent_path));
                                parent_find_len++;
                                free(last_parent);
                                last_parent = NULL;
                            }

                            parent_find = realloc(parent_find, (parent_find_len+1)*sizeof(struct config_entry_parent_t));
                            configutil_zero(&(parent_find[parent_find_len]), sizeof(struct config_entry_parent_t));
                            configutil_cpy(parent_find[parent_find_len].parent_path, tmp_path, 0, tmp_path_len);
                            parent_find_len++;
                        }
                        free(path_parts[x]);
                        path_parts[x] = NULL;
                    }
                    configutil_zero(tmp_path, 512);
                    free(tmp_path);
                    tmp_path = NULL;
                    
                    free(path_parts);
                    path_parts = NULL;
                }
            }
        }
        else
        {
            path_exists = 1;

            parent_find = realloc(parent_find, (parent_find_len+1)*sizeof(struct config_entry_parent_t));
            configutil_zero(&(parent_find[parent_find_len]), sizeof(struct config_entry_parent_t));
            configutil_cpy(parent_find[parent_find_len].parent_path, path, 0, configutil_len(path));
            parent_find_len++;
        }
    }

    #ifdef DEBUG

    if(parent_find != NULL)
    {
        int x;
        for(x = 0; x < parent_find_len; x++)
        {
            printf("parent_find[%d] = (%s)\r\n", x, parent_find[x].parent_path);
        }
    }

    #endif

    if(path_exists == 0)
    {
        int x;
        for(x = 0; x < parent_find_len; x++)
        {
            int xx, indent = x*5;
            for(xx = 0; xx < indent; xx++)
            {
                fprintf(newfp, " ");
            }
            fprintf(fp, "%s: [ \r\n", parent_find[x].parent_path);
            if(x == parent_find_len-1)
            {
                for(xx = 0; xx < indent+5; xx++)
                {
                    fprintf(newfp, " ");
                }
                fprintf(newfp, "%s: %s \r\n", entry_str, value);
            }
        }
        for(x = 0; x < parent_find_len; x++)
        {
            int xx, indent = x*5;
            for(xx = 0; xx < indent+5; xx++)
            {
                fprintf(newfp, " ");
            }
            fprintf(newfp, "] \r\n");
        }

        fclose(fp);
        fclose(newfp);
        unlink(cfg->location);
        rename(newfile, cfg->location);

        char cfg_path[1024];
        configutil_zero(cfg_path, 1024);
        configutil_cpy(cfg_path, cfg->location, 0, configutil_len(cfg->location));

        free_config(&cfg);

        cfg = load_config(cfg_path);

        return cfg;
    }

    uint8_t replaced = 0;
    uint8_t flush = 1;
    int in_group = 0;
    int group_count = 0;

    char *buffer = malloc(1024);
    configutil_zero(buffer, 1024);
    while(fgets(buffer, 1024, fp) != NULL)
    {
        flush = 1;
        int pos, first_parent = 0;

        for(pos = 0; pos < 1024; pos++)
        {
            if(buffer[pos] == '\r')
            {
                buffer[pos] = 0;
                continue;
            }
            if(buffer[pos] == '\n')
            {
                buffer[pos] = 0;
                continue;
            }
        }

        //check for larger group
        for(pos = 0; pos < 1024; pos++)
        {
            if(buffer[pos] == '[')
            {
                in_group++;
                group_count++;
                first_parent = 1;
                break;
            }
        }

        // get parent of group
        if(in_group > 0 && first_parent == 1)
        {
            struct config_entry_parent_t *parent = malloc(sizeof(struct config_entry_parent_t));

            parent->prev = NULL;
            parent->next = NULL;
            configutil_zero(parent->parent_path, 1024);
            parent->value_indent = 0;

            uint8_t end = 0;
            for(pos = 0; pos < 1024; pos++)
            {
                if(buffer[pos] == ':')
                {
                    if(curr == NULL)
                    {
                        configutil_cpy(parent->parent_path, buffer+configutil_get_white_end(buffer, 1024), 0, pos-configutil_get_white_end(buffer, 1024));
                        parent->prev = NULL;
                        curr = parent;
                        parent->value_indent = 1;
                    }
                    else
                    {
                        int parent_path_len = 0;
                        configutil_cpy(parent->parent_path, curr->parent_path, 0, configutil_len(curr->parent_path));
                        parent_path_len = configutil_len(parent->parent_path);
                        configutil_cpy(parent->parent_path+parent_path_len, ".", 0, 1);
                        parent_path_len = configutil_len(parent->parent_path);
                        configutil_cpy(parent->parent_path+parent_path_len, buffer+configutil_get_white_end(buffer, 1024), 0, pos-configutil_get_white_end(buffer, 1024));
                        parent->value_indent += curr->value_indent+1;
                        parent->prev = curr;
                        curr = parent;
                        #ifdef DEBUG
                        printf("Got Parent %s\r\n", parent->parent_path);
                        #endif
                    }
                    first_parent = 0;
                    break;
                }
            }
        }
        else if (in_group > 0)// process children of a parent
        {
            for(pos = 0; pos < 1024; pos++)
            {
                if(buffer[pos] == ']')
                {
                    if(replaced == 0 && curr != NULL && parent_find != NULL)
                    {
                        if(strcmp(curr->parent_path, parent_find[0].parent_path) == 0 && value != NULL)
                        {
                            int indent = curr->value_indent;
                            if(parent_find_len == 1)
                            {
                                char tmp[512];
                                int j;

                                configutil_zero(tmp, 512);
                                for(j = 0; j < indent*5; j++)
                                {
                                    configutil_cpy(tmp+configutil_len(tmp), " ", 0, 1);
                                }

                                fprintf(newfp, "%s%s: %s\r\n", tmp, entry_str, value);
                                fflush(newfp);
                                replaced = 1;
                                flush = 1;
                                
                                #ifdef DEBUG
                                printf("[5] write %s:%s with %s\r\n", path, entry_str, value);
                                #endif
                            }
                            else
                            {
                                int x;
                                for(x = 1; x < parent_find_len; x++)
                                {
                                    char tmp[512];
                                    int j;

                                    configutil_zero(tmp, 512);

                                    for(j = 0; j < indent*5; j++)
                                    {
                                        configutil_cpy(tmp+configutil_len(tmp), " ", 0, 1);
                                    }

                                    int path_parts_count = 0;
                                    uint8_t **path_parts = configutil_tokenize(parent_find[x].parent_path, configutil_len(parent_find[x].parent_path), &path_parts_count, '.');

                                    if(path_parts != NULL)
                                    {
                                        if(path_parts_count > 0)
                                        {
                                            fprintf(newfp, "%s%s: [\r\n", tmp, path_parts[path_parts_count-1]);
                                            fflush(newfp);
                                            for(j = 0; j < path_parts_count; j++)
                                            {
                                                free(path_parts[j]);
                                                path_parts[j] = NULL;
                                            }
                                        }
                                        free(path_parts);
                                        path_parts = NULL;
                                    }
                                    
                                    indent++;

                                    configutil_zero(tmp, 512);

                                    
                                    if(x == parent_find_len-1)
                                    {
                                        for(j = 0; j < indent*5; j++)
                                        {
                                            configutil_cpy(tmp+configutil_len(tmp), " ", 0, 1);
                                        }

                                        fprintf(newfp, "%s%s: %s\r\n", tmp, entry_str, value);
                                        fflush(newfp);
                                        replaced = 1;
                                        flush = 1;
                                        
                                        #ifdef DEBUG
                                        printf("[3] write %s:%s with %s\r\n", path, entry_str, value);
                                        #endif
                                    }
                                }

                                for(x = 1; x < parent_find_len; x++)
                                {
                                    char tmp[512];
                                    int j;

                                    configutil_zero(tmp, 512);

                                    indent--;

                                    for(j = 0; j < indent*5; j++)
                                    {
                                        configutil_cpy(tmp+configutil_len(tmp), " ", 0, 1);
                                    }

                                    fprintf(newfp, "%s]\r\n", tmp);
                                    fflush(newfp);
                                    
                                    indent--;
                                }
                            }
                        }
                    }
                    in_group--;
                    if(curr != NULL && curr->prev != NULL)
                    {
                        curr->prev->next = curr;
                        curr = curr->prev;
                        free(curr->next);
                    }
                    else if(curr != NULL && curr->prev == NULL)
                    {
                        free(curr);
                        curr = NULL;
                    }
                    else
                    {
                        curr = NULL;
                    }
                    break;
                }
                if(buffer[pos] == ':')
                {
                    struct config_entry_t *entry = malloc(sizeof(struct config_entry_t));
                    entry->parent = curr;

                    configutil_zero(entry->location, 1024);
                    configutil_zero(entry->value, 1024);
                    configutil_cpy(entry->location, buffer+configutil_get_white_end(buffer, 1024), 0, pos-configutil_get_white_end(buffer, 1024));
                    configutil_cpy(entry->value, buffer+pos+2, 0, 256);
                    if(parent_find != NULL && strcmp(parent_find[0].parent_path, entry->parent->parent_path) == 0)
                    {
                        if(strcmp(entry->location, entry_str) == 0)
                        {
                            if(value == NULL)
                            {
                                replaced = 1;
                                flush = 0;
                                #ifdef DEBUG
                                printf("No Value! \r\n");
                                #endif
                            }
                            else 
                            {
                                int whitespace = configutil_get_white_end(buffer, 1024);
                                int a = 0;
                                for(a = 0; a < whitespace; a++)
                                {
                                    fprintf(newfp, " ");
                                }
                                fprintf(newfp, "%s: %s\r\n", entry->location, value);
                                fflush(newfp);
                                replaced = 1;
                                flush = 0;
                                #ifdef DEBUG
                                printf("[1] replace %s:%s with %s\r\n", parent_find[0].parent_path, entry->location, value);
                                #endif
                            }
                        }
                    }
                    free(entry);
                    entry = NULL;
                    break;
                }
            }
        }
        else
        {
            for(pos = 0; pos < 1024; pos++)
            {
                if(buffer[pos] == ':')
                {
                    struct config_entry_t *entry = malloc(sizeof(struct config_entry_t));
                    entry->parent = NULL;

                    configutil_zero(entry->location, 1024);
                    configutil_zero(entry->value, 1024);

                    configutil_cpy(entry->location, buffer+configutil_get_white_end(buffer, 1024), 0, pos-configutil_get_white_end(buffer, 1024));
                    configutil_cpy(entry->value, buffer+pos+2, 0, 256);
                    if(path == NULL)
                    {
                        if(strcmp(entry->location, entry_str) == 0)
                        {
                            if(value == NULL)
                            {
                                replaced = 1;
                                flush = 0;
                                #ifdef DEBUG
                                printf("No Value!2\r\n");
                                #endif
                            }
                            else 
                            {
                                int whitespace = configutil_get_white_end(buffer, 1024);
                                int a = 0;
                                for(a = 0; a < whitespace; a++)
                                {
                                    fprintf(newfp, " ");
                                }
                                fprintf(newfp, "%s: %s\r\n", entry->location, value);
                                fflush(newfp);
                                replaced = 1;
                                flush = 0;
                                #ifdef DEBUG
                                printf("[2] replace %s:%s with %s\r\n", path, entry->location, value);
                                #endif
                            }
                        }
                    }
                    free(entry);
                    entry = NULL;
                    break;
                }
            }
        }

        if(flush == 1)
        {
            fprintf(newfp, "%s\r\n", buffer);
            fflush(newfp);
        }
        else
        {
            flush = 1;
        }
        configutil_zero(buffer, 1024);
    }

    if(replaced == 0)
    {
        if(path == NULL && value != NULL)
        {
            fprintf(newfp, "%s: %s\r\n", entry_str, value);
            fflush(newfp);
            #ifdef DEBUG
            printf("[4] replace %s with %s\r\n", entry_str, value);
            #endif
        }
        else if(path != NULL && value != NULL)
        {

        }
    }

    configutil_zero(buffer, 1024);
    free(buffer);
    buffer = NULL;

    fclose(fp);
    fclose(newfp);
    unlink(cfg->location);
    rename(newfile, cfg->location);

    char cfg_path[1024];
    configutil_zero(cfg_path, 1024);
    configutil_cpy(cfg_path, cfg->location, 0, configutil_len(cfg->location));

    free_config(&cfg);

    cfg = load_config(cfg_path);

    return cfg;
}
