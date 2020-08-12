//pagefile.sys file 4gb ,used in windows for hole in th file
/*swap partion part(part of HD which looks like RAM) :swapping the data from RAM to HD ,when RAM don't have  space,later */


// #ifdef _MSC_VER
// #define _CRT_SECURE_NO_WARNINGS
// #endif
// #pragma warning(disable : 4996)
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>  //linux
//#include<io.h>    //windows
//#include<iostream>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>

#include<dirent.h>

#define MAXINODE 5      //50

#define READ 1
#define WRITE 2

#define MAXFILESIZE 10  //2048

#define REGULAR 1
#define SPECIAL 2

#define START 0
#define CURRENT 1
#define END 2


typedef struct superblock
{
int Totalnodes;
int FreeInode;
} SUPERBLOCK, * PSUPERBLOCK;


typedef struct inode
{
char FileName[50];
int inodeNumber;
int FileSize;
int FileActualSize;
int FileType;
char* Buffer;
int LinkCount;
int ReferenceCount;
int permission;
struct inode* next;
} INODE, * PINODE, ** PPINODE;


typedef struct filetable
{
int readoffset;
int writeoffset;
int count;
int mode;
PINODE ptrinode;
}FILETABLE, * PFILETABLE;


typedef struct uftd
{
PFILETABLE ptrfiletable;
}UFTD;

UFTD UFTDArr[MAXINODE];

SUPERBLOCK SUPERBLOCKobj;
PINODE head = NULL;


void man(char *name)
{
    if(name==NULL)
        return;
    
    if(strcmp(name,"create")==0)
    {
        printf("Description : Used to create new regular files\n");
        printf("usage : create File_name Permission\n");
        printf("permissions are as :- only read : 1  ,  only write : 2 ,  read + write : 3 \n");
    }
    else if(strcmp(name,"read")==0)
    {
        printf("Description : Used to read from regular files\n");
        printf("usage : read File_name No_of_bytes_to_read\n");
    }
    else if(strcmp(name,"write")==0)
    {
        printf("Description : Used to write in regular files\n");
        printf("usage : write File_name\n After this enter the data that we want to write \n");
    }
    else if(strcmp(name,"ls")==0)
    {
        printf("Description : Used to list all information of files\n");
        printf("usage : ls\n");
    }
    else if(strcmp(name,"stat")==0)
    {
        printf("Description : Used to display information of file\n");
        printf("usage : stat File_name\n");
    }
    else if(strcmp(name,"fstat")==0)
    {
        printf("Description : Used to display information of file\n");
        printf("usage : stat File_Descriptor\n");
    }
    else if(strcmp(name,"truncate")==0)
    {
        printf("Description : Used to remove data from file\n");
        printf("usage : truncate File_name\n");
    }
    else if(strcmp(name,"open")==0)
    {
        printf("Description : Used to open existing file\n");
        printf("usage : open File_name mode\n");
    }
    else if(strcmp(name,"close")==0)
    {
        printf("Description : Used to close opened file\n");
        printf("usage : close File_name\n");
    }
    else if(strcmp(name,"closeall")==0)
    {
        printf("Description : Used to close all opened files\n");
        printf("usage : closeall\n");
    }
    else if(strcmp(name,"lseek")==0)
    {
        printf("Description : Used to change file offset\n");
        printf("usage : lseek File_Name ChangeInOffset StartPoint\n");
    }
    else if(strcmp(name,"rm")==0)
    {
        printf("Description : Used to delete the file\n");
        printf("usage : rm File_Name\n");
    }
    else
    {
        printf("ERROR : No manual entry available.\n");
    }
    
}

void DisplayHelp()
{
printf("ls : List out all the files \n");
printf("clear : To clear the console \n");
printf("open : To open the file \n");
printf("close : TO close the file \n");
printf("closeall : To close all the files \n");
printf("read : To Read the contents of the file \n");
printf("write : To write the contents of the file \n");
printf("stat : To Display information of file using name \n");
printf("fstat : To Display information of file using file descriptor \n");
printf("truncate : To remove all the data from file \n");
printf("rm : To delete the file \n");
printf("create : To Create new file \n");
printf("lseek : To change the read and write offset of specified file \n");
printf("exit : To terminate the file system \n");
printf("\nTo Know about specific command use : man command_name \n");

}

int GetFDFromName(char* name)
{
int i = 0;
while (i < MAXINODE)
{
if (UFTDArr[i].ptrfiletable != NULL)
{
if(strcmp((UFTDArr[i].ptrfiletable->ptrinode->FileName), name) == 0)
{
break;
}
}
i++;
}

if (i == MAXINODE)
{
return -1;
}

else
{
return i;
}
}


PINODE Get_Inode(char* name)
{
PINODE temp = head;
int i = 0;

if (name == NULL)
{
return NULL;
}

while (temp != NULL)
{
if (strcmp(name, temp->FileName) == 0)
{
break;
}

temp = temp->next;
}
return temp;
}

void CreateDILB()
{
    int i=1;
    PINODE newn=NULL;
    PINODE temp=head;
    
    while(i<=MAXINODE)
    {
        newn = (PINODE)malloc(sizeof(INODE));
        
        newn->LinkCount=0;
        newn->ReferenceCount=0;
        newn->FileType=0;
        newn->FileSize=0;
        
        newn->Buffer = NULL;
        newn->next=NULL;
        
        newn->inodeNumber = i;
        
        if(temp==NULL)
        {
            head=newn;
            temp=head;
        }
        else
        {
            temp->next=newn;
            temp=temp->next;
        }
        i++;
    }
    printf("DILB created successfully\n");
}

void InitialiseSuperBlock()
{
    int i = 0;

    while (i < MAXINODE)
    {
    UFTDArr[i].ptrfiletable = NULL;
    i++;
    }

    SUPERBLOCKobj.Totalnodes = MAXINODE;
    SUPERBLOCKobj.FreeInode = MAXINODE;
}


int CreateFile(char* name, int permission)
{
    int i = 0;
    PINODE temp = head;

    if ((name == NULL) || (permission == 0) || (permission > 3))
    {
        return -1;
    }

    if (SUPERBLOCKobj.FreeInode == 0)
    {
        return -2;
    }

    (SUPERBLOCKobj.FreeInode)--;

    if (Get_Inode(name) != NULL)
    {
        return -3;
    }

    while (temp != NULL)
    {
        if (temp->FileType == 0)
        {
            break;
        }
        temp = temp->next;
    }

    while (i < MAXINODE)
    {
        if (UFTDArr[i].ptrfiletable == NULL)
        {
            break;
        }

        i++;
    }

    UFTDArr[i].ptrfiletable = (PFILETABLE)malloc(sizeof(FILETABLE));

    UFTDArr[i].ptrfiletable->count = 1;
    UFTDArr[i].ptrfiletable->mode = permission;
    UFTDArr[i].ptrfiletable->readoffset = 0;
    UFTDArr[i].ptrfiletable->writeoffset = 0;

    UFTDArr[i].ptrfiletable->ptrinode = temp;

    strcpy(UFTDArr[i].ptrfiletable->ptrinode->FileName, name);
    UFTDArr[i].ptrfiletable->ptrinode->FileType = REGULAR;
    UFTDArr[i].ptrfiletable->ptrinode->ReferenceCount = 1;
    UFTDArr[i].ptrfiletable->ptrinode->LinkCount = 1;
    UFTDArr[i].ptrfiletable->ptrinode->FileSize = MAXFILESIZE;
    UFTDArr[i].ptrfiletable->ptrinode->FileActualSize = 0;
    UFTDArr[i].ptrfiletable->ptrinode->permission = permission;
    UFTDArr[i].ptrfiletable->ptrinode->Buffer = (char *)malloc(MAXFILESIZE);
    return i;

}


int rm_File(char* name)     //remove or delete  open=cat , rm=unlink
{
    int fd = 0;
    fd = GetFDFromName(name);
    
    if (fd == -1)
    {
        return -1;
    }

    (UFTDArr[fd].ptrfiletable->ptrinode->LinkCount)--;

    if (UFTDArr[fd].ptrfiletable->ptrinode->LinkCount == 0)
    {
        UFTDArr[fd].ptrfiletable->ptrinode->FileType = 0;
        strcpy(UFTDArr[fd].ptrfiletable->ptrinode->FileName,"");

        UFTDArr[fd].ptrfiletable->ptrinode->ReferenceCount=0;
        UFTDArr[fd].ptrfiletable->ptrinode->permission=0;
        UFTDArr[fd].ptrfiletable->ptrinode->FileActualSize=0;

        free(UFTDArr[fd].ptrfiletable->ptrinode->Buffer);
        free(UFTDArr[fd].ptrfiletable);
    }

    UFTDArr[fd].ptrfiletable = NULL;
    (SUPERBLOCKobj.FreeInode)++;

}

void Free_all()
{
    int i=0;
    PINODE temp=head;
    while(i<MAXINODE)
    {
        if(UFTDArr[i].ptrfiletable!=NULL)
        {
            free(UFTDArr[i].ptrfiletable->ptrinode->Buffer);
            free(UFTDArr[i].ptrfiletable);
        }
        head=temp->next;
        free(temp);
        temp=head;
        i++;
    }
}

int ReadFile(int fd, char* arr, int isize)
{
int read_size = 0;

if (UFTDArr[fd].ptrfiletable == NULL)
{
return -1;
}

if ((UFTDArr[fd].ptrfiletable->mode != READ) && (UFTDArr[fd].ptrfiletable->mode != READ + WRITE))
{
return -2;
}

if ((UFTDArr[fd].ptrfiletable->ptrinode->permission != READ) && (UFTDArr[fd].ptrfiletable->ptrinode->permission != READ + WRITE))
{
return -2;
}

if (UFTDArr[fd].ptrfiletable->readoffset == UFTDArr[fd].ptrfiletable->ptrinode->FileActualSize)
{
return -3;
}

if ((UFTDArr[fd].ptrfiletable->ptrinode->FileType) != REGULAR)
{
return -4;
}

read_size = (UFTDArr[fd].ptrfiletable->ptrinode->FileActualSize) - (UFTDArr[fd].ptrfiletable->readoffset);

if (read_size < isize)
{
strncpy(arr, (UFTDArr[fd].ptrfiletable->ptrinode->Buffer) + (UFTDArr[fd].ptrfiletable->readoffset), read_size);

UFTDArr[fd].ptrfiletable->readoffset = UFTDArr[fd].ptrfiletable->readoffset + read_size;

return read_size;       //new
}

else
{
strncpy(arr, (UFTDArr[fd].ptrfiletable->ptrinode->Buffer) + (UFTDArr[fd].ptrfiletable->readoffset), isize);

UFTDArr[fd].ptrfiletable->readoffset = UFTDArr[fd].ptrfiletable->readoffset + isize;
}

return isize;
}


int WriteFile(int fd,char*arr,int isize)
{
if ((UFTDArr[fd].ptrfiletable->mode != WRITE) && (UFTDArr[fd].ptrfiletable->mode != READ + WRITE))
{
return -1;
}

if ((UFTDArr[fd].ptrfiletable->ptrinode->permission != WRITE) && (UFTDArr[fd].ptrfiletable->ptrinode->permission != READ + WRITE))
{
return -1;
}

// if ((UFTDArr[fd].ptrfiletable->writeoffset) == MAXFILESIZE)
// {
// return -2;
// }

if ((UFTDArr[fd].ptrfiletable->ptrinode->FileType) != REGULAR)
{
return -3;
}

if((UFTDArr[fd].ptrfiletable->ptrinode->FileSize-(UFTDArr[fd].ptrfiletable->writeoffset+isize))<=0)
{
    UFTDArr[fd].ptrfiletable->ptrinode->FileSize=UFTDArr[fd].ptrfiletable->ptrinode->FileSize + MAXFILESIZE;
    UFTDArr[fd].ptrfiletable->ptrinode->Buffer=(char *)realloc(UFTDArr[fd].ptrfiletable->ptrinode->Buffer, UFTDArr[fd].ptrfiletable->ptrinode->FileSize);
}

strncpy((UFTDArr[fd].ptrfiletable->ptrinode->Buffer) + (UFTDArr[fd].ptrfiletable->writeoffset), arr,isize);

(UFTDArr[fd].ptrfiletable->writeoffset) = (UFTDArr[fd].ptrfiletable->writeoffset) + isize;

(UFTDArr[fd].ptrfiletable->ptrinode->FileActualSize) = (UFTDArr[fd].ptrfiletable->ptrinode->FileActualSize) + isize;
return isize;
}



int OpenFile(char* name, int mode)
{
int i = 0;
PINODE temp = NULL;

if ((name == NULL) || (mode <= 0))
{
return -1;
}

temp = Get_Inode(name);

if (temp == NULL)
{
return -2;
}

if (temp->permission < mode)
{
return -3;
}

while (i < 50)
{
if (UFTDArr[i].ptrfiletable == NULL)
{
break;
}
i++;
}

UFTDArr[i].ptrfiletable = (PFILETABLE)malloc(sizeof(FILETABLE));
if (UFTDArr[i].ptrfiletable == NULL)
{
return -1;
}

UFTDArr[i].ptrfiletable->count = 1;
UFTDArr[i].ptrfiletable->mode = mode;
if (mode == READ + WRITE)
{
UFTDArr[i].ptrfiletable->readoffset = 0;
UFTDArr[i].ptrfiletable->writeoffset = 0;
}

else if (mode == READ)
{
UFTDArr[i].ptrfiletable->readoffset = 0;
}

else if (mode == WRITE)
{
UFTDArr[i].ptrfiletable->writeoffset = 0;
}

UFTDArr[i].ptrfiletable->ptrinode = temp;
(UFTDArr[i].ptrfiletable->ptrinode->ReferenceCount)++;

}

void CloseFileByFileDescriptor(int fd)
{
UFTDArr[fd].ptrfiletable->readoffset = 0;
UFTDArr[fd].ptrfiletable->writeoffset = 0;
(UFTDArr[fd].ptrfiletable->ptrinode->ReferenceCount)--;
}


int CloseFileByName(char* name)
{
int i = 0;
i = GetFDFromName(name);

if (i == -1)
{
return -1;
}

UFTDArr[i].ptrfiletable->readoffset = 0;
UFTDArr[i].ptrfiletable->writeoffset = 0;
(UFTDArr[i].ptrfiletable->ptrinode->ReferenceCount)--;
return 0;

}

void CloseAllFile()
{
int i = 0;
while (i < 5)
{
if (UFTDArr[i].ptrfiletable != NULL)
{
UFTDArr[i].ptrfiletable->readoffset = 0;
UFTDArr[i].ptrfiletable->writeoffset = 0;
//(UFTDArr[i].ptrfiletable->ptrinode->ReferenceCount)--;
UFTDArr[i].ptrfiletable->ptrinode->ReferenceCount=0;    
}
i++;
}
}


int LseekFile(int fd, int size, int from)
{
if ((fd < 0) || (from > 2))
{
return -1;
}

if (UFTDArr[fd].ptrfiletable == NULL)
{
return -1;
}

if ((UFTDArr[fd].ptrfiletable->mode == READ) || (UFTDArr[fd].ptrfiletable->mode == READ + WRITE))
{
if (from == CURRENT)
{
if (((UFTDArr[fd].ptrfiletable->readoffset) + size) > (UFTDArr[fd].ptrfiletable->ptrinode->FileActualSize))
{
return -1;
}

if (((UFTDArr[fd].ptrfiletable->readoffset) + size) < 0)
{
return -1;
}

(UFTDArr[fd].ptrfiletable->readoffset) = (UFTDArr[fd].ptrfiletable->readoffset) + size;

}
else if (from == START)
{
if (size > (UFTDArr[fd].ptrfiletable->ptrinode->FileActualSize))
{
return -1;
}

if (size < 0)
{
return -1;
}

(UFTDArr[fd].ptrfiletable->readoffset) = size;
}

else if (from == END)
{
if (((UFTDArr[fd].ptrfiletable->ptrinode->FileActualSize) + size) > UFTDArr[fd].ptrfiletable->ptrinode->FileSize)//MAXFILESIZE
{
return -1;
}

if (((UFTDArr[fd].ptrfiletable->readoffset) + size) < 0)
{
return -1;
}

(UFTDArr[fd].ptrfiletable->readoffset) = ((UFTDArr[fd].ptrfiletable->ptrinode->FileActualSize) + size);

}

}

else if ((UFTDArr[fd].ptrfiletable->mode == WRITE))
{
if (from == CURRENT)
{
if (((UFTDArr[fd].ptrfiletable->writeoffset) + size) > UFTDArr[fd].ptrfiletable->ptrinode->FileSize)//MAXFILESIZE
{
return -1;
}

if (size < 0)
{
return -1;
}

if (size > (UFTDArr[fd].ptrfiletable->ptrinode->FileActualSize))
{
(UFTDArr[fd].ptrfiletable->ptrinode->FileActualSize) = size;
}

(UFTDArr[fd].ptrfiletable->writeoffset) = size;
}

else if (from == START)
{
if (size > UFTDArr[fd].ptrfiletable->ptrinode->FileSize)//MAXFILESIZE
{
return -1;
}

if (size < 0)
{
return -1;
}

if (size > (UFTDArr[fd].ptrfiletable->ptrinode->FileActualSize))
{
(UFTDArr[fd].ptrfiletable->ptrinode->FileActualSize) = size;
}
(UFTDArr[fd].ptrfiletable->writeoffset) = size;
}

else if (from == END)
{
if (((UFTDArr[fd].ptrfiletable->ptrinode->FileActualSize) + size) > UFTDArr[fd].ptrfiletable->ptrinode->FileSize)//MAXFILESIZE
{
return -1;
}

if (((UFTDArr[fd].ptrfiletable->writeoffset) + size) < 0)
{
return -1;
}

(UFTDArr[fd].ptrfiletable->writeoffset) = ((UFTDArr[fd].ptrfiletable->ptrinode->FileActualSize) + size);
}

}
}


 void ls_file()
 {
int i = 0;
PINODE temp = head;

if (SUPERBLOCKobj.FreeInode == MAXINODE)
{
printf("ERROR : There are no files \n");
return;
}

printf("\n File Name \t Inode Number \t File Size \t Link Count \n ");
printf("----------------------------------------------------------\n");
while (temp != NULL)
{
if (temp->FileType != 0)
{
printf("%s\t\t%d\t\t%d\t\t%d\n", temp->FileName, temp->inodeNumber, temp->FileActualSize, temp->LinkCount);
}

temp = temp->next;

}

printf("--------------------------------------------------------------\n");
}

 int fstat_file(int fd)
 {
PINODE temp = head;
int i = 0;

if (fd < 0)
{
return -1;
}

if ((UFTDArr[fd].ptrfiletable) == NULL)
{
return -2;
}

temp = UFTDArr[fd].ptrfiletable->ptrinode;

printf("----------------Statistical  Information about the file---------------------\n");
printf("File Name %s\n", temp->FileName);
printf("Inode Number %d\n", temp->inodeNumber);
printf("File size %d\n", temp->FileSize);
printf("Actual File size %d\n", temp->FileActualSize);
printf("Link Count %d\n", temp->LinkCount);
printf("File size %d\n", temp->FileSize);
printf("Reference Count %d\n", temp->ReferenceCount);

if ((temp->permission) == 1)
{
printf("File Permisssion if Read Only\n");
}


else if ((temp->permission) == 2)
{
printf("File Permisssion if Write Only\n");
}

else if ((temp->permission) == 3)
{
printf("File Permisssion if Read & Write \n");
}

return 0;
 }


 int stat_file(char* name)
 {
PINODE temp = head;
int i = 0;

if (name == NULL)
{
return -1;
}

while (temp != NULL)
{
if (strcmp(name, temp->FileName) == 0)
{
break;
}
temp = temp->next;
}

if (temp == NULL)
{
return -2;
}

printf("----------------Statistical  Information about the file---------------------\n");
printf("File Name %s\n", temp->FileName);
printf("Inode Number %d\n", temp->inodeNumber);
printf("File size %d\n", temp->FileSize);
printf("Actual File size %d\n", temp->FileActualSize);
printf("Link Count %d\n", temp->LinkCount);
printf("File size %d\n", temp->FileSize);
printf("Reference Count %d\n", temp->ReferenceCount);

if ((temp->permission) == 1)
{
printf("File Permisssion if Read Only\n");
}


else if ((temp->permission) == 2)
{
printf("File Permisssion if Write Only\n");
}

else if ((temp->permission) == 3)
{
printf("File Permisssion if Read & Write \n");
}

return 0;
 }

 int truncate_File(char* name)
 {
int fd = GetFDFromName(name);
if (fd == -1)
{
return -1;
}

memset(UFTDArr[fd].ptrfiletable->ptrinode->Buffer, 0, MAXFILESIZE);
UFTDArr[fd].ptrfiletable->readoffset = 0;
UFTDArr[fd].ptrfiletable->writeoffset = 0;
UFTDArr[fd].ptrfiletable->ptrinode->FileActualSize = 0;
 }

void Save_all()
{
    int i=0,j=0,size;
    char name[]="/home/nav/Desktop/LB/savedvfs/";
    
    size=strlen(name);
    
    char *acname=(char *)malloc(50);
    
    strcpy(acname,name);
 
    char *buff=(char *)malloc(sizeof(char)*MAXFILESIZE);
    int iret=0,fd=0;
    i=0;
    
    while(i<MAXINODE)
    {
        if(UFTDArr[i].ptrfiletable!=NULL)
        {
            j=strlen(UFTDArr[i].ptrfiletable->ptrinode->FileName);
            strcpy((acname+size),UFTDArr[i].ptrfiletable->ptrinode->FileName);
            acname[size+j]='\0';
    
            fd=creat(acname,0777);
            
            UFTDArr[i].ptrfiletable->readoffset = 0;
            while((iret=ReadFile(i,buff,MAXFILESIZE))!=-3)
            {
                write(fd,buff,iret);
            }
          close(fd);      
        }
        i++;
    }
    free(buff);
    free(acname);
    printf("All files on virtual filesystem are saved in ./savedvfs \n");
}

void load_all()
{ 
    char name[]="/home/nav/Desktop/LB/savedvfs/";
    char *add=name;
    int fd=0,i=0,j=0,fd1=0,size=0;

    char *acname=(char *)malloc(sizeof(char)*100);
   
    i=strlen(name);
    strcpy(acname,name);
    memset(add,'\0',i);
    
    DIR *dir;
    
    dir=opendir("savedvfs");
    
    if(dir==NULL)
    {
        printf("cr \n");
        mkdir("savedvfs",0777);
        dir=opendir("savedvfs");
        
    }
    struct dirent *dirtable;
    dirtable=readdir(dir);
    
    while(dirtable!=NULL)
    {
        if(!(strcmp(dirtable->d_name,".")) || !(strcmp(dirtable->d_name,"..")))
        {
            //printf("hidden");
        }
        else
        {
            j=strlen(dirtable->d_name);
            //printf("%s\n",dirtable->d_name);
            strcpy((acname+i),dirtable->d_name);
            acname[i+j]='\0';
            //printf("%s\n",acname);
            
            CreateFile(dirtable->d_name,3);
            fd1=GetFDFromName(dirtable->d_name);
            fd=open(acname,O_RDONLY);
            
            
            if(fd==-1)
                printf("error");
        
            while((size=read(fd,add,10))!=0)
            {
                WriteFile(fd1,add,size);
            }
            close(fd);
            unlink(acname);
        }
        dirtable=readdir(dir);
    }
    
    free(acname);
    
}


int main()
{
    char *ptr = NULL;
    int ret=0,fd=0,count=0;
    char command[4][80],str[80],arr[1024];
    
    InitialiseSuperBlock();
    CreateDILB();
   
    load_all();
    
    while(1)
    {
        fflush(stdin);
        strcpy(str,"");
        
        printf("\nFor Help use : --help \n");
        printf("\nCustomized VFS : >");
        fgets(str,80,stdin);
        
        count=sscanf(str,"%s %s %s %s",command[0],command[1],command[2],command[3]);
        
        if(count==1)
        {
            if(strcmp(command[0],"ls")==0)
            {
                ls_file();
            }
            
            else if(strcmp(command[0],"closeall")==0)
            {
                CloseAllFile();
                printf("All files closed successfully\n");
                continue;
            }
            
            else if(strcmp(command[0],"clear")==0)
            {
                system("clear");
                continue;
            }
            
            else if(strcmp(command[0],"--help")==0)
            {
                DisplayHelp();
                continue;
            }
            
            else if(strcmp(command[0],"exit")==0)
            {
                Save_all();
                Free_all();
                printf("Terminating the Customized Virtual File system\n");
                break;
            }
            
            else
            {
                printf("\n error : command not found!!!\n");
                continue;
            }
            
        }
        
        else if(count==2)
        {
            if(strcmp(command[0],"stat")==0)
            {
                ret=stat_file(command[1]);
                if(ret==-1)
                    printf("Error : Incorrect parameters\n");
                if(ret==-2)
                    printf("Error : There is no such file\n");
                continue;
            }
            
            else if(strcmp(command[0],"close")==0)
            {
                ret=CloseFileByName(command[1]);    //close Demo.txt
                if(ret==-1)
                {
                    printf("Error : there is no such file\n");
                }
                continue;
            }
            
            else if(strcmp(command[0],"rm")==0)
            {
                ret=rm_File(command[1]);    //rm demo.txt
                if(ret==-1)
                    printf("Error : There is no such file\n");
                continue;
            }
            
            else if(strcmp(command[0],"man")==0)
            {
                man(command[1]);
            }
            
            else if(strcmp(command[0],"write")==0)
            {
                fflush(stdin);
                fd=GetFDFromName(command[1]);
                if(fd == -1)
                {
                    printf("Error : Incorrect parameters\n");
                    continue;
                }
                printf("Enter the data : \n");
                scanf("%[^\n]",arr);
                
                ret=strlen(arr);
                if(ret==0)
                {
                    printf("Error : Incorrect parameter\n");
                    continue;
                }
                
                ret=WriteFile(fd,arr,ret);
                
                if(ret==-1)
                    printf("Error : Permission denied\n");
                
                if(ret==-2)
                    printf("Error : There is no sufficient memory to write\n");
                
                if(ret==-3)
                    printf("Error : It is not regular file\n");
                
                if(ret==-4)
                    printf("Error : There is no sufficient memory available");
            }
            
            else if(strcmp(command[0],"truncate")==0)
            {
                ret=truncate_File(command[1]);
                if(ret==-1)
                    printf("Error : Incorrect parameter\n");
            }
            else
            {
                printf("\n Error : Command not found!!!\n");
            }
            
        }
        
        else if(count==3)
        {
            if(strcmp(command[0],"create")==0)
            {
        
                ret=CreateFile(command[1],atoi(command[2]));
                if(ret>=0)
                    printf("File is successfully created with file descriptor : %d \n",ret);
                if(ret==-1)
                    printf("Error : Incorect parameters\n");
                if(ret==-2)
                    printf("Error : There is no inodes\n");
                if(ret==-3)
                    printf("Error : File already exists\n");
                if(ret==-4)
                    printf("Error : Memory allocation failure\n");
                continue;
            }
            else if(strcmp(command[0],"open")==0)
            {
                ret=OpenFile(command[1],atoi(command[2]));
                if(ret>=0)
                    printf("File is successfully opened with file descriptor : %d \n",ret);
                if(ret==-1)
                    printf("Error : Incoorect parameters\n");
                if(ret==-2)
                    printf("Error : File not present\n");
                if(ret==-3)
                    printf("Error : Permission Denied\n");
                continue;
            }
            else if(strcmp(command[0],"read")==0)
            {
                fd=GetFDFromName(command[1]);
                if(fd==-1)
                {
                    printf("Error : Incorrect parameter\n");
                    continue;
                }
                ptr=(char *)malloc(sizeof(atoi(command[2]))+1);
                
                if(ptr==NULL)
                {
                    printf("Error : Memory Allocation failure\n");
                    continue;
                }
                ret=ReadFile(fd,ptr,atoi(command[2]));
                if(ret==-1)
                    printf("Error : File Not existing\n");
                if(ret==-2)
                    printf("Error : Permission Denied\n");
                if(ret==-3)
                    printf("Error : Reached at end of file\n");
                if(ret==-4)
                    printf("Error : It is not regular file\n");
                if(ret==0)
                    printf("Error : File empty\n");
                if(ret>0)
                {
                    write(2,ptr,ret);
                }
                free(ptr);
                ptr=NULL;
                continue;
                
            }
            else
            {
                printf("\nError : Command not found!!!\n");
                continue;
            }
        }
        
        else if(count==4)
        {
            if(strcmp(command[0],"lseek")==0)
            {
                fd=GetFDFromName(command[1]);
                if(fd==-1)
                {
                    printf("Error : Incorrect parameter\n");
                    continue;
                }
                ret=LseekFile(fd,atoi(command[2]),atoi(command[3]));
                if(ret==-1)
                {
                    printf("Unable to perform lseek\n");
                }
            }
            else
            {
                printf("\nError : Command not found!!!\n");
                continue;
            }
        }
        
        else
        {
            printf("\n Error : Command not found!!!");
            continue;
        }
        
    }
    return 0;
}
