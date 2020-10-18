#include <pthread.h>

void* start_verifier(void* args)
{
    return 0x0;
}

void* start_prover(void* args)
{
    return 0x0;
}

int main()
{    
    pthread_t verifier;
    pthread_create(&verifier, NULL, start_verifier, 0x0);
    pthread_t prover;
    pthread_create(&prover, NULL, start_prover, 0x0);
    pthread_join(verifier, NULL);
}
