#define FUSE_USE_VERSION 28
#define HAVE_SETXATTR

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

static const char *dirpath = "/home/administrator/Documents/";	//JANGAN LUPA GANTI NAMA USER

static int xmp_getattr(const char *path, struct stat *stbuf)
{
	int res;
	char fpath[1000];

	sprintf(fpath,"%s%s",dirpath, path);
	res = lstat(fpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

	char fpath[1000];

	sprintf(fpath,"%s%s",dirpath, path);

	dp = opendir(fpath);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd;
	int res;
	char new_str[100],dir[100];
	char fpath[1000],gpath[1000];

	sprintf(fpath,"%s%s",dirpath, path);	

	(void) fi;
	if (!strcmp(strrchr(fpath, '\0') - 4, ".txt") || !strcmp(strrchr(fpath, '\0') - 4, ".doc") || !strcmp(strrchr(fpath, '\0') - 4, ".pdf")){
		system("notify-send \"Terjadi kesalahan! File berisi konten berbahaya.\"");
		
		struct stat st = {0};

		if (stat("/home/administrator/Documents/rahasia", &st) == -1) {
   			mkdir("/home/administrator/Documents/rahasia", 0777); //buat directory jika belum ada
		}

		strncpy(dir, dirpath, strlen(dirpath));
		strcat(dir, "rahasia/");

		sprintf(gpath,"%s%s",dir, path);

		strncpy(new_str, gpath, strlen(gpath));
		strcat(new_str, ".ditandai");
		
		int status;
		status = rename(fpath,new_str);					//ganti tempat
		if(status == 0 ) printf("%s\n",new_str );		//debug
		else printf("%s\n%s\n%s\n",gpath,new_str,dir );	//debug

		chmod(new_str,0000); //ganti permission
		return 0;

	}
	else{
		fd = open(path, O_RDONLY);
		if (fd == -1)
			return -errno;

		res = pread(fd, buf, size, offset);
		if (res == -1)
			res = -errno;

		close(fd);
		return res;
	}
}

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.readdir 	= xmp_readdir,
	.read 		= xmp_read,
};

int main(int argc, char *argv[])
{
	umask(0);
	return fuse_main(argc, argv, &xmp_oper, NULL);
}