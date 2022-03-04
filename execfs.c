#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include "stretchy-buffer.h"

typedef Sb(char) sb_char;

static int
cmd_bounds(char const *str, size_t *begin, size_t *end)
{
	size_t len = strlen(str), b = 0, e = len;

	while (b < e && str[e] != ')')
		++b, --e;
	--b;

	if (str[b] != '(' || str[e] != ')' || e - b < 2)
		return 0;

	if (memcmp(str, str + e + 1, b) != 0)
		return 0;

	*begin = b;
	*end = e;
	return 1;
}

static int
cmd_validate(char const *str)
{
	size_t b, e;
	return cmd_bounds(str, &b, &e);
}

static char *
cmd_extract(char const *str)
{
	size_t begin, end;
	if (!cmd_bounds(str, &begin, &end))
		return 0;

	char *ret = malloc(end - begin);
	memcpy(ret, str + begin + 1, end - begin -1);
	ret[end - begin - 1] = 0;
	return ret;
}

static void
cmd_escape(char *str)
{
	char * it = str;
	while (it[0]) {
		if (it[0] != '@') {
			*str++ = *it++;
			continue;
		}

		switch(it[1]) {
		case ',': *str++ = '\''; it += 2; break;
		case ';': *str++ = '"';  it += 2; break;
		case '/': *str++ = '\\'; it += 2; break;
		case '@': *str++ = '@';  it += 2; break;
		case ' ': it += 2; break;
		default: *str++ = *it++; break;
		}
	}
	*str = 0;
}

static sb_char
readproc(char const *cmd)
{
	fprintf(stderr, "executing: %s\n", cmd);
	FILE *f = popen(cmd, "r");
	sb_char str = {0};
	sb_setlen(&str, 32);
	size_t i = 0, n;
	while ((n = fread(str.at + i, 1, sb_len(str) - i, f))) {
		i += n;
		sb_setlen(&str, i + n*2);
	}
	sb_setlen(&str, i);
	pclose(f);
	return str;
}

static int
execfs_getattr(char const *path, struct stat *stbuf, struct fuse_file_info *fi)
{
	fprintf(stderr, "getattr: %s\n", path);

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (cmd_validate(path + 1)) {
		stbuf->st_mode = S_IFREG | 0666;
		stbuf->st_nlink = 1;
		stbuf->st_size = 1024*1024*32;
	} else {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}

	return 0;
}

static int
execfs_open(char const *path, struct fuse_file_info *fi)
{
	fprintf(stderr, "open: %s\n", path);

	if ((fi->flags & O_ACCMODE) != O_RDONLY)
		return -EACCES;

	char *cmd = cmd_extract(path + 1);
	if (!cmd) return 0;
	cmd_escape(cmd);
	puts(cmd);

	sb_char *str = calloc(1, sizeof *str);
	*str = readproc(cmd); /* skip /$ */
	fi->fh = (uint64_t)(intptr_t)(void*)str;
	fi->direct_io = 1;
	free(cmd);

	return 0;
}

static int
execfs_release(char const *path, struct fuse_file_info *fi)
{
	fprintf(stderr, "release: %s\n", path);
	sb_char *sb = (void*)(intptr_t)(uint64_t)fi->fh;
	sb_free(sb);
	free(sb);
	return 0;
}

static int
execfs_read(char const *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	fprintf(stderr, "read: %s\n", path);
	size_t off = (size_t)offset;
	sb_char *str = (void*)(intptr_t)(uint64_t)fi->fh;
	if (off < sb_len(*str)) {
		if (off + size > sb_len(*str))
			size = sb_len(*str) - off;
		memcpy(buf, str->at + off, size);
	} else {
		size = 0;
	}

	return size;
}

static struct fuse_operations const fsops = {
	.getattr = execfs_getattr,
	.open    = execfs_open,
	.release = execfs_release,
	.read    = execfs_read,
};

int
main(int argc, char **argv)
{
	return fuse_main(argc, argv, &fsops, (void*)0);
}
