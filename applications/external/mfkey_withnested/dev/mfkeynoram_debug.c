// TODO: Only runs with NUM_CHUNKS=1 and CHUNK_DIV=1
// TODO: Unused "s" in recover?
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// 64 loops, ~82 KB of total RAM per loop
// (5 << 19)/64 = 40960
//#define NUM_CHUNKS 64
//#define CHUNK_DIV 128
//#define NUM_CHUNKS 4
//#define CHUNK_DIV 4
#define NUM_CHUNKS 1
#define CHUNK_DIV 2
#define LF_POLY_ODD (0x29CE5C)
#define LF_POLY_EVEN (0x870804)
#define BIT(x, n) ((x) >> (n) & 1)
#define BEBIT(x, n) BIT(x, (n) ^ 24)
#define SWAPENDIAN(x) (x = (x >> 8 & 0xff00ff) | (x & 0xff00ff) << 8, x = x >> 16 | x << 16)
#define SIZEOF(arr) sizeof(arr) / sizeof(*arr)
struct Crypto1State {uint32_t odd, even;};
struct Crypto1Params {uint64_t key;uint32_t nr0_enc, uid_xor_nt0, uid_xor_nt1, nr1_enc, p64b, ar1_enc;};
uint32_t prng_successor(uint32_t x, uint32_t n) {
    SWAPENDIAN(x);
    while (n--)
        x = x >> 1 | (x >> 16 ^ x >> 18 ^ x >> 19 ^ x >> 21) << 31;
    return SWAPENDIAN(x);
}
static inline int filter(uint32_t const x) {
    uint32_t f;
    f  = 0xf22c0 >> (x       & 0xf) & 16;
    f |= 0x6c9c0 >> (x >>  4 & 0xf) &  8;
    f |= 0x3c8b0 >> (x >>  8 & 0xf) &  4;
    f |= 0x1e458 >> (x >> 12 & 0xf) &  2;
    f |= 0x0d938 >> (x >> 16 & 0xf) &  1;
    return BIT(0xEC57E80A, f);
}
static inline uint8_t evenparity32(uint32_t x) {
    return (__builtin_parity(x) & 0xFF);
}
int binsearch(int data[], int start, int stop) {
    int mid, val = data[stop] & 0xff000000;
    while (start != stop) {
        mid = (stop - start) >> 1;
        if ((data[start + mid] ^ 0x80000000) > (val ^ 0x80000000))
            stop = start + mid;
        else
            start += mid + 1;
    }
    return start;
}
void quicksort(int array[], int low, int high) {
    if (SIZEOF(array) == 0)
        return;
    if (low >= high)
        return;
    int middle = low + (high - low) / 2;
    int pivot = array[middle];
    int i = low, j = high;
    while (i <= j) {
        while (array[i] < pivot) {
            i++;
        }
        while (array[j] > pivot) {
            j--;
        }
        if (i <= j) { // swap
            int temp = array[i];
            array[i] = array[j];
            array[j] = temp;
            i++;
            j--;
        }
    }
    if (low < j) {
        quicksort(array, low, j);
    }
    if (high > i) {
        quicksort(array, i, high);
    }
}
void update_contribution(int data[], int item, int mask1, int mask2) {
    int p = data[item] >> 25;
    p = p << 1 | evenparity32(data[item] & mask1);
    p = p << 1 | evenparity32(data[item] & mask2);
    data[item] = p << 24 | (data[item] & 0xffffff);
}
int extend_table(int data[], int tbl, int end, int bit, int m1, int m2) {
    for (data[tbl] <<= 1; tbl <= end; data[++tbl] <<= 1) {
        if ((filter(data[tbl]) ^ filter(data[tbl] | 1)) != 0) {
            data[tbl] |= filter(data[tbl]) ^ bit;
            update_contribution(data, tbl, m1, m2);
        } else if (filter(data[tbl]) == bit) {
            data[++end] = data[tbl + 1];
            data[tbl + 1] = data[tbl] | 1;
            update_contribution(data, tbl, m1, m2);
            tbl++;
            update_contribution(data, tbl, m1, m2);
        } else {
            data[tbl--] = data[end--];
        }
    }
    return end;
}
int extend_table_simple(int data[], int tbl, int end, int bit) {
    for(data[ tbl ] <<= 1; tbl <= end; data[++tbl] <<= 1) {
        if ((filter(data[ tbl ]) ^ filter(data[ tbl ] | 1)) !=0 )
            data[ tbl ] |= filter(data[ tbl ]) ^ bit;
        else if (filter(data[ tbl ]) == bit) {
            data[ ++end ] = data[ ++tbl ];
            data[ tbl ] = data[ tbl - 1 ] | 1;
        } else {
            data[ tbl-- ] = data[ end-- ];
        }
    }
    return end;
}
void crypto1_get_lfsr(struct Crypto1State *state, uint64_t *lfsr) {
    int i;
    for (*lfsr = 0, i = 23; i >= 0; --i) {
        *lfsr = *lfsr << 1 | BIT(state->odd, i ^ 3);
        *lfsr = *lfsr << 1 | BIT(state->even, i ^ 3);
    }
}
uint8_t crypt_or_rollback_bit(struct Crypto1State *s, uint32_t in, int x, int is_crypt) {
    uint8_t ret;
    uint32_t feedin, t;
    if (is_crypt == 0) {
        s->odd &= 0xffffff;
        t = s->odd, s->odd = s->even, s->even = t;
    }
    ret = filter(s->odd);
    feedin = ret & (!!x);
    if (is_crypt == 0) {
        feedin ^= s->even & 1;
        feedin ^= LF_POLY_EVEN & (s->even >>= 1);
    } else {
        feedin ^= LF_POLY_EVEN & s->even;
    }
    feedin ^= LF_POLY_ODD & s->odd;
    feedin ^= !!in;
    if (is_crypt == 0) {
        s->even |= (evenparity32(feedin)) << 23;
    } else {
        s->even = s->even << 1 | (evenparity32(feedin));
        t = s->odd, s->odd = s->even, s->even = t;
    }
    return ret;
}
uint32_t crypt_or_rollback_word(struct Crypto1State *s, uint32_t in, int x, int is_crypt) {
    uint32_t ret = 0;
    int i;
    if (is_crypt == 0) {
        for (i = 31; i >= 0; i--) {
            ret |= crypt_or_rollback_bit(s, BEBIT(in, i), x, 0) << (24 ^ i);
        }
    } else {
        for (i = 0; i <= 31; i++) {
            ret |= crypt_or_rollback_bit(s, BEBIT(in, i), x, 1) << (24 ^ i);
        }
    }
    return ret;
}
int key_already_found_for_nonce(uint64_t *keyarray, size_t keyarray_size, uint32_t uid_xor_nt1, uint32_t nr1_enc, uint32_t p64b, uint32_t ar1_enc) {
    int k = 0, found = 0;
    for(k = 0; k < keyarray_size; k++) {
        struct Crypto1State temp = {0, 0};
        int i;
        for (i = 0; i < 24; i++) {
            (&temp)->odd |= (BIT(keyarray[k], 2*i+1) << (i ^ 3));
            (&temp)->even |= (BIT(keyarray[k], 2*i) << (i ^ 3));
        }
        crypt_or_rollback_word(&temp, uid_xor_nt1, 0, 1);
        crypt_or_rollback_word(&temp, nr1_enc, 1, 1);
        if (ar1_enc == (crypt_or_rollback_word(&temp, 0, 0, 1) ^ p64b)) {
            found = 1;
            break;
        }
    }
    return found;
}
int check_state(struct Crypto1State *t, struct Crypto1Params *p) {
    uint64_t key = 0;
    int found = 0;
    struct Crypto1State temp = {0, 0};
    if (t->odd | t->even) {
        crypt_or_rollback_word(t, 0, 0, 0);
        crypt_or_rollback_word(t, p->nr0_enc, 1, 0);
        crypt_or_rollback_word(t, p->uid_xor_nt0, 0, 0);
        temp.odd = t->odd;
        temp.even = t->even;
        crypt_or_rollback_word(t, p->uid_xor_nt1, 0, 1);
        crypt_or_rollback_word(t, p->nr1_enc, 1, 1);
        if (p->ar1_enc == (crypt_or_rollback_word(t, 0, 0, 1) ^ p->p64b)) {
            crypto1_get_lfsr(&temp, &key);
            p->key = key;
            found = 1;
        }
    }
    return found;
}
int recover(int odd[], int o_head, int o_tail, int oks, int even[], int e_head, int e_tail, int eks, int rem,
            int s, struct Crypto1Params *p) {
    int o, e, i;
    if (rem == -1) {
        for (e = e_head; e <= e_tail; ++e) {
            even[e] = (even[e] << 1) ^ evenparity32(even[e] & LF_POLY_EVEN);
            for (o = o_head; o <= o_tail; ++o, ++s) {
                struct Crypto1State temp = {0, 0};
                temp.even = odd[o];
                temp.odd = even[e] ^ evenparity32(odd[o] & LF_POLY_ODD);
                if (check_state(&temp, p)) {
                    return s;
                }
            }
        }
        return s;
    }
    for (i = 0; (i < 4) && (rem-- != 0); i++) {
        oks >>= 1;
        eks >>= 1;
        o_tail = extend_table(odd, o_head, o_tail, oks & 1, LF_POLY_EVEN << 1 | 1, LF_POLY_ODD << 1);
        if (o_head > o_tail)
            return s;
        e_tail = extend_table(even, e_head, e_tail, eks & 1, LF_POLY_ODD, LF_POLY_EVEN << 1 | 1);
        if (e_head > e_tail)
            return s;
    }
    quicksort(odd, o_head, o_tail);
    quicksort(even, e_head, e_tail);
    while (o_tail >= o_head && e_tail >= e_head) {
        if (((odd[o_tail] ^ even[e_tail]) >> 24) == 0) {
            o_tail = binsearch(odd, o_head, o = o_tail);
            e_tail = binsearch(even, e_head, e = e_tail);
            s = recover(odd, o_tail--, o, oks, even, e_tail--, e, eks, rem, s, p);
        } else if ((odd[o_tail] ^ 0x80000000) > (even[e_tail] ^ 0x80000000)) {
            o_tail = binsearch(odd, o_head, o_tail) - 1;
        } else {
            e_tail = binsearch(even, e_head, e_tail) - 1;
        }
    }
    return s;
}

uint64_t lfsr_recovery32(int ks2, struct Crypto1Params *p) {
  int odd_head = 0, odd_tail = -1, oks = 0;
  int even_head = 0, even_tail = -1, eks = 0;
  int *odd = malloc((5 << 19)/NUM_CHUNKS);
  int *even = malloc((5 << 19)/NUM_CHUNKS);
  int chunk_size = (1 << 20)/CHUNK_DIV;
  int i = 0, j = 0, k = 0, n = 0;
  /* DEBUG */
  int dup_odd_head = 0, dup_odd_tail = -1, dup_oks = 0;
  int dup_even_head = 0, dup_even_tail = -1, dup_eks = 0;
  int *dup_odd = calloc(1, 5 << 19);
  int *dup_even = calloc(1, 5 << 19);
  //int i;
  for (i = 31; i >= 0; i -= 2)
    dup_oks = dup_oks << 1 | BEBIT(ks2, i);
  for (i = 30; i >= 0; i -= 2)
    dup_eks = dup_eks << 1 | BEBIT(ks2, i);
  for (i = 1 << 20; i >= 0; --i) {
    if (filter(i) == (dup_oks & 1))
      dup_odd[++dup_odd_tail] = i;
    if (filter(i) == (eks & 1))
      dup_even[++dup_even_tail] = i;
  }
  for(k = 0; k < 4; k++) {
      dup_odd_tail = extend_table_simple(dup_odd, dup_odd_head, dup_odd_tail, ((dup_oks >>= 1) & 1));
      dup_even_tail = extend_table_simple(dup_even, dup_even_head, dup_even_tail, ((dup_eks >>= 1) & 1));
  }
  // Dump dup_odd and dup_even arrays to files named "dup_odd" and "dup_even"
  FILE *dup_odd_file = fopen("dup_odd", "w");
  FILE *dup_even_file = fopen("dup_even", "w");
  fwrite(dup_odd, sizeof(int), dup_odd_tail+1, dup_odd_file);
  fwrite(dup_even, sizeof(int), dup_even_tail+1, dup_even_file);
  fclose(dup_odd_file);
  fclose(dup_even_file);
  /* DEBUG */
  for(i = 1 << 20; i > 0; i -= chunk_size) {
    memset(odd,  0, (5 << 19)/NUM_CHUNKS);
    memset(even, 0, (5 << 19)/NUM_CHUNKS);
    odd_tail  = -1;
    even_tail = -1;
    odd_head = 0;
    even_head = 0;
    oks = 0;
    eks = 0;
    for (n = 31; n >= 0; n -= 2)
      oks = oks << 1 | BEBIT(ks2, n);
    for (n = 30; n >= 0; n -= 2)
      eks = eks << 1 | BEBIT(ks2, n);
    for(j = 0; j < chunk_size; j++) {
      // Do not write to negative indexes
      if (i-j < 0) {
        break;
      }
      //printf("i-j is %i\n", i-j);
      if (filter(i-j) == (oks & 1)) {
        //printf("ooff: %i\n", i-j);
        odd[++odd_tail] = i-j;
      }
      if (filter(i-j) == (eks & 1)) {
        //printf("eoff: %i\n", i-j);
        even[++even_tail] = i-j;
      }
    }
    // Run filter once for zero to mirror opt1 (TODO: Most likely not needed)
    if (i-j == 0) {
        if ((oks & 1) == 0) {
            //printf("ooff: %i\n", i-j);
            odd[++odd_tail] = 0;
        }
        if ((eks & 1) == 0) {
            //printf("eoff: %i\n", i-j);
            even[++even_tail] = 0;
        }
    }
    printf("even tail: %i, odd tail: %i\n", even_tail, odd_tail);
    for(k = 0; k < 4; k++) {
      odd_tail = extend_table_simple(odd, odd_head, odd_tail, ((oks >>= 1) & 1));
      even_tail = extend_table_simple(even, even_head, even_tail, ((eks >>= 1) & 1));
    }
    printf("even tail: %i, odd tail: %i\n", even_tail, odd_tail);
    printf("odd 0-2: %i %i %i, odd tail: %i, even 0-2: %i %i %i, even tail: %i\n", odd[0], odd[1], odd[2], odd[odd_tail], even[0], even[1], even[2], even[even_tail]);
    // Dump odd and even arrays to file named "odd_<chunk>" and "even_<chunk>"
    char odd_filename[20];
    char even_filename[20];
    sprintf(odd_filename, "odd_%i", i/chunk_size);
    sprintf(even_filename, "even_%i", i/chunk_size);
    printf(odd_filename);printf("\n");
    FILE *odd_file = fopen(odd_filename, "w");
    FILE *even_file = fopen(even_filename, "w");
    fwrite(odd, sizeof(int), odd_tail+1, odd_file);
    fwrite(even, sizeof(int), even_tail+1, even_file);
    fclose(odd_file);
    fclose(even_file);
    /*
    // check odd and dup_odd arrays are equivalent
    for (i = 0; i <= odd_tail; i++) {
      if (odd[i] != dup_odd[i]) {
        printf("odd[%i] is %i, dup_odd[%i] is %i\n", i, odd[i], i, dup_odd[i]);
        exit(1);
      }
    }
    printf("Identical odd arrays\n");
    // check even and dup_even arrays are equivalent
    for (i = 0; i <= even_tail; i++) {
      if (even[i] != dup_even[i]) {
        printf("even[%i] is %i, dup_even[%i] is %i\n", i, even[i], i, dup_even[i]);
        exit(1);
      }
    }
    printf("Identical even arrays\n");
    */
    //recover(odd, odd_head, odd_tail, oks, even, even_head, even_tail, eks, 11, 0, p);
  }
  free(odd);
  free(even);
  return p->key;
}

int main(int argc, char *argv[]) {
    struct Crypto1State *s, *t;
    uint64_t key;     // recovered key
    uint32_t uid;     // serial number
    uint32_t nt0;      // tag challenge first
    uint32_t nt1;      // tag challenge second
    uint32_t nr0_enc; // first encrypted reader challenge
    uint32_t ar0_enc; // first encrypted reader response
    uint32_t nr1_enc; // second encrypted reader challenge
    uint32_t ar1_enc; // second encrypted reader response
    int i;
    char *rest;
    char *token;
    size_t keyarray_size;
    uint64_t *keyarray = malloc(sizeof(uint64_t)*1);

    keyarray_size = 0;
    FILE* filePointer;
    int bufferLength = 255;
    char buffer[bufferLength];
    filePointer = fopen(".mfkey32.log", "r"); // TODO: In FAP, use full path

    while(fgets(buffer, bufferLength, filePointer)) {
        rest = buffer;
        for (i = 0; i <= 17; i++) {
            token = strtok_r(rest, " ", &rest);
            switch(i){
                case 5: sscanf(token, "%x", &uid); break;
                case 7: sscanf(token, "%x", &nt0); break;
                case 9: sscanf(token, "%x", &nr0_enc); break;
                case 11: sscanf(token, "%x", &ar0_enc); break;
                case 13: sscanf(token, "%x", &nt1); break;
                case 15: sscanf(token, "%x", &nr1_enc); break;
                case 17: sscanf(token, "%x", &ar1_enc); break;
                default: break; // Do nothing
            }
        }
        uint32_t p64 = prng_successor(nt0, 64);
        uint32_t p64b = prng_successor(nt1, 64);
        struct Crypto1Params p = {0, nr0_enc, uid ^ nt0, uid ^ nt1, nr1_enc, p64b, ar1_enc};
        key = lfsr_recovery32(ar0_enc ^ p64, &p);
        int found = 0;
        for(i = 0; i < keyarray_size; i++) {
            if (keyarray[i] == key) {
                found = 1;
                break;
            }
        }
        if (found == 0) {
            keyarray = realloc(keyarray, sizeof(uint64_t)*(keyarray_size+1));
            keyarray_size += 1;
            keyarray[keyarray_size-1] = key;
        }
    }

    fclose(filePointer);
    printf("Unique keys found:\n");
    for(i = 0; i < keyarray_size; i++) {
        printf("%012" PRIx64 , keyarray[i]);
        printf("\n");
    }
    // TODO: Append key to user dictionary file if missing in file
    return 0;
}
