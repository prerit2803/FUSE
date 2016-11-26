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

void Display()
{
	item *temp=head;
	fprintf(stdout, "%s\n", temp->name);
	temp=temp->subDir;
	while(temp!=NULL)
	{
		fprintf(stdout, "%s\n", temp->name);
		temp=temp->sibling;
	}
}

item* getParent(item *temp, char *path){
	fprintf(stdout, "in getParent for path: %s with temp: %s\n Node structure: ", path,temp->name);
	Display();
	if (temp==NULL)
	{fprintf(stdout, "in null of getParent\n");
		return NULL;
	}
	if (strcmp(temp->location,path)==0)
	{fprintf(stdout, "in strcmp of getParent: %s, %s\n",temp->name, temp->location);
		return temp;
	}
	else if (temp->subDir!=NULL)
	{fprintf(stdout, "in child of getParent\n");
		getParent(temp->subDir,path);
	}
	else if (temp->sibling!=NULL)
	{fprintf(stdout, "in sibling of getParent\n");
		getParent(temp->sibling,path);
	}
	else
	{fprintf(stdout, "in finding parent with not null of getParent\n");
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
}

static int fs_getattr(const char *path, struct stat *stbuf){
	fprintf(stdout, "getattr path: %s\n", path);
	char *p;
	p=strdup(path);
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
		fprintf(stdout, "in else of getattr\n");
		item *node=getParent(head,p);
		//fprintf(stdout, "Parent in getattr: %s\n", node->name);
		if (node!=NULL)
		{
			if (node->isFile==1)
			{fprintf(stdout, "is File\n");
				stbuf->st_mode= S_IFREG | 0755;
				stbuf->st_nlink=1;
			}
			else
			{fprintf(stdout, "is Dir\n");
				stbuf->st_mode= S_IFDIR | 0755;
				stbuf->st_nlink=2;
			}
			stbuf->st_size=1024;
		}
		else
		{fprintf(stdout, "Parent is null of getattr\n");
			return -ENOENT;
		}
	}
	fprintf(stdout, "exiting getattr\n");
	Display();
	//free(p);
	return 0;
}

static int fs_opendir(const char *path, struct fuse_file_info *fi){
	return 0;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi){
	filler( buf, ".", NULL, 0 ); 
	filler( buf, "..", NULL, 0 );
	item *nodePrint;
	char *p=strdup(path), *file_name;
	if (path==NULL)
	{fprintf(stdout, "in path null of readdir\n");
		return -EPERM;
	}

	fprintf(stdout, "in readdir and Node struct\n");
	Display();
	nodePrint=getParent(head, p);
	fprintf(stdout, "after getting parent in readdir: %s\n", nodePrint->name);
	if (nodePrint==NULL)
	{
		return -ENOENT;
	}
	item *temp=nodePrint->subDir;
	while(temp!=NULL)
	{fprintf(stdout, "temp is not null goes into while of readdir\n");
fprintf(stdout, "Path parent: %s\n", temp->name);
		file_name=strndup(temp->name,strlen(temp->name));
		fprintf(stdout, "Name: %s\n", file_name);
		filler(buf, file_name, NULL, 0);
		fprintf(stdout, "Name after filler: %s\n", file_name);
		//free(file_name);
		temp=temp->sibling;
	}
	fprintf(stdout, "temp is null now of readdir and Node struct\n");
	Display();
	//free(p);
	fprintf(stdout, "after free in readdir\n");
	return 0;

}

static int fs_mkdir(const char *path, mode_t mode){
	if ((fsinfo->freeBytes- sizeof(item) - sizeof(stat)) < 0)
	{//no space
		return -ENOSPC;
	}
	printf("in mkdir\n");
	char temp1[1024], temp2[1024], *dirName, *Dir;
	strcpy(temp1,path);
	strcpy(temp2,path);
	Dir=basename(temp1);
	dirName=dirname(temp2);

	item *parentNode;

	item *newNode = (item *)malloc(sizeof(item));
	newNode->details=(struct stat *)malloc(sizeof(struct stat));

	newNode->sibling=NULL;
	newNode->subDir=NULL;
	newNode->isFile=0;

	newNode->details->st_mode = mode;
	newNode->details->st_nlink = 2;
	newNode->details->st_uid=getuid();
	newNode->details->st_gid=getgid();

	newNode->name=strndup(Dir,strlen(Dir));
	newNode->location=strndup(temp1,strlen(temp1));

	parentNode=getParent(head,dirName);
	fprintf(stdout, "Parent in mkdir: %s\n", parentNode->name);
	if (parentNode==NULL)
	{
		printf("Parent Node does not exist\n");
		exit(EXIT_FAILURE);
	}
	newNode->supDir=parentNode;
	parentNode->details->st_nlink+=1;

	if (parentNode->subDir==NULL)
	{fprintf(stdout, "first child\n");
		parentNode->subDir=newNode;
	}
	else
	{fprintf(stdout, "traversing node\n");
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
	return size;
}

static int fs_write(const char *path, const char *buf, size_t size,off_t offset, struct fuse_file_info *fi){
	return 0;
}

static int fs_create(const char *path , mode_t mode, struct fuse_file_info *fi){
	return 0;
}

static int initFuse(char *argv[]){
	item *rootNode=(item *)malloc(sizeof(item));
	rootNode->details=(struct stat *)malloc(sizeof(stat));

	rootNode->sibling=NULL;
	rootNode->subDir=NULL;
	rootNode->supDir=NULL;
	rootNode->isFile=0;
	rootNode->location = strdup("/");
fprintf(stdout, "Root name: %s\n", rootNode->name);
#if DEBUG
	printf("Mountpoint of init: %s\n", rootNode->location);
#endif

	rootNode->details->st_mode = S_IFDIR | 0755;
	rootNode->details->st_nlink = 2;
	rootNode->details->st_uid=0;
	rootNode->details->st_gid=0;
	rootNode->location = strdup("/");
	rootNode->name = strndup(argv[1],strlen(argv[1]));
	fsinfo->freeBytes = fsinfo->totalSize - sizeof(item) - sizeof(stat);
	if (fsinfo->freeBytes < 0)
	{//no space
		return -ENOSPC;
	}
	head=rootNode;
	fprintf(stdout, "head: %s, %s\n", head->name, head->location);
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
		//exit(EXIT_FAILURE);
	}
	fsinfo = (FSInfo *)malloc(sizeof(FSInfo));
	fsinfo->totalSize=((long)atoi(argv[2])) * 1024 * 1024;
	fsinfo->freeBytes=((long)atoi(argv[2])) * 1024 * 1024;
	fsinfo->NumberOfDir=0;
	fsinfo->NumberOfFiles=0;
	fsinfo->mountpoint=strndup(argv[1],strlen(argv[1]));
	
	initFuse(argv);
#if DEBUG
	printf("Mountpoint: %s\n", fsinfo->mountpoint);
#endif
	argc--;
	return fuse_main(argc, argv, &fuseOps, NULL);
}