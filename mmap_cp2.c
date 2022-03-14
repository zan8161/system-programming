#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[])
{

    int inputFd, outputFd;
    char *inputPtr, *outputPtr;
    ssize_t numIn, numOut;
    ssize_t fileSize = 0, blocksize = 0;
    // char buffer[BUF_SIZE];

    if (argc != 3)
    {
        fprintf("wrong command!", stderr);
        exit(0);
    }
    //只可讀取模式打開
    inputFd = open(argv[1], O_RDONLY);
    if (inputFd == -1)
    {
        perror("cannot open the file for read");
        exit(1);
    }

    // open後可對該檔案＊＊『可讀可寫』＊＊（因為mmap的需求），如果沒有該檔案，就建立該檔案。如果要建立，設定該檔案的屬性為owner可讀可寫
    outputFd = open(argv[2], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (outputFd == -1)
    {
        perror("canot open the file for write");
        exit(1);
    }

    // lseek的回傳是該檔案的絕對位址，因此lseek(0, seek_end)相當於檔案大小
    // linux有專門讀取檔案大小的函數，但我習慣用這一個
    fileSize = lseek(inputFd, 0, SEEK_END);
    printf("file size = %ld\n", fileSize);
    // 用lseek回到檔案初始位置
    lseek(inputFd, 0, SEEK_SET);

    // NULL，不指定映射到記憶體的哪個位置。通常不指定
    // filesize，將檔案中多少內容映射到記憶體
    // prot_read，只會對該段記憶體做讀取
    // MAP_SHARED，對mmap出的記憶體的所有修改讓整個系統裡的人都看到。因此底藏的檔案會跟著變更
    // inputFd從哪個檔案映射進來
    // 0, 映射的起點為 0
    inputPtr = mmap(NULL, fileSize, PROT_READ, MAP_SHARED, inputFd, 0); //🐶 🐱 🐭 🐹 🐰 🦊
    perror("mmap");
    printf("inputPtr = %p\n", inputPtr);
    // assert(madvise(inputPtr, fileSize, MADV_SEQUENTIAL|MADV_WILLNEED|MADV_HUGEPAGE)==0);

    // ftruncate的名字是：縮小
    //實際上是設定檔案大小
    ftruncate(outputFd, fileSize);                                         //🐶 🐱 🐭 🐹 🐰 🦊
    outputPtr = mmap(NULL, fileSize, PROT_WRITE, MAP_SHARED, outputFd, 0); //🐶 🐱 🐭 🐹 🐰 🦊
    perror("mmap, output");
    printf("outputPtr = %p\n", outputPtr);
    // madvise(inputPtr, fileSize, MADV_SEQUENTIAL|MADV_WILLNEED|MADV_HUGEPAGE);

    printf("memory copy\n");
    // 用於確認洞以及資料的大小及位置
    off_t data_off = 0, hole_off = 0, cur_off = 0;
    // 用於測量時間
    time_t timer1, timer2;
    timer1 = time(NULL);
    while (1)
    {
        //尋找資料的位置
        cur_off = lseek(inputFd, cur_off, SEEK_DATA);
        data_off = cur_off;
        cur_off = lseek(inputFd, cur_off, SEEK_HOLE);
        hole_off = cur_off;
        // 若洞在前，資料在後，則需要特別處理
        if (data_off > hole_off)
        {
            continue;
        }
        blockSize = hole_off - data_off;
        // 檔案複製
        memcpy(outputPtr + data_off, inputPtr + data_off, blockSize);
        // 用lseek到每個hole的尾端
        lseek(outputFd, cur_off, SEEK_SET);
        // 確認是否至檔案結尾
        if (lseek(outputFd, 0, SEEK_CUR) == fileSize)
            break;
    }
    timer2 = time(NULL);

    printf("time(memcpy) = %ld sec \n", timer2 - timer1);

    assert(munmap(inputPtr, fileSize) == 0);
    assert(munmap(outputPtr, fileSize) == 0);

    assert(close(inputFd) == 0);
    assert(close(outputFd) == 0);

    return (EXIT_SUCCESS);
}