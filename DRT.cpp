
#if 0
g++   -o  DRT.exe  -D_DEBUG  DRT.cpp
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
    bkInfo->ubType  = ptrBuf[0];
    bkInfo->exType  = ptrBuf[1];
    bkInfo->blkOffs = ptrBuf - (fInfo->ptrBuf);
    bkInfo->cbTotal = 0;
    bkInfo->ptrAddr = ptrBuf;

    LpcReadBuf  pR  = ptrBuf + 2;

    for ( ;; ) {
        size_t  cbSize  = *(pR ++ );
        if ( cbSize == 0 ) { break; }
        pR  += cbSize;
    }

    return ( bkInfo->cbTotal = (pR - ptrBuf) );
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

    const   UByte8  flg     = *((ptrBuf + 10));
    gifHead->flgGCR         = (flg & 0x80);
    gifHead->colorResol     = (flg >> 4) & 0x07;
    gifHead->flgSort        = (flg & 0x08);
    gifHead->sizeGCR        = (flg & 0x07);

    gifHead->bgIndex        = *((ptrBuf + 11));
    gifHead->aspectRatio    = *((ptrBuf + 12));

    if ( gifHead->flgGCR ) {
        //  gifHead->gColorSize = 1 << (gifHead->sizeGCR + 1);
        gifHead->gColorSize = 2 << (gifHead->sizeGCR);
    } else {
        gifHead->gColorSize = 0;
    }

    const   size_t  gpSize  = (gifHead->gColorSize) * 3;
    memcpy(gifHead->gColorTable, ptrBuf + 13, gpSize * 3);

    return ( gpSize + 13 );
}


//----------------------------------------------------------------
/**   イメージブロックを読み込む。
**
**  @param [in] ptrBuf
**  @param [in] fInfo
**  @param[out] bkInfo
**  @return     読み込んだバイト数。
**/

size_t
readImageBlock(
        const  LpcReadBuf   ptrBuf,
        const  FileInfo *   fInfo,
        BlockInfo  *        bkInfo,
        ImageBlock  *       imgBlk)
{
    imgBlk->imgSep  = ptrBuf[0];
    imgBlk->blkOffs = ptrBuf - (fInfo->ptrBuf);
    imgBlk->cbTotal = 0;
    imgBlk->ptrAddr = ptrBuf;

    imgBlk->imgLeft = *((unsigned short *)(ptrBuf + 1));
    imgBlk->imgTop  = *((unsigned short *)(ptrBuf + 3));
    imgBlk->imgW    = *((unsigned short *)(ptrBuf + 5));
    imgBlk->imgH    = *((unsigned short *)(ptrBuf + 7));

    const   UByte8  flg     = *((ptrBuf + 9));
    imgBlk->packed  = flg;
    imgBlk->flgLCT  = (flg & 0x80);
    imgBlk->flgIntr = (flg & 0x40);
    imgBlk->flgSort = (flg & 0x20);
    imgBlk->pfRsrv  = (flg >> 3) & 0x03;
    imgBlk->sizeLCT = (flg & 0x07);

    if ( imgBlk->flgLCT ) {
        //  imgBlk->lColorSize  = 1 << (imgBlk->sizeLCT + 1);
        imgBlk->lColorSize  = 2 << (imgBlk->sizeLCT);
    } else {
        imgBlk->lColorSize  = 0;
    }

    const   size_t  lpSize  = (imgBlk->lColorSize * 3);
    memcpy(imgBlk->lColorTable, ptrBuf + 10, lpSize);
    imgBlk->minCode = *(ptrBuf + 10 + lpSize);

    LpcReadBuf  pR  = ptrBuf + 11 + lpSize;
    imgBlk->ptrImgs = pR;

    for ( ;; ) {
        const   size_t  cbSize  = *(pR ++);
        if ( cbSize == 0 ) { break; }
        pR  += cbSize;
    }

    imgBlk->cbTotal = (pR - ptrBuf);
    imgBlk->cbImgs  = (pR - (imgBlk->ptrImgs));

#if defined( _DEBUG )
    fprintf(stderr,
            "#DEBUG : Image: Total = %ld, CT = %ld, Imgs = %ld\n",
            imgBlk->cbTotal, imgBlk->lColorSize, imgBlk->cbImgs);
#endif

    bkInfo->ubType  = imgBlk->imgSep;
    bkInfo->exType  = 0;
    bkInfo->blkOffs = imgBlk->blkOffs;
    bkInfo->cbTotal = imgBlk->cbTotal;
    bkInfo->ptrAddr = imgBlk->ptrAddr;

    return ( imgBlk->cbTotal );
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

    size_t  retVal  = 0;

    if ( ubType == 0x3B ) {
        //  フッター。  //
        bkInfo->exType  = 0;
        bkInfo->blkOffs = ptrBuf - (fInfo->ptrBuf);
        bkInfo->cbTotal = 1;
        bkInfo->ptrAddr = ptrBuf;

        return ( 1 );
    } else if ( ubType == 0x2C ) {
        retVal  = readImageBlock(
                        ptrBuf, fInfo, bkInfo,
                        &(bkInfo->imgBlk) );
    } else if ( ubType == 0x21 ) {
        retVal  = readExtensionBlock(
                        ptrBuf, fInfo, bkInfo);
    }

    return ( retVal );
}


//----------------------------------------------------------------
/**   イメージブロックを書き込む。
**
**  @param [in] fInfo
**  @param [in] gifHead
**  @param [in] imgBlk
**  @param[out] ptrBuf
**  @return     書き込んだバイト数。
**/

size_t
writeImageBlock(
        const   FileInfo  *     fInfo,
        const   FileHeader  *   gifHead,
        const   ImageBlock  *   imgBlk,
        LpWriteBuf  const       ptrBuf)
{
    //  ローカルカラーテーブルの有無を確認し、  //
    //  なければ、グローバルからコピーする。    //
    if ( imgBlk->flgLCT ) {
        memcpy(ptrBuf, imgBlk->ptrAddr, imgBlk->cbTotal);
        return ( imgBlk->cbTotal );
    }

    memcpy(ptrBuf, imgBlk->ptrAddr,  10);
    ptrBuf[9]   = (imgBlk->flgIntr)
            | (imgBlk->flgSort)
            | (imgBlk->pfRsrv)
            | ((gifHead->sizeGCR) & 0x07) | 0x80;
    const   size_t  gpSize  = (gifHead->gColorSize) * 3;
    memcpy(ptrBuf + 10, gifHead->gColorTable, gpSize);

    LpWriteBuf  pW  = ptrBuf + 10 + (gpSize);
    pW[1]   = ptrBuf[10];
    pW      += 2;

    memcpy(pW, imgBlk->ptrImgs, imgBlk->cbImgs);

    return ( pW - ptrBuf );
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
        pR  +=  cbRead;

        while ( cbRead > 2 ) {
            BlockInfo   bkInfo;

            cbRead  = readNextBlock(pR, &fiIn, &bkInfo);
#if defined( _DEBUG )
            fprintf(stderr,
                    "#DEBUG : Read: "
                    "%ld bytes, Type = %02x %02x, Offs = %ld\n",
                    cbRead, bkInfo.ubType, bkInfo.exType,
                    bkInfo.blkOffs);
#endif

            if ( cbRead == 0 ) {
                //  無効なデータフォーマット。  //
                fprintf(stderr, "Invalid Format Finded.\n");
                exit ( 1 );
            }
            if ( cbRead == 1 ) {
                //  終了ブロック。  //
                break;
            }

            if ( (bkInfo.ubType == 0x21) && (bkInfo.exType == 0xFF) ) {
                //  アプリケーションブロック。          //
                //  先頭に一度だけでよいのでスキップ。  //
                pR  += (cbRead);
#if defined( _DEBUG )
                fprintf(stderr, "#DEBUG : Skip Application Block.\n");
#endif
                continue;
            }

            if ( (bkInfo.ubType) == 0x2C ) {
                //  イメージブロック。  //
                size_t  cbWork  = 0;
                cbWork  = writeImageBlock(
                                &fiIn, &gifHead, &(bkInfo.imgBlk),
                                pW);
                pR      += cbRead;
                cbWrite += cbWork;
#if defined( _DEBUG )
                fprintf(stderr, "#DEBUG : Write Image: %ld (%ld)\n",
                        cbWork, cbRead);
#endif
                continue;
            }

            memcpy(pW + cbWrite, pR, cbRead);
            pR      += (cbRead);
            cbWrite += (cbRead);
        }

        closeGIFFile(&fiIn, 0);
    }

    //  最後にフッターを書き込む。  //
    pW[cbWrite] = 0x3B;
    ++  cbWrite;

    fprintf(stderr, "Total Write Size: %ld Bytes.\n", cbWrite);
    closeGIFFile(&fiOut, cbWrite);

    return ( 0 );
}
