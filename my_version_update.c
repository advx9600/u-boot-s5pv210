#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* gVerFiled="my_version";

static void* isRightLine(char* buf)
{
  long i=0;

  if (strlen(buf) < 20) return NULL;
  for (i=0;i<strlen(buf)-strlen(gVerFiled)+1;i++)
  {
	if (!strncmp(gVerFiled,buf+i,strlen(gVerFiled))){
	  return buf+i;
	}
  }

  return NULL;
}

static void changeVer(char* addr,const char* ver)
{
  int i,j;
  int count = strlen(ver);
  for (i=0;i<strlen(addr);i++)
  {
    if (addr[i] == '='){
	for (j=0;j<count;j++)
	{
	  addr[i+1+j]=ver[j];
	}
	break;
    }
  }
}

int main(int argc,char* argv[])
{
  const char* inFile=argv[1];
  const char* hFile = argv[2];
  int i,j;

  FILE* fd =fopen(inFile,"r");
  if (fd == NULL){
	perror("fopen");
	return -1;
  }

  char buf[1024];
  char lastLine[100];
  fseek(fd,-100,SEEK_END);
  while( !feof(fd) )
  {
	if (fgets(buf,sizeof(buf),fd) == NULL) continue;

	if (strlen(buf) <5) continue;
	
	strcpy(lastLine,buf);
  }

  fclose(fd); fd = NULL;
  
  char ver[10];
  for (i=0;i<strlen(lastLine);i++)
  {
	if (lastLine[i]>='0' && lastLine[i]<='9'){
	  memcpy(ver,lastLine,sizeof(ver)-1);
	  for (j=0;j<sizeof(ver);j++)
	  {
		if ( !(ver[j]>='0'&& ver[j]<='9') \
		   &&	ver[j] != '.'){
		  ver[j]='\0';
		  break;
		}
	  }
	  break;
	}
  }

  printf("ver:%s\n",ver);


  FILE* fdH= fopen(hFile,"r+");
  if (fdH == NULL){
	perror("fopen");
	return -1;
  }

  while( !feof(fdH) )
  {
	if (fgets(buf,sizeof(buf),fdH) == NULL) continue;
	
	if (isRightLine(buf)){
	  changeVer(isRightLine(buf),ver);
	  printf("chageBuf:%s\n",buf);
	  fseek(fdH,-strlen(buf),SEEK_CUR);
	  fwrite(buf,1,strlen(buf),fdH);
	  break;
	}
  }

  fclose(fdH);
}
