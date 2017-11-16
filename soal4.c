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

static const char *dirpath = "/home/khawari/Downloads/"; 	//JANGAN LUPA GANTI NAMA USER
static const char *dir = "/home/khawari/Downloads/simpanan"; 	//JANGAN LUPA GANTI NAMA USER

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

#define BUFF_SIZE 1024

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd;
	int res;
	char fpath[1000];
	printf("read\n");

	sprintf(fpath,"%s%s",dirpath, path);	
	printf("%s\n",fpath );
	(void) fi;

	if (!strcmp(strrchr(fpath, '\0') - 5, ".copy")){
		system("notify-send \"Terjadi kesalahan! File berisi konten berbahaya.\"");
		return 0;
	}
	else{
		fd = open(fpath, O_RDONLY);
		if (fd == -1)
			return -errno;


		res = pread(fd, buf, size, offset);
		if (res == -1)
			res = -errno;

		close(fd);
		return res;
	}
}

static int xmp_truncate(const char *path, off_t size)
{
	int res;

	char fpath[1000],dir[1000],gpath[1000];

	sprintf(fpath,"%s%s",dirpath, path);	

	strncpy(dir, dirpath, 	strlen(dirpath));
	strcat(dir, "simpanan/");


	sprintf(gpath,"%s%s.copy",dirpath, path);

		struct stat st = {0};
	if (stat("/home/khawari/Downloads/simpanan", &st) == -1) {
   			mkdir("/home/khawari/Downloads/simpanan", 0777); //buat directory jika belum ada
		}

	//-------------------------------------------------------
    int files[2];
    int nbread;
    char *buff[BUFF_SIZE];

    /* Check for insufficient parameters */
    files[0] = open(fpath, O_RDONLY);
    if (files[0] == -1) /* Check if file opened */
        return -1;
    files[1] = open(gpath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (files[1] == -1) /* Check if file opened (permissions problems ...) */
    {
        close(files[0]);
        return -1;
    }

	while((nbread = read(files[0],buff,BUFF_SIZE)) > 0)
	{
		printf("%d\n",nbread );
		if(write(files[1],buff,nbread) != nbread)
			printf("\nError in writing data to %s\n",gpath);
	}
	//-------------------------------------------------------------


	printf("%s\ntruncate\n",fpath);
	res = truncate(gpath, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;
		printf("write\n");
	char fpath[1000],gpath[1000],hpath[1000];

//	strncpy(dir, dirpath, 	strlen(dirpath));
//	strcat(dir, "simpanan/");

	sprintf(fpath,"%s%s.copy",dirpath, path);

	(void) fi;
	fd = open(fpath, O_WRONLY );
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	sprintf(gpath,"%s%s.copy",dir, path);

	int status;
	status = rename(fpath,gpath);					//ganti tempat
	if(status == 0 ) printf("%s\n",gpath );			//debug
	else printf("%s\n%s\n%s\n",fpath,dir,path );	//debug

	sprintf(hpath,"%s%s",dirpath, path);

//--------------check file yang disave apakah berubah atau tidak---------------------------

	FILE *fp1, *fp2;
	int ch1, ch2;
 
	fp1 = fopen(hpath, "r");
	fp2 = fopen(gpath, "r");
 
	if (fp1 == NULL) {
    	printf("Cannot open %s for reading ", hpath);
    	
   	} else if (fp2 == NULL) {
    	printf("Cannot open %s for reading ", gpath);
    	
   	} else {
    	ch1 = getc(fp1);
    	ch2 = getc(fp2);
 		
    	printf("cek\n");

    	while ((ch1 != EOF) && (ch2 != EOF) && (ch1 == ch2)) {
        	ch1 = getc(fp1);
        	ch2 = getc(fp2);
      	}

 //--------jika file sama maka dihapus-------------------

    	if (ch1 == ch2){
        	printf("Files are identical \n");
        	int del;
        	del = remove(gpath);
 
   			if( del == 0 )
    			printf("%s file deleted successfully.\n",gpath);
   			else
    		{
      			printf("Unable to delete the file\n");
      			perror("Error");
   			}

//---------check directori apakah berisi atau tidak------------------

   			int n = 0;
			struct dirent *d;
  			DIR *direc = opendir(dir);
  			if (direc == NULL) //Not a directory or doesn't exist
    			return 1;
  			while ((d = readdir(direc)) != NULL) {
    			if(++n > 2)
      			break;
  			}
  			closedir(direc);

//---------jika directori kosong maka dihapus-----------------------

  			if (n <= 2){ //Directory Empty
  				printf("directory is empty\n");
    			rmdir(dir);
    			printf("deleted directory success\n");
    		}
    	}
      	else if (ch1 != ch2){
        	printf("Files are Not identical \n");
      	}
 
      	fclose(fp1);
      	fclose(fp2);
   	}

   	printf("finish\n");


	close(fd);
	return res;
}

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.readdir 	= xmp_readdir,
	.read 		= xmp_read,
	.write 		= xmp_write,
	.truncate 	= xmp_truncate,
};

int main(int argc, char *argv[])
{
	umask(0);
	return fuse_main(argc, argv, &xmp_oper, NULL);
}