#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include "ngx_macro.h"

static char* ngx_printf_num(char* buf,char* last,uint64_t ui64,char zero,int hexadecimal,int width);


//将格式化字符打印
char* ngx_vslprintf(char* buf,char* last,const char* fmt,va_list args){
  while(*fmt!='\0'&&buf<last){
    uint64_t ui64=0;
    uint32_t ui32=0;
    int64_t i64=0;
    int32_t i32=0;
    int width=0;//数字打印所占的位数
    int frac_width=0;
    int sign=1;
    double frac=0;
    uint64_t scale=1;
    int hex=0;
    char zero=' ';


    if(*fmt!='%'){
      *buf++=*fmt++;
    }
    else{//*fmt==%
      zero=(*++fmt=='0'?'0':' ');
      if(*fmt=='%'){//连续百分号输出一个%
        *buf++='%';
        continue;
      }

      while(*fmt>='0'&&*fmt<='9'){
        width=width*10+(*fmt++-'0');
      }
      if(*fmt=='d')
      {
        i64=(uint64_t)va_arg(args,int);
        if(i64<0){
          ui64=-i64;
          if(buf<last){*buf++='-';}
        }
        else ui64=(uint64_t)i64;
        buf=ngx_printf_num(buf,last,ui64,zero,0,width);
        fmt++;
        continue;
      }

      else if(*fmt=='.'||*fmt=='f')
      {
        if(*fmt=='.'){
          fmt++;
          while(*fmt<='9'&&*fmt>='0'){
            frac_width=frac_width*10+(*fmt++-'0');
          }
        }
        frac=va_arg(args,double);
        if(frac<0){
          *buf++='-';
          frac=-frac;
        }
        ui64=(uint64_t)frac;
        if(frac_width>0){
          frac=frac-(double)ui64;
          for(int i=0;i<frac_width;++i){
            frac*=10;
            scale*=10;
          }
          frac+=0.5;
          if(scale==(uint16_t)frac){
            frac=0;
            ui64+=1;
            zero='0';
          }
          buf=ngx_printf_num(buf,last,ui64,zero,0,width);
          *buf++='.';
          buf=ngx_printf_num(buf,last,(uint64_t)frac,zero,0,frac_width);
        }
        else{buf=ngx_printf_num(buf,last,ui64,zero,0,width);}
        fmt++;
        continue;
      }

      if(*fmt=='s')
      {
        char* str=va_arg(args,char*);
        while(*str&&buf<last){*buf++=*str++;}
        fmt++;
        continue;
      }
      if(*fmt=='x'||*fmt=='X')
      {
        i64=va_arg(args,int);
        if(i64<0){
          *buf++='-';
          i64=-i64;
        }
        ui64=(uint64_t)i64;
        hex=(*fmt=='x')?1:2;
        buf=ngx_printf_num(buf,last,ui64,zero,hex,width);
        ++fmt;
        continue;
      }
      
      if(*fmt=='u')
      {
        ui64=(uint32_t)va_arg(args,int);
        buf=ngx_printf_num(buf,last,ui64,zero,0,width);
        fmt++;
        continue;
      }
      if(*fmt== 'P')
      {
        ui64=(uint32_t)va_arg(args,pid_t);
        buf=ngx_printf_num(buf,last,ui64,zero,0,width);
        fmt++;
        continue;
      }
    }
  }
  return buf;
}


//打印日志
char* ngx_slprintf(char*buf,char* last,const char* fmt,...){
  va_list args;
  va_start(args,fmt);
  char* p=ngx_vslprintf(buf,last,fmt,args);
  va_end(args);
  return p;
}








//以各种格式打印数字字符
static char* ngx_printf_num(char* buf,char* last,uint64_t ui64,char zero,int hexadecimal,int width){
  char tmp[NGX_INT64_LEN+1];
  memset(tmp,0,sizeof(tmp));
  char* p=tmp+NGX_INT64_LEN;

  char hex[17]="0123456789abcdef";
  char HEX[17]="0123456789ABCDEF";

  if(hexadecimal==0)
  {
    do{
      *--p=((ui64%10)+'0');
    }while(ui64/=10);
  }
  else if(hexadecimal==1)
  { 
    do{
      *--p=hex[(ui64&0xf)];
    }while(ui64>>=4);
  }
  else
  {
    do{
      *--p=HEX[(ui64)&0xf];
    }while(ui64>>=4);
  }

  int len=tmp+NGX_INT64_LEN-p;
  while(len++<width&&buf<last){
    *buf++=zero;
  }
  len=tmp+NGX_INT64_LEN-p;//拷贝字符的数量
  if(buf+len>last){//防止越界
    len=last-buf;
  }
  char* ret=ngx_memcpy(buf,p,len);
  return ret;
}