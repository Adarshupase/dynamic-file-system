#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <stdint.h>

typedef uint32_t u32;

#define INODE_BLOCKS 20
#define MAX_BUFFER_SIZE 1024
#define TOTAL_GROUPS 64
#define GROUP_SIZE 64

char* group_bitmap;
int bitmap_size;
FILE* disk_file;


typedef struct {
    u32 magic_number;
    u32 total_blocks;
    u32 inode_start;
    u32 inode_blocks;
    u32 group_info_block;
    char unused[4096 - 20];
} __attribute__((packed)) superblock;

typedef struct group_info {
    u32 group_no;
    u32 start_block;
    u32 end_block;
    u32 next_Block;
    u32 next_group;
} __attribute__((packed)) group_info;

typedef struct {
    u32 total_gbs;
    group_info* groups[TOTAL_GROUPS];
    char* group_bitmap;
    u32 First_unused_group;
} __attribute__((packed)) GROUP_BLOCK;

typedef struct {
    int inumber;
    int start_group;
    int total_blocks;
    int valid;
} inode;

typedef struct {
    inode inodes[1024];
} __attribute__((packed)) inode_Block;

typedef struct {
    union info {
        group_info ginfo;
        char data0[sizeof(group_info)];
    };
    char data[4096 - sizeof(union info)];
} __attribute__((packed)) Data_block;

typedef struct {
    inode_Block* ib[INODE_BLOCKS];
    superblock sb;
    GROUP_BLOCK gb;
    Data_block* dataBlocks;
} __attribute__((packed)) DISK;

typedef struct {
    char Buffer[MAX_BUFFER_SIZE];
    int BufferLength;
} Buffer_;


void update_bitmap(char* bitmap, int GroupNo);
int findGroup(GROUP_BLOCK gb, int blocks_needed);
int getInodeNumber(inode_Block* iNodeB);
void create_File(DISK* disk, inode_Block* inodeB, int fileSize, char* File_ct);
void update_File(DISK* disk, inode_Block* inodeB, int updateSize, char* File_ct, int inumber);
void read_File(int inumber, Buffer_ *buffer);
void create_FFS(const char* disk_filename);

void update_bitmap(char* bitmap, int GroupNo) {
    bitmap[GroupNo / 8] |= (1 << (GroupNo % 8)); // Mark as allocated
    fseek(disk_file, sizeof(superblock), SEEK_SET);
    fwrite(bitmap, bitmap_size, 1, disk_file);  // Write the entire bitmap at once
}

int findGroup(GROUP_BLOCK gb, int blocks_needed) {
    for (int i = 0; i < TOTAL_GROUPS; i++) {
        group_info* group = gb.groups[i];
        int total_Blocks_available = group->end_block - group->next_Block + 1;

        if (blocks_needed <= total_Blocks_available) {
            return i;
        }
    }
    return -1; // No suitable group found
}

int getInodeNumber(inode_Block* iNodeB) {
    srand(time(NULL));

    int randomNumber;
    do {
        randomNumber = rand() % 1024;
    } while (iNodeB->inodes[randomNumber].valid == 1);

    return randomNumber;
}

void create_File(DISK* disk, inode_Block* inodeB, int fileSize, char* File_ct) {
    Buffer_ buffer;
    int blocks_needed = (fileSize + MAX_BUFFER_SIZE - 1) / MAX_BUFFER_SIZE;
    int GroupNo = findGroup(disk->gb, blocks_needed);

    if (GroupNo == -1) {
        printf("No suitable group found\n");
        return;
    }

    group_info* group = disk->gb.groups[GroupNo];
    int total_Blocks_available = group->end_block - group->next_Block + 1;

    if (blocks_needed > total_Blocks_available) {
        printf("Not enough space in the group\n");
        return;
    }

    int inumber = getInodeNumber(inodeB);

    inodeB->inodes[inumber].inumber = inumber;
    inodeB->inodes[inumber].start_group = GroupNo;
    inodeB->inodes[inumber].total_blocks = blocks_needed;
    inodeB->inodes[inumber].valid = 1;

    // Update the bitmap
    update_bitmap(disk->gb.group_bitmap, GroupNo);

    // Update the group info for the remaining blocks
    int remaining_blocks = total_Blocks_available - blocks_needed;
    if (remaining_blocks > 0) {
        group_info* next_group = disk->gb.groups[GroupNo + 1]; 
        next_group->start_block = group->next_Block + blocks_needed;
        next_group->next_Block = next_group->start_block;
        group->end_block = group->next_Block + blocks_needed - 1;
    } else {
        group->next_group = -1;
    }

    group->next_Block = -1; // Indicates no blocks are available

    // Copy file content into the data blocks
    int remaining_size = fileSize;
    int data_offset = 0;

    for (int i = 0; i < blocks_needed; i++) {
        int block_index = GroupNo * GROUP_SIZE + i;
        int copy_size = (remaining_size > MAX_BUFFER_SIZE) ? MAX_BUFFER_SIZE : remaining_size;
        fseek(disk_file, sizeof(superblock) + bitmap_size + sizeof(GROUP_BLOCK) + block_index * sizeof(Data_block), SEEK_SET);
        fwrite(File_ct + data_offset, 1, copy_size, disk_file);
        remaining_size -= copy_size;
        data_offset += copy_size;
    }

    printf("File created with inode number %d, starting at block %d\n", inodeB->inodes[inumber].inumber, inodeB->inodes[inumber].start_group);
}

void update_File(DISK* disk, inode_Block* inodeB, int updateSize, char* File_ct, int inumber) {
    Buffer_ buffer;
    int blocks_needed = (updateSize + MAX_BUFFER_SIZE - 1) / MAX_BUFFER_SIZE;
    int GroupNo = findGroup(disk->gb, blocks_needed);

    if (GroupNo == -1) {
        printf("No suitable group found\n");
        return;
    }

    group_info* group = disk->gb.groups[GroupNo];
    int total_Blocks_available = group->end_block - group->next_Block + 1;

    if (blocks_needed > total_Blocks_available) {
        printf("Not enough space in the group\n");
        return;
    }

    inodeB->inodes[inumber].total_blocks += blocks_needed;
    inodeB->inodes[inumber].start_group = GroupNo;

    update_bitmap(disk->gb.group_bitmap, GroupNo);

    // Update the group info for the remaining blocks
    int remaining_blocks = total_Blocks_available - blocks_needed;
    if (remaining_blocks > 0) {
        group_info* next_group = disk->gb.groups[GroupNo + 1];
        next_group->start_block = group->next_Block + blocks_needed;
        next_group->next_Block = next_group->start_block;
        group->end_block = next_group->start_block + blocks_needed - 1;
    } else {
        group->next_group = -1;
    }

    group->next_Block = -1; // Indicates no blocks are available

    // Copy file content into the data blocks
    int remaining_size = updateSize;
    int data_offset = 0;

    for (int i = 0; i < blocks_needed; i++) {
        int block_index = GroupNo * GROUP_SIZE + i;
        int copy_size = (remaining_size > MAX_BUFFER_SIZE) ? MAX_BUFFER_SIZE : remaining_size;
        fseek(disk_file, sizeof(superblock) + bitmap_size + sizeof(GROUP_BLOCK) + block_index * sizeof(Data_block), SEEK_SET);
        fwrite(File_ct + data_offset, 1, copy_size, disk_file);
        remaining_size -= copy_size;
        data_offset += copy_size;
    }

    printf("File updated with inode number %d, starting at block %d\n", inodeB->inodes[inumber].inumber, inodeB->inodes[inumber].start_group);
}

void read_File(int inumber, Buffer_ *buffer) {
    DISK disk;
    inode_Block inodeB;
    fseek(disk_file, sizeof(superblock) + sizeof(GROUP_BLOCK) + bitmap_size, SEEK_SET);
    fread(&inodeB, sizeof(inode_Block), 1, disk_file);

    inode* inode = &inodeB.inodes[inumber];
    if (!inode->valid) {
        printf("Invalid inode number\n");
        return;
    }

    buffer->BufferLength = inode->total_blocks * MAX_BUFFER_SIZE;
    fseek(disk_file, sizeof(superblock) + sizeof(GROUP_BLOCK) + sizeof(inode_Block) + inode->start_group * sizeof(Data_block), SEEK_SET);
    fread(buffer->Buffer, 1, buffer->BufferLength, disk_file);
}

void create_FFS(const char* disk_filename) {
    disk_file = fopen(disk_filename, "wb+");
    if (!disk_file) {
        printf("Failed to create disk image\n");
        exit(1);
    }

    superblock sb = {0};
    sb.magic_number = 0xDEADBEEF;
    sb.total_blocks = 1024;
    sb.inode_start = sizeof(superblock) + sizeof(GROUP_BLOCK);
    sb.inode_blocks = INODE_BLOCKS;
    sb.group_info_block = sizeof(superblock);

    fwrite(&sb, sizeof(superblock), 1, disk_file);


    GROUP_BLOCK gb = {0};
    group_info* groups[TOTAL_GROUPS];
    bitmap_size = (TOTAL_GROUPS + 7) / 8;
    group_bitmap = calloc(bitmap_size, 1);

    for (int i = 0; i < TOTAL_GROUPS; i++) {
        groups[i] = calloc(1, sizeof(group_info));
        groups[i]->group_no = i;
        groups[i]->start_block = i * GROUP_SIZE;
        groups[i]->end_block = groups[i]->start_block + GROUP_SIZE - 1;
        groups[i]->next_Block = groups[i]->start_block;
        groups[i]->next_group = (i == TOTAL_GROUPS - 1) ? -1 : i + 1;
        gb.groups[i] = groups[i];
    }

    gb.group_bitmap = group_bitmap;
    gb.First_unused_group = 0;

    fwrite(&gb, sizeof(GROUP_BLOCK), 1, disk_file);
    fwrite(group_bitmap, bitmap_size, 1, disk_file);

    inode_Block inodeB = {0};
    fwrite(&inodeB, sizeof(inode_Block), 1, disk_file);


    Data_block* dataBlocks = calloc(1024, sizeof(Data_block));
    fwrite(dataBlocks, 1024 * sizeof(Data_block), 1, disk_file);

    free(dataBlocks);
    fclose(disk_file);

    printf("Disk image created successfully\n");
}

int main() {
    const char* disk_filename = "disk.img";
    create_FFS(disk_filename);

    // Re-open disk file for further operations
    disk_file = fopen(disk_filename, "rb+");
    if (!disk_file) {
        printf("Failed to open disk image\n");
        return 1;
    }

    DISK disk;
    inode_Block inodeB;

    fseek(disk_file, sizeof(superblock) + bitmap_size + sizeof(GROUP_BLOCK), SEEK_SET);
    fread(&inodeB, sizeof(inode_Block), 1, disk_file);

    // Example: Create a file
    char file_content[2048];
    memset(file_content, 'A', 2048);
    create_File(&disk, &inodeB, 2048, file_content);

    // Example: Read a file
    Buffer_ buffer;
    read_File(0, &buffer);
    printf("File content: %.*s\n", buffer.BufferLength, buffer.Buffer);

    // Example: Update a file
    char update_content[1024];
    memset(update_content, 'B', 1024);
    update_File(&disk, &inodeB, 1024, update_content, 0);

    // Clean up
    free(group_bitmap);
    fclose(disk_file);

    return 0;
}
