#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include "Reliable.h"

void usage(char *name)
{
    printf("usage: %s [filename]\n", name);
    printf("    -h,                   show help message and exit\n");
    printf("    -p local_port,        port for the local end (default 10000)\n");
    printf("    -r remote_port,       port for the remote end (default 50001)\n");
    printf("    -n sequence_number,   initial sequence number in SYN (default at random)\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    struct option longopts[] = {
        {"local_port", required_argument, NULL, 'p'},
        {"remote_port", required_argument, NULL, 'r'},
        {0, 0, 0, 0}};

    int c, local_port = 10000, remote_port = 50001;
    unsigned long n = 0;
    bool nflag = false;
    while ((c = getopt_long(argc, argv, "p:r:n:h", longopts, NULL)) != -1)
    {
        switch (c)
        {
        case 'p':
            local_port = atoi(optarg);
            break;
        case 'r':
            remote_port = atoi(optarg);
            break;
        case 'n':
            n = strtoul(optarg, NULL, 10);
            nflag = true;
            break;
        case '?':
        case 'h':
        default:
            usage(argv[0]);
            break;
        }
    }

    if (optind >= argc) //getopt() cannot permutate options on MacOS
        usage(argv[0]);

    Reliable reli(local_port, remote_port);
    reli.connect(nflag, n);

    FILE *fin = fopen(argv[optind], "r");
    while (true)
    {
        Task block(BLOCK_SIZE);
        block.len = fread(block.buf, 1, block.len, fin);
        if (block.len == 0)
            break;
        reli.send(block);
    }
    fclose(fin);
    reli.Close();
    return 0;
}