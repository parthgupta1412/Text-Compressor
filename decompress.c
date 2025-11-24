// decompress.c
// Reverse pipeline: output.bin (Huffman) -> intermediate.rle -> mtf.bin -> bwt
// -> recovered.txt Expects metadata in output.bin.meta written as: int32_t
// primary_index; uint32_t original_length; Uses functions from your existing
// files:
//   void decompress_huffman(const char* input_file, const char* output_file);
//   // main_huffman.c void decompress_rle(const char* input, const char*
//   output);                // main_rle.c char* bwt_decode(const char* bwt, int
//   original_index);                     // main_bwt.c

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Declarations from your existing modules (we don't reimplement them here) */
void decompress_huffman(const char* input_file,
                        const char* output_file);  // from main_huffman.c
void decompress_rle(const char* input, const char* output);  // from main_rle.c
char* bwt_decode(const char* bwt, int original_index);       // from main_bwt.c
unsigned char* mtf_decode(const unsigned char* input,
                          size_t input_len,
                          size_t* output_len);

static long filesize(const char* path) {
  FILE* f = fopen(path, "rb");
  if (!f)
    return -1;
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return -1;
  }
  long s = ftell(f);
  fclose(f);
  return s;
}

int main(void) {
  const char* huff_in = "output.bin";
  const char* meta_file = "output.bin.meta";
  const char* rle_tmp = "intermediate.rle";
  const char* mtf_tmp = "mtf.bin";
  const char* final_txt = "recovered.txt";

  // 1) Read metadata (primary index and original length)
  FILE* m = fopen(meta_file, "rb");
  if (!m) {
    fprintf(stderr,
            "Error: metadata file '%s' not found. Expected primary index + "
            "original length.\n",
            meta_file);
    return 1;
  }
  int32_t primary_index = -1;
  uint32_t original_len = 0;
  if (fread(&primary_index, sizeof(primary_index), 1, m) != 1 ||
      fread(&original_len, sizeof(original_len), 1, m) != 1) {
    fclose(m);
    fprintf(stderr, "Error: failed to read metadata from %s\n", meta_file);
    return 1;
  }
  fclose(m);
  printf("Read metadata: primary_index=%d, original_length=%u\n", primary_index,
         original_len);

  // 2) Huffman-decode output.bin -> intermediate.rle
  printf("Running Huffman decompression: %s -> %s\n", huff_in, rle_tmp);
  decompress_huffman(huff_in,
                     rle_tmp);  // uses your main_huffman.c function.
                                // :contentReference[oaicite:3]{index=3}

  // Check RLE file exists
  long rle_size = filesize(rle_tmp);
  if (rle_size <= 0) {
    fprintf(stderr,
            "Error: RLE intermediate '%s' is empty or missing (size=%ld)\n",
            rle_tmp, rle_size);
    return 1;
  }
  printf("Intermediate RLE size: %ld bytes\n", rle_size);

  // 3) RLE-decode -> mtf.bin
  printf("Running RLE decompression: %s -> %s\n", rle_tmp, mtf_tmp);
  decompress_rle(rle_tmp, mtf_tmp);  // uses your main_rle.c function.
                                     // :contentReference[oaicite:4]{index=4}

  long mtf_size = filesize(mtf_tmp);
  if (mtf_size <= 0) {
    fprintf(stderr,
            "Error: MTF intermediate '%s' is empty or missing (size=%ld)\n",
            mtf_tmp, mtf_size);
    return 1;
  }
  printf("MTF buffer size: %ld bytes\n", mtf_size);

  // 4) Read mtf.bin into memory
  FILE* f = fopen(mtf_tmp, "rb");
  if (!f) {
    fprintf(stderr, "Error opening %s\n", mtf_tmp);
    return 1;
  }
  unsigned char* mtf_buf = malloc(mtf_size ? (size_t)mtf_size : 1);
  if (!mtf_buf) {
    fclose(f);
    fprintf(stderr, "Out of memory\n");
    return 1;
  }
  size_t got = fread(mtf_buf, 1, (size_t)mtf_size, f);
  fclose(f);
  if ((long)got != mtf_size) {
    free(mtf_buf);
    fprintf(stderr, "Error reading %s\n", mtf_tmp);
    return 1;
  }

  // 5) inverse MTF
  printf("Running inverse MTF (length %zu)\n", got);
  size_t bwt_len = 0;
  unsigned char* bwt_buf = mtf_decode(mtf_buf, got, &bwt_len);
  free(mtf_buf);
  if (!bwt_buf) {
    fprintf(stderr, "MTF decode failed\n");
    return 1;
  }

  // ensure null-terminated string for bwt_decode (it expects a C string)
  char* bwt_cstr = malloc(bwt_len + 1);
  if (!bwt_cstr) {
    free(bwt_buf);
    fprintf(stderr, "Out of memory\n");
    return 1;
  }
  memcpy(bwt_cstr, bwt_buf, bwt_len);
  bwt_cstr[bwt_len] = '\0';
  free(bwt_buf);

  // 6) inverse BWT
  printf("Running inverse BWT (primary index %d, length %zu)\n", primary_index,
         bwt_len);
  char* orig = bwt_decode(
      bwt_cstr, primary_index);  // uses your main_bwt.c function.
                                 // :contentReference[oaicite:5]{index=5}
  free(bwt_cstr);
  if (!orig) {
    fprintf(stderr, "BWT decode failed\n");
    return 1;
  }

  // 7) Optionally check that orig length matches original_len (metadata)
  size_t outlen = strlen(orig);
  if ((uint32_t)outlen != original_len) {
    fprintf(stderr,
            "Warning: decoded length %zu differs from metadata original length "
            "%u\n",
            outlen, original_len);
    // still proceed to write what we got
  }

  // 8) Write final output
  FILE* out = fopen(final_txt, "wb");
  if (!out) {
    free(orig);
    fprintf(stderr, "Cannot open %s for writing\n", final_txt);
    return 1;
  }
  fwrite(orig, 1, outlen, out);
  fclose(out);
  free(orig);

  printf("Decompression complete — result written to %s (size %zu bytes)\n",
         final_txt, outlen);

  // done
  return 0;
}
