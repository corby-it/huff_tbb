#ifndef SEQ_HUFFMAN_H
#define SEQ_HUFFMAN_H

#include "huffman.h"


class SeqHuffman : public Huffman {


public:

	void compress(std::string filename);
	void decompress(std::string in_file, std::string out_file);

private:

};

#endif /*SEQ_HUFFMAN_H*/