
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


//----------------------------------------------------------------
/**   拡張ブロックを読み込む。
**
**  @param [in] ptrBuf
**  @param [in] fInfo
**  @param[out] bkInfo
**  @return     読み込んだバイト数。
**/

size_t
readExtensionBlock(
        const  LpcReadBuf   ptrBuf,
        const  FileInfo *   fInfo,
        BlockInfo  *        bkInfo)
{
}


//----------------------------------------------------------------
/**   ファイルヘッダを読み込む。
**
**  @param [in] ptrBuf
**  @param [in] fInfo
**  @param[out] exBlock
**  @return     読み込んだバイト数。
**/

size_t
readFileHeader(
        const  LpcReadBuf   ptrBuf,
        const  FileInfo  *  fInfo,
        FileHeader  *       gifHead)
{
    memcpy(gifHead->signature, ptrBuf + 0, 3);
    memcpy(gifHead->version,   ptrBuf + 3, 3);

    if ( (ptrBuf[0] == 'G') && (ptrBuf[1] == 'I') && (ptrBuf[2] == 'F') )
    {
        //  OK  //
    } else {
        fprintf(stderr, "Invalid File Header.\n");
        return ( 0 );
    }

    if ( (ptrBuf[3] == '8') && (ptrBuf[4] == '9') && (ptrBuf[5] == 'a') )
    {
        //  OK  //
    } else {
        fprintf(stderr, "Unsupported Version.\n");
        return ( 0 );
    }
    gifHead->screenWidth    = *((unsigned short *)(ptrBuf + 6));
    gifHead->screenHeight   = *((unsigned short *)(ptrBuf + 8));
    unsigned char   flg     = *((ptrBuf + 10));

    gifHead->flgGCR         = (flg & 0x80);
    gifHead->colorResol     = (flg >> 4) & 0x07;
    gifHead->flgSort        = (flg & 0x08);
    gifHead->sizeGCR        = (flg & 0x07);

    gifHead->bgIndex        = *((ptrBuf + 11));
    gifHead->aspectRatio    = *((ptrBuf + 12));

    if ( gifHead->flgGCR ) {
        gifHead->gColorSize = 0;
    } else {
        //  gifHead->gColorSize = 1 << (gifHead->sizeGCR + 1);
        gifHead->gColorSize = 2 << (gifHead->sizeGCR);
    }

    const   size_t  gpSize  = (gifHead->gColorSize) * 3;
    memcpy(gifHead->gColorTable, ptrBuf + 13, gpSize * 3);

    return ( gpSize + 13 );
}


//----------------------------------------------------------------
/**   次のブロックを読み込む。
**
**  @param [in] ptrBuf
**  @param [in] fInfo
**  @param[out] bkInfo
**  @return     読み込んだバイト数。
**/

size_t
readNextBlock(
        const  LpcReadBuf   ptrBuf,
        const  FileInfo *   fInfo,
        BlockInfo  *        bkInfo)
{
    const   UByte8  ubType  = *(ptrBuf);
    bkInfo->ubType  = ubType;

    if ( ubType == 0x2C ) {
    } else if ( ubType == 0x21 ) {
    }
}


//----------------------------------------------------------------
/**   エントリポイント。
**
**/

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
    size_t  cbRead  = 0;

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

    //  先頭のファイルを開く。  //
    {
        FileInfo    fiIn;
        FileHeader  gifHead;

        LpcReadBuf  pR  = openGIFfile(argv[2], &fiIn);
        cbRead  = readFileHeader(pR, &fiIn, &gifHead);

        memcpy(pW + cbWrite, pR, fiIn.cbSize - 1);
        cbWrite += (fiIn.cbSize - 1);

        closeGIFFile(&fiIn, 0);
    }

    for ( int i = 3; i < argc; ++ i ) {
        FileInfo    fiIn;
        FileHeader  gifHead;

        LpcReadBuf  pR  = openGIFfile(argv[i], &fiIn);
        cbRead  = readFileHeader(pR, &fiIn, &gifHead);

        memcpy(pW + cbWrite, pR + cbRead, (fiIn.cbSize) - cbRead - 1);
        cbWrite += (fiIn.cbSize - cbRead - 1);

        closeGIFFile(&fiIn, 0);
    }

    fprintf(stderr, "Total Write Size: %ld Bytes.\n", cbWrite);
    closeGIFFile(&fiOut, cbWrite);

    return ( 0 );
}
