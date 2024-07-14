#ifndef FAT_H_
#define FAT_H_

#include <stdint.h>

// FAT32 文件系统中 BPB 的各个数据，具体含义请参照 FAT 规范
struct Fat32BPB {
    uint8_t BS_jmpBoot[3]; // 跳转指令，用于启动扇区
    uint8_t BS_oemName[8]; // OEM 名称，OEM 名称标识。可以通过 FAT 的实现设置为任何想要的值。通常这被标识为是什么系统格式化了该卷。
    uint16_t BPB_BytsPerSec; //512// 每扇区的字节数,这个值通常只接受以下值：512, 1024, 2048 or 4096 ,
    uint8_t BPB_SecPerClus; //8// 每簇的扇区数,每个分配单元的扇区数。这个值必须是大于 0 的 2 的整数次方。合法值如下，1,2,4,8,16,32,64 和 128。
    uint16_t BPB_RsvdSecCnt; //32// 从卷的第一个扇区开始的保留区域中的保留扇区数。
    uint8_t BPB_NumFATs; //2// FAT 表的个数,通常推荐是 2, 即使 1 也是可以用的。
    uint16_t BPB_rootEntCnt; //0// 根目录项数（对于 FAT32，这个值通常是 0）
    uint16_t BPB_totSec16; //0// 16 位总扇区数（对于 FAT32，这个值通常是 0）
    uint8_t BPB_media; // 媒体描述符
    uint16_t BPB_FATSz16; //0// 16 位 FAT 表扇区数（对于 FAT32，这个值通常是 0）
    uint16_t BPB_SecPerTrk; // 每磁道的扇区数
    uint16_t BPB_NumHeads; // 磁头数
    uint32_t BPB_HiddSec; // 隐藏扇区数
    uint32_t BPB_TotSec32; //131072// 32 位总扇区数
    uint32_t BPB_FATSz32; //128// 32 位 FAT 表扇区数
    uint16_t BPB_ExtFlags; // 扩展标志
    uint16_t BPB_FSVer; // 文件系统版本
    uint32_t BPB_RootClus; //2// 根目录簇号
    uint16_t BPB_FSInfo; // 文件系统信息扇区号
    uint16_t BPB_bkBootSec; // 备份引导扇区
    uint8_t BPB_reserved[12]; // 保留字段
    uint8_t BS_DrvNum; // 驱动器号
    uint8_t BS_Reserved1; // 保留字段
    uint8_t BS_BootSig; // 引导签名
    uint32_t BS_VolID; // 卷序列号
    uint8_t BS_VolLab[11]; // 卷标
    uint8_t BS_FileSysTye[8]; // 文件系统类型
    uint8_t  __padding_1[420]; // 填充字段，确保 BPB 结构的大小为 512 字节
    uint16_t Signature_word; // 签名字，通常是 0xAA55
} __attribute__((packed));

// 目录项 (短文件名) 32bytes
struct DirEntry {
    uint8_t DIR_Name[11]; // 文件名（8 字符）和扩展名（3 字符）
    uint8_t DIR_Attr; // 文件属性
    uint8_t DIR_NTRes; // 保留字段
    uint8_t DIR_CrtTimeTenth; // 创建时间的十分之一秒
    uint16_t DIR_CrtTime; // 创建时间
    uint16_t DIR_CrtDate; // 创建日期
    uint16_t DIR_LstAccDate; // 最后访问日期
    uint16_t DIR_FstClusHI; // 文件起始簇号的高 16 位
    uint16_t DIR_WrtTime; // 最后写入时间
    uint16_t DIR_WrtDate; // 最后写入日期
    uint16_t DIR_FstClusLO; // 文件起始簇号的低 16 位
    uint32_t DIR_FileSize; // 文件大小
} __attribute__((packed));

// 目录项的属性，即 DIR_Attr 字段
enum DirEntryAttributes {
    READ_ONLY       = 0x01, // 只读
    HIDDEN          = 0x02, // 隐藏
    SYSTEM          = 0x04, // 系统
    VOLUME_ID       = 0x08, // 卷标
    DIRECTORY       = 0x10, // 目录
    ARCHIVE         = 0x20, // 归档
    LONG_NAME       = 0x0F, // 长文件名
    LONG_NAME_MASK  = 0x3F, // 长文件名掩码
};

// 用于存储文件名及文件大小
struct FileInfo {
    uint8_t DIR_Name[11]; // 文件名（8 字符）和扩展名（3 字符）
    uint32_t DIR_FileSize; // 文件大小
};

// 用于存储一个目录文件中包含的所有文件的文件名及文件大小
struct FilesInfo {
    struct FileInfo *files;
    int size;
};

// 你应当在 fat.c 中实现以下函数 (fat_mount 已实现)
extern int fat_mount(const char *path);
extern int fat_open(const char *path);
extern int fat_close(int fd);
extern int fat_pread(int fd, void *buffer, int count, int offset);
extern struct FilesInfo* fat_readdir(const char *path);

#endif
