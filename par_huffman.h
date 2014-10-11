#ifndef PAR_HUFFMAN_H
#define PAR_HUFFMAN_H

#include "huffman.h"


class ParHuffman : public Huffman {


public:

	void compress(std::string filename);
	void decompress(std::string in_file, std::string out_file);

private:

};

#endif /*PAR_HUFFMAN_H*/