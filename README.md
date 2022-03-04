# ExecFS

Proof of concept userspace filesystem that executes filenames as shell commands and makes the result accessible though reading the file.

```bash
$ ./execfs mnt # or make mount
$ cat 'mnt/(uname)'
Linux
$ cat "mnt/unique text(seq 10 | awk '{c+=\$1}END{print(c)}')unique text"
55
$ fusermount -u mnt # or make umount
```

This can be exploited for all kinds of shenanigans in combination with the c preprocessor, because it allows for arbitrary code execution at compile time. (Please don't use this in production)

```bash
$ cat examples.h
char seed[8] = {
#include "mnt/(head -c 8 /dev/urandom | xxd -i)"
};

double pi_squared =
#include "mnt/(python -c @;import math;print(math.pi * math.pi);@;)"
;

char *kernel =
#include "mnt/(uname -sr | sed @,s/^/@;/;s/$/@;/@,)"
;
$ cpp -w -P examples.h
char seed[8] = {
  0xf9, 0x17, 0x93, 0x83, 0xf6, 0x90, 0xca, 0xa7
};
double pi_squared =
9.86960440109
;
char *kernel =
"Linux 5.16.0-1-amd64"
;
```

The command must be wrapped in parentheses, possibly preceded and succeeded by a unique string that doesn't occur inside the command.
This is needed to distinguish between actual commands that should be executed, and other OS related filesystem quarries.

Since include directives don't allow for the inclusion of  `'`, `\`, `"`, `//`, and `/*` characters, the following escape sequences are supported:

| sequence | replacement |
| -------- | ----------- |
| `@,`     | `'`         |
| `@;`     | `"`         |
| `@/`     | `\\`        |
| `@ `     | nothing     |
| `@@`     | `@`         |


## Licensing
This project is licensed under [LICENSE](LICENSE).

