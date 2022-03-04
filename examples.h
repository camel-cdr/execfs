
char *kernel =
#include "mnt/(uname -sr | sed @,s/^/@;/;s/$/@;/@,)"
;

double pi_squared =
#include "mnt/(python -c @;import math;print(math.pi * math.pi);@;)"
;

char seed[8] = {
#include "mnt/(head -c 8 /dev/urandom | xxd -i)"
};

