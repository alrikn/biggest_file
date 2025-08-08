#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>

#define TOP_NUM 100

typedef struct list {
    char **dir_list;      // dynamically growing array of directory names
    char *names[TOP_NUM];     // top 100 file names
    long long sizes[TOP_NUM];      // their corresponding sizes
    long long num_dirs;        // number of directories found
    int file_taken;     // how many top files have been found
} list_t;

char **realloc_char_array(list_t *list, const char *new_name)
{
    size_t i = list->num_dirs;
    char **new_array;

    new_array = realloc(list->dir_list, sizeof(char *) * (i + 2));
    if (!new_array)
        return NULL;
    new_array[i] = strdup(new_name);
    if (!new_array[i]) {
        new_array[i] = NULL;
        return NULL;
    }
    new_array[i + 1] = NULL;
    list->dir_list = new_array;
    list->num_dirs++;
    return new_array;
}

void find_big(list_t *list, const char *file_name, long long size)
{
    int index = 0;
    long smallest_found = list->sizes[0];

    if (file_name == NULL)
        return;
    if (list->file_taken < TOP_NUM) {
        list->names[list->file_taken] = strdup(file_name);
        list->sizes[list->file_taken] = size;
        list->file_taken += 1;
        return;
    }
    for (int i = 1; i < TOP_NUM; i++)
        if (list->sizes[i] < smallest_found) {
            smallest_found = list->sizes[i];
            index = i;
        }
    if (smallest_found < size) {
        free(list->names[index]);
        list->names[index] = strdup(file_name);
        list->sizes[index] = size;
    }
}

void main_loop(list_t *list)
{
    long current_dir_index = 0;
    struct stat st;
    char full_path[4097];
    struct dirent *entry;
    char *dir_name;
    DIR *dir;

    while (current_dir_index < list->num_dirs) {
        if (current_dir_index % 1000 == 0) {
            if (current_dir_index == 200000)
                break;
            printf("searched through %ld directories, %lld left to search\n", current_dir_index, list->num_dirs);
            printf("currently at dir %s\n", list->dir_list[current_dir_index]);
        }
        dir_name = list->dir_list[current_dir_index];
        dir = opendir(dir_name);
        if (!dir) {
            current_dir_index++;
            continue;
        }
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_name, entry->d_name);
            if (stat(full_path, &st) == 0) {
                if (S_ISDIR(st.st_mode)) {
                    realloc_char_array(list, full_path);
                }
                if (S_ISREG(st.st_mode))
                    find_big(list, full_path, st.st_size);
            }
        }
        closedir(dir);
        current_dir_index++;
    }
}

/*
biggest goes at list_t->names[99]
smallest at list->names[0]
*/
void basic_sorter(list_t *list)
{
    long long size_temp;
    char *file_temp;

    for (int i = 0; i < list->file_taken; i++) {
        for (int j = i + 1; j < list->file_taken; j++) {
            if (list->sizes[i] > list->sizes[j]) {
                size_temp = list->sizes[i];
                file_temp = list->names[i];
                list->sizes[i] = list->sizes[j];
                list->names[i] = list->names[j];
                list->sizes[j] = size_temp;
                list->names[j] = file_temp;
            }
        }
    }
}

/*
** TODO:    -L for level
**          -d for dir (already implemented)
**          -h for help
**          -a for hidden (so we should ignore hidden until this flag is set)
** TODO:    make the dir list array grow by factor of 2 when it needs (to avoid unecessary sys calls)
*/
int main(int argc, char **argv)
{
    list_t list = {0};
    const char *start_dir = (argc > 1) ? argv[1] : ".";

    list.dir_list = NULL;
    realloc_char_array(&list, start_dir);
    list.file_taken = 0;
    main_loop(&list);
    basic_sorter(&list);
    printf("searched through %lld directories.\n", list.num_dirs);
    printf("Top files:\n");
    for (int i = 0; i < list.file_taken; i++)
        printf("%lld bytes: %s\n", list.sizes[i], list.names[i]);
    for (int i = 0; i < list.num_dirs; i++)
        free(list.dir_list[i]);
    free(list.dir_list);
    for (int i = 0; i < list.file_taken; i++)
        free(list.names[i]);
    return 0;
}
