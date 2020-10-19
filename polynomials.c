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
                s_1 = rand() % 3;
                a = rand() % 3;
                int* enc_s = malloc(sizeof(int)*(degree+1)*2);
                for (int i = 0; i <= degree; i++) {
                    enc_s[i] = (int)pow(g, pow(s_1, i)) % n;
                    enc_s[degree+1+i] = (int)pow(g, a*pow(s_1,i)) % n;
                }
                printf("%d %d\n", s_1, a);
                for (int i = 0; i < 8; i++) printf("%d ", enc_s[i]);
                printf("\n");
                msg = enc_s;
            } else if (recvlen == 12) {
                if (pow(((int*)buf)[0], a) == ((int*)buf)[1]) {
                    printf(" VERIFIER │ Polynomial of valid form used! pow(%d, %d) == %d\n", ((int*)buf)[0], a, ((int*)buf)[1]);
                }
                else {
                    printf(" VERIFIER │ Polynomial of invalid form used! pow(%d, %d) ≠ %d\n", ((int*)buf)[0], a, ((int*)buf)[1]);
                    return 0x1;
                }
                if (((int*)buf)[0] == pow(((int*)buf)[2], (s_1-2)*(s_1-1))) {
                    printf(" VERIFIER │ Valid proof! pow(%d, %d) == %d\n", ((int*)buf)[2], (s_1-2)*(s_1-1), ((int*)buf)[0]);
                }
                else {
                    printf(" VERIFIER │ Invalid proof! pow(%d, %d) ≠ %d\n", ((int*)buf)[2], (s_1-2)*(s_1-1), ((int*)buf)[0]);
                    return 0x1;
                }
            }
            if (msg != 0x0) sendto(s, msg, 32, 0, (struct sockaddr *) &remaddr, addrlen);
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
        printf("Test\n");
        if (recvlen > 0) {
            buf[recvlen] = 0;
            printf(" Prover   │ Received %d-byte message from server: \"%s\"\n", recvlen, buf);
            for (int i = 0; i < 8; i++) printf("%d ", ((int*)buf)[i]);
            printf("\n");
            int enc_ph[3] = {1,1,1};
            for (int i = 0; i <= degree; i++) {
                enc_ph[0] *= (int)pow(((int*)buf)[i], constants[i]);
                printf("%d %d\n", enc_ph[0], (int)pow(((int*)buf)[i], constants[i]));
                enc_ph[1] *= (int)pow(((int*)buf)[i+degree+1], constants[i]);
                printf("%d %d\n", enc_ph[1], (int)pow(((int*)buf)[i+1+degree], constants[i]));
            }
            enc_ph[2] = (((int*)buf)[1]); // Use actual polynomial division in the future?
            printf("%d %d %d\n", enc_ph[0], enc_ph[1], enc_ph[2]);
            sendto(s, enc_ph, 12, 0, (struct sockaddr*)NULL, sizeof(addr));
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
