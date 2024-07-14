#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include "fat32.h"
static inline int sector_to_offset(int sector);
static void parse_directory_entries(void *cluster_data, struct FilesInfo *files_info);
static struct FilesInfo *read_directory(int cluster_number);
static int find_in_directory(const char *filename, struct FilesInfo *dir_files);
static void *get_cluster_data(int cluster);
static int next_cluster(int cluster);
static int read_directory_entry(int dir_sector, int index, struct DirEntry *entry);
static int find_path(const char *path, int *cluster, int *size);
static int find_path_1(const char *path, int *cluster, int *size);
struct Fat32BPB *hdr; // 指向 BPB 的数据
int mounted = -1;     // 是否已挂载成功

// 定义打开文件表
#define MAX_OPEN_FILES 128
struct OpenFile
{
    int cluster;
    int size;
};
struct OpenFile open_files[MAX_OPEN_FILES];

static inline int sector_to_offset(int sector)
{
    return sector * hdr->BPB_BytsPerSec;
}

static void parse_directory_entries(void *cluster_data, struct FilesInfo *files_info)
{
    struct DirEntry *entry = (struct DirEntry *)cluster_data;
    for (int i = 0; i < (hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus) / sizeof(struct DirEntry); i++)
    {
        if (entry->DIR_Name[0] == 0x00)
        {
            break;
        }
        if (entry->DIR_Name[0] != 0xE5)
        {
            strncpy((char *)files_info->files[files_info->size].DIR_Name, (char *)entry->DIR_Name, 11);
            files_info->files[files_info->size].DIR_FileSize = entry->DIR_FileSize;
            files_info->size++;
        }
        entry++;
    }
}
static struct FilesInfo *read_directory(int cluster_number)
{
    struct FilesInfo *files_info = malloc(sizeof(struct FilesInfo));
    if (!files_info)
        return NULL;
    files_info->files = malloc(4096 * sizeof(struct FileInfo));
    if (!files_info->files)
    {
        free(files_info);
        return NULL;
    }
    files_info->size = 0;

    void *cluster_data;
    while (cluster_number < 0x0FFFFFF8)
    {
        cluster_data = get_cluster_data(cluster_number);
        if (!cluster_data)
        {
            //free(cluster_data);
            //free(files_info->files);
            //free(files_info);
            return NULL;
        }
        parse_directory_entries(cluster_data, files_info);
        cluster_number = next_cluster(cluster_number);
        printf("11111: %x\n",cluster_number);
    }
    printf("%d\n",files_info->size);
    return files_info;
}

static void *get_cluster_data(int cluster)
{

    int sector = hdr->BPB_RsvdSecCnt + (hdr->BPB_NumFATs * hdr->BPB_FATSz32) + ((cluster - hdr->BPB_RootClus) * hdr->BPB_SecPerClus);
    return (void *)((char *)hdr + sector_to_offset(sector));
}

static int next_cluster(int cluster)
{
    int fat_sector = hdr->BPB_RsvdSecCnt + (cluster * 4 / (hdr->BPB_BytsPerSec));
    int fat_offset = (cluster * 4) % hdr->BPB_BytsPerSec;
    uint32_t *fat_entry = (uint32_t *)((char *)hdr + sector_to_offset(fat_sector) + fat_offset);
    return *fat_entry & 0x0FFFFFFF; 
}

/////////////
static int find_in_directory(const char *filename, struct FilesInfo *dir_files)
{
    for (int i = 0; i < dir_files->size; ++i)
    {
        struct FileInfo *file_info = &dir_files->files[i];
        char *file_name = strdup(filename);
        char *file_ext = strchr(file_name, '.');
        if (file_ext)
        {
            *file_ext = '\0'; 
            file_ext++;      
        }
        char dos_filename[11];
        strncpy(dos_filename, file_name, 8); 
        dos_filename[8] = '\0';              
        for (int j = 0; dos_filename[j] != '\0'; ++j)
        {
            dos_filename[j] = toupper(dos_filename[j]);
        }
        for (int j = strlen(dos_filename); j < 8; ++j)
        {
            dos_filename[j] = ' ';
        }
        if (strncmp(dos_filename, file_info->DIR_Name, 8) == 0)
        {
            if (file_ext)
            {
                char dos_extension[4];
                strncpy(dos_extension, file_ext, 3);
                dos_extension[3] = '\0';             
                for (int j = 0; dos_extension[j] != '\0'; ++j)
                {
                    dos_extension[j] = toupper(dos_extension[j]);
                }
                for (int j = strlen(dos_extension); j < 3; ++j)
                {
                    dos_extension[j] = ' '; 
                }
                if (strncmp(dos_extension, &file_info->DIR_Name[8], 3) != 0)
                {
                    continue; 
                }
            }
            else
            {
                char *ext_check = &file_info->DIR_Name[8];
                if (ext_check[0] != ' ' || ext_check[1] != ' ' || ext_check[2] != ' ')
                {
                    continue;
                }
            }
            return i; 
        }
    }

    return -1;
}
static int read_directory_entry(int dir_cluster, int logical_index, struct DirEntry *entry)
{
    int entries_per_cluster = (hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus) / sizeof(struct DirEntry);
    int current_cluster = dir_cluster;
    int physical_index = 0;
    while (logical_index >= 0)
    {
        void *dir_data = get_cluster_data(current_cluster);
        if (!dir_data)
        {
            return -1;
        }
        for (int i = 0; i < entries_per_cluster; ++i)
        {
            struct DirEntry *target_entry = (struct DirEntry *)((char *)dir_data + i * sizeof(struct DirEntry));
            if (target_entry->DIR_Name[0] == 0xE5)
            {
                continue;
            }
            if (physical_index == logical_index)
            {
                memcpy(entry, target_entry, sizeof(struct DirEntry));
                return 0; 
            }
            physical_index++;
        }
        int next_cluster_num = next_cluster(current_cluster);
        if (next_cluster_num < 0 || next_cluster_num >= 0x0FFFFFF8)
        {
            return -1;
        }
        current_cluster = next_cluster_num;
    }
    return -1;
}

static int find_path(const char *path, int *target_cluster, int *size)
{
    int current_cluster = hdr->BPB_RootClus; // 2
    char *path_copy = strdup(path);
    struct FilesInfo *current_dir_files = read_directory(current_cluster);
    char *token = strtok(path_copy, "/");
    while (token != NULL)
    {
        int file_index = find_in_directory(token, current_dir_files);
        struct DirEntry *dir_entry = malloc(sizeof(struct DirEntry));
        read_directory_entry(current_cluster, file_index, dir_entry);
        if ((dir_entry->DIR_Attr & DIRECTORY) == 0)
        {
            *target_cluster = (dir_entry->DIR_FstClusHI << 16) | dir_entry->DIR_FstClusLO; 
            *size = current_dir_files->files[file_index].DIR_FileSize;
            free(current_dir_files->files);
            free(current_dir_files);
            return 0;
        }
        current_cluster = (dir_entry->DIR_FstClusHI << 16) | dir_entry->DIR_FstClusLO;
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 挂载磁盘镜像
int fat_mount(const char *path)
{
    if (mounted == 0)
    { // 重复挂载
        return -1;
    }
    // 只读模式打开磁盘镜像
    int fd = open(path, O_RDWR);
    if (fd < 0)
    {
        // 打开失败
        return -1;
    }
    // 获取磁盘镜像的大小
    off_t size = lseek(fd, 0, SEEK_END);
    if (size == -1)
    {
        // 获取失败
        return -1;
    }
    // 将磁盘镜像映射到内存
    hdr = (struct Fat32BPB *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (hdr == (void *)-1)
    {
        // 映射失败
        return -1;
    }
    // 关闭文件
    close(fd);

    assert(hdr->Signature_word == 0xaa55);                   // MBR 的最后两个字节应该是 0x55 和 0xaa
    assert(hdr->BPB_TotSec32 * hdr->BPB_BytsPerSec == size); // 不相等表明文件的 FAT32 文件系统参数可能不正确
    // Data Region Start = BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FATSz32) + (BPB_RootEntCnt * sizeof(struct DirEntry))
    // date_region_start = hdr->BPB_RsvdSecCnt + (hdr->BPB_NumFATs *hdr->BPB_FATSz32)+ (hdr->BPB_rootEntCnt*sizeof(struct  DirEntry));

    // printf("hdr addr:%x\n",(uint32_t)(void*)hdr);
    //  打印 BPB 的部分信息
    printf("Some of the information about BPB is as follows\n");
    printf("OEM-ID \"%s\", \n", hdr->BS_oemName);
    printf("sectors/cluster %d, \n", hdr->BPB_SecPerClus);
    printf("bytes/sector %d, \n", hdr->BPB_BytsPerSec);
    printf("sectors %d, \n", hdr->BPB_TotSec32);
    printf("sectors/FAT %d, \n", hdr->BPB_FATSz32);
    printf("FATs %d, \n", hdr->BPB_NumFATs);
    printf("reserved sectors %d, \n", hdr->BPB_RsvdSecCnt);

    // 挂载成功
    mounted = 0;
    return 0;
}

// 打开文件
int fat_open(const char *path)
{
    if (mounted != 0)
        return -1; // 未挂载

    // 解析路径，找到文件对应的起始簇号和大小
    int cluster = 0;
    int size = 0;
    int result = find_path(path, &cluster, &size);

    if (size == 0) // 是一个目录
    {
        // printf("cannot open a dir!\n");
        return -1;
    }

    if (result != 0)
    {
        // printf("path_parse failed!\n");
        return -1; // 路径解析失败
    }

    for (int i = 0; i < MAX_OPEN_FILES; ++i)
    {
        if (open_files[i].cluster == 0)
        {                                    // 找到一个空闲的文件描述符
            open_files[i].cluster = cluster; // 假定 cluster 是路径解析后得到的簇号
            open_files[i].size = size;       // 假定 size 是路径解析后得到的文件大小
            // printf("cluster:%d,size:%d,fd:%d\n", cluster, size, i);
            return i; // 返回文件描述符
        }
    }
    // printf("cannot open more file!\n");
    return -1; // 打开文件数达到上限
}

// 关闭文件
int fat_close(int fd)
{
    if (mounted != 0)
    {
        return -1;
    }
    if (fd < 0 || fd >= MAX_OPEN_FILES || open_files[fd].cluster == 0)
        return -1;

    open_files[fd].cluster = 0; // 清空文件描述符
    open_files[fd].size = 0;
    return 0;
}

// 读取普通文件内容
int fat_pread(int fd, void *buffer, int count, int offset)
{
    // 在读取到文件末尾时，返回值可能会小于 count；
    // 如果输入参数 count 值为 0 或者 offset 超过了文件末尾，则应返回 0。
    // 对于其它读取失败的情况，返回 -1。
    if (mounted != 0)
    {
        return -1;
    }
    if (fd < 0 || fd >= MAX_OPEN_FILES || open_files[fd].cluster == 0)
        return -1;
    // 获取文件大小
    int fileSize = open_files[fd].size;

    // 当count为0或者offset超过文件末尾时返回0
    if (count == 0 || offset >= fileSize)
        return 0;

    // 计算实际可读取的最大字节数
    if (offset + count > fileSize)
        count = fileSize - offset;

    // 计算读取的起始簇和偏移量
    int cluster = open_files[fd].cluster;
    int cluster_size = hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus;
    int cluster_offset = offset % cluster_size;
    int start_cluster = offset / cluster_size;

    // 跳转到正确的簇
    for (int i = 0; i < start_cluster; ++i)
    {
        cluster = next_cluster(cluster);
    }
    // printf("111: %d \n", cluster);
    // 读取数据
    int bytes_read = 0;
    char *buf = (char *)buffer;
    while (bytes_read < count)
    {
        // 计算当前簇可读字节数
        int readable = cluster_size - cluster_offset;
        if (readable > count - bytes_read)
            readable = count - bytes_read;

        // 读取数据到缓冲区
        void *data = get_cluster_data(cluster);
        memcpy(buf + bytes_read, data + cluster_offset, readable);
        // printf("diff:%s %p %d\n",buf + bytes_read, data + cluster_offset, readable);
        bytes_read += readable;
        cluster_offset = 0; // 下一个簇从开始位置读

        // 检查是否需要读取下一个簇
        if (bytes_read < count)
        {
            cluster = next_cluster(cluster);
            if (cluster >= 0x0FFFFFF8 || cluster <= 0)
                break; // 文件结束
        }
    }
    return bytes_read;
}

// 读取目录内容的函数
struct FilesInfo *fat_readdir(const char *path)
//
{
    if (mounted != 0)
    {
        return NULL;
    }
    int cluster = 0;
    int size = 0;
    if (find_path(path, &cluster, &size) != 0)
    {
        return NULL;
    }

    if (size != 0) // 是一个file
    {
        // printf("cannot read a file!size:%d\n",size);
        return NULL;
    }
    printf("kjijjjjjjjjjjjjjjjjjjj\n");
    return read_directory(cluster);
}