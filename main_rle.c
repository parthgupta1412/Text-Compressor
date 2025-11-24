#include <stdio.h>
#include <stdlib.h>

void compress_rle(const char* input, const char* output) {
  FILE* in = fopen(input, "rb");
  FILE* out = fopen(output, "wb");

  if (in == NULL) {
    fprintf(stderr, "Error opening input file '%s'\n", input);
    if (out)
      fclose(out);
    return;
  }
  if (out == NULL) {
    fprintf(stderr, "Error opening output file '%s'\n", output);
    fclose(in);
    return;
  }

  unsigned char current, previous;
  unsigned char count = 1;
  long originalsize = 0, compressedsize = 0;

  /* Read first byte to initialize 'previous' */
  if (fread(&previous, 1, 1, in) != 1) {
    /* empty file: nothing to do */
    fclose(in);
    fclose(out);
    printf("File is empty — nothing to compress.\n");
    return;
  }
  originalsize = 1;

  while (fread(&current, 1, 1, in) == 1) {
    originalsize++;
    if (current == previous && count < 255) {
      count++;
    } else {
      /* write run (count, previous) */
      fwrite(&count, 1, 1, out);
      fwrite(&previous, 1, 1, out);
      compressedsize += 2;

      /* start new run */
      previous = current;
      count = 1;
    }
  }

  /* write the final run */
  fwrite(&count, 1, 1, out);
  fwrite(&previous, 1, 1, out);
  compressedsize += 2;

  fclose(in);
  fclose(out);

  printf("\nFile compressed successfully.\n");
  printf("\n--- Compression Statistics ---\n");
  printf("Original Size     : %ld bytes\n", originalsize);
  printf("Compressed Size   : %ld bytes\n", compressedsize);

  if (originalsize > 0) {
    float ratio = (float)compressedsize / (float)originalsize;
    printf("Compression Ratio : %.2f\n", ratio);
  }
}

void decompress_rle(const char* input, const char* output) {
  FILE* in = fopen(input, "rb");
  FILE* out = fopen(output, "wb");

  if (in == NULL || out == NULL) {
    printf("Error opening file!\n");
    if (in)
      fclose(in);
    if (out)
      fclose(out);
    return;
  }

  unsigned char count, value;

  while (fread(&count, 1, 1, in) == 1 && fread(&value, 1, 1, in) == 1) {
    for (int i = 0; i < count; i++) {
      fwrite(&value, 1, 1, out);
    }
  }

  fclose(in);
  fclose(out);

  printf("File decompressed successfully!\n");
}

size_t compress_rle_buffer(const unsigned char* input,
                           size_t input_len,
                           unsigned char* output,
                           size_t output_capacity) {
  if (!input || !output)
    return 0;
  if (input_len == 0)
    return 0;

  size_t in_pos = 0, out_pos = 0;
  unsigned char current = input[0];
  unsigned int count = 1;
  in_pos = 1;

  while (in_pos < input_len) {
    unsigned char c = input[in_pos++];
    if (c == current && count < 255) {
      count++;
    } else {
      if (out_pos + 2 > output_capacity)
        return 0;
      output[out_pos++] = (unsigned char)count;
      output[out_pos++] = current;
      current = c;
      count = 1;
    }
  }
  if (out_pos + 2 > output_capacity)
    return 0;
  output[out_pos++] = (unsigned char)count;
  output[out_pos++] = current;
  return out_pos;
}

// int main() {
//   int choice;
//   char input[260], output[260];

//   printf("1. Compress a file\n");
//   printf("2. Decompress a file\n");
//   printf("Enter your choice: ");
//   if (scanf("%d", &choice) != 1)
//     return 1;

//   printf("Enter input file name: ");
//   scanf("%s", input);
//   printf("Enter output file name: ");
//   scanf("%s", output);

//   if (choice == 1)
//     compress_rle(input, output);
//   else if (choice == 2)
//     decompress_rle(input, output);
//   else
//     printf("Invalid choice!\n");

//   return 0;
// }
