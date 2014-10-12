#ifndef PAR_HUFFMAN_H
#define PAR_HUFFMAN_H

#include "huffman.h"


class ParHuffman : public Huffman {


public:

	void compress(std::string filename);
	void decompress(std::string filename);

private:

};

#endif /*PAR_HUFFMAN_H*/