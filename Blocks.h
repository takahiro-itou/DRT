
#if !defined( DRT_INCLUDED_BLOCKS_H )
#    define   DRT_INCLUDED_BLOCKS_H

typedef     char *          LpWriteBuf;
typedef     const  char *   LpcReadBuf;

struct  FileInfo
{
    int         fd;         /**<  ファイルディスクリプタ。  **/
    size_t      cbSize;     /**<  ファイルサイズ。          **/
    LpWriteBuf  ptrBuf;     /**<  ファイルのデータ。        **/
};

struct  Header
{
    char    signature[3];
    char    version[3];
    int     screenWidth;
    int     screenHeight;
    int     flgGCR;
    int     colorResol;
    int     flgSort;
    int     sizeGCR;
    int     bgIndex;
    int     aspectRatio;
    char    gColorTable[256 * 3];
};

#endif
