#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"


#define EOC 0xFFFF

/* TODO: Phase 1 */
struct superBlock {
    uint8_t name[FS_FILENAME_LEN];
    uint16_t blkCount;
    uint16_t rootCount;
    uint16_t dataBlkStartIndex;
    uint16_t dataBlkCount;
    uint8_t fatCount;
    uint8_t unused[4079];
} __attribute__((__packed__));

struct rootDirBlock {
    uint8_t name[FS_FILENAME_LEN];
    uint32_t fileSize;
    uint16_t firstData;
    uint8_t unused[10];
} __attribute__((__packed__));

struct fatBlock {
    uint16_t fat;
} __attribute__((__packed__));

struct file {
    char   name[FS_FILENAME_LEN];
    int index;
    size_t offset;
    struct file *next;
};

static struct file *initialFile;
struct rootDirBlock *rootBlk;
struct superBlock superBlk;
struct fatBlock *fatBlk;
struct file *currentFile;
int fileUsed = 0;


int fs_mount(const char *diskname)
{
    /* TODO: Phase 1 */
    //if the file input valid
    int blkSize;

    if(block_disk_open(diskname) != 0){
        return -1;
    }
    if(block_disk_count() != 0){
        return -1;
    }
    if(block_read(blkSize, &superBlk) != 0 ){
        return -1;
    }

    int blkNeed = (int)superBlk.dataBlkCount;

    int dataBlkCount = (int) superBlk.dataBlkCount * BLOCK_SIZE;

    rootBlk = malloc(BLOCK_SIZE * sizeof(struct rootDirBlock));

    fatBlk = malloc(dataBlkCount * sizeof(struct fatBlock));

    for(int i = 0; i < blkNeed; i++){
        block_read(i+1, fatBlk + (i * BLOCK_SIZE));
    }
    if(block_read(superBlk.rootCount, &superBlk) != 0 ){
        return -1;
    }

    return 0;
}

int fs_umount(void)
{
    /* TODO: Phase 1 */
    int fat = (int)fatBlk->fat;

    if (block_disk_count() != 0) {
        return -1;
    }
    if(block_write(0, &superBlk) != 0) {
        printf("failure to write to block \n");
        return -1;
    }
    for (int i = 0; i < superBlk.fatCount; i++) {
        block_write(i + 1, (void*)(fat + (i * BLOCK_SIZE)));
    }
    if(	block_write(superBlk.rootCount, rootBlk) != 0){
        return -1;
    }

    free(fatBlk);
    free(rootBlk);
    memset(&superBlk, 0, BLOCK_SIZE);

    if (block_disk_close() < 0) {
        return -1;
    }

    return 0;
}

int fs_info(void)
{
    /* TODO: Phase 1 */
    int blkCount = (int) superBlk.blkCount;
    int fatCount = (int) superBlk.fatCount;
    int rootIndex = fatCount + 1;
    int dataIndex = rootIndex + 1;
    int dataCount = (int) superBlk.dataBlkCount;

    int fatFree, rootFree;

    for (int i = 0; i < dataCount; i++) {
        if (fatBlk[i].fat == 0) {
            fatFree += 1;
        }
    }

    for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
        if (rootBlk[i].name[0] == '\0') {
            rootFree++;
        }
    }

    printf("FS Info:\n");
    printf("total_blk_count=%d\n", blkCount);
    printf("fat_blk_count=%d\n", fatCount);
    printf("rdir_blk=%d\n", rootIndex);
    printf("data_blk=%d\n", dataIndex);
    printf("data_blk_count=%d\n", dataCount);
    printf("fat_free_ratio=%d/%d\n", fatFree, dataCount);
    printf("rdir_free_ratio=%d/%d\n", rootFree, FS_FILE_MAX_COUNT);
    return 0;
}

//check if file exist
int file_check(const char *filename){
    for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
        if (!strcmp((char*)rootBlk[i].name, filename) && (!strcmp(rootBlk[i].name, ""))) {
            printf("File exists");
            return -1;
        }
    }
    return 0;
}
//fine next available fat block
//not completed
size_t find_next_fat(){
    int location = 0;
    return location;
}

//fine file location
//not completed
size_t find_file_location(const char *filename){
    size_t location = 0;
    for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
        if (!strcmp((char*)rootBlk[i].name, filename)) {
            location = i;
            break;
        }else if(i = FS_FILE_MAX_COUNT){
            return -1;
        }
    }
    return location;
}

int fs_create(const char *filename)
{
    /* TODO: Phase 2 */
    //check file name
    if(file_check(filename) != 0)
        return -1;

    //check next available space
    for (int i = 0; i < FS_FILE_MAX_COUNT; i++){
        if(i == FS_FILE_MAX_COUNT){
            printf("reach max");
            return -1;
        }
        if (rootBlk[i].name == NULL) {
            strcpy(rootBlk[i].name, filename);
            rootBlk[i].fileSize  = 0;
            rootBlk[i].firstData = find_next_fat();
            fatBlk[rootBlk[i].firstData].fat = EOC;
            break;
        }
    }
    return 0;
}

int fs_delete(const char *filename)
{
    /* TODO: Phase 2 */
    if (file_check(filename) == 0){
        printf("file does not exist");
        return -1;
    }

    size_t file_location = find_file_location(filename);

    currentFile = initialFile;

    for(int i = 0; i < 32; i++){
        if(!strcmp(currentFile->name, filename)){
            printf("file open, fail to delete");
            return -1;
        }else{
            currentFile = currentFile->next;
        }
    }

    uint16_t temp = fatBlk[(int)rootBlk[file_location].firstData].fat;

    while(fatBlk[temp].fat != EOC){
        temp = fatBlk[(int)rootBlk[file_location].firstData].fat;
        fatBlk[temp].fat = 0;
    }


    strcpy(rootBlk[file_location].name, "");
    rootBlk[file_location].fileSize = 0;

    return 0;
}

int fs_ls(void)
{
    /* TODO: Phase 2 */
    if (block_disk_count() < 0) {
        return -1;
    }
    printf("FS Ls:\n");
    for(int i = 0; i < FS_FILE_MAX_COUNT; i++) {
        if(!strcmp(rootBlk[i].name, "")){
            printf("file: %s, size: %d, ", rootBlk[i].name, rootBlk[i].fileSize);
            printf("data_blk: %d\n", rootBlk[i].firstData);
        }
    }
    return 0;

}

int fs_open(const char *filename)
{
    /* TODO: Phase 3 */
    //check whether the file already exist
    for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
        if (!strcmp((char*)rootBlk[i].name, filename)|| !(strcmp((char*)rootBlk[i].name, ""))) {
            printf("File does not exist");
            return -1;
        }
    }

    //if file reaches max
    if (fileUsed +1 >= 32) {
        printf("Files reach max");
        return -1;
    }

    currentFile = initialFile;

    for(int i = 0; i <32; i++) {
        if (!strcmp(currentFile->name, "")) {
            fileUsed = 0;
            break;
        } else {
            currentFile = currentFile->next;
            fileUsed++;
        }
    }

    strcpy(currentFile->name, filename);
    currentFile->offset = 0;
    currentFile->index = fileUsed;

    return currentFile->index;
}

int fs_close(int fd)
{
    /* TODO: Phase 3 */
    if (fd >= 32){
        printf("invalid fd");
        return -1;
    }

    currentFile = initialFile;

    for(int i = 0; i < 32; i++){
        if(currentFile->index == fd){
            break;
        }else{
            currentFile = currentFile->next;
        }
    }

    if(!strcmp(currentFile->name, "")){
        printf("file %d does not exist", fd);
        return -1;
    }else{
        currentFile = currentFile->next;
        fileUsed--;
        return 0;
    }
}

int fs_stat(int fd)
{
    /* TODO: Phase 3 */
    if (fd >= 32){
        printf("invalid fd");
        return -1;
    }

    currentFile = initialFile;

    for(int i = 0; i < 32; i++){
        if(currentFile->index == fd){
            break;
        }else{
            currentFile = currentFile->next;
        }
    }

    if(!strcmp(currentFile->name, "")){
        printf("file %d does not exist, descriptor invalid", fd);
        return -1;
    }

    for (int i = 0; i < 32; i++) {
        if (!strcmp((char*)rootBlk[i].name, currentFile->name)) {
            return (int)rootBlk[i].fileSize;
        }
    }
    return -1;
}

int fs_lseek(int fd, size_t offset)
{
    /* TODO: Phase 3 */
    if (fd >= 32){
        printf("invalid fd");
        return -1;
    }

    if((int)offset < 0 || (int)offset > fs_stat(fd)){
        printf("offset error");
        return -1;
    }

    currentFile = initialFile;

    for(int i = 0; i < 32; i++){
        if(currentFile->index == fd){
            break;
        }else{
            currentFile = currentFile->next;
        }
    }

    if(!strcmp(currentFile->name, "")){
        printf("file %d does not exist, lseek failure", fd);
        return -1;
    }

    currentFile->offset = offset;
    return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
    /* TODO: Phase 4 */
}

int fs_read(int fd, void *buf, size_t count)
{
    /* TODO: Phase 4 */
}

