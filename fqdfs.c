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

   // 把错误报给日志文件
    static int fqd_error(char *str)
    {
        int ret = -errno;
        log_msg("    ERROR %s: %s\n",str,strerror(errno));
        return ret;
    }
 /*
检查授权用户是否被允许做给定的操作
我看到的所有的路径是相对安装的根文件系统。为了得到底层的文件系统，我需要
挂载点。我将它保存在main（），然后每当我需要寻找某东西路径，我会找到给这个挂载点去
构造它
*/
    static void fqd_fullpath(char fpath[PATH_MAX], const char *path)
    {
        strcpy(fpath, BB_DATA->rootdir);
        strncat(fpath, path, PATH_MAX); 
        log_msg("    fqd_fullpath:  rootdir = \"%s\", path = \"%s\", fpath = \"%s\"\n",
    	    BB_DATA->rootdir, path, fpath);
    }

/*
 类似于Stat()函数，'st_dev' 和 'st_blksize'文件时被忽视的， 除非'use_ino'挂载给出，
 否则'st_ino'是被忽略的。
*/
    int fqd_getattr(const char *path, struct stat *statbuf)
    {
        int retstat = 0;
        char fpath[PATH_MAX];
        log_msg("\nfqd_getattr(path=\"%s\", statbuf=0x%08x)\n",
    	  path, statbuf);
        fqd_fullpath(fpath, path);
        retstat = lstat(fpath, statbuf);
        if (retstat != 0)
    	retstat = fqd_error("fqd_getattr lstat");
        log_stat(statbuf);
        return retstat;
    }

/** 读一个符号链接的目标
 *
缓冲区应充满一个空结束的字符串。
缓冲区的大小参数包括终止的空间空字符。
如果linkname是太长，不匹配在缓冲区，它应该被截断。
如果成功返回值应该是0。
考虑到系统readlink（）系统将截断并且失去终结的空字符。因此，
传递到readlink（）系统的大小必须小于传递到fqd_readlink（）。
 */
    int fqd_readlink(const char *path, char *link, size_t size)
    {
    int retstat = 0;
        char fpath[PATH_MAX];
        log_msg("fqd_readlink(path=\"%s\", link=\"%s\", size=%d)\n",
    	  path, link, size);
        fqd_fullpath(fpath, path);
            retstat = readlink(fpath, link, size - 1);
        if (retstat < 0)
    	retstat = fqd_error("fqd_readlink readlink");
        else  {
    	link[retstat] = '\0';
    	retstat = 0;
        }
        return retstat;
    }
/** 创建文件节点
 我们没有写create（）函数，这里的mknod（）将创建所有 non-directory和
  non-symlink 节点。
 */
    int fqd_mknod(const char *path, mode_t mode, dev_t dev)
    {
            int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nfqd_mknod(path=\"%s\", mode=0%3o, dev=%lld)\n",
      path, mode, dev);
    fqd_fullpath(fpath, path);
    if (S_ISREG(mode)) {
        retstat = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
    if (retstat < 0)
        retstat = fqd_error("fqd_mknod open");
        else {
            retstat = close(retstat);
        if (retstat < 0)
        retstat = fqd_error("fqd_mknod close");
    }
    } else
    if (S_ISFIFO(mode)) {
        retstat = mkfifo(fpath, mode);
        if (retstat < 0)
        retstat = fqd_error("fqd_mknod mkfifo");
    } else {
        retstat = mknod(fpath, mode, dev);
        if (retstat < 0)
        retstat = fqd_error("fqd_mknod mknod");
    }
    return retstat;
    }
   /* 创建一个目录 */
    int fqd_mkdir(const char *path, mode_t mode)
    {
          int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nfqd_mkdir(path=\"%s\", mode=0%3o)\n",
        path, mode);
    fqd_fullpath(fpath, path);//加载路径
    
    retstat = mkdir(fpath, mode);
    if (retstat < 0)
    retstat = fqd_error("fqd_mkdir mkdir");
    
    return retstat;
        
    }
/* 删除文件 */
    int fqd_unlink(const char *path)
    {
        int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("fqd_unlink(path=\"%s\")\n",
        path);
    fqd_fullpath(fpath, path);
    
    retstat = unlink(fpath);//删除链接
    if (retstat < 0)
    retstat = fqd_error("fqd_unlink unlink");
    
    return retstat;

    }

    int fqd_rmdir(const char *path)
    {
         int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("fqd_rmdir(path=\"%s\")\n",
        path);
    fqd_fullpath(fpath, path);
    
    retstat = rmdir(fpath);
    if (retstat < 0)
    retstat = fqd_error("fqd_rmdir rmdir");
    
    return retstat;

    }

/*
创建一个符号链接 
 这里的参数是有点混乱，但是符合symlink()系统的调用。
 “路径”是其中的连结点， 而“链接”是链接本身。
  因此，我们要使的路径不改变，但插入到安装目录的链接。
 */
    int fqd_symlink(const char *path, const char *link)
    {
        int retstat = 0;
    char flink[PATH_MAX];
    
    log_msg("\nfqd_symlink(path=\"%s\", link=\"%s\")\n",
        path, link);
    fqd_fullpath(flink, link);
    
    retstat = symlink(path, flink);
    if (retstat < 0)
    retstat = fqd_error("fqd_symlink symlink");
    
    return retstat;
    }

    //重命名
    int fqd_rename(const char *path, const char *newpath)
    {
     int retstat = 0;
    char fpath[PATH_MAX];
    char fnewpath[PATH_MAX];
    
    log_msg("\nfqd_rename(fpath=\"%s\", newpath=\"%s\")\n",
        path, newpath);
    fqd_fullpath(fpath, path);
    fqd_fullpath(fnewpath, newpath);
    
    retstat = rename(fpath, fnewpath);
    if (retstat < 0)
    retstat = fqd_error("fqd_rename rename");
    
    return retstat;

    }
 //创建一个文件的硬链接
    int fqd_link(const char *path, const char *newpath)
    {
         int retstat = 0;
    char fpath[PATH_MAX], fnewpath[PATH_MAX];
    
    log_msg("\nfqd_link(path=\"%s\", newpath=\"%s\")\n",
        path, newpath);
    fqd_fullpath(fpath, path);
    fqd_fullpath(fnewpath, newpath);
    
    retstat = link(fpath, fnewpath);
    if (retstat < 0)
    retstat = fqd_error("fqd_link link");
    return retstat;
    }
//改变文件的quanxian
    int fqd_chmod(const char *path, mode_t mode)
    {
         int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nfqd_chmod(fpath=\"%s\", mode=0%03o)\n",
        path, mode);
    fqd_fullpath(fpath, path);
    
    retstat = chmod(fpath, mode);
    if (retstat < 0)
    retstat = fqd_error("fqd_chmod chmod");
    
    return retstat;

    }
//改变文件的组和所属人
    int fqd_chown(const char *path, uid_t uid, gid_t gid)
    {
        int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nfqd_chown(path=\"%s\", uid=%d, gid=%d)\n",
        path, uid, gid);
    fqd_fullpath(fpath, path);
    
    retstat = chown(fpath, uid, gid);
    if (retstat < 0)
    retstat = fqd_error("fqd_chown chown");
    
    return retstat;

    }
/** 改变文件大小*/
    int fqd_truncate(const char *path, off_t newsize)
    {
         int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nfqd_truncate(path=\"%s\", newsize=%lld)\n",
        path, newsize);
    fqd_fullpath(fpath, path);
    
    retstat = truncate(fpath, newsize);
    if (retstat < 0)
    fqd_error("fqd_truncate truncate");
    
    return retstat;

    }

    int fqd_utime(const char *path, struct utimbuf *ubuf)
    {
         int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nfqd_utime(path=\"%s\", ubuf=0x%08x)\n",
        path, ubuf);
    fqd_fullpath(fpath, path);
    
    retstat = utime(fpath, ubuf);
    if (retstat < 0)
    retstat = fqd_error("fqd_utime utime");
    
    return retstat;

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
    log_msg("\nfqd_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
        path, buf, size, offset, fi);
    // no need to get fpath on this one, since I work from fi->fh not the path
    log_fi(fi);
    
    retstat = pread(fi->fh, buf, size, offset);
    if (retstat < 0)
    retstat = fqd_error("fqd_read read");
    
    return retstat;
        

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
