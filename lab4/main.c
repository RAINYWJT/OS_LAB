#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat32.h"

#define MAX_COMMAND_LENGTH 100

int main(int argc, char *argv[])
{
    // if (argc < 2) {
    //     printf("Usage: %s <image_path>\n", argv[0]);
    //     return 1;
    // }

    const char *image_path = "fat32.img";
    int mounted = fat_mount(image_path);
    if (mounted != 0)
    {
        printf("Error: unable to mount image %s\n", image_path);
        return 1;
    }

    char command[MAX_COMMAND_LENGTH];
    while (1)
    {
        //printf("Enter command (type 'exit' to quit): ");
        fgets(command, MAX_COMMAND_LENGTH, stdin);
        command[strcspn(command,"\n")]='\0';
        //printf("now cmd:%s",command);
        if (strcmp(command, "exit") == 0)
        {
            break;
        }

        char *token = strtok(command, " ");
        if (!token)
        {
            printf("Error: invalid command\n");
            continue;
        }

        const char *command_str = token;
        if (strcmp(command_str, "close") == 0)
        {
            int close_num = atoi(strtok(NULL, " "));
            int fd = fat_close(close_num);
            printf("File close: %d\n", fd);
        }
        if (strcmp(command_str, "open") == 0)
        {

            char *path = strtok(NULL, " ");

            printf("path:%s\n", path);
            path[strlen(path)] = '\0';
            int fd = fat_open(path);
            printf("File descriptor for %s: %d\n", path, fd);
        }
        else if (strcmp(command_str, "pread") == 0)
        {

            int fd = atoi(strtok(NULL, " "));
            int count = atoi(strtok(NULL, " "));
            int offset = atoi(strtok(NULL, " "));

            if (fd!=-1)
            {
                char *buffer = (char *)malloc(count + 1);
                printf("cnt111: %d\n",count);
                int read_size = fat_pread(fd, buffer, count, offset);
                buffer[read_size] = '\0'; // Ensure null-terminated string
                printf("Read %d bytes from file_fd %d: %s\n", read_size, fd, buffer);
                free(buffer);
                //fat_close(fd);
            }
            else
            {
                printf("Error: unable to open file_fd %d\n", fd);
            }
        }
        else if (strcmp(command_str, "read") == 0)
        {

            const char *path = strtok(NULL, " ");
            int count = atoi(strtok(NULL, " "));
            int offset = atoi(strtok(NULL, " "));
            int fd = fat_open(path);
            if (fd != -1)
            {
                char *buffer = (char *)malloc(count + 1);
                int read_size = fat_pread(fd, buffer, count, offset);
                buffer[read_size] = '\0'; // Ensure null-terminated string
                printf("Read %d bytes from file %s: %s\n", read_size, path, buffer);
                free(buffer);
                fat_close(fd);
            }
            else
            {
                printf("Error: unable to open file %s\n", path);
            }
        }
        else if (strcmp(command_str, "readdir") == 0)
        {

            const char *path = strtok(NULL, " ");
            struct FilesInfo *files = fat_readdir(path);
            if (files)
            {
                printf("Files in directory %s:\n", path);
                for (int i = 0; i < files->size; ++i)
                {
                    printf("Name: %s, Size: %u\n", files->files[i].DIR_Name, files->files[i].DIR_FileSize);
                }
                free(files->files);
                free(files);
            }
            else
            {
                printf("Error: unable to read directory %s\n", path);
            }
        }
    }

    //fat_mount(NULL); // Unmount the image
    return 0;
}