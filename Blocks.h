
#if !defined( DRT_INCLUDED_BLOCKS_H )
#    define   DRT_INCLUDED_BLOCKS_H

typedef     unsigned  char      UByte8;

typedef     UByte8              ColorTable[256 * 3];

typedef     UByte8  *           LpWriteBuf;
typedef     const  UByte8  *    LpcReadBuf;

//========================================================================
/**
**    ファイル情報構造体。
**/

typedef  struct
{
    int         fd;         /**<  ファイルディスクリプタ。  **/
    size_t      cbSize;     /**<  ファイルサイズ。          **/
    LpWriteBuf  ptrBuf;     /**<  ファイルのデータ。        **/

} FileInfo;

//========================================================================
/**
**    ファイルヘッダ構造体。
**/

typedef  struct
{
    UByte8      signature[3];
    UByte8      version[3];
    int         screenWidth;
    int         screenHeight;
    int         flgGCR;
    int         colorResol;
    int         flgSort;
    int         sizeGCR;
    int         bgIndex;
    int         aspectRatio;
    int         gColorSize;
    ColorTable  gColorTable;

} FileHeader;


//========================================================================
/**
**    ブロック情報構造体。
**/

typedef  struct
{
    UByte8      ubType;         /**<  ブロックタイプ。      **/
    UByte8      exType;         /**<  拡張ブロックタイプ。  **/
    size_t      blkOffset;      /**<  ブロックオフセット。  **/
    size_t      cbTotal;        /**<  ブロックサイズ。      **/
    LpcReadBuf  ptrAddr;        /**<  データ先頭アドレス。  **/
} BlockInfo;

#endif
