#ifndef SEQ_HUFFMAN_H
#define SEQ_HUFFMAN_H

#include "huffman.h"


//! SeqHuffman class, used to compress and decompress only in a sequential way.
/*!
  This class is used to compress and decompress files without using TBB library, using only a sequential process
  This class inehrits from the Huffman class the basic data structures and functions and specializes
  the compression and decompression functions.
*/
class SeqHuffman : public Huffman {

public:

	//! Create histogram function
    /*!
	  This function computes the histogram over a specific chunk of the input file.
      \param histo A uint32_t vector object used to create the histogram.
	  \param chunk_dim The chunk length.
    */
	void create_histo(std::vector<uint32_t>& histo, std::uint64_t chunk_dim);

	//! Create code map function
    /*!
	  This function, given the histogram, assigns the canonical codes to the symbols found in the histogram
      \param tbbhr The histogram object.
	  \return returns a map that contains symbols, canonical codes and codes lengths( <symbol, <code, code_len>>).
    */
	CodeVector create_code_map(std::vector<uint32_t>& histo);

	//! Write compressed chunks function
    /*!
	  This function write a compressed chunk of the original file into the output vector.
	  NOTE: this function does not write anything on the hard drive.
      \param available_ram A uint64_t containing the total amount of available ram.
	  \param macrochunk_dim A uint64_t containing the length of the current file chunk to compress.
	  \param codes_map The codes map computed from the histogram.
	  \param btw A reference to the bit writer object used to write to the output vector.
    */
	void write_chunks_compressed(std::uint64_t available_ram, std::uint64_t macrochunk_dim, CodeVector codes_map, BitWriter& btw);
	
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

	//! Chunked decompress function
    /*!
	  This function decompresses the the given file.
      \param filename The current file's name.
    */
	void decompress_chunked(std::string filename);

private:

};

#endif /*SEQ_HUFFMAN_H*/