#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h> 
#include "fat32.h"
#include <string.h>

#define MAX_OPEN_FILE_NUM 128

struct OpenFile
{
    int cluster;
    int size;
    int open;
};
struct OpenFile open_files[MAX_OPEN_FILE_NUM];

struct Fat32BPB *hdr; // 指向 BPB 的数据
int mounted = -1; // 是否已挂载成功

//////////////////////////////////////////////////////////////////////////////
//fat 32 assistance function

static void parse_directory_entries_1(void *cluster_data, struct FilesInfo *files_info) {
    struct DirEntry *entry = (struct DirEntry *)cluster_data;
    int entries_count = (hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus) / sizeof(struct DirEntry);
    for(int i = 0; i < entries_count; i++, entry++) {
        uint32_t attr = ((struct DirEntry *)cluster_data)[i].DIR_Attr;
        if (entry->DIR_Name[0] == 0x00)
        {
            break;
        }
        if (entry->DIR_Name[0] != 0xE5 &&entry->DIR_Name[0]!= 0x20 && (attr == 0x01 || attr == 0x02 ||
        attr == 0x04 || attr == 0x08 || attr == 0x10 || attr == 0x20))
        {
            strncpy((char *)files_info->files[files_info->size].DIR_Name, (char *)entry->DIR_Name, 11);
            files_info->files[files_info->size].DIR_FileSize = entry->DIR_FileSize;
            files_info->size++;
        }
    }
}

static void parse_directory_entries(void *cluster_data, struct FilesInfo *files_info) {
    struct DirEntry *entry = (struct DirEntry *)cluster_data;
    int entries_count = (hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus) / sizeof(struct DirEntry);
    for(int i = 0; i < entries_count; i++, entry++) {
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
    }
}


static int next_cluster(int cluster) {
    int fat_sector = hdr->BPB_RsvdSecCnt + (cluster * 4 / hdr->BPB_BytsPerSec);
    int fat_offset = (cluster * 4) % hdr->BPB_BytsPerSec;
    uint32_t *fat_entry = (uint32_t *)((char *)hdr + (fat_sector * hdr->BPB_BytsPerSec) + fat_offset);
    // printf("fat entry : %x\n",fat_entry);
    return *fat_entry & 0x0FFFFFFF;
}

static void *get_cluster_data(int cluster) {
    int sector = hdr->BPB_RsvdSecCnt + (hdr->BPB_NumFATs * hdr->BPB_FATSz32) +
                 ((cluster - hdr->BPB_RootClus) * hdr->BPB_SecPerClus);
    return(void *)((char *)hdr +(sector* hdr->BPB_BytsPerSec));
}

static struct FilesInfo *read_directory(int cluster_number) {
    struct FilesInfo *files_info = malloc(sizeof(struct FilesInfo));
    if (!files_info)
        return NULL;
    // printf("%d\n",files_info->size);
    files_info->files = malloc(4096 *2 * sizeof(struct FileInfo));
    if(!files_info->files) {
        free(files_info);
        return NULL;
    }
    files_info->size = 0;
    void *cluster_data;
    int count = 0;
    while(cluster_number < 0x0FFFFFF8) {
        cluster_data = get_cluster_data(cluster_number);
        //printf("%d\n",cluster_data);
        parse_directory_entries(cluster_data, files_info);
        printf("before Cluster number : %x\n",cluster_number);
        cluster_number = next_cluster(cluster_number);
        printf("after seek Cluster number : %x\n",cluster_number);
        count ++;
    }
    printf("-------------count------------:  %d\n",count);
    //printf("%d\n",files_info->size);
    return files_info;
}

static struct FilesInfo *read_directory_1(int cluster_number) {
    struct FilesInfo *files_info = malloc(sizeof(struct FilesInfo));
    if (!files_info)
        return NULL;
    // printf("%d\n",files_info->size);
    files_info->files = malloc(4096 *2 * sizeof(struct FileInfo));
    if(!files_info->files) {
        free(files_info);
        return NULL;
    }
    files_info->size = 0;
    void *cluster_data;
    int count = 0;
    while(cluster_number < 0x0FFFFFF8) {
        cluster_data = get_cluster_data(cluster_number);
        //printf("%d\n",cluster_data);
        parse_directory_entries_1(cluster_data, files_info);
        printf("before Cluster number : %x\n",cluster_number);
        cluster_number = next_cluster(cluster_number);
        printf("after seek Cluster number : %x\n",cluster_number);
        count ++;
    }
    printf("-------------count------------:  %d\n",count);
    //printf("%d\n",files_info->size);
    return files_info;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// 找文件
/////////////转换成8，3
void convert_to_dos_filename(const char *filename, char *dos_filename, char *dos_extension) {
    char *file_name = strdup(filename);
    char *file_ext = strchr(file_name, '.');
    if(file_ext) {
        *file_ext = '\0';
        file_ext++;
    }

    strncpy(dos_filename, file_name, 8);
    dos_filename[8] = '\0';
    for(int j = 0; dos_filename[j] != '\0'; ++j) {
        dos_filename[j] = toupper(dos_filename[j]);
    }
    for(int j = strlen(dos_filename); j < 8; ++j) {
        dos_filename[j] = ' ';
    }

    if(file_ext) {
        strncpy(dos_extension, file_ext, 3);
        dos_extension[3] = '\0';
        for (int j = 0; dos_extension[j] != '\0'; ++j) {
            dos_extension[j] = toupper(dos_extension[j]);
        }
        for (int j = strlen(dos_extension); j < 3; ++j) {
            dos_extension[j] = ' ';
        }
    }else {
        dos_extension[0] = ' ';
        dos_extension[1] = ' ';
        dos_extension[2] = ' ';
        dos_extension[3] = '\0';
    }

    free(file_name);
}

// 查找目录中的文件
static int find_in_directory(const char *filename, struct FilesInfo *dir_files) {
    char dos_filename[9];
    char dos_extension[4];
    convert_to_dos_filename(filename, dos_filename, dos_extension);

    for(int i = 0; i < dir_files->size; i++) {
        //printf("cmp : %s, %s\n", dos_extension, dir_files->files[i].DIR_Name);
        if(strncmp(dos_filename, dir_files->files[i].DIR_Name, 8) == 0 &&
            strncmp(dos_extension, dir_files->files[i].DIR_Name + 8, 3) == 0) {
            return i;
        }
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

static int read_directory_entry(int dir_cluster, int logic_idx, struct DirEntry *entry)
{
    int entries_per_cluster = (hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus) / sizeof(struct DirEntry);
    int current_cluster = dir_cluster;
    int physical_index = 0;
    while (logic_idx >= 0)
    {
        void *dir_data = get_cluster_data(current_cluster);
        if (!dir_data)
        {
            return -1;
        }
        for (int i = 0; i < entries_per_cluster; i++)
        {
            struct DirEntry *target_entry = (struct DirEntry *)((char *)dir_data + i * sizeof(struct DirEntry));
            if (target_entry->DIR_Name[0] == 0xE5)
            {
                continue;
            }
            if (physical_index == logic_idx)
            {
                memcpy(entry, target_entry, sizeof(struct DirEntry));
                return 0; 
            }
            physical_index++;
        }
        // 获取下一个簇号
        int next_cluster_num = next_cluster(current_cluster);
        if (next_cluster_num < 0 || next_cluster_num >= 0x0FFFFFF8)
        {
            return -1;
        }
        current_cluster = next_cluster_num;
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct {
    int current_cluster;
    int next_index;
} DirectoryIterator;

void init_directory_iterator(DirectoryIterator *iterator, int root_cluster) {
    iterator->current_cluster = root_cluster;
    iterator->next_index = 0;
}

int iterate_next_entry(DirectoryIterator *iterator, struct DirEntry *entry) {
    void *dir_data = get_cluster_data(iterator->current_cluster);
    if(!dir_data){
        return -1;
    }
    int entries_per_cluster = (hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus) / sizeof(struct DirEntry);
    while(iterator->next_index >= entries_per_cluster) { // 需要的扇区多一些
        iterator->next_index -= entries_per_cluster; 
        iterator->current_cluster = next_cluster(iterator->current_cluster); 
        if(iterator->current_cluster < 0 || iterator->current_cluster >= 0x0FFFFFF8){
            return -1;
        }
        dir_data = get_cluster_data(iterator->current_cluster);
        if(!dir_data) {
            return -1;
        }
    }
    memcpy(entry, (struct DirEntry *)((char *)dir_data + iterator->next_index * sizeof(struct DirEntry)), sizeof(struct DirEntry));
    iterator->next_index++;
    return 0;
}

static int find_path(const char *path, int *target_cluster, int *size) {
    if(path[0] != '/' || (strlen(path) == 1 && path[0] == '/')) {
        return -1; 
    }
    int current_cluster = hdr->BPB_RootClus;
    char *token = strtok((char *)path, "/");
    struct DirEntry entry;
    struct FilesInfo *dir_files = read_directory(current_cluster);
    printf("path1!!!\n");
    while(token != NULL) {
        printf("pat1h1!!!\n");
        if(!dir_files) {
            return -1; // 读取目录失败
        }
        int file_index = find_in_directory(token, dir_files);
        if(file_index == -1) {
            free(dir_files->files);
            free(dir_files);
            return -1; // 文件或目录未找到
        }
        if(read_directory_entry(current_cluster, file_index, &entry) != 0) {
            free(dir_files->files);
            free(dir_files);
            return -1;
        }
        if(!(entry.DIR_Attr & DIRECTORY)) {
            // 如果是文件
            *target_cluster = (entry.DIR_FstClusHI << 16) | entry.DIR_FstClusLO;
            *size = entry.DIR_FileSize;
            free(dir_files->files);
            free(dir_files);
            return 0;
        }else {
            // 如果是目录
            current_cluster = (entry.DIR_FstClusHI << 16) | entry.DIR_FstClusLO;
            free(dir_files->files);
            free(dir_files);
            dir_files = read_directory(current_cluster);
            token = strtok(NULL, "/");
        }
    }
    *target_cluster = current_cluster;
    *size = 0;
    return 0;
}


static int find_path_1(const char *path, int *target_cluster, int *size) {
    int current_cluster = hdr->BPB_RootClus;
    char *token = strtok((char *)path, "/");
    struct DirEntry entry;
    struct FilesInfo *dir_files = read_directory(current_cluster);
    printf("path1!!!\n");
    while(token != NULL) {
        printf("pat1h1!!!\n");
        if(!dir_files) {
            return -1; // 读取目录失败
        }
        int file_index = find_in_directory(token, dir_files);
        if(file_index == -1) {
            free(dir_files->files);
            free(dir_files);
            return -1; // 文件或目录未找到
        }
        if(read_directory_entry(current_cluster, file_index, &entry) != 0) {
            free(dir_files->files);
            free(dir_files);
            return -1;
        }
        if(!(entry.DIR_Attr & DIRECTORY)) {
            // 如果是文件
            *target_cluster = (entry.DIR_FstClusHI << 16) | entry.DIR_FstClusLO;
            *size = entry.DIR_FileSize;
            free(dir_files->files);
            free(dir_files);
            return 0;
        }else {
            // 如果是目录
            current_cluster = (entry.DIR_FstClusHI << 16) | entry.DIR_FstClusLO;
            free(dir_files->files);
            free(dir_files);
            dir_files = read_directory(current_cluster);
            token = strtok(NULL, "/");
        }
    }
    *target_cluster = current_cluster;
    *size = 0;
    return 0;
}

int byte_read(int fd, void *buffer, int count, int offset){
    int cluster = open_files[fd].cluster;
    int cluster_size = hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus;
    int cluster_offset = offset % cluster_size;
    int start_cluster = offset / cluster_size;
    for (int i = 0; i < start_cluster; i++) {
        cluster = next_cluster(cluster);
        if (cluster >= 0x0FFFFFF8 || cluster <= 0) {
            return -1; 
        }
    }
    int bytes_read = 0;
    char *buf = (char *)buffer;
    while (bytes_read < count) {
        int readable = cluster_size - cluster_offset;
        if (readable > count - bytes_read) {
            readable = count - bytes_read;
        }
        void *data = get_cluster_data(cluster);
        if (!data) {
            return -1; 
        }
        memcpy(buf + bytes_read, data + cluster_offset, readable);
        bytes_read += readable;
        cluster_offset = 0;
        if (bytes_read < count) {
            cluster = next_cluster(cluster);
            if (cluster >= 0x0FFFFFF8 || cluster <= 0) {
                break; 
            }
        }
    }
    printf("111: %d\n",bytes_read);
    return bytes_read;
}
//////////////////////////////////////////////////////////////////////////////

// 挂载磁盘镜像
int fat_mount(const char *path) {
    // 只读模式打开磁盘镜像
    int fd = open(path, O_RDWR);
    if (fd < 0){
        // 打开失败
        return -1;
    }
    // 获取磁盘镜像的大小
    off_t size = lseek(fd, 0, SEEK_END);
    if (size == -1){
        // 获取失败
        return -1;
    }
    // 将磁盘镜像映射到内存
    hdr = (struct Fat32BPB *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (hdr == (void *)-1){
        // 映射失败
        return -1;
    }
    // 关闭文件
    close(fd);

    assert(hdr->Signature_word == 0xaa55); // MBR 的最后两个字节应该是 0x55 和 0xaa
    assert(hdr->BPB_TotSec32 * hdr->BPB_BytsPerSec == size); // 不相等表明文件的 FAT32 文件系统参数可能不正确

    // 打印 BPB 的部分信息
    printf("Some of the information about BPB is as follows\n");
    printf("OEM-ID \"%s\", \n", hdr->BS_oemName);
    printf("sectors/cluster %d, \n", hdr->BPB_SecPerClus);
    printf("bytes/sector %d, \n", hdr->BPB_BytsPerSec);
    printf("sectors %d, \n", hdr->BPB_TotSec32);
    printf("sectors/FAT %d, \n", hdr->BPB_FATSz32);
    printf("FATs %d, \n", hdr->BPB_NumFATs);
    printf("reserved sectors %d, \n", hdr->BPB_RsvdSecCnt);

    // Date Region = 保留扇区数 + FAT占用扇区数
    uint32_t data_start_sector = hdr->BPB_RsvdSecCnt + (hdr->BPB_NumFATs * hdr->BPB_FATSz32);
    printf("Data Region starts at sector %d\n", data_start_sector);

    // 挂载成功
    mounted = 0;
    return 0;
}

// 打开文件
int fat_open(const char *path){
    if(mounted != 0){
        return -1;
    }
    int size = 0;
    int cluster = 0;
    int find = find_path(path, &cluster, &size);
    // printf("find %d\n",find);
    if (find != 0 || size == 0)
    {
        return -1;
    }
    for (int i = 0; i < MAX_OPEN_FILE_NUM; ++i)
    {
        //printf("open %d\n",i);
        if (open_files[i].cluster == 0)
        {                                    
            open_files[i].cluster = cluster; 
            open_files[i].size = size;   
            open_files[i].open = 1;    
            return i; 
        }
    }
    return -1; 
}

// 关闭文件
int fat_close(int fd){
    if((mounted != 0) || (fd < 0 || fd >= MAX_OPEN_FILE_NUM || open_files[fd].cluster == 0)){
        return -1;
    }
    open_files[fd].cluster = 0;
    open_files[fd].size = 0;
    open_files[fd].open = 0;    
    return 0;
}

int fat_pread(int fd, void *buffer, int count, int offset) {
    if (mounted != 0 || fd < 0 || fd >= MAX_OPEN_FILE_NUM) {
        printf("fd:%d\n",fd);
        return -1; 
    }
    int size = open_files[fd].size;
    //printf("%d\n", open_files[fd].cluster);
    //printf("112: %d\n",size);
    if (size == 0 && open_files[fd].open == 0){
        return -1;
    }
    if (count == 0 || offset >= size) {
        return 0; 
    }
    if (offset + count > size) {
        count = size - offset;
    }
    //printf("cnt: %d\n",size);
    return byte_read(fd, buffer, count, offset);
}


// 读取目录文件内容 (目录项)
struct FilesInfo* fat_readdir(const char *path) {
    printf("%s\n",path);
    if(mounted != 0 ){
        return NULL;
    }
    if (path[0] != '/') {
        return NULL;
    }
    int cluster = 0;
    int size = 0;
    printf("-----------------------------------------------\n");
    if (find_path_1(path, &cluster, &size) != 0)
    {
        return NULL;
    }
    if (size != 0)
    {
        return NULL;
    }
    printf("kjijjjjjjjjjjjjjjjjjjj\n");
    struct FilesInfo *res = read_directory_1(cluster);
    /*
    int valid_count = 0;
    
    for (int i = 0; i < res->size; i++) {
        //printf("%x %d\n",res->files[i].DIR_Name[i], res->files[i].DIR_FileSize);
        if (res->files[i].DIR_FileSize <= 0x0FFFFFF8) {
            // 合规的文件项(乱写的，把其他的AE, A0排除掉即可)
            res->files[valid_count] = res->files[i];
            valid_count++;
        }
    }

    res->size = valid_count;
    */
    // for (int i = 0; i < res->size; i++) {
    //    printf("Name: %s, Size: %u\n", res->files[i].DIR_Name, res->files[i].DIR_FileSize);
    //}
    printf("-----------------------------------------------\n");
    return res;
}
