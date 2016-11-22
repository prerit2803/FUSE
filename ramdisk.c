#include <stdio.h>
#include <string.h>
#include <fuse.h>

typedef struct item{
	struct stat *details;
	struct item *sibling;
	struct item *subDir;
	char *name;
	char *location;
	int isFile;
	char *content;
}item;

typedef struct fileSystemInfo{
	long int totalSize;
	long int freeBytes;
	long int availableBytes;
	long int NumberOfFiles;
	long int NumberOfDir;
	char *mountpoint;
}*FSInfo;

item *head=NULL;

void initFuse(char *mountpoint){
	item *rootNode=(item *)malloc(sizeof(item));
	rootNode->sibling=NULL;
	rootNode->subDir=NULL;
	rootNode->content=NULL;
	rootNode->isFile=0;
	strcpy(rootNode->name,mountpoint);
	strcpy(rootNode->location,mountpoint);

}

static struct fuse_ops fuse_ops = {
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
	FSInfo *fsinfo = (FSInfo *)malloc(sizeof(FSInfo));
	fsinfo->totalSize=(long)atoi(argv[2]) * 1024 * 1024;
	fsinfo->freeBytes=(long)atoi(argv[2]) * 1024 * 1024;
	fsinfo->availableBytes=0;
	fsinfo->NumberOfDir=0;
	fsinfo->NumberOfFiles=0;
	strcpy(fsinfo->mountpoint,argv[1]);
	initFuse(argv[1]);
	return fuse_main(argc-1, argv, &fuse_ops, NULL);
}