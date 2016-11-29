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

#define DEBUG 0

typedef struct item{
	struct stat *details;
	struct item *sibling;
	struct item *subDir;
	struct item *supDir;
	char *name;
	char *location;
	char *data;
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
static int fs_utimens(const char * path, const struct timespec tv[2]);
static int fs_chmod(const char *path, mode_t mode);
static int fs_chown(const char *path, uid_t uid, gid_t gid);
static int fs_truncate (const char * path , off_t offset);
static int fs_rename(const char *src, const char *dest);

item* getParent(item *temp, char *path){
	if (temp==NULL)
	{
		return NULL;
	}
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
				return NULL;
			}
		}
		getParent(temp->sibling,path);
	}
}

static int fs_getattr(const char *path, struct stat *stbuf){
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
		item *node=getParent(head,p);
		if (node!=NULL)
		{
			if (node->isFile==1)
			{
				stbuf->st_mode= S_IFREG | 0666;
				stbuf->st_nlink=1;
			}
			else
			{
				stbuf->st_mode= S_IFDIR | 0755;
				stbuf->st_nlink=2;
			}
			stbuf->st_size=node->details->st_size;
		}
		else
		{
			return -ENOENT;
		}
	}
	return 0;
}

static int fs_opendir(const char *path, struct fuse_file_info *fi){
	return 0;
}

static int fs_truncate (const char * path , off_t offset){
	return 0;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi){
	filler( buf, ".", NULL, 0 ); 
	filler( buf, "..", NULL, 0 );
	item *nodePrint;
	char *p=strdup(path), *file_name;
	if (path==NULL)
	{
		return -EPERM;
	}

	nodePrint=getParent(head, p);

	if (nodePrint==NULL)
	{
		return -ENOENT;
	}
	item *temp=nodePrint->subDir;
	while(temp!=NULL)
	{
		file_name=strndup(temp->name,strlen(temp->name));
		filler(buf, file_name, NULL, 0);
		temp=temp->sibling;
	}

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
	newNode->data=NULL;
	newNode->isFile=0;

	newNode->details->st_mode = mode;
	newNode->details->st_nlink = 2;
	newNode->details->st_uid=getuid();
	newNode->details->st_gid=getgid();

	newNode->name=strndup(Dir,strlen(Dir));
	newNode->location=strndup(temp1,strlen(temp1));

	parentNode=getParent(head,dirName);

	if (parentNode==NULL)
	{
		exit(EXIT_FAILURE);
	}
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

	fsinfo->freeBytes = fsinfo->freeBytes - sizeof(item) - sizeof(stat);
	if (fsinfo->freeBytes < 0)
	{//no space
		return -ENOSPC;
	}
	return 0;
}

static int fs_unlink(const char *path){
	if (path==NULL || strcmp(path,"/")==0)
	{
		return -EPERM;
	}
	item *node, *parNode, *temp, *prev;
	char *p=strdup(path);
	node=getParent(head,p);
	parNode=node->supDir;
	temp=parNode->subDir;
	if (strcmp(temp->name,node->name)==0)
	{
		if (node->sibling==NULL)
		{
			parNode->subDir=NULL;
		}
		else
		{
			parNode->subDir=node->sibling;
		}
	}
	else
	{
		prev=temp;
		while(strcmp(temp->name,node->name)!=0)
		{
			prev=temp;
			temp=temp->sibling;
		}
		prev->sibling=node->sibling;
	}
	long int dataSize;
	if (node->data!=NULL)
	{
		dataSize=strlen(node->data);
	}
	fsinfo->freeBytes=fsinfo->freeBytes + sizeof(item) + sizeof(stat) + dataSize;
	free(node->name);
	free(node->location);
	free(node->details);
	free(node);
	return 0;
}

static int fs_rmdir(const char *path){
	if (path==NULL || strcmp(path,"/")==0)
	{
		return -EPERM;
	}
	item *node, *parNode, *temp, *prev;
	char *p=strdup(path);
	node=getParent(head,p);

	if (node->subDir!=NULL)
	{
		return -ENOTEMPTY;
	}
	parNode=node->supDir;
	temp=parNode->subDir;
	if (strcmp(temp->name,node->name)==0)
	{
		if (node->sibling==NULL)
		{
			parNode->subDir=NULL;
		}
		else
		{
			parNode->subDir=node->sibling;
		}
	}
	else
	{
		prev=temp;
		while(strcmp(temp->name,node->name)!=0)
		{
			prev=temp;
			temp=temp->sibling;
		}
		prev->sibling=node->sibling;
	}
	fsinfo->freeBytes=fsinfo->freeBytes + sizeof(item) + sizeof(stat);
	free(node->name);
	free(node->location);
	free(node->details);
	free(node);
	return 0;
}

static int fs_open(const char *path, struct fuse_file_info *fi){
	char *p=strdup(path);
	if(getParent(head,p) != NULL)
		return 0;
	else
		return -ENOENT;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi){
	char *p=strdup(path);
	item *node;
	int length;
	node=getParent(head, p);
	if (node->isFile==0)
	{
		return -EISDIR;
	}
	if (node->data!=NULL)
	{
		length =strlen(node->data);
		if (offset < length)
		{
			if (offset + size > length)
				size = length - offset;
			memcpy(buf, (node->data) + offset, size);
			buf[size] = '\0';
		}
		else
			size = 0;
	}
	else
		size=0;

	return size;
}

static int fs_write(const char *path, const char *buf, size_t size,off_t offset, struct fuse_file_info *fi){
	if (size > fsinfo->freeBytes)
	{
		return -ENOSPC;
	}

	char *p=strdup(path);
	item *node;
	int length;
	node=getParent(head, p);
	if (node->isFile==0)
	{
		return -EISDIR;
	}
	if (node->data == NULL)
	{
		length=0;
	}
	else
	{
		length = strlen(node->data);
	}
	if (size > 0)
	{
		if (length != 0)
		{
			if (offset > length)
			{
				offset = length;
			}
			char *copy=(char *)realloc(node->data, sizeof(char) * (offset + size));
			if (copy==NULL)
			{
				return -ENOSPC;
			}
			else{
				node->data=copy;
				memcpy((node->data) + offset, buf, size);
				node->details->st_size= (offset + size);
				node->details->st_ctime=time(NULL);
				node->details->st_mtime=time(NULL);
				fsinfo->freeBytes = fsinfo->freeBytes - size -offset;
			}
		}
		else
		{
			offset=0;
			node->data=(char *)malloc(sizeof(char) * size);
			memcpy((node->data) + offset, buf, size);
			node->details->st_size=(offset + size);
			node->details->st_ctime=time(NULL);
			node->details->st_mtime=time(NULL);
			fsinfo->freeBytes = fsinfo->freeBytes - size;
		}
	}
	return size;
}

static int fs_utimens(const char * path, const struct timespec tv[2]){
	return 0;
}

static int fs_chmod(const char *path, mode_t mode){
	return 0;
}
static int fs_chown(const char *path, uid_t uid, gid_t gid){
	return 0;
}

static int fs_create(const char *path , mode_t mode, struct fuse_file_info *fi){
	if ((fsinfo->freeBytes- sizeof(item) - sizeof(stat)) < 0)
	{//no space
		return -ENOSPC;
	}

	char temp1[1024], temp2[1024], *dirName, *Dir;
	strcpy(temp1,path);
	strcpy(temp2,path);
	Dir=basename(temp1);
	dirName=dirname(temp2);

	item *parentNode;

	item *newNode = (item *)malloc(sizeof(item));
	newNode->details=(struct stat *)malloc(sizeof(struct stat));

	newNode->sibling=NULL;
	newNode->data=NULL;
	newNode->subDir=NULL;
	newNode->isFile=1;

	newNode->details->st_mode = mode;
	newNode->details->st_nlink = 1;
	newNode->details->st_uid=getuid();
	newNode->details->st_gid=getgid();

	newNode->name=strndup(Dir,strlen(Dir));
	newNode->location=strndup(temp1,strlen(temp1));

	parentNode=getParent(head,dirName);
	if (parentNode==NULL)
	{
		exit(EXIT_FAILURE);
	}
	newNode->supDir=parentNode;
	//parentNode->details->st_nlink+=1;

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

	fsinfo->freeBytes = fsinfo->freeBytes - sizeof(item) - sizeof(stat);
	return 0;
}

void setDetails(item **dest, item *src)
{
	(*dest)->details->st_atime = src->details->st_atime;
	(*dest)->details->st_mtime = src->details->st_mtime;
	(*dest)->details->st_ctime = src->details->st_ctime;
	
	(*dest)->details->st_mode = src->details->st_mode;
	(*dest)->details->st_nlink = src->details->st_nlink;
	
	(*dest)->details->st_uid = src->details->st_uid;
	(*dest)->details->st_gid = src->details->st_gid;
}

static int fs_rename(const char *src, const char *dest)
{
	char *s=strdup(src);
	char *d=strdup(dest);
	item *from=getParent(head,s);
	item *to=getParent(head,d);
	if (from==NULL)
	{
		return -ENOENT;
	}
	if (to==NULL)//destination not present, we will create one then
	{

		if (from->isFile==1)
		{
			fs_create(d,from->details->st_mode, NULL);
			to=getParent(head,d);
			setDetails(&to, from);
			fs_unlink(s);
			return 0;
		}
		else if (from->isFile==0)
		{
			fs_mkdir(d, from->details->st_mode);
			to=getParent(head,d);
			setDetails(&to, from);
			to->details->st_size=from->details->st_size;
			if(from->details->st_size > 0) {
				to->data = (char *)malloc(sizeof(char)* from->details->st_size);
			if(to->data) {
				strcpy(to->data, from->data);
				fsinfo->freeBytes = fsinfo->freeBytes - from->details->st_size;
			}
			else
				return -ENOSPC;
}
			fs_unlink(s);
			fs_rmdir(s);
			return 0;
		}
		else
			return -ENOENT;
	}
	else//destination present
	{
		if (to->isFile==1)
		{
			setDetails(&to, from);
		}
	}
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
    .utimens 	= 	fs_utimens,
    .chmod      = 	fs_chmod,
	.chown 		= 	fs_chown,
	.truncate	= 	fs_truncate,
	.rename		= 	fs_rename,
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
	
	initFuse(argv);
#if DEBUG
	printf("Mountpoint: %s\n", fsinfo->mountpoint);
#endif
	argc--;
	return fuse_main(argc, argv, &fuseOps, NULL);
}