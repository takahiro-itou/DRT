
#if 0
g++   -o  DRT.exe  DRT.cpp
exit  0
#endif

#include    <fcntl.h>
#include    <memory.h>
#include    <sys/types.h>
#include    <sys/mman.h>
#include    <unistd.h>

#include    <stdio.h>
#include    <stdlib.h>

#include    "Blocks.h"

//----------------------------------------------------------------
/**   ファイルを閉じる。
**
**  @param [in,out] fi    ファイル情報。
**  @return     void.
**/

void
closeGIFFile(
        FileInfo  *  const  fi,
        const  int          bTrunc)
{
    munmap(fi->ptrBuf, fi->cbSize);

    if ( fi->fd != -1 ) {
        if ( bTrunc ) {
            if ( ftruncate(fi->fd, bTrunc) == -1 ) {
                perror("ftruncate");
            }
        }
        close (fi->fd);
    }

    fi->fd      = -1;
    fi->cbSize  = 0;
    fi->ptrBuf  = NULL;
}


//----------------------------------------------------------------
/**   ファイルを書き込みモードで開く.
**
**  @param [in] fileName    ファイル名。
**  @param [in] fileSize    予約するバッファサイズ。
**  @param[out] fi          ファイル情報。
**  @return     書き込み用のバッファの先頭アドレス。
**      エラーが発生した場合は NULL を返す。
**/

LpWriteBuf  const
createGIFfile(
        const  char  *      fileName,
        const  long         fileSize,
        FileInfo  *  const  fi)
{
    int         fd;
    struct stat stbuf;

    if ( (fd = open(fileName, O_RDWR | O_CREAT, 0666)) == -1 ) {
        perror("open");
        return ( NULL );
    }

    void *  ptr     = NULL;
    ftruncate(fd, fileSize);

    //  マッピングを作成する。      //
    ptr = mmap(0, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if ( ptr == MAP_FAILED ) {
        perror("mmap");
        goto    label_finalize_file_if_error;
    }

    fi->fd      = fd;
    fi->ptrBuf  = static_cast<LpWriteBuf>(ptr);
    fi->cbSize  = fileSize;

    //  正常終了。  //
    return ( fi->ptrBuf );

    //  ファイルを開いた後でエラーになった場合の例外処理。  //
label_finalize_file_if_error:
    close(fd);
    return ( NULL );
}

//----------------------------------------------------------------
/**   ファイルサイズを取得する。
**
**  @param [in] fileName    ファイル名。
**  @return     ファイルサイズ。
**      エラーが発生した場合はゼロを返す。
**/

size_t
getFileLength(
        const  char  *  fileName)
{
    struct stat stbuf;

    if ( stat(fileName, &stbuf) == -1 ) {
        return ( 0 );
    }

    return ( static_cast<size_t>(stbuf.st_size) );
}

//----------------------------------------------------------------
/**   ファイルを読み込みモードで開く。
**
**  @param [in] fileName    ファイル名。
**  @param[out] fi          ファイル情報。
**  @return     読み込んだデータの先頭アドレス。
**      エラーが発生した場合は NULL を返す。
**/

LpcReadBuf  const
openGIFfile(
        const  char  *      fileName,
        FileInfo  *  const  fi)
{
    int         fd;
    struct stat stbuf;

    if ( (fd = open(fileName, O_RDONLY)) == -1 ) {
        return ( NULL );
    }

    long    cbSize  = 0;
    void *  ptr     = NULL;

    //  ファイルサイズを取得する。  //
    if ( fstat(fd, &stbuf) == -1 ) {
        goto    label_finalize_file_if_error;
    }

    //  マッピングを作成する。      //
    cbSize  = stbuf.st_size;
    ptr     = mmap(0, cbSize, PROT_READ, MAP_SHARED, fd, 0);
    if ( ptr == MAP_FAILED ) {
        goto    label_finalize_file_if_error;
    }

    fi->fd      = -1;
    fi->ptrBuf  = static_cast<LpWriteBuf>(ptr);
    fi->cbSize  = cbSize;

    //  ファイルディスクリプタはここで閉じて構わない。  //
    close(fd);

    //  正常終了。  //
    return ( fi->ptrBuf );

    //  ファイルを開いた後でエラーになった場合の例外処理。  //
label_finalize_file_if_error:
    close(fd);
    return ( NULL );
}


int  main(int argc,  char * argv[])
{
    if ( argc <= 2 ) {
        fprintf(stderr, "Usage: %s (OUT) (IN...)\n", argv[0]);
        exit ( 1 );
    }

    FileInfo    fiOut;


    //  各入力ファイルのサイズを取得する。      //
    size_t  cbTotal = 0;
    size_t  cbReqs  = 0;
    size_t  cbWrite = 0;

    for ( int i = 2; i < argc; ++ i ) {
        cbTotal += getFileLength(argv[i]);
    }
    fprintf(stderr, "Total Input Size: %ld Bytes.\n", cbTotal);

    //  先頭のファイル名を出力ファイルにする。  //
    cbReqs  = cbTotal + (256 * 3 + 8) * (argc - 2);
    fprintf(stderr, "Estimate Write Size: %ld Bytes.\n", cbReqs);

    LpWriteBuf  pW  = createGIFfile(argv[1], cbReqs, &fiOut);
    if ( pW == NULL ) {
        exit ( 1 );
    }

    for ( int i = 2; i < argc; ++ i ) {
        FileInfo    fiIn;
        LpcReadBuf  pR  = openGIFfile(argv[i], &fiIn);

        if ( i == 2 ) {
            memcpy(pW + cbWrite, pR, (fiIn.cbSize) - 1);
            cbWrite += (fiIn.cbSize - 1);
        } else {
            memcpy(pW + cbWrite, pR + 19, (fiIn.cbSize) - 1);
            cbWrite += (fiIn.cbSize - 20);
        }

        closeGIFFile(&fiIn, 0);
    }

    fprintf(stderr, "Total Write Size: %ld Bytes.\n", cbWrite);
    closeGIFFile(&fiOut, cbWrite);

    return ( 0 );
}
