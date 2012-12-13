/* canonicalize a path
 *
 * 2012/12/12
 *
 * The author of this code (a) does not expect anything from you, and (b)
 * is not responsible for any of the problems you may have using this code.
 *
 * compile: gcc -o pathcanon pathcanon.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct dir_component {
    char * dir;         // one component of the path (no separators)
    int    len;         // length of this component
};


// a way to print struct dir_component entries for debug
static char lbuf[32];
char *
dc_str(struct dir_component * dir)
{
    int len = dir->len;

    if (len > sizeof(lbuf)-1)
        len = sizeof(lbuf)-1;

    if (len)
        strncpy(lbuf, dir->dir, len);
    lbuf[len] = '\0';

    return lbuf;
}


/*
 * return the canonical form of a path
 *
 * modifies the input buffer (the result is always
 * shorter than, or the same length as the input path).
 *
 * return NULL if the path is invalid,
 * else return the input buffer.
 */
char *
canonicalize_path(char * path, int debug)
{
    struct dir_component * dirs;
    struct dir_component * dp;
    char * p;
    char * q;
    int    n_dirs;
    int    first_dir;
    int    i;
    int    j;

    if (path[0] == '\0')
        return NULL;

    //
    // estimate the number of directory components in path
    //

    n_dirs = 1;
    for (p = path; *p; p++)
        if (*p == '/')
            n_dirs++;

    if (debug) {
        printf("========\n");
        printf("in: %s (%d)\n", path, n_dirs);
    }

    dirs = malloc(sizeof(struct dir_component) * n_dirs);
    if (dirs == NULL) {
        printf("canonicalize_path: out of memory\n");
        return NULL;
    }

    //
    // break path into component parts
    //
    // path separators mark the start of each directory component
    // (except maybe the first)
    //

    n_dirs = 0;
    dirs[n_dirs].dir = path;
    dirs[n_dirs].len = 0;

    for (p = path; *p; p++) {
        if (*p == '/') {
            // found a new component, save its location
            // and start counting its length
            n_dirs++;
            dirs[n_dirs].dir = p + 1;
            dirs[n_dirs].len = 0;
        } else {
            dirs[n_dirs].len++;
        }
    }
    n_dirs++;

    if (debug) {
        for (i = 0; i < n_dirs; i++)
            printf(" %2d %s\n", dirs[i].len, dc_str(&dirs[i]));
    }

    //
    // canonicalize '.' and '..'
    //

    for (i = 0; i < n_dirs; i++) {
        dp = &dirs[i];

        // mark '.' as an empty component
        if (dp->len == 1  &&  dp->dir[0] == '.') {
            dirs[i].len = 0;
            continue;
        }

        // '..' eliminates the previous non-empty component
        if (dp->len == 2  &&  dp->dir[0] == '.'  &&  dp->dir[1] == '.') {
            dirs[i].len = 0;
            for (j = i - 1; j >= 0; j--)
                if (dirs[j].len > 0)
                    break;
            if (j < 0) {
                free(dirs);     // no previous components are available,
                return NULL;    // the path is invalid
            }
            dirs[j].len = 0;
        }
    }

    if (debug) {
        printf("----\n");
        for (i = 0; i < n_dirs; i++)
            printf(" %2d %s\n", dirs[i].len, dc_str(&dirs[i]));
    }

    //
    // reconstruct path using non-zero length components
    //

    p = path;
    if (*p == '/')              // if path was absolute, keep it that way
        p++;
    first_dir = 1;
    for (i = 0; i < n_dirs; i++)
        if (dirs[i].len > 0) {
            if (!first_dir)
                *p++ = '/';
            q = dirs[i].dir;
            j = dirs[i].len;
            while (j--)
                *p++ = *q++;
            first_dir = 0;
        }
    *p = '\0';

    free(dirs);
    return path;
}



struct tests {
    char * test;
    char * expected;
    int    debug;
} Tests[] = {
    {"/", "/", 0},
    {"//", "/", 0},
    {"///", "/", 0},
    {"/abc", "/abc", 0},
    {"//abc", "/abc", 0},
    {"///abc", "/abc", 0},
    {"abc", "abc", 0},
    {"abc/", "abc", 0},
    {"abc//", "abc", 0},
    {"abc/123", "abc/123", 0},
    {"abc//123", "abc/123", 0},
    {"abc///123", "abc/123", 0},
    {"abc/./123", "abc/123", 0},
    {"abc/x/../123", "abc/123", 0},
    {"..", NULL, 0},
    {"/..", NULL, 0},
    {"../123", NULL, 0},
    {"/../123", NULL, 0},
    {"//../123", NULL, 0},
    {"./../123", NULL, 0},
    {"./", "", 0},
    {".//", "", 0},
    {".///", "", 0},
    {"./abc", "abc", 0},
    {"././abc", "abc", 0},
    {"./../abc", NULL, 0},
    {"abc/.", "abc", 0},
    {"abc/./.", "abc", 0},
    {"/abc/.", "/abc", 0},
    {"/abc/./.", "/abc", 0},
    {"/./abc/.", "/abc", 0},
    {"/abc/././123", "/abc/123", 0},
    {"abc/../123", "123", 0},
    {"/abc/../123", "/123", 0},
    {"abc/./../123", "123", 0},
    {"/abc/./../123", "/123", 0},
    {"abc/def/../123", "abc/123", 0},
    {"/abc/def/../123", "/abc/123", 0},
    {"abc/def/../../123", "123", 0},
    {"/abc/def/../../123", "/123", 0},
    {"/abc/..", "/", 0},
    {"abc/..", "", 0},
    {"abc/123/..", "abc", 0},
    {"/abc/123/..", "/abc", 0},
    {"abc/123/../..", "", 0},
    {"/abc/123/../..", "/", 0},
    {"abc/123/../../.", "", 0},
    {"/abc/123/../../.", "/", 0},
    {"abc/123/.././..", "", 0},
    {"/abc/123/.././..", "/", 0},
    {"abc////..////z////", "z", 0},
    {"/////abc////..////z////", "/z", 0},
    {"d/./e/.././o/f/g/./h/../../.././n/././e/./i/..", "d/o/n/e", 0},
    {0, 0, 0}
};

int
main(int ac, char * av[])
{
    struct tests * tp;
    char         * buf;
    char         * result;

    if (ac == 2) {
        printf("path: %s\n", canonicalize_path(av[1], 1));
    } else {
        for (tp = Tests; tp->test; tp++) {
            buf = strdup(tp->test);
            result = canonicalize_path(buf, tp->debug);
            printf("%s -> %s\n", tp->test, result);
            if (result == NULL  &&  tp->expected == NULL) {
                free(buf);
                continue;
            }
            if ((result == NULL && tp->expected != NULL)
            ||  strcmp(result, tp->expected)) {
                printf("failed: %s, expected %s, got %s\n", tp->test,
                       tp->expected, result);
                result = canonicalize_path(buf, 1);
            }
            free(buf);
        }
    }
}

/* vi: set tabstop=8 expandtab softtabstop=4 shiftwidth=4: */
