// main.c
// Pipeline: read <user file> -> BWT -> MTF -> RLE -> Huffman (output.bin)
// Produces output.bin (Huffman payload) and output.bin.meta (primary index +
// original length)

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Prototypes for functions implemented in your uploaded files */
char* bwt_encode(const char* input, int* original_index);  // from main_bwt.c
void compress_huffman_s(const char* input_file,
                        const char* output_file);          // from main_huffman.c
size_t compress_rle_buffer(const unsigned char* input,
                           size_t input_len,
                           unsigned char* output,
                           size_t output_capacity);

/* MTF encode (you must implement this somewhere, e.g. mtf.c) */
unsigned char* mtf_encode(const unsigned char* input,
                          size_t input_len,
                          size_t* out_len);

int main(void) {
  char input_path[512];
  const char* intermediate_rle = "intermediate.rle";
  const char* output_bin = "output.bin";
  const char* meta_file = "output.bin.meta";

  // --- Ask user for input file path ---
  printf("Enter input file path: ");
  if (!fgets(input_path, sizeof(input_path), stdin)) {
    fprintf(stderr, "Error: failed to read input path\n");
    return 1;
  }
  // remove trailing newline (if any)
  size_t ip_len = strlen(input_path);
  if (ip_len > 0 && (input_path[ip_len - 1] == '\n' || input_path[ip_len - 1] == '\r')) {
    input_path[ip_len - 1] = '\0';
  }

  // --- Read entire file into buffer ---
  FILE* f = fopen(input_path, "rb");
  if (!f) {
    fprintf(stderr, "Error: cannot open %s\n", input_path);
    return 1;
  }
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (fsize < 0) {
    fclose(f);
    fprintf(stderr, "Error reading file size\n");
    return 1;
  }

  char* inbuf = malloc(fsize + 1);
  if (!inbuf) {
    fclose(f);
    fprintf(stderr, "Out of memory\n");
    return 1;
  }
  size_t read = fread(inbuf, 1, fsize, f);
  fclose(f);
  inbuf[read] = '\0';  // null-terminate for bwt routines that use strlen

  // --- BWT ---
  int primary_index = -1;
  char* bwt_out = bwt_encode(inbuf, &primary_index);  // from main_bwt.c
  if (!bwt_out) {
    fprintf(stderr, "BWT failed\n");
    free(inbuf);
    return 1;
  }
  size_t bwt_len = strlen(bwt_out);

  // --- MTF ---
  size_t mtf_len = 0;
  unsigned char* mtf_out =
      mtf_encode((const unsigned char*)bwt_out, bwt_len, &mtf_len);
  if (!mtf_out) {
    fprintf(stderr, "MTF failed\n");
    free(inbuf);
    free(bwt_out);
    return 1;
  }

  // --- RLE ---
  size_t rle_capacity = mtf_len * 2 + 16;  // safe upper bound
  unsigned char* rle_out = malloc(rle_capacity);
  if (!rle_out) {
    fprintf(stderr, "Out of memory (RLE)\n");
    free(inbuf);
    free(bwt_out);
    free(mtf_out);
    return 1;
  }
  size_t rle_len = compress_rle_buffer(mtf_out, mtf_len, rle_out, rle_capacity);
  if (rle_len == 0) {
    fprintf(stderr, "RLE failed (insufficient buffer?)\n");
    free(inbuf);
    free(bwt_out);
    free(mtf_out);
    free(rle_out);
    return 1;
  }

  // --- write intermediate RLE to file (binary) ---
  FILE* rfile = fopen(intermediate_rle, "wb");
  if (!rfile) {
    fprintf(stderr, "Cannot write %s\n", intermediate_rle);
    free(inbuf);
    free(bwt_out);
    free(mtf_out);
    free(rle_out);
    return 1;
  }
  if (fwrite(rle_out, 1, rle_len, rfile) != rle_len) {
    fprintf(stderr, "Failed to write intermediate file\n");
    fclose(rfile);
    free(inbuf);
    free(bwt_out);
    free(mtf_out);
    free(rle_out);
    return 1;
  }
  fclose(rfile);

  // --- Huffman compress the intermediate file into output.bin ---
  compress_huffman_s(intermediate_rle, output_bin);

  // --- write metadata (primary index and original length) ---
  FILE* meta = fopen(meta_file, "wb");
  if (meta) {
    int32_t idx = (int32_t)primary_index;
    uint32_t orig_len = (uint32_t)read;
    fwrite(&idx, sizeof(idx), 1, meta);
    fwrite(&orig_len, sizeof(orig_len), 1, meta);
    fclose(meta);
  } else {
    fprintf(stderr, "Warning: can't write metadata file %s\n", meta_file);
  }

  printf("Pipeline complete.\n");
  printf("Input file : %s\n", input_path);
  printf("Input bytes : %zu\n", (size_t)read);
  printf("BWT length  : %zu (primary index %d)\n", bwt_len, primary_index);
  printf("MTF length  : %zu\n", mtf_len);
  printf("RLE length  : %zu\n", rle_len);
  printf("Final Huffman output : %s\n", output_bin);
  printf("Metadata written to %s (primary index + original length)\n",
         meta_file);

  // cleanup
  free(inbuf);
  free(bwt_out);
  free(mtf_out);
  free(rle_out);

  return 0;
}
