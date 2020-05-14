// Andrew ID: yuanpenc
// Name: Yuanpeng Cao
#include "cachelab.h"
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define true 1
#define false 0

typedef struct {
    int valid; /* flag for cache validation */
    unsigned long long tag;
    int dirtyByte;
    unsigned long long timeStamp;

} setLine;

typedef setLine *cacheSet;
typedef cacheSet *cache;

typedef struct {
    int b;
    int E;
    int verbosity; /* flag for showing trace info */
    int s;
    int S;
    int B;

} parameter;

typedef struct {
    // result
    int hit;
    int miss;
    int eviction;
    int d_cache;
    int d_evict;
} resultCache;

void printHelp();
resultCache simulator(resultCache result, FILE *traceFile, parameter cachePara,
                      cache myCache);
cache allocateCache(parameter cachePara);
void freeMemory(parameter cachePara, cache myCache);
resultCache loadAndStore(parameter cachePara, resultCache result,
                         cacheSet cur_Set, uint64_t tag, int sFlag);
void updateTimeStamp(cache myCache, parameter cachePara);

int main(int argc, char *const argv[]) {
    parameter cachePara;
    cache myCache = NULL;
    cachePara.verbosity = 0;
    char inputChar;
    char *path;
    while ((inputChar = (getopt(argc, argv, "hvs:E:b:t:"))) != -1) {
        switch (inputChar) {
        case 'h':
            printHelp();
            break;
        case 'v':
            cachePara.verbosity = 1;
            printf("verbosity=1\n");
            break;
        case 's':
            // printf("%c\n",inputChar);
            cachePara.s = atoi(optarg);
            break;
        case 'E':
            // printf("%c\n",inputChar);
            cachePara.E = atoi(optarg);
            break;
        case 'b':
            // printf("%c\n",inputChar);
            cachePara.b = atoi(optarg);
            break;
        case 't':

            path = optarg;
            // printf("%s\n",path);
            break;
        default:
            printHelp();
            exit(-1);
        }
    }

    FILE *traceFile = fopen(path, "r");
    if (cachePara.s < 0 || cachePara.E <= 0 || cachePara.b < 0 ||
        traceFile == NULL) {
        printf("invaild parameter\n");
        exit(-1);
    }
    // calculate all parameter
    cachePara.S = 1 << cachePara.s;
    cachePara.B = 1 << cachePara.b;
    resultCache result;
    result.miss = result.hit = result.eviction = 0;
    // initializeCache

    myCache = allocateCache(cachePara);
    // get the result
    result = simulator(result, traceFile, cachePara, myCache);
    freeMemory(cachePara, myCache);
    printSummary(result.hit, result.miss, result.eviction, result.d_cache,
                 result.d_evict);
    return 0;
}

void printHelp() {
    printf(
        "Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n"
        "  -h Optional help flag that prints usage info\n"
        "  -v Optional verbose flag that display trace info\n"
        "  -s <s>: Number of set index bits (S = 2s is the number of sets) \n"
        "  -E <E>: Associativity (number of lines per set)\n"
        "  -b <b>: Number of block bits (B = 2b is the block size)\n"
        "  -t <tracefile>: Name of the memory trace to replay\n");
}

cache allocateCache(parameter cachePara) {
    cache newCache = NULL;
    newCache = (cache)calloc(cachePara.S, sizeof(cacheSet));
    if (newCache == NULL) {
        printf("calloc cache space failure");
        exit(-1);
    }
    int count = 0;
    while (count < cachePara.S) {
        newCache[count] = calloc(cachePara.E, sizeof(setLine));
        if (newCache[count] == NULL) {
            printf("calloc set space failure");
            exit(-1);
        }
        count++;
    }
    return newCache;
}

void freeMemory(parameter cachePara, cache myCache) {
    if (myCache == NULL) {
        printf("null pointer when free cache");
    }
    for (int i = 0; i < cachePara.S; i++) {
        free(myCache[i]);
    }
    free(myCache);
}

resultCache simulator(resultCache result, FILE *tFile, parameter cachePara,
                      cache myCache) {
    char operation;
    uint64_t address;
    int size;
    result.d_cache = result.d_evict = result.eviction = result.hit =
        result.miss = 0;
    // printf("%d\n",(fscanf(tFile," %c %lx,%d",&operation,&address,&size)));
    while ((fscanf(tFile, " %c %lx,%d\n", &operation, &address, &size) == 3)) {
        uint64_t bite_Set = address >> cachePara.b;
        uint64_t mark_Set = (1 << cachePara.s) - 1;
        uint64_t tag = (address >> cachePara.s) >> cachePara.b;
        uint64_t index_Set = bite_Set & mark_Set;
        cacheSet cur_Set = myCache[index_Set];
        // printf("%c!!!!!\n",operation);
        switch (operation) {
        case 'L':
            result = loadAndStore(cachePara, result, cur_Set, tag, 0);
            break;
        case 'S':
            result = loadAndStore(cachePara, result, cur_Set, tag, 1);
            break;

        default:
            printf("Unknown opreation\n");
            break;
        }
        // updateTimeStamp(myCache, cachePara);
    }
    // calculate dirty bytes count
    result.d_cache = result.d_cache * cachePara.B;
    result.d_evict = result.d_evict * cachePara.B;
    return result;
}

resultCache loadAndStore(parameter cachePara, resultCache result,
                         cacheSet cur_Set, uint64_t tag, int sFlag) {

    // updata timestamp
    for (int i = 0; i < cachePara.E; i++) {
        if (cur_Set[i].valid == 1) {
            cur_Set[i].timeStamp += 1;
        }
    }

    // hit
    for (int i = 0; i < cachePara.E; i++) {
        if ((cur_Set[i].valid == 1) && (cur_Set[i].tag == tag)) {
            if (sFlag) {
                if (cur_Set[i].dirtyByte != 1) {
                    cur_Set[i].dirtyByte = 1;
                    result.d_cache += 1;
                }
            }
            result.hit += 1;
            // refrash timeStamp
            cur_Set[i].timeStamp = 0;

            if (cachePara.verbosity) {
                // printf("hit= %d\n",result.hit);
            }
            return result;
        }
    }
    // miss
    // first, check for empty line and use it.
    for (int i = 0; i < cachePara.E; i++) {
        if (cur_Set[i].valid == 0) {
            cur_Set[i].tag = tag;
            cur_Set[i].valid = 1;
            cur_Set[i].dirtyByte = 0;
            cur_Set[i].timeStamp = 0;
            result.miss += 1;
            if (sFlag) {
                cur_Set[i].dirtyByte = 1;
                result.d_cache += 1;
            }
            return result;
        }
    }
    // not empty line as well as miss, replace the least recent one.
    result.miss += 1;
    result.eviction += 1;
    uint64_t max_timeStamp = 0;
    uint64_t max_index = -1;

    for (int i = 0; i < cachePara.E; i++) {
        if (cur_Set[i].timeStamp > max_timeStamp) {
            max_timeStamp = cur_Set[i].timeStamp;
            max_index = i;
        }
    }
    // printf("%lx",max_index);
    cur_Set[max_index].tag = tag;
    cur_Set[max_index].timeStamp = 0;
    if (sFlag) {
        if (cur_Set[max_index].dirtyByte == 1) {
            cur_Set[max_index].dirtyByte = 1;
            result.d_evict += 1;
        } else {
            cur_Set[max_index].dirtyByte = 1;
            result.d_cache += 1;
        }

    } else {
        if (cur_Set[max_index].dirtyByte == 1) {

            cur_Set[max_index].dirtyByte = 0;
            result.d_evict += 1;
            result.d_cache -= 1;
        }
    }
    return result;
}

void updateTimeStamp(cache myCache, parameter cachePara) {
    for (int i = 0; i < cachePara.S; i++) {
        for (int j = 0; j < cachePara.E; j++) {
            if (myCache[i][j].valid) {
                myCache[i][j].timeStamp += 1;
            }
        }
    }
}