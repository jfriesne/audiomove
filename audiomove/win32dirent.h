#ifndef _WIN32_DIRENT_H
#define _WIN32_DIRENT_H


#ifdef __cplusplus
extern "C"
{
#endif

typedef struct DIR DIR;

struct dirent
{
    char *d_name;
};

DIR           *opendir(const char *);
int           closedir(DIR *);
struct dirent *readdir(DIR *);
void          rewinddir(DIR *);


#ifdef __cplusplus
}
#endif

#endif
