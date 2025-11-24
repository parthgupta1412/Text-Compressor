#include <stdio.h>
#include <stdlib.h>
#include <string.h>


unsigned char* mtf_encode(const unsigned char* input,
                          size_t input_len,
                          size_t* out_len) {
  if (!input || input_len == 0) {
    *out_len = 0;
    return NULL;
  }

  // initialize list 0..255
  unsigned char list[256];
  for (int i = 0; i < 256; ++i)
    list[i] = (unsigned char)i;

  unsigned char* out = malloc(input_len);
  if (!out) {
    *out_len = 0;
    return NULL;
  }

  for (size_t i = 0; i < input_len; ++i) {
    unsigned char symbol = input[i];
    // find index in list
    int pos = 0;
    while (pos < 256 && list[pos] != symbol)
      pos++;
    if (pos == 256)
      pos = 255;  // shouldn't happen

    out[i] = (unsigned char)pos;

    // move-to-front: shift elements right from 0..pos-1, place symbol at 0
    memmove(&list[1], &list[0], pos);
    list[0] = symbol;
  }

  *out_len = input_len;
  return out;
}

unsigned char* mtf_decode(const unsigned char* input,
                          size_t input_len,
                          size_t* output_len) {
  if (!input || input_len == 0) {
    *output_len = 0;
    return NULL;
  }

  unsigned char* out = malloc(input_len);
  if (!out) {
    *output_len = 0;
    return NULL;
  }

  unsigned char list[256];
  for (int i = 0; i < 256; ++i)
    list[i] = (unsigned char)i;

  for (size_t i = 0; i < input_len; ++i) {
    unsigned int pos = input[i];  // position in the list
    if (pos > 255)
      pos = 255;  // defensive
    unsigned char symbol = list[pos];
    out[i] = symbol;

    // move symbol to front
    memmove(&list[1], &list[0], pos);
    list[0] = symbol;
  }

  *output_len = input_len;
  return out;
}