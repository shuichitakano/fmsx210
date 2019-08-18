#ifndef E6806387_D134_1650_3270_AFE1D4F18F25
#define E6806387_D134_1650_3270_AFE1D4F18F25

#include <sd_card/ff.h>

#ifdef __cplusplus
extern "C"
{
#endif

    struct dirent
    {
        char *d_name;
    };

    DIR *opendir(const char *name);
    int closedir(DIR *dir);
    struct dirent *readdir(DIR *dir);
    void rewinddir(DIR *dir);

#ifdef __cplusplus
}
#endif

#endif /* E6806387_D134_1650_3270_AFE1D4F18F25 */
