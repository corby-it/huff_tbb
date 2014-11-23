#ifndef SEQ_HUFFMAN_H
#define SEQ_HUFFMAN_H

#include "huffman.h"


class SeqHuffman : public Huffman {


public:

	void compress(std::string filename);
	void create_histo(std::vector<uint32_t>& histo, std::uint64_t chunk_dim);
	std::map<std::uint8_t, std::pair<std::uint32_t,std::uint32_t>> create_code_map(std::vector<uint32_t>& histo);
	BitWriter write_header(std::map<std::uint8_t, std::pair<std::uint32_t,std::uint32_t>> codes_map);
	void write_chunks_compressed(std::uint64_t available_ram, std::uint64_t macrochunk_dim, std::map<std::uint8_t, std::pair<std::uint32_t,std::uint32_t>> codes_map, BitWriter& btw);
	void decompress(std::string filename);
	void decompress_chunked(std::string filename);

private:

};

#endif /*SEQ_HUFFMAN_H*/