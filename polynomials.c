#include <pthread.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <gmp.h>

#define PORT 5000
#define BUFSIZE 1024

void* start_verifier(void* args)
{
    // Intialize socket with AF_INET IP family and SOCK_DGRAM datagram service, exit if failed
    int s;
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        exit(1);
    }
    // Establish sockaddr_in struct to pass into bind function
    struct sockaddr_in addr;
    memset((char *)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET; // Specify address family.
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY just 0.0.0.0, machine IP address
    addr.sin_port = htons(PORT); // Specify port.
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        exit(1);
    }

    // IP address of client
    struct sockaddr_in remaddr;
    socklen_t addrlen = sizeof(remaddr);
    int recvlen;
    unsigned char buf[BUFSIZE];

    int connections = 0;
    int clients[500];
    int unproven = 1;
    int g = 6;
    int degree = 3;
    int n = 17;
    int s_1;
    int a;
    while (unproven) {
        recvlen = recvfrom(s, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
        if (recvlen > BUFSIZE) sendto(s, "ERR: Too long.", 14, 0, (struct sockaddr *) &remaddr, addrlen);
        else if (recvlen > 0) {
            void* msg = 0x0;
            buf[recvlen] = 0;
            printf(" VERIFIER │ Received %d-byte message from %i: \"%s\"\n", recvlen, remaddr.sin_port, buf);
            if (strcmp((const char*) buf, "Begin proof.")==0) {
                clients[connections] = remaddr.sin_port;
                connections++;
                srand(time(0));
                s_1 = rand() % 100;
                a = rand() % 100;
                printf("%d %d\n", s_1, a);
                mpz_t* enc_s = malloc(sizeof(mpz_t)*(degree+1)*2);
                for (int i = 0; i <= degree; i++) {
                    mpz_init(enc_s[i]);
                    mpz_ui_pow_ui(enc_s[i], g, (int)pow(s_1, i));
                    mpz_init(enc_s[degree+1+i]);
                    mpz_ui_pow_ui(enc_s[i+1+degree], g, (int)a*pow(s_1, i));
                }
                //for (int i = 0; i < 8; i++) gmp_printf("%Zd ", enc_s[i]);
                //printf("\n");
                msg = enc_s;
            } else if (recvlen == sizeof(mpz_t)*3) {
                for (int i = 0; i < 3; i++) printf("%Zd ", ((mpz_t*)buf)[i]);
                printf("\n");
                if (((int*)buf)[0] == pow(((int*)buf)[2], s_1)) printf("Valid roots!\n");
                if (((int*)buf)[1] == pow(((int*)buf)[0], a)) printf("Valid form!\n");
            }
            if (msg != 0x0) sendto(s, msg, sizeof(mpz_t)*(degree+1)*2, 0, (struct sockaddr *) &remaddr, addrlen);
        }
    }
    
    return 0x0;
}

void* start_prover(void* args)
{
    // Same socket is needed on client end so initialize all over again.
    int s;
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("\n Error : Socket Failed \n");
    }
    struct sockaddr_in addr;
    memset((char *)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET; // Specify address family.
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY just 0.0.0.0, machine IP address
    addr.sin_port = htons(PORT); // Specify port.
    // Connect to server
    if(connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("ERR: Connect failed.\n");
        return 0x0;
    }
    // NOTE This can and will not work if flag argument set to 1
    sendto(s, "Begin proof.", BUFSIZE, 0, (struct sockaddr*)NULL, sizeof(addr));
    printf(" Prover   │ Informed server of existence.\n");
    char buf[BUFSIZE];
    int recvlen;
    socklen_t len = sizeof(addr);
    int phase = 0;
    int constants[4] = {0, 2, -3, 1};
    int degree = 3;
    while (1==1) {
        recvlen = recvfrom(s, buf, BUFSIZE, 0, (struct sockaddr *) &addr, &len);
        if (recvlen > 0) {
            buf[recvlen] = 0;
            printf(" Prover   │ Received %d-byte message from server: \"%s\"\n", recvlen, buf);
            mpz_t* enc_ph = malloc(sizeof(mpz_t)*3);
            mpz_init_set_ui(enc_ph[0], 1);
            mpz_init_set_ui(enc_ph[1], 1);
            for (int i = 0; i < degree+1; i++) {
                mpz_pow_ui(((mpz_t*)buf)[i], abs(constants[i]));
                mpz_mul(enc_ph[0], enc_ph[0], ((mpz_t*)buf)[i]);
                mpz_pow_ui(((mpz_t*)buf)[i+degree+1], abs(constants[i]));
                mpz_mul(enc_ph[1], enc_ph[1], ((mpz_t*)buf)[i+1+degree]);
            }
            mpz_init_set_ui(enc_ph[2], ((int*)buf)[1]);
            sendto(s, enc_ph, sizeof(mpz_t)*3, 0, (struct sockaddr*)NULL, sizeof(addr));
        }
    }
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
