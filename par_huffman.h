#ifndef PAR_HUFFMAN_H
#define PAR_HUFFMAN_H

#include "bitwriter.h"
#include "huffman.h"
#include "tbb/tbb.h"

#include <map>

struct TBBHistoReduce{
	std::vector<tbb::atomic<std::uint32_t>> _histo; 

	TBBHistoReduce() {
		tbb::atomic<uint32_t> j;
		j = 0;
		_histo.assign(256,j);
	}

	TBBHistoReduce(TBBHistoReduce& tbbhr, tbb::split) {
		tbb::atomic<uint32_t> j;
		j = 0;
		_histo.assign(256,j);

	}

	void operator()(const tbb::blocked_range<std::uint8_t*>& r){
		for(std::uint8_t* u=r.begin(); u!=r.end(); ++u)
			_histo[*u]++;
		
	}

	void join(TBBHistoReduce& tbbhr){
		for(std::size_t i=0; i<256; ++i)
			_histo[i] += tbbhr._histo[i];
	}
};

class ParHuffman : public Huffman {


public:

	void compress(std::string filename);
	void create_histo(TBBHistoReduce& histo, std::uint64_t chunk_dim);
	std::map<std::uint8_t, std::pair<std::uint32_t,std::uint32_t>> ParHuffman::create_code_map(TBBHistoReduce& tbbhr);
	BitWriter ParHuffman::write_header(std::map<std::uint8_t, std::pair<std::uint32_t,std::uint32_t>> codes_map);
	void ParHuffman::write_chunks_compressed(std::uint64_t available_ram, std::uint64_t macrochunk_dim, std::map<std::uint8_t, std::pair<std::uint32_t,std::uint32_t>> codes_map, BitWriter& btw);

	void decompress(std::string filename);

private:

};

#endif /*PAR_HUFFMAN_H*/