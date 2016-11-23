#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <fuse.h>
#include <unistd.h>
#include <sys/types.h>
#include <libgen.h>

#define DEBUG 1

typedef struct item{
	struct stat *details;
	struct item *sibling;
	struct item *subDir;
	struct item *supDir;
	char *name;
	char *location;
	int isFile;
}item;

typedef struct fileSystemInfo{
	long int totalSize;
	long int freeBytes;
	long int NumberOfFiles;
	long int NumberOfDir;
	char *mountpoint;
}FSInfo;

item *head=NULL;
FSInfo *fsinfo=NULL;

static int fs_getattr(const char *path, struct stat *stbuf);
static int fs_opendir(const char *path, struct fuse_file_info *fi);
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi);
static int fs_mkdir(const char *path, mode_t mode);
static int fs_unlink(const char *path);
static int fs_rmdir(const char *path);
static int fs_open(const char *path, struct fuse_file_info *fi);
static int fs_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi);
static int fs_write(const char *path, const char *buf, size_t size,off_t offset, struct fuse_file_info *fi);
static int fs_create(const char *path , mode_t mode, struct fuse_file_info *fi);

item* getParent(item *temp, char *path){
	if (strcmp(temp->location,path)==0)
	{
		return temp;
	}
	else if (temp->subDir!=NULL)
	{
		getParent(temp->subDir,path);
	}
	else if (temp->sibling!=NULL)
	{
		getParent(temp->sibling,path);
	}
	else
	{
		while(temp->sibling==NULL)
		{
			temp=temp->supDir;
			if (temp==NULL)
			{
				printf("Cannot find the path requested\n");
				return NULL;
			}
		}
		getParent(temp->sibling,path);
	}
	return NULL;
}

static int fs_getattr(const char *path, struct stat *stbuf){
	stbuf->st_uid=getuid();
	stbuf->st_gid=getgid();
	stbuf->st_atime=time(NULL);
	stbuf->st_mtime=time(NULL);

	if (strcmp(path,"/")==0)
	{
		stbuf->st_mode= S_IFDIR | 0755;
		stbuf->st_nlink=2;
	}
	else
	{
		stbuf->st_mode= S_IFREG | 0644;
		stbuf->st_nlink=1;
		stbuf->st_size=1024;
	}
	return 0;
}

static int fs_opendir(const char *path, struct fuse_file_info *fi){
	return 0;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi){
	return 0;
}

static int fs_mkdir(const char *path, mode_t mode){
	if ((fsinfo->freeBytes- sizeof(item) - sizeof(stat)) < 0)
	{//no space
		return -ENOSPC;
	}
	char *temp1, *temp2, *dirName;
	temp1=strdup(path);
	temp2=strdup(path);
	dirName=dirname(temp2);
	item *parentNode;
#if DEBUG
	printf("Path: %s\n", path);
#endif

	item *newNode = (item *)malloc(sizeof(item));
	newNode->details=(struct stat *)malloc(sizeof(stat));

	newNode->sibling=NULL;
	newNode->subDir=NULL;
	newNode->isFile=0;

	strcpy(newNode->name,basename(temp1));
	strcpy(newNode->location,dirName);

	newNode->details->st_mode = mode;
	newNode->details->st_nlink = 2;
	newNode->details->st_uid=getuid();
	newNode->details->st_gid=getgid();

	parentNode=getParent(head,dirName);
	newNode->supDir=parentNode;
	parentNode->details->st_nlink+=1;

	if (parentNode->subDir==NULL)
	{
		parentNode->subDir=newNode;
	}
	else
	{
		parentNode=parentNode->subDir;
		while(parentNode->sibling!=NULL)
			parentNode=parentNode->sibling;

		parentNode->sibling=newNode;
	}

	fsinfo->freeBytes = fsinfo->totalSize - sizeof(item) - sizeof(stat);
	if (fsinfo->freeBytes < 0)
	{//no space
		return -ENOSPC;
	}
	return 0;
}

static int fs_unlink(const char *path){
	return 0;
}

static int fs_rmdir(const char *path){
	return 0;
}

static int fs_open(const char *path, struct fuse_file_info *fi){
	return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi){
	return 0;
}

static int fs_write(const char *path, const char *buf, size_t size,off_t offset, struct fuse_file_info *fi){
	return 0;
}

static int fs_create(const char *path , mode_t mode, struct fuse_file_info *fi){
	return 0;
}

static int initFuse(char *mountpoint){

	item *rootNode=(item *)malloc(sizeof(item));
	rootNode->details=(struct stat *)malloc(sizeof(stat));

	rootNode->sibling=NULL;
	rootNode->subDir=NULL;
	rootNode->supDir=NULL;
	rootNode->isFile=0;
	rootNode->name = strndup(mountpoint,strlen(mountpoint));
	rootNode->location = strndup(mountpoint,strlen(mountpoint));
	//strcpy(rootNode->location, "/");
	//strcat(rootNode->location,mountpoint);
#if DEBUG
	printf("Mountpoint: %s\n", rootNode->location);
#endif
	rootNode->details->st_mode = S_IFDIR | 0755;
	rootNode->details->st_nlink = 2;
	rootNode->details->st_uid=0;
	rootNode->details->st_gid=0;

	fsinfo->freeBytes = fsinfo->totalSize - sizeof(item) - sizeof(stat);
	if (fsinfo->freeBytes < 0)
	{//no space
		return -ENOSPC;
	}
	head=rootNode;
	return 0;
}

static struct fuse_operations fuseOps = {
    .getattr    =   fs_getattr,
    .readdir    =   fs_readdir,
    .mkdir      =   fs_mkdir,
    .create     =   fs_create,
    .open       =   fs_open,
    .read       =   fs_read,
    .rmdir      =   fs_rmdir,
    .opendir    =   fs_opendir,
    .unlink     =   fs_unlink,
    .write      =   fs_write,
};

int main(int argc, char *argv[]){
	if (argc!=3)
	{
		printf("USAGE ERROR: ./ramdisk <mount point> <size>\n");
		exit(EXIT_FAILURE);
	}
	fsinfo = (FSInfo *)malloc(sizeof(FSInfo));
	fsinfo->totalSize=((long)atoi(argv[2])) * 1024 * 1024;
	fsinfo->freeBytes=((long)atoi(argv[2])) * 1024 * 1024;
	fsinfo->NumberOfDir=0;
	fsinfo->NumberOfFiles=0;
	fsinfo->mountpoint=strndup(argv[1],strlen(argv[1]));
	
#if DEBUG
	printf("Mountpoint: %s\n", fsinfo->mountpoint);
#endif
	initFuse(argv[1]);
	argc--;
	return fuse_main(argc, argv, &fuseOps, NULL);
}