#include <ctype.h>
#include <errno.h>
#include <fuse.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <stdio.h>
#include <stdlib.h>

#include <log.h>

static int fqd_error(char *str)
{

}

static void fqd_fullpath(char fpath[PATH_MAX], const char *path)
{

}

int fqd_getattr(const char *path, struct stat *statbuf)
{

}

int fqd_readlink(const char *path, char *link, size_t size)
{

}

int fqd_mknod(const char *path, mode_t mode, dev_t dev)
{

}

int fqd_mkdir(const char *path, mode_t mode)
{
    
}

int fqd_unlink(const char *path)
{

}

int fqd_rmdir(const char *path)
{

}

int fqd_symlink(const char *path, const char *link)
{

}

int fqd_rename(const char *path, const char *newpath)
{

}

int fqd_link(const char *path, const char *newpath)
{

}

int fqd_chmod(const char *path, mode_t mode)
{

}

int fqd_chown(const char *path, uid_t uid, gid_t gid)
{

}

int fqd_truncate(const char *path, off_t newsize)
{

}

int fqd_utime(const char *path, struct utimbuf *ubuf)
{

}

int fqd_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    int fd;
    char fpath[PATH_MAX];
    log_msg("\nfqd_open(path\"%s\",fi=0x%08x)\n",path,fi);
    fqd_fullpath(fpath,path);
    fd = open(fpath,fi->flags);
    if(fd<0)
        retstat = fqd_error("fqd_open open");
    fi->fh = fd;
    log_fi(fi);
    return retstat;
}

int fqd_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg

}

int fqd_write(const char *path, const char *buf, size_t size, off_t offset,struct fuse_file_info *fi)
{

}

int fqd_statfs(const char *path, struct statvfs *statv)
{

}

int fqd_flush(const char *path, struct fuse_file_info *fi)
{

}

int fqd_release(const char *path, struct fuse_file_info *fi)
{

}

int fqd_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{

}

int fqd_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{

}

int fqd_getxattr(const char *path, const char *name, char *value, size_t size)
{

}

int fqd_listxattr(const char *path, char *list, size_t size)
{

}

int fqd_removexattr(const char *path, const char *name)
{

}

int fqd_opendir(const char *path, struct fuse_file_info *fi)
{

}

int fqd_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,struct fuse_file_info *fi)
{

}

int fqd_releasedir(const char *path, struct fuse_file_info *fi)
{

}

int fqd_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi)
{

}

void *fqd_init(struct fuse_conn_info *conn)
{

}

void fqd_destroy(void *userdata)
{

}

int fqd_access(const char *path, int mask)
{

}

int fqd_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{

}

int fqd_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{

}

int fqd_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{

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
    .destroy  = fqd_destroy,
    .access = fqd_access,
    .create = fqd_create,
    .ftruncate = fqd_ftruncate,
    .fgetattr = fqd_fgetattr
};

int int main(int argc, const char *argv[])
{
    
    return 0;
}
