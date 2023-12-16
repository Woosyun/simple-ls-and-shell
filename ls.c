#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

void print_permission(struct stat sbuf){
    if((sbuf.st_mode & S_IFMT) == S_IFDIR){
        printf("d");
    } else if ((sbuf.st_mode & S_IFMT) == S_IFREG){
        printf("-");
    } else {
        printf("%c", (sbuf.st_mode & S_IFMT) >> 12);
    }
    // Check owner read permission
    if (sbuf.st_mode & S_IRUSR) {
    printf("r");
    } else {
    printf("-");
    }
    // Check owner write permission
    if (sbuf.st_mode & S_IWUSR) {
    printf("w");
    } else {
    printf("-");
    }
    // Check owner execute permission
    if (sbuf.st_mode & S_IXUSR) {
    printf("x");
    } else {
    printf("-");
    }
    // Check group read permission
    if (sbuf.st_mode & S_IRGRP) {
    printf("r");
    } else {
    printf("-");
    }
    // Check group write permission
    if (sbuf.st_mode & S_IWGRP) {
    printf("w");
    } else {
    printf("-");
    }
    // Check group execute permission
    if (sbuf.st_mode & S_IXGRP) {
    printf("x");
    } else {
    printf("-");
    }
    // Check others read permission
    if (sbuf.st_mode & S_IROTH) {
    printf("r");
    } else {
    printf("-");
    }
    // Check others write permission
    if (sbuf.st_mode & S_IWOTH) {
    printf("w");
    } else {
    printf("-");
    }
    // Check others execute permission
    if (sbuf.st_mode & S_IXOTH) {
    printf("x");
    } else {
    printf("-");
    }
    printf("  ");
}

void lprint(char *dir_name, char *file_name){
    //set file_path
    char file_path[255];
    strcpy(file_path, dir_name);
    strcat(file_path, "/");
    strcat(file_path, file_name);
    
	struct stat sbuf;
    char buf[1024];
    int len;
    //report error if it cannot stat
    if(stat(file_path, &sbuf)<0){
        fprintf(stderr, "cannot stat %s\n", file_path);
        exit(EXIT_FAILURE);
    }

    //permissions of file
    print_permission(sbuf);

    //number of file's hard link
    printf("%2lo  ", sbuf.st_nlink);
    //file owner
    printf("%4o  ", sbuf.st_uid);
    //file group
    printf("%4o  ", sbuf.st_gid);
    //file size
    printf("%6ld  ", sbuf.st_size);
    //time stamp, last modification time
    time_t time = sbuf.st_mtime;
    struct tm *tm = localtime(&time);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm);
    printf("%s  ", time_str);

    //print file name
    printf("%s  ", file_name);
    //soft link
    len = readlink(file_path, buf, sizeof(buf));
    buf[len] = '\0';
    if(len>0)
        printf("--> %s", buf);

    printf("\n");
}

bool check_option(char *options, char a){
    for(int i=1; i<strlen(options); i++)
        if(options[i]==a)
            return true;
    return false;
}

int main(int argc, char **argv) {
	DIR *dp;
	struct dirent *entry;

    // report error when name of directory was not provided
	if (argc>3) {
		fprintf(stderr, "usage: %s [options] dir_name\n", argv[0]);
		exit(1);
	}

    //get options
    bool is_option = argc>1 && argv[1][0] == '-';
    char *options = is_option ? argv[1] : "";

    //get directory name if exists
    char dir_name[255];
    if(argc==1 || (argc==2 && is_option))
        strcpy(dir_name, ".");
    else if(argc==2 && !is_option)
        strcpy(dir_name, argv[1]);
    else if(argc==3)
        strcpy(dir_name, argv[2]);

    // report error when name of directory is not valid name
	if ((dp = opendir(dir_name)) == NULL ) {
		fprintf(stderr, "%s is not valid directory name\n", argv[1]);
		exit(1);
	}

    // print the result
    char *file_name;
	while ((entry = readdir(dp)) != NULL ){
        file_name = entry->d_name;

        if(strcmp(file_name, ".")==0 || strcmp(file_name, "..")==0){
            if(check_option(options, 'a')){//if -a option is on, then print . and ..
                if(check_option(options, 'l'))//if -l options is on, then print stats
                    lprint(dir_name, file_name);
                else
                    printf("%s  ", file_name);
            }
        }
        else{
            if(check_option(options, 'l'))//if -l options is on, then print stat
                lprint(dir_name, file_name);
            else
                printf("%s  ", file_name);
        }
    }
    if(!check_option(options, 'l'))
        printf("\n");

	closedir(dp);
	return(0);
}
