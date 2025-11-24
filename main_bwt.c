// fast_bwt.c  -- replace bwt_encode implementation with this file's contents
// Uses suffix array building by doubling + counting/radix sort (O(n log n))

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cmp_suffixes_by_rank(const int* rank, int a, int b, int k, int n) {
  if (rank[a] != rank[b])
    return rank[a] < rank[b] ? -1 : 1;
  int ra = (a + k < n) ? rank[a + k] : -1;
  int rb = (b + k < n) ? rank[b + k] : -1;
  if (ra != rb)
    return ra < rb ? -1 : 1;
  return 0;
}

/* Build suffix array using doubling + counting/radix sort approach.
   Returns pointer to int array of length n (caller must free).
*/
static int* build_suffix_array(const unsigned char* s, int n) {
  int i, k;
  int* sa = malloc(n * sizeof(int));
  int* rank = malloc(n * sizeof(int));
  int* tmp = malloc(n * sizeof(int));
  if (!sa || !rank || !tmp) {
    free(sa);
    free(rank);
    free(tmp);
    return NULL;
  }

  for (i = 0; i < n; ++i) {
    sa[i] = i;
    rank[i] = s[i];
  }

  for (k = 1; k < n; k <<= 1) {
    // Sort by (rank[i], rank[i+k]) using radix/counting sort twice.
    // First sort by second key (rank[i+k]).
    int maxv = 256;
    // But ranks can grow beyond 256; find max rank currently present
    for (i = 0; i < n; ++i)
      if (rank[i] + 1 > maxv)
        maxv = rank[i] + 1;

    int* cnt = calloc(maxv + 2, sizeof(int));
    if (!cnt) {
      free(sa);
      free(rank);
      free(tmp);
      return NULL;
    }

    // sort by second key
    for (i = 0; i < n; ++i) {
      int key = (i + k < n) ? rank[i + k] + 1 : 0;  // 0 for -1
      cnt[key]++;
    }
    for (i = 1; i <= maxv + 1; ++i)
      cnt[i] += cnt[i - 1];

    // produce an ordering by second key into tmp_sa
    int* tmp_sa = malloc(n * sizeof(int));
    if (!tmp_sa) {
      free(cnt);
      free(sa);
      free(rank);
      free(tmp);
      return NULL;
    }
    for (i = n - 1; i >= 0; --i) {
      int idx = i;
      int key = (idx + k < n) ? rank[idx + k] + 1 : 0;
      tmp_sa[--cnt[key]] = idx;
    }
    free(cnt);

    // Now sort tmp_sa by first key (rank[tmp_sa[i]]) using counting sort
    // find max first key
    int maxr = 0;
    for (i = 0; i < n; ++i)
      if (rank[i] > maxr)
        maxr = rank[i];
    cnt = calloc((maxr + 2), sizeof(int));
    if (!cnt) {
      free(tmp_sa);
      free(sa);
      free(rank);
      free(tmp);
      return NULL;
    }
    for (i = 0; i < n; ++i)
      cnt[rank[i]]++;
    for (i = 1; i <= maxr + 1; ++i)
      cnt[i] += cnt[i - 1];

    for (i = n - 1; i >= 0; --i) {
      int idx = tmp_sa[i];
      int key = rank[idx];
      sa[--cnt[key]] = idx;
    }
    free(cnt);
    free(tmp_sa);

    // now compute new ranks into tmp[]
    tmp[sa[0]] = 0;
    for (i = 1; i < n; ++i) {
      tmp[sa[i]] = tmp[sa[i - 1]] +
                   (cmp_suffixes_by_rank(rank, sa[i - 1], sa[i], k, n) < 0);
    }
    // copy tmp -> rank
    for (i = 0; i < n; ++i)
      rank[i] = tmp[i];

    if (rank[sa[n - 1]] == n - 1)
      break;  // all ranks distinct -> done
  }

  free(rank);
  free(tmp);
  return sa;
}

/* bwt_encode: build suffix array, compute BWT string and primary index.
   input: null-terminated C string
   returns allocated char* (caller frees), sets *original_index.
*/
char* bwt_encode(const char* input, int* original_index) {
  if (!input)
    return NULL;
  size_t n = strlen(input);
  if (n == 0) {
    *original_index = 0;
    char* out = malloc(1);
    if (out)
      out[0] = '\0';
    return out;
  }

  // For safety, we will work with unsigned char buffer
  unsigned char* s = malloc(n);
  if (!s)
    return NULL;
  memcpy(s, input, n);

  int* sa = build_suffix_array(s, (int)n);
  if (!sa) {
    free(s);
    return NULL;
  }

  // Build BWT: last column characters
  char* bwt = malloc(n + 1);
  if (!bwt) {
    free(s);
    free(sa);
    return NULL;
  }

  int primary = -1;
  for (int i = 0; i < (int)n; ++i) {
    int pos = sa[i];
    int lastpos = (pos == 0) ? (int)n - 1 : pos - 1;
    bwt[i] = (char)s[lastpos];
    // original string corresponds to suffix starting at 0
    if (pos == 0)
      primary = i;
  }
  bwt[n] = '\0';

  *original_index = primary;

  free(s);
  free(sa);
  return bwt;
}

int cmp_uc(const void *a, const void *b) {
    unsigned char ca = *(const unsigned char*)a;
    unsigned char cb = *(const unsigned char*)b;
    return (int)ca - (int)cb;
}

char* bwt_decode(const char* bwt, int original_index) {
  int len = strlen(bwt);
  char* F = malloc(len + 1);
  memcpy(F, bwt, len);
  F[len] = '\0';
  qsort(F, len, sizeof(char), cmp_uc);

  int countF[256] = {0};
  for (int i = 0; i < len; i++)
    countF[(unsigned char)F[i]]++;

  int* rankL = calloc(len, sizeof(int));
  int occ[256] = {0};
  for (int i = 0; i < len; i++) {
    unsigned char uc = (unsigned char)bwt[i];
    rankL[i] = occ[uc]++;
  }

  int firstOcc[256];
  int total = 0;
  for (int c = 0; c < 256; c++) {
    if (countF[c] > 0) {
      firstOcc[c] = total;
      total += countF[c];
    } else
      firstOcc[c] = -1;
  }

  int* LF = malloc(len * sizeof(int));
  for (int i = 0; i < len; i++)
    LF[i] = firstOcc[(unsigned char)bwt[i]] + rankL[i];

  char* decoded = malloc(len + 1);
  int pos = original_index;
  for (int i = len - 1; i >= 0; i--) {
    decoded[i] = bwt[pos];
    pos = LF[pos];
  }
  decoded[len] = '\0';

  free(F);
  free(rankL);
  free(LF);
  return decoded;
}