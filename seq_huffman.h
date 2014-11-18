#ifndef SEQ_HUFFMAN_H
#define SEQ_HUFFMAN_H

#include "huffman.h"


class SeqHuffman : public Huffman {


public:

	void compress(std::string filename);
	void create_histo(std::vector<uint32_t>& histo, std::uint64_t chunk_dim);
	void decompress(std::string filename);

private:

};

#endif /*SEQ_HUFFMAN_H*/