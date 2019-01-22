/**
*   @file image.c Image: opera��es imagens. Escreve e le formato TGA.
*
*
*   @author 
*         - Marcelo Gattass
*         - Fab�ola Maffra
*         - Bruno Nunes
*         - Bruno Costa
*         - Rodolfo Caldeira
*
*   @date
*         Criado em:      1 de Dezembro de 2002
*         �ltima Modifica��o:   1 de maio de 2005
*
*   @version 3.0
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <memory.h>

#include <time.h>

#include "image.h"

#define ROUND(_) (int)floor( (_) + 0.5 )


struct Image_imp {
   int dcs;        /* define a dim do espaco de cor (dimension of the color space): 3=RGB, 1=luminancia */
   int width;      /* numero de pixels na direcao horizontal da imagem */
   int height;     /* numero de pixels na direcao vertical da imagem   */
   float *buf;      /* vetor de dimensao dcs*width*height que armazena consecutivamente as componentes de cor */
                   /* de cada pixel a partir do canto inferior esquerdo da imagem.  */
                   /* A posicao das componentes de cor do pixel (x,y) fica armazenada */
                   /* a partir da posicao: (y*width*dcs) + (x*dcs)  */
};


struct pixel_imp
{
	unsigned char cores[3];
	unsigned int distancia;
	int group;
};
typedef struct pixel_imp pixel;

struct pivo_imp 
{
	unsigned char cores[3];
	int meanColors[3];
	int numPixeis;
	int hasChange;
};
typedef struct pivo_imp pivo;

/************************************************************************/
/* Definicao das Funcoes Privadas                                       */
/************************************************************************/

/*  getuint e putuint:
* Funcoes auxiliares para ler e escrever inteiros na ordem (lo-hi)
* Note  que  no Windows as variaveis tipo "unsigned short int" sao 
* armazenadas  no disco em dois bytes na ordem inversa. Ou seja, o 
* numero  400, por exemplo, que  pode ser escrito como 0x190, fica 
* armazenado  em dois bytes consecutivos 0x90 e 0x01. Nos sistemas 
* UNIX e Mac este mesmo inteiro seria armazenado  na  ordem 0x01 e 
* 0x90. O armazenamento do Windows e' chamado  de  "little endian"  
* (i.e., lowest-order byte stored first), e  no  sitemas  Unix sao
* "big-endian" (i.e., highest-order byte stored first). 
*/

/***************************************************************************
* Reads an unsigned integer from input                                     *
***************************************************************************/
static int getuint(unsigned short *uint, FILE *input)
{
   int got;
   unsigned char temp[2];
   unsigned short tempuint;

   got = (int) fread(&temp, 1, 2, input);
   if (got != 2) return 0;

   tempuint = ((unsigned short)(temp[1])<<8) | ((unsigned short)(temp[0]));

   *uint = tempuint;

   return 1;
}

/***************************************************************************
* Writes an unsigned integer in output                                     *
***************************************************************************/
static int putuint(unsigned short uint, FILE *output)
{
   int put;
   unsigned char temp[2];

   temp[0] = uint & 0xff;
   temp[1] = (uint >> 8) & 0xff;

   put = (int) fwrite(&temp, 1, 2, output);
   if (put != 2) return 0;

   return 1;
}

/***************************************************************************
* Reads a long integer from input                                          *
***************************************************************************/ 
static int getlong(FILE *input, long int *longint)
{
   int got;
   unsigned char temp[4];
   long int templongint;

   got = (int)fread(&temp, 1, 4, input);
   if (got != 4) return 0;

   templongint = ((long int)(temp[3])<<24) | ((long int)(temp[2])<<16) 
      | ((long int)(temp[1])<<8) | ((long int)(temp[0]));

   *longint = templongint;

   return 1;
}

/***************************************************************************
* Writes a long integer in output                                          *
***************************************************************************/ 
static int putlong(FILE *output, long int longint)
{
   int put;
   unsigned char temp[4];

   temp[0] = (unsigned char)longint & 0xff;
   temp[1] = (unsigned char)(longint >> 8) & 0xff;
   temp[2] = (unsigned char)(longint >> 16) & 0xff;
   temp[3] = (unsigned char)(longint >> 24) & 0xff;

   put = (int)fwrite(&temp, 1, 4, output);

   if (put != 4) return 0;

   return 1;
}

/***************************************************************************
* Reads a word from input                                                  *
***************************************************************************/ 
static int getword(FILE *input, unsigned short int *word)
{
   int got;
   unsigned char temp[2];
   unsigned short int tempword;

   got = (int)fread(&temp, 1, 2, input);
   if (got != 2) return 0;

   tempword = ((unsigned short int)(temp[1])<<8) | ((unsigned short int)(temp[0]));

   *word = tempword;

   return 1;
}

/***************************************************************************
* Writes a word in output                                                  *
***************************************************************************/ 
static int putword(FILE *output, unsigned short int word)
{
   int put;
   unsigned char temp[2];

   temp[0] = word & 0xff;
   temp[1] = (word >> 8) & 0xff;

   put = (int)fwrite(&temp, 1, 2, output);
   if (put != 2) return 0;

   return 1;
}

/***************************************************************************
* Reads a double word from input                                           *
***************************************************************************/ 
static int getdword(FILE *input, unsigned long int *dword)
{
   int got;
   unsigned char temp[4];
   unsigned long int tempdword;

   got = (int)fread(&temp, 1, 4, input);
   if (got != 4) return 0;

   tempdword = ((unsigned long int)(temp[3])<<24) | ((unsigned long int)(temp[2])<<16) 
      | ((unsigned long int)(temp[1])<<8) | ((unsigned long int)(temp[0]));

   *dword = tempdword;

   return 1;
}

/***************************************************************************
* Writes a double word in output                                           *
***************************************************************************/ 
static int putdword(FILE *output, unsigned long int dword)
{
   int put;
   unsigned char temp[4];

   temp[0] = (unsigned char) (dword & 0xff);
   temp[1] = (unsigned char) ((dword >>  8) & 0xff);
   temp[2] = (unsigned char) ((dword >> 16) & 0xff);
   temp[3] = (unsigned char) ((dword >> 24) & 0xff);

   put = (int)fwrite(&temp, 1, 4, output);

   if (put != 4) return 0;

   return 1;
}

static float luminance(float red, float green, float blue)
{
 return 0.2126f*red +0.7152f*green+0.0722f*blue;
}


/************************************************************************/
/* Definicao das Funcoes Exportadas                                     */
/************************************************************************/

Image* imgCreate(int w, int h, int dcs)
{
   Image* image = (Image*) malloc (sizeof(Image));
   assert(image);
   image->width  = w;
   image->height = h;
   image->dcs = dcs;
   image->buf = malloc (w * h * dcs * sizeof(float));
   assert(image->buf);
   return image;
}

void imgDestroy(Image* image)
{
   if (image)
   {
      if (image->buf) free (image->buf);
      free(image);
   }
}

Image* imgCopy(Image* image)
{
   int w = imgGetWidth(image);
   int h = imgGetHeight(image);
   int dcs = imgGetDimColorSpace(image);
   Image* img1=imgCreate(w,h,dcs);
   int x,y;
   float rgb[3];

   for (y=0;y<h;y++){
      for (x=0;x<w;x++) {
         imgGetPixel3fv(image,x,y,rgb);
         imgSetPixel3fv(img1,x,y,rgb);
      }
   }

   return img1;
}



Image* imgGrey(Image* image)
{
   int w = imgGetWidth(image);
   int h = imgGetHeight(image);
   Image* img1=imgCreate(w,h,1);
   int x,y;
   float rgb[3],grey[3];

   for (y=0;y<h;y++){
      for (x=0;x<w;x++) {
         imgGetPixel3fv(image,x,y,rgb);
         grey[0]=luminance(rgb[0],rgb[1],rgb[2]);
         grey[1]=grey[0];
         grey[2]=grey[0];
         imgSetPixel3fv(img1,x,y,grey);
      }
   }
   return img1;
}

Image* imgResize(Image* img0, int w1, int h1) 
{
   Image* img1 = imgCreate(w1,h1,img0->dcs);
   float w0 = (float) img0->width;  /* passa para float para fazer contas */
   float h0 = (float) img0->height;

   int x0,y0,x1,y1;
   float color[3];

   for (y1=0;y1<h1;y1++)
      for (x1=0;x1<w1;x1++)
      {
         x0=ROUND(w0*x1/w1);   /* pega a cor do pixel mais proxima */
         y0=ROUND(h0*y1/h1);
         imgGetPixel3fv(img0,x0,y0,color);
         imgGetPixel3fv(img1,x1,y1,color);
      }
      return img1;
}

Image* imgAdjust2eN(Image*img0)
{
   Image* img1;
   int i=0,valid[14]={1,2,4,8,16,32,64,128,512,1024,2048,4096,8192,16384};
   int w0=img0->width;
   int h0=img0->height;
   int w1,h1;
   int x,y;
   float rgb[3],black[3]={0.f,0.f,0.f};

   for (i=0;i<14&&valid[i]<w0;i++);
   w1=valid[i];

   for (i=0;i<14&&valid[i]<h0;i++);
   h1=valid[i];

   img1=imgCreate(w1,h1, img0->dcs);

   for (y=0;y<h1;y++){
      for (x=0;x<w1;x++) {
         if (x<w0&&y<h0) {
            imgGetPixel3fv(img0,x,y,rgb);
            imgSetPixel3fv(img1,x,y,rgb);
         }
         else
            imgSetPixel3fv(img1,x,y,black);
      }
   }

   return img1;
}

float imgDif(Image*img0, Image*img1, float gamma)
{
   int w = imgGetWidth(img0);
   int h = imgGetHeight(img0);
   int x,y;
   float rgb[3],rgb0[3],rgb1[3],avg=0.f,tot=0.f;

   for (y=0;y<h;y++){
      for (x=0;x<w;x++) {
            float lum,new_lum,ratio;
         
            imgGetPixel3fv(img0,x,y,rgb0);
            imgGetPixel3fv(img1,x,y,rgb1);

            /* calcula o modulo da diferenca */
            rgb[0]=(rgb1[0]>rgb0[0])? rgb1[0]-rgb0[0] : rgb0[0]-rgb1[0] ;
            rgb[1]=(rgb1[1]>rgb0[1])? rgb1[1]-rgb0[1] : rgb0[1]-rgb1[1] ;
            rgb[2]=(rgb1[2]>rgb0[2])? rgb1[2]-rgb0[2] : rgb0[2]-rgb1[2] ;

            /* acumula a soma */
            avg+=(rgb[0]+rgb[1]+rgb[2]);
            tot+=(rgb0[0]+rgb0[1]+rgb0[2]);

            /* corrige por gamma */
            lum = luminance(rgb[0],rgb[1],rgb[2]);
            new_lum = (float) pow(lum,1./gamma);
            ratio = new_lum/lum;
            rgb[0]*=ratio; rgb[1]*=ratio; rgb[2]*=ratio;

            imgSetPixel3fv(img0,x,y,rgb);
      }
   }
    avg=100*avg/tot;
    return avg;
}

float imgErr(Image*img0, Image*img1)
{
   int w = imgGetWidth(img0);
   int h = imgGetHeight(img0);
   int x,y;
   float rgb0[3],rgb1[3],delta_rgb[3],avg=0.f,tot=0.f;

   for (y=0;y<h;y++){
      for (x=0;x<w;x++) {
            imgGetPixel3fv(img0,x,y,rgb0);
            imgGetPixel3fv(img1,x,y,rgb1);

            /* calcula o modulo da diferenca */
            delta_rgb[0]=(rgb1[0]>rgb0[0])? rgb1[0]-rgb0[0] : rgb0[0]-rgb1[0] ;
            delta_rgb[1]=(rgb1[1]>rgb0[1])? rgb1[1]-rgb0[1] : rgb0[1]-rgb1[1] ;
            delta_rgb[2]=(rgb1[2]>rgb0[2])? rgb1[2]-rgb0[2] : rgb0[2]-rgb1[2] ;

            /* acumula a soma */
            avg+=(delta_rgb[0]+delta_rgb[1]+delta_rgb[2]);
            tot+=(rgb0[0]+rgb0[1]+rgb0[2]);
      }
   }
   avg=avg/tot;
   return 100*avg;
}

int imgGetWidth(Image* image)
{
   return image->width;
}

int imgGetHeight(Image* image)
{
   return image->height;
}

int imgGetDimColorSpace(Image* image)
{
   return image->dcs;
}

float* imgGetData(Image* image)
{
   return image->buf;
}

void imgSetPixel3fv(Image* image, int x, int y, float*  color)
{
   int pos = (y*image->width*image->dcs) + (x*image->dcs);
   switch (image->dcs) {
      case 3:
         image->buf[pos  ] = color[0];
         image->buf[pos+1] = color[1];
         image->buf[pos+2] = color[2];
         break;
      case 1:
         image->buf[pos  ] = luminance(color[0],color[1],color[2]);
         break;
      default:
         break;
   }
}

void imgGetPixel3fv(Image* image, int x, int y, float* color)
{
   int pos = (y*image->width*image->dcs) + (x*image->dcs);
   switch (image->dcs) {
      case 3:
         color[0] = image->buf[pos  ];
         color[1] = image->buf[pos+1];
         color[2] = image->buf[pos+2];
         break;
      case 1:
         color[0] = image->buf[pos  ];
         color[1] = color[0];
         color[2] = color[0];
         break;
      default:
         break;
   }
}

void imgSetPixel3ubv(Image* image, int x, int y, unsigned char * color)
{
   int pos = (y*image->width*image->dcs) + (x*image->dcs);
   switch (image->dcs) {
      case 3:
         image->buf[pos  ] = (float)(color[0]/255.);
         image->buf[pos+1] = (float)(color[1]/255.);
         image->buf[pos+2] = (float)(color[2]/255.);
         break;
      case 1:
         image->buf[pos  ] = luminance((float)(color[0]/255.),(float)(color[1]/255.),(float)(color[2]/255.));
         break;
      default:
         break;
   }
}

void imgGetPixel3ubv(Image* image, int x, int y, unsigned char *color)
{
   int pos = (y*image->width*image->dcs) + (x*image->dcs);
   int r,g,b;
   switch (image->dcs) {
      case 3:
         r= ROUND(255*image->buf[pos]);
         g= ROUND (255*image->buf[pos+1]);
         b= ROUND (255*image->buf[pos+2]);
         color[0] = (unsigned char)(r<256) ? r : 255 ;
         color[1] = (unsigned char)(g<256) ? g : 255 ;
         color[2] = (unsigned char)(b<256) ? b : 255 ;
         break;
      case 1:
         r=g=b= ROUND(255*image->buf[pos]);
         color[0] = (unsigned char)(r<256) ? r : 255 ;
         color[1] = (unsigned char)(g<256) ? g : 255 ;
         color[2] = (unsigned char)(b<256) ? b : 255 ;
         break;
      default:
         break;
   }
}

Image* imgReadTGA(char *filename) 
{
   FILE        *filePtr;

   Image       *image;                /* imagem a ser criada */
   unsigned char *buffer;             /* buffer para ler o vetor de rgb da imagem  */

   unsigned char imageType;           /* 2 para imagens RGB */ 
   unsigned short int imageWidth;     /* largura da imagem  */
   unsigned short int imageHeight;    /* altura da imagem   */
   unsigned char bitCount;            /* numero de bits por pixel */

   int x,y;     /* posicao de um pixel  */

   unsigned char ucharSkip;      /* dado lixo unsigned char */
   short int     sintSkip;       /* dado lixo short int */

   /* abre o arquivo com a imagem TGA */
   filePtr = fopen(filename, "rb");
   assert(filePtr);

   /* pula os primeiros dois bytes que devem ter valor zero */
   ucharSkip = getc(filePtr); /* tamanho do descritor da imagem (0) */
   if (ucharSkip != 0) printf("erro na leitura de %s: imagem com descritor\n", filename);

   ucharSkip = getc(filePtr); 
   if (ucharSkip != 0) printf("erro na leitura de %s: imagem com tabela de cores\n", filename);

   /* le o tipo de imagem (que deve ser obrigatoriamente 2).  
   nao estamos tratando dos outros tipos */
   imageType=getc(filePtr);
   assert(imageType == 2);

   /* pula 9 bytes relacionados com a tabela de cores 
   (que nao existe quando a imagem e' RGB, imageType=2) */
   getuint((short unsigned int *)&sintSkip,filePtr);
   getuint((short unsigned int *)&sintSkip,filePtr);
   ucharSkip = getc(filePtr); 

   /* especificacao da imagem */
   getuint((short unsigned int *)&sintSkip,filePtr);      /* origem em x (por default = 0) */
   getuint((short unsigned int *)&sintSkip,filePtr);      /* origem em y (por default = 0) */ 
   getuint(&imageWidth,filePtr);    /* largura */
   getuint(&imageHeight,filePtr);   /* altura  */

   /* read image bit depth */
   bitCount=getc(filePtr);
   assert(bitCount == 24);  /* colorMode -> 3 = BGR (24 bits) */

   /* read 1 byte of garbage data */
   ucharSkip = getc(filePtr); 

   /* cria uma instancia do tipo Imagem */
   image = imgCreate(imageWidth,imageHeight,3);
   buffer = (unsigned char *) malloc(3*imageWidth*imageHeight*sizeof(unsigned char));
   assert(image);
   assert(buffer);

   /* read in image data */
   fread(buffer, sizeof(unsigned char), 3*imageWidth*imageHeight, filePtr);

   /* copia e troca as compontes de BGR para RGB */
   for (y=0;y<imageHeight;y++) {
      for (x=0;x<imageWidth;x++) {
         unsigned char color[3];
         int pos = (y*imageWidth*3) + (x*3);
         color[0] = buffer[pos+2];
         color[1] = buffer[pos+1];
         color[2] = buffer[pos  ];
         imgSetPixel3ubv(image,x,y,color);
      }
   }
   free(buffer);
   fclose(filePtr);
   return image;
}

int imgWriteTGA(char *filename, Image* image)
{
   unsigned char imageType=2;      /* RGB(A) sem compress�o */
   unsigned char bitDepth=24;      /* 24 bits por pixel     */

   FILE          *filePtr;        /* ponteiro do arquivo    */
   unsigned char * buffer;        /* buffer de bytes        */
   int  x,y;   

   unsigned char byteZero=0;      /* usado para escrever um byte zero no arquivo      */
   short int     shortZero=0;     /* usado para escrever um short int zero no arquivo */

   if (!image) return 0;

   /* cria um arquivo binario novo */
   filePtr = fopen(filename, "wb");
   assert(filePtr);

   /* cria o buffer */
   buffer = (unsigned char *) malloc(3*image->width*image->height*sizeof(unsigned char));
   assert(buffer);

   /* copia e troca as compontes de BGR para RGB */
   for (y=0;y<image->height;y++) {
      for (x=0;x<image->width;x++) {
         unsigned char color[3];
         int pos = (y*image->width*3) + (x*3);
         imgGetPixel3ubv(image,x,y,color);
         buffer[pos+2] = color[0];
         buffer[pos+1] = color[1];
         buffer[pos  ] = color[2];
      }
   }

   /* escreve o cabecalho */
   putc(byteZero,filePtr);          /* 0, no. de caracteres no campo de id da imagem     */
   putc(byteZero,filePtr);          /* = 0, imagem nao tem palheta de cores              */
   putc(imageType,filePtr);         /* = 2 -> imagem "true color" (RGB)                  */
   putuint(shortZero,filePtr);      /* info sobre a tabela de cores (inexistente)        */
   putuint(shortZero,filePtr);      /* idem                                              */
   putc(byteZero,filePtr);          /* idem                                              */
   putuint(shortZero,filePtr);      /* =0 origem em x                                    */
   putuint(shortZero,filePtr);      /* =0 origem em y                                    */
   putuint(image->width,filePtr);   /* largura da imagem em pixels                       */
   putuint(image->height,filePtr);  /* altura da imagem em pixels                        */
   putc(bitDepth,filePtr);          /* numero de bits de um pixel                        */
   putc(byteZero, filePtr);         /* =0 origem no canto inf esquedo sem entrelacamento */

   /* escreve o buf de cores da imagem */
   fwrite(buffer, sizeof(unsigned char), 3*image->width*image->height, filePtr);

   free(buffer);
   fclose(filePtr);
   return 1;
}


/* Compiler dependent definitions */
typedef unsigned char BYTE;       
typedef unsigned short int USHORT;  
typedef unsigned short int WORD;  
typedef long int LONG;            
typedef unsigned long int DWORD;


Image* imgReadBMP(char *filename)
{
   FILE  *filePtr;            /* ponteiro do arquivo */
   Image*image;            /* imagem a ser criada */
   BYTE *linedata;

   USHORT  bfType;             /* "BM" = 19788           */
   LONG    biWidth;            /* image width in pixels  */
   LONG    biHeight;           /* image height in pixels */
   WORD    biBitCount;         /* bitmap color depth     */
   DWORD   bfSize;

   USHORT  ushortSkip;         /* dado lixo USHORT */
   DWORD   dwordSkip;         /* dado lixo DWORD  */
   LONG    longSkip;         /* dado lixo LONG   */
   WORD    wordSkip;         /* dado lixo WORD   */

   LONG i, j, k, l, linesize, got;

   /* abre o arquivo com a imagem BMP */
   filePtr = fopen(filename, "rb");
   assert(filePtr);

   /* verifica se eh uma imagem bmp */
   getuint(&bfType, filePtr);
   assert(bfType == 19778);

   /* pula os 12 bytes correspondentes a bfSize, Reserved1 e Reserved2 */
   getdword(filePtr, &bfSize);
   getuint(&ushortSkip, filePtr);  /* Reserved1, deve ter valor 0 */
   assert(ushortSkip == 0);
   getuint(&ushortSkip, filePtr);  /* Reserved2, deve ter valor 0 */
   assert(ushortSkip == 0);

   /* pula os 4 bytes correspondentes a bfOffBits, que deve ter valor 54 */
   getdword(filePtr, &dwordSkip);
   assert(dwordSkip == 54);

   /* pula os 4 bytes correspondentes a biSize, que deve ter valor 40 */
   getdword(filePtr, &dwordSkip);
   assert(dwordSkip == 40);

   /* pega largura e altura da imagem */
   getlong(filePtr, &biWidth);
   getlong(filePtr, &biHeight);

   /* verifica que o numero de quadros eh igual a 1 */
   getword(filePtr, &wordSkip);
   assert(wordSkip == 1);

   /* Verifica se a imagem eh de 24 bits */
   getword(filePtr, &biBitCount);
   if(biBitCount != 24)
   {
      fprintf(stderr, "imgReadBMP: Not a bitmap 24 bits file.\n");      
      fclose(filePtr);
      return (NULL);
   }

   /* pula os demais bytes do infoheader */
   getdword(filePtr, &dwordSkip);
   assert(dwordSkip == 0);
   getdword(filePtr, &dwordSkip);
   getlong(filePtr, &longSkip);
   getlong(filePtr, &longSkip);
   getdword(filePtr, &dwordSkip);
   getdword(filePtr, &dwordSkip);

   image = imgCreate(biWidth, biHeight,3);

   /* a linha deve terminar em uma fronteira de dword */
   linesize = 3*image->width;
   if (linesize & 3) {
      linesize |= 3;
      linesize++;
   }

   /* aloca espaco para a area de trabalho */ 
   linedata = (BYTE *) malloc(linesize);
   if (linedata == NULL) {
      fprintf(stderr, "get24bits: Not enough memory.\n");
      return 0;
   }

   /* pega as componentes de cada pixel */
   for (k=0, i=0; i<image->height; i++) {
      got = (unsigned long int)fread(linedata, linesize, 1, filePtr);
      if (got != 1) {
         free(linedata);
         fprintf(stderr, "get24bits: Unexpected end of file.\n");
      }
      for (l=1, j=0; j<image->width; j++, l=l+3) {
         image->buf[k++] = (float)(linedata[l+1]/255.);
         image->buf[k++] = (float)(linedata[l  ]/255.); 
         image->buf[k++] = (float)(linedata[l-1]/255.);
      }
   }

   free(linedata);
   fclose(filePtr);
   return image;
}

int imgWriteBMP(char *filename, Image* bmp)
{
   FILE          *filePtr;         /* ponteiro do arquivo */
   unsigned char *filedata;
   DWORD bfSize;
   int i, k, l;

   int linesize, put;

   if (!bmp) return 0;

   /* cria um novo arquivo binario */
   filePtr = fopen(filename, "wb");
   assert(filePtr);

   /* a linha deve terminar em uma double word boundary */
   linesize = bmp->width * 3;
   if (linesize & 3) {
      linesize |= 3;
      linesize ++;
   }

   /* calcula o tamanho do arquivo em bytes */
   bfSize = 14 +                     /* file header size */
      40 +                     /* info header size */
      bmp->height * linesize;       /* image data  size */

   /* Preenche o cabe�alho -> FileHeader e InfoHeader */
   putuint(19778, filePtr);              /* type = "BM" = 19788                             */
   putdword(filePtr, bfSize);            /* bfSize -> file size in bytes                    */
   putuint(0, filePtr);                  /* bfReserved1, must be zero                       */
   putuint(0, filePtr);                  /* bfReserved2, must be zero                       */
   putdword(filePtr, 54);                /* bfOffBits -> offset in bits to data             */

   putdword(filePtr, 40);                /* biSize -> structure size in bytes                 */
   putlong(filePtr, bmp->width);         /* biWidth -> image width in pixels                  */         
   putlong(filePtr, bmp->height);        /* biHeight -> image height in pixels                */
   putword(filePtr, 1);                  /* biPlanes, must be 1                               */
   putword(filePtr, 24);                 /* biBitCount, 24 para 24 bits -> bitmap color depth */
   putdword(filePtr, 0);                 /* biCompression, compression type -> no compression */
   putdword(filePtr, 0);                 /* biSizeImage, nao eh usado sem compressao          */
   putlong(filePtr, 0);                  /* biXPelsPerMeter                                   */
   putlong(filePtr, 0);                  /* biYPelsPerMeter                                   */
   putdword(filePtr, 0);                 /* biClrUsed, numero de cores na palheta             */
   putdword(filePtr, 0);                 /* biClrImportant, 0 pq todas sao importantes        */

   /* aloca espacco para a area de trabalho */
   filedata = (unsigned char *) malloc(linesize);
   assert(filedata);

   /* a linha deve ser zero padded */
   for (i=0; i<(linesize-(3*bmp->width)); i++) 
      filedata[linesize-1-i] = 0;

   for (k=0; k<bmp->height;k++)
   {
      l = 1;
      /* coloca as componentes BGR no buffer */    
      for (i=0; i<bmp->width; i++) {
         unsigned char color[3];
         int r,g,b;
         imgGetPixel3ubv(bmp,i,k,color);
         r= color[0];
         g= color[1];
         b= color[2];
         filedata[l-1] = (unsigned char)(b<256) ? b : 255 ;
         filedata[l  ] = (unsigned char)(g<256) ? g : 255 ;
         filedata[l+1] = (unsigned char)(r<256) ? r : 255 ;
         l+=3;
      }

      /* joga para o arquivo */
      put = (int)fwrite(filedata, linesize, 1, filePtr);
      if (put != 1) {
         fprintf(stderr, "put24bits: Disk full.");
         free(filedata);
         return 0;
      }
   }

   /* operacao executada com sucesso */
   fprintf(stdout,"imgWriteBMP: %s successfuly generated\n",filename);
   free(filedata);
   fclose(filePtr);
   return 1;
}


/*- PFM Interface Functions  ---------------------------------------*/

Image* imgReadPFM(char *filename) 
{
  FILE *fp;
  Image* img;
  float scale;
  int w,h;

  char line[256];
 
  fp = fopen(filename, "rb");
  if (fp == NULL) {  printf("%s nao pode ser aberto\n",filename); return NULL;}

  fgets(line,256,fp);

  if(strcmp(line,"PF\n")) 
  {
    return 0;
  }

  while (fscanf( fp, " %d ", &w ) != 1)
     fgets(line, 256, fp);
  while (fscanf( fp, " %d ", &h ) != 1)
     fgets(line, 256, fp);
  while (fscanf( fp, " %f", &scale ) != 1)
     fgets(line, 256, fp);

  fgetc(fp);

  img = imgCreate(w,h,3);
  fread( img->buf, 3*w*h, sizeof(float), fp );

   fprintf(stdout,"imgReadPFM: %s successfuly loaded\n",filename);
  fclose(fp);
  return img;
}



int imgWritePFM(char * filename, Image* img)
{
  FILE * fp;
  float  scale=1.f;
 
  if ((fp = fopen(filename, "wb")) == NULL) {
    printf("\nN�o foi possivel abrir o arquivo %s\n",filename);
    return 0;
  }

  /* the ppm file header */
  fprintf(fp,"PF\n%d %d\n%f\n", img->width, img->height, scale);

  fwrite( img->buf, 3*img->width*img->height, sizeof(float), fp );

  fprintf(stdout,"imgWritePFM: %s successfuly created\n",filename);
  fclose(fp);
  return 1;
}

static int comparaCor3(const void * p1, const void * p2)
{
   int *c1 = (int *) p1;  /* aponta para o vermelho quantizado da cor 1 */
   int *c2 = (int *) p2;  /* aponta para o vermelho quantizado da cor 2 */

   /* compara o canal vermelho */
   if (*c1 < *c2) return -1;
   if (*c1 > *c2) return  1;

   /* compara  o canal verde, uma vez que o vermelho e' igual */
   c1++; c2++;
   if (*c1 < *c2) return -1;
   if (*c1 > *c2) return  1;

   /* compara o canal azul, uma vez que o vermelho e o azul sao iguais */
   c1++; c2++;
   if (*c1 < *c2) return -1;
   if (*c1 > *c2) return  1;

   /* sao iguais */
   return 0;
}

static int comparaCor1(const void * p1, const void * p2)
{
   int *c1 = (int *) p1;  /* aponta para a luminosidade quantizada da cor 1 */
   int *c2 = (int *) p2;  /* aponta para a luminosidade quantizada da cor 2 */

   /* compara o canal de luminosidade */
   if (*c1 < *c2) return -1;
   if (*c1 > *c2) return  1;

   /* sao iguais */
   return 0;
}

int imgCountColor(Image * img, float tol)
{
   int numCor = 1;
   int w = imgGetWidth(img);
   int h = imgGetHeight(img);
   int dcs = imgGetDimColorSpace(img);
   float* buf=imgGetData(img);
   int *vet=(int*) malloc(3*w*h*sizeof(int));
   int i;


   /* copia o buffer da imagem no vetor de floats fazendo 
    uma quantizacao para (1/tol) tons de cada componente de cor */
   for (i=0;i<dcs*w*h;i++) 
      vet[i]  = (int)(buf[i]/tol+0.5);

   /* ordena o vetor */
   if (dcs==3) 
      qsort(vet,w*h,3*sizeof(int),comparaCor3);
   else 
      qsort(vet,w*h,sizeof(int),comparaCor1);



   /* conta o numero de cores diferentes */
   if (dcs==3) {
      for (i=3; i<3*w*h; i+=3)
         if (comparaCor3(&vet[i-3],&vet[i])!=0) numCor++;
   } else {
      for (i=1; i<w*h; i+=1)
         if (comparaCor1(&vet[i-1],&vet[i])!=0) numCor++;
   }


   free(vet);
   return numCor;
}




static float apply(float c[9], float v[9])
{
    return c[0]*v[0]+c[1]*v[1]+c[2]*v[2]+c[3]*v[3]+c[4]*v[4]+c[5]*v[5]+c[6]*v[6]+c[7]*v[7]+c[8]*v[8];
}
static float applyV(float c[9], float v[3*9], float out[3])
{
    out[0]= c[0]*v[0*3  ]+c[1]*v[1*3  ]+c[2]*v[2*3  ]+c[3]*v[3*3  ]+c[4]*v[4*3  ]+c[5]*v[5*3  ]+c[6]*v[6*3  ]+c[7]*v[7*3  ]+c[8]*v[8*3  ];
    out[0]= c[0]*v[0*3+1]+c[1]*v[1*3+1]+c[2]*v[2*3+1]+c[3]*v[3*3+1]+c[4]*v[4*3+1]+c[5]*v[5*3+1]+c[6]*v[6*3+1]+c[7]*v[7*3+1]+c[8]*v[8*3+1];
    out[0]= c[0]*v[0*3+2]+c[1]*v[1*3+2]+c[2]*v[2*3+2]+c[3]*v[3*3+2]+c[4]*v[4*3+2]+c[5]*v[5*3+2]+c[6]*v[6*3+2]+c[7]*v[7*3+2]+c[8]*v[8*3+2];
}

static float gauss[9]={1.f/16, 2.f/16, 1.f/16,  2.f/16, 4.f/16, 2.f/16,  1.f/16, 2.f/16, 1.f/16 };
static float sobel_x[9]={-1, 0, 1,  -2, 0, 2,  -1, 0, 1 };
static float sobel_y[9]={1, 2, 1,  0, 0, 0,  -1, -2, -1 };


void imgGauss(Image* img_dst, Image* img_src) 
{
    int w   = imgGetWidth(img_dst);
    int h   = imgGetHeight(img_dst);
    int dcs = imgGetDimColorSpace(img_dst);

    float* src_buffer = imgGetData(img_src);
    float* dst_buffer = imgGetData(img_dst);
    int x,y;


    if (dcs==1) {
        for (y=1;y<h-1;y++){
            for (x=1;x<w-1;x++) {
                int k = y*w+x;
                float v[9] = {src_buffer[k+w-1], src_buffer[k+w],src_buffer[k+w+1],
                    src_buffer[k-1],src_buffer[k],src_buffer[k+1],
                    src_buffer[k-w-1],src_buffer[k-w],src_buffer[k-w+1]};
                    dst_buffer[k] = apply(gauss,v);
            }
        }
    } else {
        for (y=1;y<h-1;y+=1){
            for (x=1;x<w-1;x+=1) {
                int k0 = y*w*3+x*3;
                int k1 = k0+1;
                int k2 = k0+2;
                float r[9] = {src_buffer[k0+3*w-3],src_buffer[k0+3*w],src_buffer[k0+3*w+3],
                    src_buffer[k0-3]    ,src_buffer[k0]    ,src_buffer[k0+3],
                    src_buffer[k0-3*w-3],src_buffer[k0-3*w],src_buffer[k0-3*w+3]};
                float g[9] = {src_buffer[k1+3*w-3],src_buffer[k1+3*w],src_buffer[k1+3*w+3],
                    src_buffer[k1-3]    ,src_buffer[k1]    ,src_buffer[k1+3],
                    src_buffer[k1-3*w-3],src_buffer[k1-3*w],src_buffer[k1-3*w+3]};
                float b[9] = {src_buffer[k2+3*w-3],src_buffer[k2+3*w],src_buffer[k2+3*w+3],
                    src_buffer[k2-3]    ,src_buffer[k2]    ,src_buffer[k2+3],
                    src_buffer[k2-3*w-3],src_buffer[k2-3*w],src_buffer[k2-3*w+3]};

                dst_buffer[k0] = apply(gauss,r);
                dst_buffer[k1] = apply(gauss,g);
                dst_buffer[k2] = apply(gauss,b);
            }
        }
    }
}



/*
* The following routines have been built from knowledge gathered
* around the Web. I am not aware of any copyright problem with
* them, so use it as you want.
* N. Devillard - 1998
*/
typedef float pixelvalue ;
#define PIX_SORT(a,b) { if ((a)>(b)) PIX_SWAP((a),(b)); }
#define PIX_SWAP(a,b) { pixelvalue temp=(a);(a)=(b);(b)=temp; }

/*----------------------------------------------------------------------------
Function : opt_med9()
In : pointer to an array of 9 pixelvalues
Out : a pixelvalue
Job : optimized search of the median of 9 pixelvalues
Notice : in theory, cannot go faster without assumptions on the
signal.
Formula from:
XILINX XCELL magazine, vol. 23 by John L. Smith
The input array is modified in the process
The result array is guaranteed to contain the median
value
in middle position, but other elements are NOT sorted.
---------------------------------------------------------------------------*/
static pixelvalue opt_med9(pixelvalue * p)
{
    PIX_SORT(p[1], p[2]) ; PIX_SORT(p[4], p[5]) ; PIX_SORT(p[7], p[8]) ;
    PIX_SORT(p[0], p[1]) ; PIX_SORT(p[3], p[4]) ; PIX_SORT(p[6], p[7]) ;
    PIX_SORT(p[1], p[2]) ; PIX_SORT(p[4], p[5]) ; PIX_SORT(p[7], p[8]) ;
    PIX_SORT(p[0], p[3]) ; PIX_SORT(p[5], p[8]) ; PIX_SORT(p[4], p[7]) ;
    PIX_SORT(p[3], p[6]) ; PIX_SORT(p[1], p[4]) ; PIX_SORT(p[2], p[5]) ;
    PIX_SORT(p[4], p[7]) ; PIX_SORT(p[4], p[2]) ; PIX_SORT(p[6], p[4]) ;
    PIX_SORT(p[4], p[2]) ; return(p[4]) ;
}


#undef PIX_SORT
#undef PIX_SWAP


void imgMedian(Image* image) 
{
    int w = imgGetWidth(image);
    int h = imgGetHeight(image);
    int dcs = imgGetDimColorSpace(image);
    Image* img = imgCopy(image);  
    float* image_buf = imgGetData(image);
    float* img_buf = imgGetData(img);
    int x,y;

    if (dcs==1) {
        for (y=1;y<h-1;y++){
            for (x=1;x<w-1;x++) {
                int k = y*w+x;
                float v[9] = {img_buf[k+w-1], img_buf[k+w],img_buf[k+w+1],
                    img_buf[k-1],img_buf[k],img_buf[k+1],
                    img_buf[k-w-1],img_buf[k-w],img_buf[k-w+1]};
                image_buf[k]= opt_med9(v);
            }
        }
    } else {
        for (y=1;y<h-1;y+=1){
            for (x=1;x<w-1;x+=1) {
                int k0 = y*w*3+x*3;
                int k1 = k0+1;
                int k2 = k0+2;
                float r[9] = {img_buf[k0+3*w-3],img_buf[k0+3*w],img_buf[k0+3*w+3],
                    img_buf[k0-3]    ,img_buf[k0]    ,img_buf[k0+3],
                    img_buf[k0-3*w-3],img_buf[k0-3*w],img_buf[k0-3*w+3]};
                float g[9] = {img_buf[k1+3*w-3],img_buf[k1+3*w],img_buf[k1+3*w+3],
                    img_buf[k1-3]    ,img_buf[k1]    ,img_buf[k1+3],
                    img_buf[k1-3*w-3],img_buf[k1-3*w],img_buf[k1-3*w+3]};
                float b[9] = {img_buf[k2+3*w-3],img_buf[k2+3*w],img_buf[k2+3*w+3],
                    img_buf[k2-3]    ,img_buf[k2]    ,img_buf[k2+3],
                    img_buf[k2-3*w-3],img_buf[k2-3*w],img_buf[k2-3*w+3]};
                image_buf[k0] = opt_med9(r);
                image_buf[k1] = opt_med9(g);
                image_buf[k2] = opt_med9(b);
            }
        }
    }
    imgDestroy(img);
}

Image* imgEdges(Image* imgIn)
{
    int w = imgGetWidth(imgIn);
    int h = imgGetHeight(imgIn);
    int dcs = imgGetDimColorSpace(imgIn);
    Image* imgOut = imgCreate(w,h,1);
    int x,y;
    Image* img;
    float max=0,inv;

    float* imgOut_buf = imgGetData(imgOut);

    float* img_buf;
    if (dcs==1) 
        img = imgIn;
    else 
        img = imgGrey(imgIn);
    img_buf=imgGetData(img);

    for (y=1;y<h-1;y++){
        for (x=1;x<w-1;x++) {
            int k = y*w+x;
            float v[9] = {img_buf[k+w-1], img_buf[k+w],img_buf[k+w+1],
                img_buf[k-1],img_buf[k],img_buf[k+1],
                img_buf[k-w-1],img_buf[k-w],img_buf[k-w+1]};
            float dx = apply(sobel_x,v);
            float dy = apply(sobel_y,v);
            float val = (float)sqrt(dx*dx+dy*dy);
            max = (max>val)?max:val;
            imgOut_buf[k] = val;
        }
    }

    inv = (max==0)? 1.f : 1.f/max;
    /* arruma a imagem */
    for (y=0;y<h;y++) {
        for (x=0;x<w;x++) {
            int k=y*w+x;
            if ((y==0)||(y==(h-1))||(x==0)||(x==(w-1))) /* borda */
                imgOut_buf[x]=0.f;
            else
                imgOut_buf[x]/=inv;
        }
    }
    return imgOut;
}

/* implementacao da reducao de cores por median cut */
typedef struct
{
    float r, g, b;
} color;

typedef struct
{
    color max;     /* maximos R's, G's e B's */
    color min;	   /* minimos R's, G's e B's */
    float var;	   /* variacao em cada cubo  */
    int ini;	   /* final do cubo  */
    int fim;       /* inicio do cubo */
    int maiorDim;  /* R=0, G=1, B=2  */
    int checked;
} colorCube;

static int comparaR(const void * p1, const void * p2)
{
    color *color1 = (color *) p1;
    color *color2 = (color *) p2;

    color c1 = *color1;
    color c2 = *color2;

    if (c1.r < c2.r) return -1;
    if (c1.r > c2.r) return  1;

    return 0;
}

static int comparaG(const void * p1, const void * p2)
{
    color *color1 = (color *) p1;
    color *color2 = (color *) p2;

    color c1 = *color1;
    color c2 = *color2;

    if (c1.g < c2.g) return -1;
    if (c1.g > c2.g) return  1;

    return 0;
}

static int comparaB(const void * p1, const void * p2)
{
    color *color1 = (color *) p1;
    color *color2 = (color *) p2;

    color c1 = *color1;
    color c2 = *color2;

    if (c1.b < c2.b) return -1;
    if (c1.b > c2.b) return  1;

    return 0;
}

static void caixaEnvolvente(colorCube* cube, color* colorVec)
{
    float r,g,b;
    int i; 

    /* inicializa com o pior caso */
    cube->min.r = cube->min.g = cube->min.b = 1.0; 
    cube->max.r = cube->max.g = cube->max.b = 0.0;

    /* percorre o cubo ajustando o dom�nio das cores */
    for (i=cube->ini;i<=cube->fim;i++){
        r = colorVec[i].r;
        if (r > cube->max.r) cube->max.r = r;
        if (r < cube->min.r) cube->min.r = r;
        g = colorVec[i].g;
        if (g > cube->max.g) cube->max.g = g;
        if (g < cube->min.g) cube->min.g = g;
        b = colorVec[i].b;
        if (b > cube->max.b) cube->max.b = b;
        if (b < cube->min.b) cube->min.b = b;
    }
}

static Image* bestColor(Image *img0, color* pal, Image* img1, int pal_size)
{
    int w = imgGetWidth(img0);
    int h = imgGetHeight(img0);
    float rgb[3];
    int x,y,i;

    for (y=0;y<h;y++){
        for (x=0;x<w;x++){
            int i_menor;
            float m_menor;
            float rgb1[3];

            imgGetPixel3fv(img0,x,y,rgb);

            m_menor = FLT_MAX;
            i_menor = 0;

            for(i=0;i<pal_size;i++)
            {
                float dif[3];
                float m;

                dif[0] = rgb[0] - pal[i].r;
                dif[1] = rgb[1] - pal[i].g;
                dif[2] = rgb[2] - pal[i].b;

                m = (float)((dif[0]*dif[0]) + (dif[1]*dif[1]) + (dif[2]*dif[2]));

                if (m < m_menor)
                {
                    i_menor = i;
                    m_menor = m;

                    rgb1[0]=pal[i_menor].r;
                    rgb1[1]=pal[i_menor].g;
                    rgb1[2]=pal[i_menor].b;
                }
            }

            imgSetPixel3fv(img1,x,y,rgb1);
        }
    }
    return img1;
}

static void paleta(color* pal, colorCube* cubeVec, color* colorVec, int numCubos)
{
    float sumvar[3];
    int count;
    int i,j;

    for(i=0;i<numCubos;i++)
    {
        sumvar[0] = sumvar[1] = sumvar[2] = 0.0;
        count = 0;

        for(j=cubeVec[i].ini;j<=cubeVec[i].fim;j++)
        {
            sumvar[0] += colorVec[j].r;
            sumvar[1] += colorVec[j].g;
            sumvar[2] += colorVec[j].b;
            count++;
        }

        pal[i].r = sumvar[0]/count;
        pal[i].g = sumvar[1]/count;
        pal[i].b = sumvar[2]/count;
    }
}

static void ordenaMaiorDim(colorCube* cubeVec, color* colorVec, int i)
{
    size_t numElem;
    size_t tamElem;
    float var[3];

    numElem = (size_t)(cubeVec[i].fim - cubeVec[i].ini + 1);
    tamElem = (size_t)(sizeof(color));

    var[0] = cubeVec[i].max.r - cubeVec[i].min.r;
    var[1] = cubeVec[i].max.g - cubeVec[i].min.g;
    var[2] = cubeVec[i].max.b - cubeVec[i].min.b;

    /* procura a maior dimensao em um cubo */
    if((var[0]>=var[1])&&(var[0]>=var[2])) cubeVec[i].maiorDim = 0;
    else
        if((var[1]>=var[0])&&(var[1]>=var[2])) cubeVec[i].maiorDim = 1;
        else
            if((var[2]>var[0]) && (var[2]>var[1])) cubeVec[i].maiorDim = 2;

    /* ordena de acordo com a maior dimensao da caixa */
    if(cubeVec[i].maiorDim == 0)
    {
        qsort(&colorVec[cubeVec[i].ini],numElem,tamElem,comparaR);
        cubeVec[i].var = var[0];
    }
    if(cubeVec[i].maiorDim == 1)
    {
        qsort(&colorVec[cubeVec[i].ini],numElem,tamElem,comparaG);
        cubeVec[i].var = var[1];
    }
    if(cubeVec[i].maiorDim == 2)
    {
        qsort(&colorVec[cubeVec[i].ini],numElem,tamElem,comparaB);
        cubeVec[i].var = var[2];
    }
}

static void cortaCubo(colorCube* cubeVec, int posCorte, int numCubos)
{
    /* divide o cubo */
    int ini = cubeVec[posCorte].ini;
    int fim = cubeVec[posCorte].fim;

    if((fim - ini)%2 != 0) /* numero par de elementos */
    {
        cubeVec[numCubos].ini = ini + (fim - ini + 1)/2;
        cubeVec[numCubos].fim = fim;
        cubeVec[posCorte].fim = cubeVec[numCubos].ini - 1;
    }
    else /* numero impar de elementos */
    {
        cubeVec[numCubos].ini = ini + (fim - ini)/2;
        cubeVec[numCubos].fim = fim;
        cubeVec[posCorte].fim = cubeVec[numCubos].ini - 1;
    }
}

static int cuboCorte(colorCube* cubeVec, int numCubos)
{
    /* escolhe o cubo a ser cortado */
    float maiorVar = -1;
    int posCorte = -1;
    int k;
    for(k=0;k<numCubos;k++)
    {
        /* cubo com uma unica cor */
        if((cubeVec[k].max.r == cubeVec[k].min.r)&&(cubeVec[k].max.g == cubeVec[k].min.g)&&(cubeVec[k].max.b == cubeVec[k].min.b))
            continue;

        if(cubeVec[k].var>maiorVar)
        {
            maiorVar = cubeVec[k].var;
            posCorte = k;
        }
    }

    return posCorte;
}

void imgReduceColors(Image * img0, Image* img1, int maxCores)
{
    int w = imgGetWidth(img0);
    int h = imgGetHeight(img0);

    int x,y,i,j,numCubos = 0;
    int posCorte = -1;
    float rgb[3];

    colorCube* cubeVec = (colorCube*)malloc(maxCores*sizeof(colorCube)); /* vetor de cubos */
    color* colorVec = (color*)malloc(w*h*sizeof(color)); /* vetor  de cores */
    color* pal = (color*)malloc(maxCores*sizeof(color)); /* paleta de cores */

    /* guarda as cores nos vetores (com repeticao) */
    i = 0;
    for (y=0;y<h;y++){
        for (x=0;x<w;x++){
            imgGetPixel3fv(img0,x,y,rgb);
            colorVec[i].r = rgb[0];
            colorVec[i].g = rgb[1];
            colorVec[i].b = rgb[2];
            i++;
        }
    }

    /* cria o cubo inicial */
    cubeVec[0].checked = 0;
    cubeVec[0].fim = w*h-1;
    cubeVec[0].ini = 0;

    numCubos = 1;

    for(j=0;j<maxCores-1;j++)
    {
        for(i=0;i<numCubos;i++)
        {
            if(cubeVec[i].checked) continue;

            /* calcula os maximos e os minimos */
            caixaEnvolvente(&cubeVec[i], colorVec);

            /* ordena de acordo com a maior dimensao */
            ordenaMaiorDim(cubeVec, colorVec, i);

            cubeVec[i].checked = 1;
        }

        /* escolhe o cubo a ser cortado */
        posCorte = cuboCorte(cubeVec, numCubos);

        if(posCorte == -1) break;

        /* divide o cubo */
        cortaCubo(cubeVec, posCorte, numCubos);

        cubeVec[numCubos].checked = 0;
        cubeVec[posCorte].checked = 0;

        numCubos++;
    }

    /* cria a paleta de cores */
    paleta(pal, cubeVec, colorVec, numCubos);

    /* preenche a imagem com as cores da paleta */
    bestColor(img0,&pal[0],img1,maxCores);

    free(colorVec);
    free(cubeVec);
    free(pal);

    printf("fim\n");

    return;
}


/***************************************************************************
* Funcoes do primeiro trabalho                                             *
***************************************************************************/
/* substitua este string com os nomes dos componentes do seu grupo */
char grupo[]="0310096 Luiz Rodolfo Caldeira\n9915985 Bruno Costa\n0115819 Bruno Nunes";

int verificarRepeticao( pivo * vtKs , int tam , unsigned char * cores )
{
	int i;
	int repetido = 0;
	for( i = 0; i < tam; i++ )
	{
		if( vtKs[i].cores[0] == cores[0] && 
			vtKs[i].cores[1] == cores[1] && 
			vtKs[i].cores[2] == cores[2] )
		{
			repetido =1;
			break;
		}

	}

	return repetido;
}

void findKs(Image* image,pivo * vtKs , int nColors)
{
	int imageWidth;
	int imageHeigth;
	int squareHeigth;
	int squareWidth;
	int i;
	int x, y;
	int * pontosX;
	int * pontosY;
	int tam = 0;
	unsigned char cores[3];

	pontosX = ( int * ) malloc ( nColors * sizeof( int ));
	pontosY = ( int * ) malloc ( nColors * sizeof( int ));

	imageWidth = imgGetWidth( image );
	imageHeigth = imgGetHeight( image );

	squareWidth = imageWidth / nColors;
	squareHeigth = imageHeigth / nColors;

	for( i = 0; i < nColors ; i++ )
	{
		rand();
		x = rand()%imageWidth;
		y = rand()%imageHeigth;

		//pegar cor dessa coordenada
		imgGetPixel3ubv(image,x,y, cores );

		if( !verificarRepeticao( vtKs , i , cores ))
		{
	
			vtKs[i].cores[0] = cores[0];
			vtKs[i].cores[1] = cores[1];
			vtKs[i].cores[2] = cores[2];

		}
		else
		{
			i--;
		}

	}	

}


void imgReduceColorsKMeans(Image* image, Image* img, int ncolors)
{
	int x, y, i, step; 
	int lastIndiceGrupo, indiceGrupo;
	int imageWidth , imageHeight;
	int hasChangeElement;
	int primeiroLoop , numElem ;
	unsigned int currDist;
	pixel * pix;
	pixel * vtPixeis;
	pivo * vtKs;
	int count = 0;


	clock_t startTime = clock();

	imageWidth = image->width;
	imageHeight = image->height;

	vtKs = ( pivo *) malloc( ncolors * sizeof(pivo));
	for( i = 0; i < ncolors; i++ )
	{
		vtKs[i].numPixeis = 0;
		vtKs[i].hasChange = 0;
		vtKs[i].meanColors[0] = 0;
		vtKs[i].meanColors[1] = 0;
		vtKs[i].meanColors[2] = 0;
	}

	vtPixeis = ( pixel * ) malloc ( imageWidth * imageHeight * sizeof( pixel ));
	
	for( x = 0; x < imageWidth; x++ )
	{
		step = x * imageHeight;
		for( y = 0; y < imageHeight; y++ )
		{
			vtPixeis[ step + y ].distancia = 300000;
			vtPixeis[ step + y ].group = ncolors + 1;
		}
	}

	//pegar os ks principais da imagem
	findKs(image , vtKs , ncolors);

	for( x = 0; x < imageWidth; x++ )
	{
		step = x * imageHeight;
		for( y = 0; y < imageHeight; y++ )
		{
			imgGetPixel3ubv(image,x,y,vtPixeis[step + y].cores);
		}
	}

	indiceGrupo = ncolors;
	hasChangeElement = 1;
	primeiroLoop = 1;
	lastIndiceGrupo = -1;
	
	while( hasChangeElement && count < 10 )
	{

		hasChangeElement = 0;
		currDist = 2;
		printf("iteracao = %i\n", count);
		
		for( x = 0; x < imageWidth; x++ )
		{
			step = x * imageHeight;
			for( y = 0; y < imageHeight; y++ )
			{
				pix = &vtPixeis[step + y];
				lastIndiceGrupo = pix->group;

				for( i = 0; i < ncolors; i++ )
				{
					currDist =  ( pix->cores[0] - vtKs[i].cores[0] ) * ( pix->cores[0] - vtKs[i].cores[0] ) +
								( pix->cores[1] - vtKs[i].cores[1] ) * ( pix->cores[1] - vtKs[i].cores[1] )	 +
								( pix->cores[2] - vtKs[i].cores[2] ) * ( pix->cores[2] - vtKs[i].cores[2] );


					if( currDist < pix->distancia )
					{
						pix->distancia = currDist;
						pix->group = i;
						indiceGrupo = i ;
					}

				}// for

				if( pix->group != lastIndiceGrupo  )
				{
					if( !primeiroLoop )
					{

						vtKs[ lastIndiceGrupo ].numPixeis--;
						vtKs[ lastIndiceGrupo ].hasChange = 1;
						vtKs[ lastIndiceGrupo ].meanColors[0]-=pix->cores[0];
						vtKs[ lastIndiceGrupo ].meanColors[1]-=pix->cores[1];
						vtKs[ lastIndiceGrupo ].meanColors[2]-=pix->cores[2];

					}

					vtKs[ indiceGrupo ].numPixeis++;
					vtKs[ indiceGrupo ].hasChange = 1;
					vtKs[ indiceGrupo ].meanColors[0]+=pix->cores[0];	
					vtKs[ indiceGrupo ].meanColors[1]+=pix->cores[1];
					vtKs[ indiceGrupo ].meanColors[2]+=pix->cores[2];
					hasChangeElement = 1;

				}// if

			}// for
		}// for

		primeiroLoop = 0;

		// calcular novos K�s
		if( hasChangeElement )
		{
			for( i = 0; i < ncolors; i++ )
			{
				if( vtKs[i].hasChange )
				{

					unsigned char novasCores[3];
					numElem = vtKs[i].numPixeis;
						if( numElem != 0 )
					{
						novasCores[0] =  vtKs[i].meanColors[0] / numElem;
						novasCores[1] =  vtKs[i].meanColors[1] / numElem;
						novasCores[2] =  vtKs[i].meanColors[2] / numElem;

						if ( !verificarRepeticao( vtKs , ncolors , novasCores ) )
						{
						vtKs[i].cores[0] =  vtKs[i].meanColors[0] / numElem;
						vtKs[i].cores[1] =  vtKs[i].meanColors[1] / numElem;
						vtKs[i].cores[2] =  vtKs[i].meanColors[2] / numElem;
						}
					}
					vtKs[i].hasChange = 0;
				}
			}
		}		

		count++;

	}// while

	for( x = 0; x < imageWidth; x++ )
	{
		step = x * imageHeight ;
		for( y = 0; y < imageHeight; y++ )
		{
			int indice = vtPixeis[ step + y ].group;

			vtPixeis[ step + y ].cores[0] = vtKs[indice].cores[0];
			vtPixeis[ step + y ].cores[1] = vtKs[indice].cores[1];
			vtPixeis[ step + y ].cores[2] = vtKs[indice].cores[2];

			imgSetPixel3ubv(image,x,y,vtPixeis[ step + y ].cores);
		}
	}

	printf("Tempo: %f\n", (double)(clock()-startTime)/CLOCKS_PER_SEC);

}

