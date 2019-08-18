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

extern "C"
{
    constexpr int FD_OFFSET = 1024;
    constexpr int MAX_FILES = 8;

    std::array<FIL, MAX_FILES> files_;
    uint32_t freeFileMask_ = MAX_FILES - 1;

    int allocateFile()
    {
        for (int i = 0; i < MAX_FILES; ++i)
        {
            if (freeFileMask_ & (1 << i))
            {
                freeFileMask_ &= ~(1 << i);
                return i + FD_OFFSET;
            }
        }
        return -1;
    }

    void freeFile(int fp)
    {
        fp -= FD_OFFSET;
        freeFileMask_ |= (1 << fp);
    }

    FIL *getFile(int fp)
    {
        return &files_[fp - FD_OFFSET];
    }

    constexpr const char *TAG = "STDFILE";

    int
    sys_file_open(const char *name, int flags, int mode)
    {
        LOGV(TAG, "open(%s, %d, %d)", name, flags, mode);

        int ffmode = 0;
        if ((flags & 3) == O_RDONLY)
        {
            LOGV(TAG, " read only.\n");
            ffmode |= FA_READ;
        }
        if ((flags & 3) == O_WRONLY)
        {
            LOGV(TAG, " write only.\n");
            ffmode |= FA_WRITE;
        }
        if ((flags & 3) == O_RDWR)
        {
            LOGV(TAG, " read/write.\n");
            ffmode |= FA_READ | FA_WRITE;
        }

        if (flags & O_CREAT)
        {
            LOGV(TAG, " create.\n");
            ffmode |= FA_CREATE_NEW;
        }

        if (flags & O_TRUNC)
        {
            LOGV(TAG, " trunc.\n");
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

    long
    sys_file_stat(const char *name, struct stat *st)
    {
        LOGV(TAG, "file_stat(%s, %p)", name, st);
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
        return 0;
    }

    int
    sys_file_close(int file)
    {
        LOGV(TAG, "file_close(%d)\n", file);

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

    int chdir(const char *path)
    {
        printf("chdir: %s\n", path);
        return 0;
    }

    DIR *opendir(const char *name)
    {
        printf("opendir: %s\n", name);
        return 0;
    }

    int closedir(DIR *dir)
    {
        printf("closedir: %p\n", dir);
        return 0;
    }

    struct dirent *readdir(DIR *dir)
    {
        printf("readdir: %p\n", dir);
        return 0;
    }

    void rewinddir(DIR *dir)
    {
        printf("rewinddir: %p\n", dir);
    }

    char *getcwd(char *buffer, size_t size)
    {
        if (size)
        {
            buffer[0] = 0;
        }
        return buffer;
    }
}
