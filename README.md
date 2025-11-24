For compressing-
"gcc -O2 -std=c11 main.c main_huffman.c main_rle.c main_bwt.c main_mtf.c -o compressor"
compressor.exe

For decompressing-
"gcc -O2 -std=c11 decompress.c main_huffman.c main_rle.c main_bwt.c main_mtf.c -o decompressor"
decompressor.exe
