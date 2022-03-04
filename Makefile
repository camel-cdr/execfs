
all: execfs

execfs: execfs.c
	$(CC) $< -Os `pkg-config fuse3 --cflags --libs` -Wall -Wextra -o $@

mount: execfs
	./execfs mnt

mount_forground: execfs
	./execfs -f mnt


umount: execfs
	fusermount -u mnt

clean:
	rm -f execfs
