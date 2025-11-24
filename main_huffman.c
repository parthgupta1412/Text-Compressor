#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TREE_HT 256

// Huffman tree node
struct MinHeapNode {
  unsigned char data;
  unsigned freq;
  struct MinHeapNode *left, *right;
};

// MinHeap structure
struct MinHeap {
  unsigned size;
  unsigned capacity;
  struct MinHeapNode** array;
};

// Create new node
struct MinHeapNode* newNode(unsigned char data, unsigned freq) {
  struct MinHeapNode* temp =
      (struct MinHeapNode*)malloc(sizeof(struct MinHeapNode));
  temp->left = temp->right = NULL;
  temp->data = data;
  temp->freq = freq;
  return temp;
}

// Create a min heap
struct MinHeap* createMinHeap(unsigned capacity) {
  struct MinHeap* minHeap = (struct MinHeap*)malloc(sizeof(struct MinHeap));
  minHeap->size = 0;
  minHeap->capacity = capacity;
  minHeap->array = (struct MinHeapNode**)malloc(minHeap->capacity *
                                                sizeof(struct MinHeapNode*));
  return minHeap;
}

// Swap
void swapMinHeapNode(struct MinHeapNode** a, struct MinHeapNode** b) {
  struct MinHeapNode* t = *a;
  *a = *b;
  *b = t;
}

// Heapify
void minHeapify(struct MinHeap* minHeap, int idx) {
  int smallest = idx;
  int left = 2 * idx + 1;
  int right = 2 * idx + 2;

  if (left < (int)minHeap->size &&
      minHeap->array[left]->freq < minHeap->array[smallest]->freq)
    smallest = left;

  if (right < (int)minHeap->size &&
      minHeap->array[right]->freq < minHeap->array[smallest]->freq)
    smallest = right;

  if (smallest != idx) {
    swapMinHeapNode(&minHeap->array[smallest], &minHeap->array[idx]);
    minHeapify(minHeap, smallest);
  }
}

// Extract minimum node
struct MinHeapNode* extractMin(struct MinHeap* minHeap) {
  struct MinHeapNode* temp = minHeap->array[0];
  minHeap->array[0] = minHeap->array[minHeap->size - 1];
  minHeap->size--;
  minHeapify(minHeap, 0);
  return temp;
}

// Insert node into heap
void insertMinHeap(struct MinHeap* minHeap, struct MinHeapNode* minHeapNode) {
  minHeap->size++;
  int i = minHeap->size - 1;
  while (i && minHeapNode->freq < minHeap->array[(i - 1) / 2]->freq) {
    minHeap->array[i] = minHeap->array[(i - 1) / 2];
    i = (i - 1) / 2;
  }
  minHeap->array[i] = minHeapNode;
}

void buildMinHeap(struct MinHeap* minHeap) {
  int n = minHeap->size - 1;
  for (int i = (n - 1) / 2; i >= 0; i--)
    minHeapify(minHeap, i);
}

int isLeaf(struct MinHeapNode* root) {
  return !(root->left) && !(root->right);
}

struct MinHeap* createAndBuildMinHeap(unsigned char data[],
                                      unsigned freq[],
                                      int size) {
  struct MinHeap* minHeap = createMinHeap(size);
  for (int i = 0; i < size; ++i)
    minHeap->array[i] = newNode(data[i], freq[i]);
  minHeap->size = size;
  buildMinHeap(minHeap);
  return minHeap;
}

struct MinHeapNode* buildHuffmanTree(unsigned char data[],
                                     unsigned freq[],
                                     int size) {
  struct MinHeapNode *left, *right, *top;
  struct MinHeap* minHeap = createAndBuildMinHeap(data, freq, size);

  while (minHeap->size != 1) {
    left = extractMin(minHeap);
    right = extractMin(minHeap);
    top = newNode('$', left->freq + right->freq);
    top->left = left;
    top->right = right;
    insertMinHeap(minHeap, top);
  }

  return extractMin(minHeap);
}

void storeCodes(struct MinHeapNode* root,
                int arr[],
                int top,
                char* codes[256]) {
  if (root->left) {
    arr[top] = 0;
    storeCodes(root->left, arr, top + 1, codes);
  }
  if (root->right) {
    arr[top] = 1;
    storeCodes(root->right, arr, top + 1, codes);
  }

  if (isLeaf(root)) {
    codes[root->data] = malloc(top + 1);
    for (int i = 0; i < top; i++)
      codes[root->data][i] = arr[i] + '0';
    codes[root->data][top] = '\0';
  }
}

void buildHuffmanCodes(unsigned char data[],
                       unsigned freq[],
                       int size,
                       char* codes[256]) {
  struct MinHeapNode* root = buildHuffmanTree(data, freq, size);
  int arr[MAX_TREE_HT], top = 0;
  storeCodes(root, arr, top, codes);
}

// Compress
void compress_huffman_s(const char* input_file, const char* output_file) {
  FILE* in = fopen(input_file, "rb");
  if (!in) {
    printf("Error opening input file\n");
    return;
  }

  unsigned freq[256] = {0};
  unsigned char ch;
  while (fread(&ch, 1, 1, in) == 1)
    freq[ch]++;
  fseek(in, 0, SEEK_SET);

  unsigned char data[256];
  int size = 0;
  for (int i = 0; i < 256; i++)
    if (freq[i] > 0)
      data[size++] = (unsigned char)i;

  char* codes[256] = {0};
  buildHuffmanCodes(data, freq, size, codes);

  FILE* out = fopen(output_file, "wb");
  if (!out) {
    printf("Error opening output file\n");
    return;
  }

  // FREQ TABLE
  fwrite(freq, sizeof(freq), 1, out);

  unsigned char buffer = 0;
  int bitcount = 0;

  while (fread(&ch, 1, 1, in) == 1) {
    char* code = codes[ch];
    for (int i = 0; code[i] != '\0'; i++) {
      buffer <<= 1;
      if (code[i] == '1')
        buffer |= 1;
      bitcount++;
      if (bitcount == 8) {
        fwrite(&buffer, 1, 1, out);
        bitcount = 0;
        buffer = 0;
      }
    }
  }

  if (bitcount > 0) {
    buffer <<= (8 - bitcount);
    fwrite(&buffer, 1, 1, out);
  }

  fclose(in);
  fclose(out);
  printf("File compressed using Huffman successfully!\n");
}

void compress_huffman(const char* input_str, const char* output_file) {
  if (!input_str) {
    printf("Null input string\n");
    return;
  }

  size_t input_len = strlen(input_str);
  unsigned freq[256] = {0};

  for (size_t i = 0; i < input_len; i++)
    freq[(unsigned char)input_str[i]]++;

  unsigned char data[256];
  int size = 0;
  for (int i = 0; i < 256; i++)
    if (freq[i] > 0)
      data[size++] = (unsigned char)i;

  char* codes[256] = {0};
  buildHuffmanCodes(data, freq, size, codes);

  FILE* out = fopen(output_file, "wb");
  if (!out) {
    printf("Error opening output file\n");
    return;
  }

  fwrite(freq, sizeof(freq), 1, out);

  unsigned char buffer = 0;
  int bitcount = 0;

  for (size_t i = 0; i < input_len; i++) {
    unsigned char ch = input_str[i];
    char* code = codes[ch];

    for (int j = 0; code[j] != '\0'; j++) {
      buffer <<= 1;
      if (code[j] == '1')
        buffer |= 1;
      bitcount++;

      if (bitcount == 8) {
        fwrite(&buffer, 1, 1, out);
        buffer = 0;
        bitcount = 0;
      }
    }
  }

  if (bitcount > 0) {
    buffer <<= (8 - bitcount);
    fwrite(&buffer, 1, 1, out);
  }

  // Free codes
  for (int i = 0; i < 256; i++)
    if (codes[i])
      free(codes[i]);

  fclose(out);
  printf("String compressed to file successfully!\n");
}

void decompress_huffman(const char* input_file, const char* output_file) {
  FILE* in = fopen(input_file, "rb");
  if (!in) {
    printf("Error opening input file\n");
    return;
  }

  unsigned freq[256];
  fread(freq, sizeof(freq), 1, in);

  unsigned char data[256];
  int size = 0;
  for (int i = 0; i < 256; i++)
    if (freq[i] > 0)
      data[size++] = (unsigned char)i;

  struct MinHeapNode* root = buildHuffmanTree(data, freq, size);

  FILE* out = fopen(output_file, "wb");
  if (!out) {
    printf("Error opening output file\n");
    return;
  }

  struct MinHeapNode* current = root;
  unsigned char byte;
  while (fread(&byte, 1, 1, in) == 1) {
    for (int i = 7; i >= 0; i--) {
      int bit = (byte >> i) & 1;
      current = bit ? current->right : current->left;

      if (isLeaf(current)) {
        fwrite(&current->data, 1, 1, out);
        current = root;
      }
    }
  }

  fclose(in);
  fclose(out);
  printf("File decompressed successfully (Huffman)!\n");
}

// int main() {
//     int choice;
//     char input[100], output[100];

//     printf("1. Huffman Compress\n2. Huffman Decompress\nEnter choice: ");
//     scanf("%d", &choice);

//     printf("Input file: ");
//     scanf("%s", input);
//     printf("Output file: ");
//     scanf("%s", output);

//     if (choice == 1)
//         compress_huffman(input, output);
//     else if (choice == 2)
//         decompress_huffman(input, output);
//     else
//         printf("Invalid choice.\n");

//     return 0;
// }
