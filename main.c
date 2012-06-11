/*
  Big Brother File System

这可以被称为一个no-op的文件系统：它不强加任何其他现有的结构上的文件系统的语义。
它简单的报告来的请求，并将它们传递到底层的文件系统。信息保存在一个名为日志，
在目录bbfs.log运行bbfs的。

  gcc -Wall `pkg-config fuse --cflags --libs` -o bbfs bbfs.c
 */

#include "params.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>

#include "log.h"

// 把错误报给日志文件，将-errno赋值给caller

static int bb_error(char *str) {
    int ret = -errno;

    log_msg("    ERROR %s: %s\n", str, strerror(errno));

    return ret;
}

//检查授权用户是否被允许做给定的操作

//  我看到的所有的路径是相对安装的根文件系统。为了得到底层的文件系统，我需要
// 挂载点。我将它保存在main（），然后每当我需要寻找某东西路径，我会找到给这个挂载点去
//构造它

static void fqd_fullpath(char fpath[PATH_MAX], const char *path) {
    strcpy(fpath, BB_DATA->rootdir);
    strncat(fpath, path, PATH_MAX); // 这个函数是有缺陷的，如果路径过长将会出错

    log_msg("    bb_fullpath:  rootdir = \"%s\", path = \"%s\", fpath = \"%s\"\n",
            BB_DATA->rootdir, path, fpath);
}

///////////////////////////////////////////////////////////
//
// 所有典型的函数和其C语言的评价保存在/usr/include/fuse.h
//

/** 好哦的文件属性
 *
 * 类似于Stat()函数，'st_dev' 和 'st_blksize'文件时被忽视的， 除非'use_ino'挂载给出，
 * 否则'st_ino'是被忽略的。
 */
int fqd_getattr(const char *path, struct stat *statbuf) {
    int retstat = 0;
    char fpath[PATH_MAX];

    log_msg("\nbb_getattr(path=\"%s\", statbuf=0x%08x)\n",
            path, statbuf);
    fqd_fullpath(fpath, path);

    retstat = lstat(fpath, statbuf);
    if (retstat != 0)
        retstat = bb_error("bb_getattr lstat");

    log_stat(statbuf);

    return retstat;
}

/** 读一个符号链接的目标
 *
 * 缓冲区应充满一个空结束的字符串。
 * 缓冲区的大小参数包括终止的空间空字符。
 * 如果linkname是太长，不匹配在缓冲区，它应该被截断。
 * 如果成功返回值应该是0。
 */

/*
考虑到系统readlink（）系统将截断并且失去终结的空字符。因此，
 传递到readlink（）系统的大小必须小于传递到bb_readlink（）。

 */
int bb_readlink(const char *path, char *link, size_t size) {
    int retstat = 0;
    char fpath[PATH_MAX];

    log_msg("bb_readlink(path=\"%s\", link=\"%s\", size=%d)\n",
            path, link, size);
    fqd_fullpath(fpath, path);

    retstat = readlink(fpath, link, size - 1);
    if (retstat < 0)
        retstat = bb_error("bb_readlink readlink");
    else {
        link[retstat] = '\0';
        retstat = 0;
    }

    return retstat;
}

/** 创建文件节点
 *
 * 我们没有写create（）函数，这里的mknod（）将创建所有 non-directory和
 *  non-symlink 节点。
 */
//

int fqd_mknod(const char *path, mode_t mode, dev_t dev) {
    int retstat = 0;
    char fpath[PATH_MAX];

    log_msg("\nbb_mknod(path=\"%s\", mode=0%3o, dev=%lld)\n",
            path, mode, dev);
    fqd_fullpath(fpath, path);

    //在Linux上，这可能只能这么写“mknod(path, mode, rdev)”但是这样做更便携
    if (S_ISREG(mode)) { //是REG类型
        retstat = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (retstat < 0)
            retstat = fqd_error("fqd_mknod open");
        else {
            retstat = close(retstat);
            if (retstat < 0)
                retstat = fqd_error("fqd_mknod close");
        }
    } else
        if (S_ISFIFO(mode)) { //是fifo类型
        retstat = mkfifo(fpath, mode);
        if (retstat < 0) {
            retstat = fqd_error("fqd_mknod mkfifo");
        }
    } else {
        retstat = mknod(fpath, mode, dev);
        if (retstat < 0) {
            retstat = fqd_error("fqd_mknod mknod");
        }
    }

    return retstat;
}

/**创建一个目录 */
int fqd_mkdir(const char *path, mode_t mode) {
    int retstat = 0;
    char fpath[PATH_MAX];

    log_msg("\nbb_mkdir(path=\"%s\", mode=0%3o)\n",
            path, mode);
    fqd_fullpath(fpath, path); //加载路径

    retstat = mkdir(fpath, mode);
    if (retstat < 0) {
        retstat = fqd_error("fqd_mkdir mkdir");
    }
    return retstat;
}

/** 删除文件 */
int fqd_unlink(const char *path) {
    int retstat = 0;
    char fpath[PATH_MAX];

    log_msg("fqd_unlink(path=\"%s\")\n",
            path);
    fqd_fullpath(fpath, path);

    retstat = unlink(fpath); //删除链接
    if (retstat < 0) {
        retstat = fqd_error("fqd_unlink unlink"); //错误处理
    }

    return retstat;
}

/** 删除目录*/
int fqd_rmdir(const char *path) {
    int retstat = 0;
    char fpath[PATH_MAX];

    log_msg("bb_rmdir(path=\"%s\")\n",
            path);
    fqd_fullpath(fpath, path);

    retstat = rmdir(fpath);
    if (retstat < 0)
        retstat = bb_error("bb_rmdir rmdir");

    return retstat;
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

int fqd_write(const char *path, const char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi) {

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

int fqd_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
        struct fuse_file_info *fi) {

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

struct fuse_operations bb_oper = {
    .getattr = bb_getattr,
    .readlink = bb_readlink,
    // no .getdir -- that's deprecated
    .getdir = NULL,
    .mknod = bb_mknod,
    .mkdir = bb_mkdir,
    .unlink = bb_unlink,
    .rmdir = bb_rmdir,
    .symlink = bb_symlink,
    .rename = bb_rename,
    .link = bb_link,
    .chmod = bb_chmod,
    .chown = bb_chown,
    .truncate = bb_truncate,
    .utime = bb_utime,
    .open = bb_open,
    .read = bb_read,
    .write = bb_write,
    /** Just a placeholder, don't set */ // huh???
    .statfs = bb_statfs,
    .flush = bb_flush,
    .release = bb_release,
    .fsync = bb_fsync,
    .setxattr = bb_setxattr,
    .getxattr = bb_getxattr,
    .listxattr = bb_listxattr,
    .removexattr = bb_removexattr,
    .opendir = bb_opendir,
    .readdir = bb_readdir,
    .releasedir = bb_releasedir,
    .fsyncdir = bb_fsyncdir,
    .init = bb_init,
    .destroy = bb_destroy,
    .access = bb_access,
    .create = bb_create,
    .ftruncate = bb_ftruncate,
    .fgetattr = bb_fgetattr
};

void fqd_usage() {

}

int main(int argc, char *argv[]) {

}

