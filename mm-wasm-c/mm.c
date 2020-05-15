#include "decoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

int main() {
    char * s = "69bb4c271c5e41bcd15e082,\
2a3b1c50ca7037e07cd91d8c995d65f6,\
2f8908de224321baccc727d2b5ccf763,\
f395f2ddb25fbeaa9748b307a9359d33,\
e570c04b9c2393b884606ca6071f000f,\
d177d9c963eb7b7f7e2a7b22b932f00a , \
611598cea8bff4759b9ac61f5366779a,\
8cd49e4a8278cfabb76705493ec3b7ff,\
3daa2c096dcec3206f95b57d209876e0,\
7f837752b3acd30a7c70b30245b8aa7d,\
f22b3b6bee1ad0137a55ac7acdcc6fd0,\
dce028255fff80eef85c16a01e1f7e2,b145a5e9e8d35b1c5bb886c5686e5b76";

    MD5_MOBILE* md5_mobile = 0;
    int hash_len =  prep_data(s, &md5_mobile);
    printf("find %d hashes\n", hash_len);

    int thread_num = sysconf(_SC_NPROCESSORS_ONLN);
    printf("starting %d threads...\n", thread_num);

    pthread_t threads[thread_num];
    THREAD_INFO ti[thread_num];
    for (int i = 0; i <thread_num; i++)
    {
        ti[i].md5_mobile = md5_mobile;
        ti[i].thread_num = thread_num;
        ti[i].threadid = i;
        ti[i].hash_len = hash_len;
        pthread_create(&threads[i], 0, thread_f, (void*) &ti[i]);
    }
    for (int i = 0; i <thread_num; i++)
    {
        pthread_join(threads[i], NULL);
    }

    for (int i = 0; i <hash_len; i++)
    {
        print_md5_mobile(md5_mobile+i);
    }

    free(md5_mobile);
    md5_mobile = 0;
}