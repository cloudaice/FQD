#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <stdio.h>
#include <stdlib.h>
#include <log.h>
/*
 Big Brother 文件系统
 *
这可以被称为一个no-op的文件系统：它不强加任何其他现有的结构上的文件系统的语义。
它简单的报告来的请求，并将它们传递到底层的文件系统。信息保存在一个名为日志，
在目录bbfs.log运行bbfs的。

  gcc -Wall `pkg-config fuse --cflags --libs` -o bbfs bbfs.c
 */


//把错误报给日志文件，将-errno赋值给caller

static int fqd_error(char *str) {
    int ret = -errno;

    log_msg("ERROR %s: %s\n", str, strerror(errno));

    return ret;

}

//检查授权用户是否被允许做给定的操作

/*我看到的所有的路径是相对安装的根文件系统。为了得到底层的文件系统，我需要 
 挂载点。我将它保存在main（），然后每当我需要寻找某东西路径，我会找到给这个挂载点去
 构造它*/
static void fqd_fullpath(char fpath[PATH_MAX], const char *path) {
    strcpy(fpath, BB_DATA->rootdir);
    strncat(fpath, path, PATH_MAX); // 这个函数是有缺陷的，如果路径过长将会出错

    log_msg(" path = \"%s\",bb_fullpath:  rootdir = \"%s\", fpath = \"%s\"\n",
            BB_DATA->rootdir, path, fpath);
}
///////////////////////////////////////////////////////////
//

//所有典型的函数和其C语言的评价保存在/usr/include/fuse.h

/** Get file attributes.
 *
 * 类似于Stat()函数，'st_dev' 和 'st_blksize'文件时被忽视的， 除非'use_ino'挂载给出，
 * 否则'st_ino'是被忽略的。
 */
int fqd_getattr(const char *path, struct stat *statbuf) {
    char fpath[PATH_MAX];

    int retstat = 0;

    log_msg("\nbb_getattr(path=\"%s\", statbuf=0x%08x)\n",
            path, statbuf);
    bb_fullpath(fpath, path);

    retstat = lstat(fpath, statbuf);
    if (retstat != 0) {
        retstat = bb_error("bb_getattr lstat");
    }

    log_stat(statbuf);

    return retstat;
}

int fqd_readlink(const char *path, char *link, size_t size) {
}

int fqd_mknod(const char *path, mode_t mode, dev_t dev) {
}

int fqd_mkdir(const char *path, mode_t mode) {
}

int fqd_unlink(const char *path) {
}

int fqd_rmdir(const char *path) {



}

int fqd_symlink(const char *path, const char *link) {
}

int fqd_rename(const char *path, const char *newpath) {
}

int fqd_link(const char *path, const char *newpath) {
}

int fqd_chmod(const char *path, mode_t mode) {



}

int fqd_chown(const char *path, uid_t uid, gid_t gid) {



}

int fqd_truncate(const char *path, off_t newsize) {
}

int fqd_utime(const char *path, struct utimbuf *ubuf) {
}

int fqd_open(const char *path, struct fuse_file_info *fi) {
}

int fqd_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
}

int fqd_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
}

int fqd_statfs(const char *path, struct statvfs *statv) {
}

int fqd_flush(const char *path, struct fuse_file_info *fi) {
}

int fqd_release(const char *path, struct fuse_file_info *fi) {
}

int fqd_fsync(const char *path, int datasync, struct fuse_file_info *fi) {
}

int fqd_setxattr(const char *path, const char *name, const char *value, size_t size, int flags) {
}

int fqd_getxattr(const char *path, const char *name, char *value, size_t size) {
}

int fqd_listxattr(const char *path, char *list, size_t size) {
}

int fqd_removexattr(const char *path, const char *name) {
}

int fqd_opendir(const char *path, struct fuse_file_info *fi) {
}

int fqd_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
}

int fqd_releasedir(const char *path, struct fuse_file_info *fi) {
}

int fqd_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi) {
}

void *fqd_init(struct fuse_conn_info *conn) {
}

void fqd_destroy(void *userdata) {
}

int fqd_access(const char *path, int mask) {
}

int fqd_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
}

int fqd_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi) {
}

int fqd_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi) {
}
static struct fuse_operations fqd_oper = {

    .getattr = fqd_getattr,

    .readlink = fqd_readlink,

    .getdir = NULL,

    .mknod = fqd_mknod,

    .mkdir = fqd_mkdir,

    .rmdir = fqd_rmdir,

    .symlink = fqd_symlink,

    .rename = fqd_rename,

    .link = fqd_link,

    .chmod = fqd_chmod,

    .chown = fqd_chown,

    .truncate = fqd_truncate,

    .utime = fqd_utime,

    .open = fqd_open,

    .read = fqd_read,

    .write = fqd_write,

    .statfs = fqd_statfs,

    .flush = fqd_flush,

    .release = fqd_release,

    .fsync = fqd_fsync,

    .setxattr = fqd_setxattr,

    .getxattr = fqd_getxattr,

    .listxattr = fqd_listxattr,

    .removexattr = fqd_removexattr,

    .opendir = fqd_opendir,

    .readdir = fqd_readdir,

    .releasedir = fqd_releasedir,

    .fsyncdir = fqd_fsncdir,

    .init = fqd_init,

    .access = fqd_access,

    .create = fqd_create,

    .ftruncate = fqd_ftruncate,

    .fgetattr = fqd_fgetattr

};

int int main(int argc, const char *argv[]) {

    return 0;

}