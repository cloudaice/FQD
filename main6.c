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
/**创建一个符号链接 */

/* 这里的参数是有点混乱，但是符合symlink()系统的调用。
 *  “路径”是其中的连结点，?而“链接”是链接本身。
 * 因此，我们要使的路径不改变，但插入到安装目录的链接。*/

int fqd_symlink(const char *path, const char *link) {
    int retstat = 0;
    char flink[PATH_MAX];

    log_msg("\nbb_symlink(path=\"%s\", link=\"%s\")\n",
            path, link);
    fqd_fullpath(flink, link);

    retstat = symlink(path, flink);
    if (retstat < 0)
        retstat = bb_error("bb_symlink symlink");

    return retstat;
}
//重命名
//新旧路径都是fs相关的

int fqd_rename(const char *path, const char *newpath) {
    int retstat = 0;
    char fpath[PATH_MAX];
    char fnewpath[PATH_MAX];

    log_msg("\nbb_rename(fpath=\"%s\", newpath=\"%s\")\n",
            path, newpath);
    fqd_fullpath(fnewpath, newpath);
    fqd_fullpath(fpath, path);


    retstat = rename(fpath, fnewpath); //把新的名赋给retstat
    if (retstat < 0) {
        retstat = fqd_error("fqd_rename rename");
    }
    return retstat;
}

//创建一个文件的硬链接

int fqd_link(const char *path, const char *newpath) {
    int retstat = 0;
    char fpath[PATH_MAX], fnewpath[PATH_MAX];

    log_msg("\nbb_link(path=\"%s\", newpath=\"%s\")\n",
            path, newpath);
    fqd_fullpath(fpath, path);
    fqd_fullpath(fnewpath, newpath);

    retstat = link(fpath, fnewpath);
    if (retstat < 0) {
        retstat = bb_error("bb_link link");
    }
    return retstat;
}
//改变文件的允许标志位

int fqd_chmod(const char *path, mode_t mode) {
    int retstat = 0;
    char fpath[PATH_MAX];

    log_msg("\nbb_chmod(fpath=\"%s\", mode=0%03o)\n",
            path, mode);
    fqd_fullpath(fpath, path);

    retstat = chmod(fpath, mode);
    if (retstat < 0) {
        retstat = fqd_error("fqd_chmod chmod");
    }
    return retstat;
}

//改变文件的组和所属人

int fqd_chown(const char *path, uid_t uid, gid_t gid) {
    int retstat = 0;
    char fpath[PATH_MAX];

    log_msg("\nbb_chown(path=\"%s\", uid=%d, gid=%d)\n",
            path, uid, gid);
    fqd_fullpath(fpath, path);

    retstat = chown(fpath, uid, gid); //改变
    if (retstat < 0) {
        retstat = fqd_error("fqd_chown chown");
    }
    return retstat;
}

/** 改变文件大小*/
int fqd_truncate(const char *path, off_t newsize) {
    int retstat = 0;
    char fpath[PATH_MAX];

    log_msg("\nbb_truncate(path=\"%s\", newsize=%lld)\n",
            path, newsize);
    bb_fullpath(fpath, path);

    retstat = truncate(fpath, newsize);
    if (retstat < 0)
        bb_error("bb_truncate truncate");

    return retstat;
}


//改变文件时间

/* note -- I'll want to change this as soon as 2.6 is in debian testing */
int fqd_utime(const char *path, struct utimbuf *ubuf) {
    int retstat = 0;
    char fpath[PATH_MAX];

    log_msg("\nbb_utime(path=\"%s\", ubuf=0x%08x)\n",
            path, ubuf);
    fqd_fullpath(fpath, path);

    retstat = utime(fpath, ubuf);
    if (retstat < 0)
        retstat = fqd_error("fqd_utime utime");

    return retstat;
}

/** 打开文件操作
 *

 * 没有截断标志（使用O_CREAT，O_EXCL的，O_TRUNC）会被传递给open（）。
 * 如果操作符允许给给定的标志位，则应先打开应该检查。可选的open也可能返回
 * 任意在在fuse_file_info结构的句柄，并且它会被传递给所有的文件操作句柄。
 *
 * Changed in version 2.2
 */
int fqd_open(const char *path, struct fuse_file_info *fi) {
    int retstat = 0;
    int fd;
    char fpath[PATH_MAX];

    log_msg("\nbb_open(path\"%s\", fi=0x%08x)\n",
            path, fi);
    bb_fullpath(fpath, path);

    fd = open(fpath, fi->flags);
    if (fd < 0)
        retstat = bb_error("bb_open open");

    fi->fh = fd;
    log_fi(fi);

    return retstat;
}

/** 从一个打开的文件中读取数据
 *

 *读操作应该返回所请求的字节数，除非出现错误，否则其取得字节将会被0代替，
 * 有一个例外，'direct_io' 指定挂载选项，在这种情况下，系统调用的返回值
 * 将会反映到这个操作的返回值当中
 *
 *
 */


int fqd_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int retstat = 0;

    log_msg("\nbb_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
            path, buf, size, offset, fi);
    //在这种情况下无需得到fpath，因为fi->fh不再路径中
    log_fi(fi);

    retstat = pread(fi->fh, buf, size, offset);
    if (retstat < 0) {
        retstat = fqd_error(" fqd_read read");
    }
    return retstat;
}

/** 向打开的文件中写数据
 *

 *读操作应该返回所请求的字节数，除非出现错误。有一个例外，'direct_io'
 * 指定挂载选项，在这种情况下，系统调用的返回值将会反映到这个
 * 操作的返回值当中
 *
 * Changed in version 2.2
 */

//对于read（）而言，write（）系统调用的文档是不连续的。

int fqd_write(const char *path, const char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi) {
    int retstat = 0;

    log_msg("\nbb_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
            path, buf, size, offset, fi
            );
    // 在这种情况下无需得到fpath，因为fi->fh不再路径中
    log_fi(fi);

    retstat = pwrite(fi->fh, buf, size, offset);
    if (retstat < 0) {
        retstat = fqd_error(" fqd_write pwrite");
    }
    return retstat;
}

/** 获取文件系统的统计数据
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' 被忽视
 *

 */
int fqd_statfs(const char *path, struct statvfs *statv) {
    int retstat = 0;
    char fpath[PATH_MAX];

    log_msg("\nbb_statfs(path=\"%s\", statv=0x%08x)\n",
            path, statv);
    fqd_fullpath(fpath, path);

    // 获得底层的文件系统的统计资料
    retstat = statvfs(fpath, statv);
    if (retstat < 0) {
        retstat = fqd_error("fqd_statfs statvfs");

        log_statvfs(statv);
    }
    return retstat;
}

/**可能刷新缓存数据
 *

 * 注意：这和fsync（）并不等价，他不能同步一个脏数据
 *

 * 一个文件描述符关闭时就应当调用flush（），所以如果一个文件系统想要
 * 返回一个写错误到close()并且文件缓存中有脏数据，那么close()可以被
 * 写回数据和错误，因为很多应用忽略close（）错误，这点很有用
 *

 *
 * 注意：一个文件被打开后，flush（）可能被调用多次，如果文件描述符对应于
 * dup(), dup2() or fork()调用，以上情况就可能发生。不太可能确定是否一个
 * fluse（）是最后一个，所以，每个flush（）因该被平等对待。多写刷新序列
 * 相对而言出现的比较少，所以构不成问题。
 *

 * 文件系统不能假设写后一定会调用flush。
 *

 */
int fqd_flush(const char *path, struct fuse_file_info *fi) {
    int retstat = 0;

    log_msg("\nbb_flush(path=\"%s\", fi=0x%08x)\n", path, fi);
    // 在这种情况下无需得到fpath，因为fi->fh不再路径中
    log_fi(fi);

    return retstat;
}

/** 释放一个打开的文件
 *

 * 如果对一个打开的文件没有引用的话就释放掉这个文件，所有的文件描述符和映射
 * 被关闭
 *
 *对于每个open（）调用，将会对应于一个release()操作，有可能一个文件
 * 被打开超过一次，这样的话只有最后的那个释放才有效，释放后将没有对文
 * 件的读写操作。释放的返回值将被忽略。
 * 
 */
int fqd_release(const char *path, struct fuse_file_info *fi) {
    int retstat = 0;

    log_msg("\nbb_release(path=\"%s\", fi=0x%08x)\n",
            path, fi);
    log_fi(fi);


    //文件需要被关闭，如果我们需要一些资源的话，我们最好在这里就关闭文件
    retstat = close(fi->fh);

    return retstat;
}

/** 同步文件的内容
 *
 * 如果datasync参数是非零，那么只有用户数据应该被刷新，而元数据不用被刷新。
 *
 * Changed in version 2.2
 */
int fqd_fsync(const char *path, int datasync, struct fuse_file_info *fi) {
    int retstat = 0;

    log_msg("\nbb_fsync(path=\"%s\", datasync=%d, fi=0x%08x)\n",
            path, datasync, fi);
    log_fi(fi);

    if (datasync) {
        retstat = fdatasync(fi->fh);
    } else if {
        retstat = fsync(fi->fh);
    }
    if (retstat < 0) {
        fqd_error(" fqd_fsync fsync");
    }
    return retstat;
}

/** 设置扩展属性 */
int fqd_setxattr(const char *path, const char *name, const char *value, size_t size, int flags) {
    int retstat = 0;
    char fpath[PATH_MAX];

    log_msg("\nbb_setxattr(path=\"%s\", name=\"%s\", value=\"%s\", size=%d, flags=0x%08x)\n",
            path, name, value, size, flags);
    fqd_fullpath(fpath, path);

    retstat = lsetxattr(fpath, name, value, size, flags);
    if (retstat < 0) {
        retstat = fqd_error(" fqd_setxattr lsetxattr");
    }
    return retstat;
}

/** 获取扩展属性*/
int fqd_getxattr(const char *path, const char *name, char *value, size_t size) {
    int retstat = 0;
    char fpath[PATH_MAX];

    log_msg("\nbb_getxattr(path = \"%s\", name = \"%s\", value = 0x%08x, size = %d)\n",
            path, name, value, size);
    fqd_fullpath(fpath, path);

    retstat = lgetxattr(fpath, name, value, size);
    if (retstat < 0) {
        retstat = fqd_error(" fqd_getxattr lgetxattr");
    } else if {
        log_msg("    value = \"%s\"\n", value);
    }
    return retstat;
}

/** 列出扩展属性 */
int fqd_listxattr(const char *path, char *list, size_t size) {
    int retstat = 0;
    char fpath[PATH_MAX];
    char *ptr;

    log_msg(" fqd_listxattr(path=\"%s\", list=0x%08x, size=%d)\n",
            path, list, size
            );
    fqd_fullpath(fpath, path);

    retstat = llistxattr(fpath, list, size);
    if (retstat < 0) {
        retstat = fqd_error(" fqd_listxattr llistxattr");
    }
    log_msg("    returned attributes (length %d):\n", retstat);
    for (ptr = list; ptr < list + retstat; ptr += strlen(ptr) + 1) {
        log_msg("    \"%s\"\n", ptr);
    }
    return retstat;
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
