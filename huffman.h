#ifndef HUFFMAN_H
#define HUFFMAN_H

#include "huffman_utils.h"
#include <cstdint>
#include <algorithm>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include "bitwriter.h"

//!  CodeVector is a struct used to store information about huffman coding.
/*!
CodeVector is a struct that is used to keep in a unique place all informations
about number of symbols present in the input file.
*/
struct CodeVector{
	//! Total number of symbols found in the file.
	std::uint32_t num_symbols;
	//! This vector contains information about which symbol is present in the codes vector
	std::vector<bool> presence_vector;
	//! This vector contains pairs <length, code>
	std::vector<std::pair<std::uint32_t,std::uint32_t>> codes_vector;

	//! Constructor
	/*!
	An empty constructor, it initializes the inner variables.
	*/
	CodeVector(){
		num_symbols=0;
		/*for(int i = 0; i<256; ++i){
			codes_vector.push_back(std::pair<uint32_t,uint32_t>(NULL,NULL));
			presence_vector.push_back(false);
		}*/
		fill(codes_vector.begin(), codes_vector.end(), std::pair<uint32_t,uint32_t>(NULL,NULL));
		fill(presence_vector.begin(), presence_vector.end(), false);
	}
};


//!  Huffman is the main Huffman compression/decompression class.
/*!
Huffman defines some common functions to all the subclasses, for operations like
reading and writing files. Some of them have the same implementation, some others
are virtual and the implementation is specific to the subclass.
*/
class Huffman {

private:

public:
	//! Input file length
	std::uint64_t _file_length;
	//! A vector that represents the input file content
	std::vector<uint8_t> _file_in;
	//! A vector that represents the output file content
	std::vector<uint8_t> _file_out;
	//! the name the file had before the operation (compression or decompression)
	std::string _original_filename;
	//! the name the file will have after the operation (compression or decompression)
	std::string _output_filename;

	//! Initialization
	/*!
	Used to set the original and the output filenames.
	\param filename The input filename.
	*/
	void init(std::string filename);

	//! Read from file
	/*!
	This function reads from the given file and write the whole file content into the _file_in vector.
	\param filename_in The input filename.
	*/
	void read_file(std::string filename_in);

	//! Read from file
	/*!
	This function is used to read only a single chunk of the input file given the chunk's length and the
	beginning position. the content is still used to fill the _file_in vector.
	\param file_in The input file represented as an ifstream.
	\param beg_pos The stream's position from which the function will start to read.
	\param chunk_dim The desired chunk's length, how many bytes the function will read.
	*/
	void read_file(std::ifstream& file_in, std::uint64_t beg_pos, std::uint64_t chunk_dim);


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
	BitWriter write_header(CodeVector& codes_map);



	//! Write to file
	/*!
	This function transfer the whole content of the _file_out vector to a file on the hard drive.
	\param filename The output filename.
	*/
	void write_on_file(std::string filename);


	// Virtual functions

	//! Chunked decompression
	/*!
	This function fill the _file_out vector with the decompressed version of the _file_in vector.
	This function also is responsible for splitting the file into chunks and operating on only
	one chunk at a time, in order to prevent memory issues during the operations.
	This function is implemented in different ways in the subclasses (parallel or sequential).
	\param filename The input filename.
	*/
	virtual void decompress_chunked(std::string filename) = 0;

	//! Chunked compression
	/*!
	This function fill the _file_out vector with the compressed version of the _file_in vector.
	This function also is responsible for splitting the file into chunks and operating on only
	one chunk at a time, in order to prevent memory issues during the operations.
	This function is implemented in different ways in the subclasses (parallel or sequential).
	\param filename The input filename.
	*/
	virtual void compress_chunked(std::string filename) = 0;
};

#endif /*HUFFMAN_H*/