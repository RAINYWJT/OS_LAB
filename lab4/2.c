
// 路径解析函数
static int parse_path(const char *path, int *target_cluster, int *size)
{
    // 根目录的簇号是 2
    int current_cluster = hdr->BPB_RootClus; // 2

    // 复制路径以避免修改原始路径
    char *path_copy = strdup(path);
    struct FilesInfo *current_dir_files = read_directory(current_cluster);
    char *token = strtok(path_copy, "/");
    while (token != NULL)
    {
        int file_index = find_in_directory(token, current_dir_files);
        if (file_index == -1)
        {
            // 如果在目录中找不到文件或子目录，释放当前目录资源并返回错误
            free(current_dir_files->files);
            free(current_dir_files);
            free(path_copy);
            return -1;
        }

        // 获取当前文件或目录的信息
        struct DirEntry *dir_entry = malloc(sizeof(struct DirEntry));
        read_directory_entry(current_cluster, file_index, dir_entry);

        // 如果找到的是文件，则返回
        if ((dir_entry->DIR_Attr & DIRECTORY) == 0)
        { // 不是目录
            // printf("is not a directory!\n");
            *target_cluster = (dir_entry->DIR_FstClusHI << 16) | dir_entry->DIR_FstClusLO; // TODO error cluster
            // printf("??cluster:%d\n", *target_cluster);
            *size = current_dir_files->files[file_index].DIR_FileSize;
            free(current_dir_files->files);
            free(current_dir_files);
            return 0; // 返回 0 表示解析成功
        }

        // 更新当前簇号
        current_cluster = (dir_entry->DIR_FstClusHI << 16) | dir_entry->DIR_FstClusLO;

        // 读取下一个目录
        free(current_dir_files->files);
        free(current_dir_files);
        current_dir_files = read_directory(current_cluster);
        token = strtok(NULL, "/");
    }

    free(current_dir_files->files);
    free(current_dir_files);
    free(path_copy);

    *target_cluster = current_cluster;
    *size = 0;
    return 0;
}


////////////////////////////////////

static int parse_path(const char *path, int *target_cluster, int *size) {
    int current_cluster = 2; // 根目录的簇号
    char *path_copy = strdup(path);
    if (!path_copy || path_copy[0] != '/') {
        free(path_copy);
        return -1;
    }

    struct FilesInfo *current_dir_files = read_directory(current_cluster);
    if (!current_dir_files) {
        free(path_copy);
        return -1;
    }

    char *token = strtok(path_copy, "/");
    while (token != NULL) {
        int file_index = find_in_directory(token, current_dir_files);
        if (file_index == -1) {
            free_directory(current_dir_files);
            free(path_copy);
            return -1;
        }

        struct DirEntry *dir_entry = malloc(sizeof(struct DirEntry));
        read_directory_entry(current_cluster, file_index, dir_entry);

        if (!(dir_entry->DIR_Attr & DIRECTORY)) {
            *target_cluster = (dir_entry->DIR_FstClusHI << 16) | dir_entry->DIR_FstClusLO;
            *size = current_dir_files->files[file_index].DIR_FileSize;
            free_directory(current_dir_files);
            free(dir_entry);
            free(path_copy);
            return 0;
        }

        free_directory(current_dir_files);
        current_cluster = (dir_entry->DIR_FstClusHI << 16) | dir_entry->DIR_FstClusLO;
        current_dir_files = read_directory(current_cluster);
        if (!current_dir_files) {
            free(dir_entry);
            free(path_copy);
            return -1;
        }

        token = strtok(NULL, "/");
    }

    *target_cluster = current_cluster;
    *size = 0;
    free_directory(current_dir_files);
    free(dir_entry);
    free(path_copy);
    return 0;
}

// 假设的辅助函数
void free_directory(struct FilesInfo *dir) {
    free(dir->files);
    free(dir);
}