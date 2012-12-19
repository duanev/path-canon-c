/* canonicalize an EFI path
 *
 * 2012/12/13
 *
 * The author of this code (a) does not expect anything from you, and (b)
 * is not responsible for any of the problems you may have using this code.
 *
 * compile: gcc -o pathcanon_efi pathcanon_efi.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>


struct dir_component {
    wchar_t * dir;         // one component of the path (no separators)
    int       len;         // length of this component
};


// a way to print struct dir_component entries for debug
static wchar_t lbuf[32];
wchar_t *
dc_str(struct dir_component * dir)
{
    int len = dir->len;

    if (len > sizeof(lbuf)/sizeof(wchar_t)-1)
        len = sizeof(lbuf)/sizeof(wchar_t)-1;

    if (len)
        wcsncpy(lbuf, dir->dir, len);
    lbuf[len] = L'\0';

    return lbuf;
}


/*
 * return the canonical form of a EFI path
 * (backslashes instead of forward, optional volume name)
 *
 * modifies the input buffer (the result is always
 * shorter than, or the same length as the input path).
 *
 * return NULL if the path is invalid,
 * else return the canonicalized input buffer.
 */
wchar_t *
canonicalize_efi_path(wchar_t * volpath, int debug)
{
    struct dir_component * dirs;
    struct dir_component * dp;
    wchar_t * path;
    wchar_t * p;
    wchar_t * q;
    int       n_dirs;
    int       first_dir;
    int       i;
    int       j;

    path = wcsstr(volpath, L":");
    if (path == NULL)
        path = volpath;
    else
        path++;

    if (path[0] == L'\0')
        return volpath;

    //
    // estimate the number of directory components in path
    //

    n_dirs = 1;
    for (p = path; *p; p++)
        if (*p == L'\\')
            n_dirs++;

    if (debug) {
        wprintf(L"========\n");
        wprintf(L"in: %ls (%d)\n", path, n_dirs);
    }

    dirs = malloc(sizeof(struct dir_component) * n_dirs);
    if (dirs == NULL) {
        wprintf(L"canonicalize_path: out of memory\n");
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
        if (*p == L'\\') {
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
            wprintf(L" %2d %ls\n", dirs[i].len, dc_str(&dirs[i]));
    }

    //
    // canonicalize '.' and '..'
    //

    for (i = 0; i < n_dirs; i++) {
        dp = &dirs[i];

        // mark '.' as an empty component
        if (dp->len == 1  &&  dp->dir[0] == L'.') {
            dirs[i].len = 0;
            continue;
        }

        // '..' eliminates the previous non-empty component
        if (dp->len == 2  &&  dp->dir[0] == L'.'  &&  dp->dir[1] == L'.') {
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
        wprintf(L"----\n");
        for (i = 0; i < n_dirs; i++)
            wprintf(L" %2d %ls\n", dirs[i].len, dc_str(&dirs[i]));
    }

    //
    // reconstruct path using non-zero length components
    //

    p = path;
    if (*p == L'\\')             // if path was absolute, keep it that way
        p++;
    first_dir = 1;
    for (i = 0; i < n_dirs; i++)
        if (dirs[i].len > 0) {
            if (!first_dir)
                *p++ = L'\\';
            q = dirs[i].dir;
            j = dirs[i].len;
            while (j--)
                *p++ = *q++;
            first_dir = 0;
        }
    *p = L'\0';

    free(dirs);
    return volpath;
}



struct tests {
    wchar_t * test;
    wchar_t * expected;
    int       debug;
} Tests[] = {
    {L"", L"", 1},
    {L"\\", L"\\", 0},
    {L"\\\\", L"\\", 0},
    {L"\\\\\\", L"\\", 0},
    {L"c:\\", L"c:\\", 0},
    {L"fs0:\\", L"fs0:\\", 0},
    {L"\\abc", L"\\abc", 0},
    {L"\\\\abc", L"\\abc", 0},
    {L"\\\\\\abc", L"\\abc", 0},
    {L"abc", L"abc", 0},
    {L"abc\\", L"abc", 0},
    {L"abc\\\\", L"abc", 0},
    {L"abc\\123", L"abc\\123", 0},
    {L"abc\\\\123", L"abc\\123", 0},
    {L"abc\\\\\\123", L"abc\\123", 0},
    {L"abc\\.\\123", L"abc\\123", 0},
    {L"abc\\x\\..\\123", L"abc\\123", 0},
    {L"c:abc", L"c:abc", 0},
    {L"fs0:abc", L"fs0:abc", 0},
    {L"..", NULL, 0},
    {L"\\..", NULL, 0},
    {L"..\\123", NULL, 0},
    {L"c:..\\123", NULL, 0},
    {L"fs0:..\\123", NULL, 0},
    {L"\\..\\123", NULL, 0},
    {L"\\\\..\\123", NULL, 0},
    {L".\\..\\123", NULL, 0},
    {L".\\", L"", 0},
    {L".\\\\", L"", 0},
    {L".\\\\\\", L"", 0},
    {L".\\abc", L"abc", 0},
    {L".\\.\\abc", L"abc", 0},
    {L".\\..\\abc", NULL, 0},
    {L"c:.\\abc", L"c:abc", 0},
    {L"fs0:.\\abc", L"fs0:abc", 0},
    {L"abc\\.", L"abc", 0},
    {L"abc\\.\\.", L"abc", 0},
    {L"\\abc\\.", L"\\abc", 0},
    {L"\\abc\\.\\.", L"\\abc", 0},
    {L"\\.\\abc\\.", L"\\abc", 0},
    {L"\\abc\\.\\.\\123", L"\\abc\\123", 0},
    {L"abc\\..\\123", L"123", 0},
    {L"\\abc\\..\\123", L"\\123", 0},
    {L"abc\\.\\..\\123", L"123", 0},
    {L"\\abc\\.\\..\\123", L"\\123", 0},
    {L"abc\\def\\..\\123", L"abc\\123", 0},
    {L"\\abc\\def\\..\\123", L"\\abc\\123", 0},
    {L"abc\\def\\..\\..\\123", L"123", 0},
    {L"\\abc\\def\\..\\..\\123", L"\\123", 0},
    {L"\\abc\\..", L"\\", 0},
    {L"abc\\..", L"", 0},
    {L"abc\\123\\..", L"abc", 0},
    {L"\\abc\\123\\..", L"\\abc", 0},
    {L"abc\\123\\..\\..", L"", 0},
    {L"\\abc\\123\\..\\..", L"\\", 0},
    {L"abc\\123\\..\\..\\.", L"", 0},
    {L"\\abc\\123\\..\\..\\.", L"\\", 0},
    {L"abc\\123\\..\\.\\..", L"", 0},
    {L"\\abc\\123\\..\\.\\..", L"\\", 0},
    {L"abc\\\\\\\\..\\\\\\\\z\\\\\\\\", L"z", 0},
    {L"\\\\\\\\\\abc\\\\\\\\..\\\\\\\\z\\\\\\\\", L"\\z", 0},
    {L"d\\.\\e\\..\\.\\o\\f\\g\\.\\h\\..\\..\\..\\.\\n\\.\\.\\e\\.\\i\\..", L"d\\o\\n\\e", 0},
    {0, 0, 0}
};

int
main(int ac, char * av[])
{
    struct tests * tp;
    wchar_t      * buf;
    wchar_t      * result;

    if (ac == 2) {
        int len = (strlen(av[1]) + 1) * sizeof(wchar_t);
        buf = malloc(len);
        mbstowcs(buf, av[1], len);
        wprintf(L"path: %ls\n", canonicalize_efi_path(buf, 1));
        free(buf);
    } else {
        for (tp = Tests; tp->test; tp++) {
            buf = wcsdup(tp->test);
            result = canonicalize_efi_path(buf, tp->debug);
            wprintf(L"%ls -> %ls\n", tp->test, result);
            if (result == NULL  &&  tp->expected == NULL) {
                free(buf);
                continue;
            }
            if ((result == NULL && tp->expected != NULL)
            ||  wcscmp(result, tp->expected)) {
                wprintf(L"failed: %ls, expected %ls, got %ls\n", tp->test,
                       tp->expected, result);
                result = canonicalize_efi_path(buf, 1);
            }
            free(buf);
        }
    }
}

/* vi: set tabstop=8 expandtab softtabstop=4 shiftwidth=4: */
