/*
 * author : Shuichi TAKANO
 * since  : Sun Aug 18 2019 15:29:28
 */

#include <stdio.h>
#include <array>
#include <fcntl.h>
#include <sd_card/ff.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#include <syslog.h>

namespace
{

    constexpr const char *TAG = "STDFILE";

    template <class T, int N>
    class Allocator
    {
        std::array<T, N> objs_;
        uint32_t freeObjMask_ = (1 << N) - 1;

    public:
        int allocate()
        {
            for (int i = 0; i < N; ++i)
            {
                if (freeObjMask_ & (1 << i))
                {
                    freeObjMask_ &= ~(1 << i);
                    return i;
                }
            }
            return -1;
        }

        void free(int i)
        {
            freeObjMask_ |= (1 << i);
        }

        void free(T *p)
        {
            free(p - objs_.data());
        }

        int getIndex(T *p)
        {
            return p - objs_.data();
        }

        T &get(int i)
        {
            return objs_[i];
        }

        static Allocator &instance()
        {
            static Allocator inst;
            return inst;
        }
    };

    constexpr int FD_OFFSET = 1024;
    constexpr int MAX_FILES = 8;
    constexpr int MAX_DIRS = 8;

    std::array<dirent, MAX_DIRS> dirents_;

    Allocator<FIL, MAX_FILES> &getFileAllocator()
    {
        return Allocator<FIL, MAX_FILES>::instance();
    }

    Allocator<DIR, MAX_FILES> &getDirAllocator()
    {
        return Allocator<DIR, MAX_DIRS>::instance();
    }

    int allocateFile()
    {
        auto i = getFileAllocator().allocate();
        if (i < 0)
        {
            LOGV(TAG, "file overflow");
            return -1;
        }
        return i + FD_OFFSET;
    }

    void freeFile(int fp)
    {
        fp -= FD_OFFSET;
        getFileAllocator().free(fp);
    }

    FIL *getFile(int fp)
    {
        return &getFileAllocator().get(fp - FD_OFFSET);
    }

    int allocateDir()
    {
        auto i = getDirAllocator().allocate();
        if (i < 0)
        {
            LOGV(TAG, "dir overflow");
            return -1;
        }
        return i;
    }

    void freeDir(int i)
    {
        getDirAllocator().free(i);
    }

    void freeDir(DIR *d)
    {
        getDirAllocator().free(d);
    }

    DIR *getDir(int i)
    {
        return &getDirAllocator().get(i);
    }

} // namespace

extern "C"
{
    int
    sys_file_open(const char *name, int flags, int mode)
    {
        //        LOGV(TAG, "open(%s, %d, %d)", name, flags, mode);

        int ffmode = 0;
        if ((flags & 3) == O_RDONLY)
        {
            //            LOGV(TAG, " read only.\n");
            ffmode |= FA_READ;
        }
        if ((flags & 3) == O_WRONLY)
        {
            //            LOGV(TAG, " write only.\n");
            ffmode |= FA_WRITE;
        }
        if ((flags & 3) == O_RDWR)
        {
            //            LOGV(TAG, " read/write.\n");
            ffmode |= FA_READ | FA_WRITE;
        }

        if (flags & O_CREAT)
        {
            //            LOGV(TAG, " create.\n");
            ffmode |= FA_CREATE_NEW;
        }

        if (flags & O_TRUNC)
        {
            //            LOGV(TAG, " trunc.\n");
            ffmode |= FA_CREATE_ALWAYS;
        }

        int fp = allocateFile();
        if (fp < 0)
        {
            return -EBADF;
        }
        auto fil = getFile(fp);
        if (f_open(fil, name, ffmode) != FR_OK)
        {
            freeFile(fp);
            return -ENOENT;
        }

        return fp;
    }

    ssize_t
    sys_file_write(int file, const void *ptr, size_t len)
    {
        //        LOGV(TAG, "file_write(%d, %p, %zd)\n", file, ptr, len);
        UINT count;
        f_write(getFile(file), ptr, len, &count);
        return count;
    }

    ssize_t
    sys_file_read(int file, void *ptr, size_t len)
    {
        //        LOGV(TAG, "file_read(%d, %p, %zd)\n", file, ptr, len);
        UINT count;
        f_read(getFile(file), ptr, len, &count);

        return count;
    }

    int
    sys_file_fstat(int file, struct stat *st)
    {
        //        LOGV(TAG, "file_fstat(%d, %p)\n", file, st);
        auto fil = getFile(file);
        memset(st, 0, sizeof(*st));
        st->st_size = f_size(fil);
        st->st_mode |= _IFREG;
        return 0;
    }

    int
    sys_file_stat(const char *name, struct stat *st)
    {
        //        LOGV(TAG, "file_stat(%s, %p)", name, st);
        FILINFO fi;
        if (f_stat(name, &fi) != FR_OK)
        {
            return -1;
        }
        memset(st, 0, sizeof(*st));
        st->st_size = fi.fsize;
        if (fi.fattrib & AM_DIR)
        {
            st->st_mode |= _IFDIR;
        }
        else
        {
            st->st_mode |= _IFREG;
        }
        //        LOGV(TAG, "st: %p, %p, %zd, d %d\n", st, st + 1, sizeof(*st), S_ISDIR(st->st_mode));
        return 0;
    }

    int
    sys_file_close(int file)
    {
        //        LOGV(TAG, "file_close(%d)\n", file);

        f_close(getFile(file));
        freeFile(file);

        return 0;
    }

    ssize_t
    sys_file_lseek(int file, ssize_t ptr, int dir)
    {
        //        LOGV(TAG, "file_lseek(%d, %d, %d)", file, (int)ptr, dir);
        auto fil = getFile(file);

        switch (dir)
        {
        case SEEK_CUR:
            ptr += f_tell(fil);
            break;

        case SEEK_END:
            ptr += f_size(fil);
            break;

        default:
            break;
        }

        if (f_lseek(fil, ptr) != FR_OK)
        {
            return -1;
        }

        return f_tell(fil);
    }

    ////
    // void rewind(FILE *fp)
    // {
    //     fseek(fp, 0L, SEEK_SET);
    // }

    DIR *opendir(const char *name)
    {
        //        printf("opendir: %s\n", name);
        auto di = allocateDir();
        if (di < 0)
        {
            return nullptr;
        }

        auto *d = getDir(di);
        if (f_opendir(d, name) != FR_OK)
        {
            freeDir(di);
            return 0;
        }
        return d;
    }

    int closedir(DIR *dir)
    {
        //        printf("closedir: %p\n", dir);
        freeDir(dir);
        return 0;
    }

    struct dirent *readdir(DIR *dir)
    {
        //        printf("readdir: %p\n", dir);

        FILINFO fi;
        do
        {
            if (f_readdir(dir, &fi) != FR_OK)
            {
                printf("read dir error.\n");
            }

            if (!fi.fname[0])
            {
                return nullptr;
            }
        } while (fi.fattrib & AM_HID);

        int di = getDirAllocator().getIndex(dir);
        auto &d = dirents_[di];
        //        printf("%d:%p:%p: name = %s\n", di, &d, d.d_name, fi.fname);
        strcpy(d.d_name, fi.fname);

        return &d;
    }

    void rewinddir(DIR *dir)
    {
        //        printf("rewinddir: %p\n", dir);

        if (f_rewinddir(dir) != FR_OK)
        {
            printf("rewind dir error.\n");
        }
    }

    std::string currentDir_;

    int chdir(const char *path)
    {
        //        printf("chdir: %s\n", path);
        f_chdir(path);
        currentDir_ = path;
        return 0;
    }

    char *getcwd(char *buffer, size_t size)
    {

        if (size)
        {
            strncpy(buffer, currentDir_.c_str(), size);
            //            f_getcwd(buffer, size);
        }
        return buffer;
    }
}
