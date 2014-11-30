#ifndef PAR_HUFFMAN_H
#define PAR_HUFFMAN_H

#include "bitwriter.h"
#include "huffman.h"
#include "tbb/tbb.h"

#include <map>

//! TBBHistoReduce class, used to compute histograms in parallel threads
/*!
  This class is used to compute an histogram over a blocked range using TBB
  parallel_reduce function, in order to run the computation on all the available
  cores in the computer.
*/
struct TBBHistoReduce{
	//! Histogram vector.
    /*! This vector contains the histogram's bin. */
	std::vector<tbb::atomic<std::uint32_t>> _histo; 

	//! Constructor.
    /*!
      An empty constructor, it creates the object and initializes data and the histogram vector
    */
	TBBHistoReduce() {
		tbb::atomic<uint32_t> j;
		j = 0;
		_histo.assign(256,j);
	}

	// non penso serva documentazione per questi costruttori/metodi, visto che sono di servizio
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

//! ParHuffman class, used to compress and decompress using TBB parallel functions
/*!
  This class is used to compress and decompress files using TBB library.
  This class inehrits from the Huffman class the basic data structures and functions and specializes
  the compression and decompression functions.
*/
class ParHuffman : public Huffman {

public:

	//! Create histogram function
    /*!
	  This function computes the histogram over a specific chunk using a TBBHistoReduce object and TBB's parallel reduce
      \param histo The TBBHistoReduce object used to create the histogram.
	  \param chunk_dim The chunk length.
    */
	void create_histo(TBBHistoReduce& histo, std::uint64_t chunk_dim);
	
	//! Create code map function
    /*!
	  This function, given the histogram, assigns the canonical codes to the symbols found in the histogram
      \param tbbhr The histogram object.
	  \return returns a map that contains symbols, canonical codes and codes lengths( <symbol, <code, code_len>>).
    */
	std::map<std::uint8_t, std::pair<std::uint32_t,std::uint32_t>> ParHuffman::create_code_map(TBBHistoReduce& tbbhr);
	
	//! Write header function
    /*!
	  This function writes the header in a compressed file. We used a custom header structured as follows:
		- 4 bytes: a magic number to identify the fomrat: BCP1 (hex: 42 43 50 01) 
		- 4 bytes: length of the original filename (m characters)
		- The m characters (1 byte each) of the original filename
		- 4 bytes: total number of symbols in the header (n symbols)
		- n pairs, each one relative to a symbol:
			-- 1 byte: the symbol itself
			-- 1 byte: the length of its canonical code
      \param codes_map The codes map object.
	  \return Returns the bit writer object used to write the header, it will be used to write all the rest of the file.
    */
	BitWriter write_header(std::map<std::uint8_t, std::pair<std::uint32_t,std::uint32_t>> codes_map);
	
	//! Write compressed chunks function
    /*!
	  This function write a compressed chunk of the original file into the output vector.
	  NOTE: this function does not write anything on the hard drive.
      \param available_ram A uint64_t containing the total amount of available ram.
	  \param macrochunk_dim A uint64_t containing the length of the current file chunk to compress.
	  \param codes_map The codes map computed from the histogram.
	  \param btw A reference to the bit writer object used to write to the output vector.
    */
	void write_chunks_compressed(std::uint64_t available_ram, std::uint64_t macrochunk_dim, std::map<std::uint8_t, std::pair<std::uint32_t,std::uint32_t>> codes_map, BitWriter& btw);

	//! Compress function
    /*!
	  This function compresses the the given file.
	  WARNING: this function does not use file chunking, it operates on the whole file by completely reading it.
	  it may cause memory issues with big files.
      \param filename The current file's name.
    */
	void compress(std::string filename);
	
	//! Chunked compress function
    /*!
	  This function compresses the the given file.
      \param filename The current file's name.
    */
	void compress_chunked(std::string filename);

	//! Decompress function
    /*!
	  This function decompresses the the given file.
	  WARNING: this function does not use file chunking, it operates on the whole file by completely reading it.
	  it may cause memory issues with big files.
      \param filename The current file's name.
    */
	void decompress(std::string filename);
	
	//! Chunked decompress function
    /*!
	  This function decompresses the the given file.
      \param filename The current file's name.
    */
	void decompress_chunked(std::string filename);

private:

};

#endif /*PAR_HUFFMAN_H*/