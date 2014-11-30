#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <vector>
#include <cstdint>
#include <string>
#include <sstream>
#include <fstream>
#include <map>

// Constants
#define HUF_MAGIC_NUMBER	0x42435001

#define HUF_ONE_GB			1000000000
#define HUF_ONE_HUNDRED_MB	100000000
#define HUF_TEN_MB			10000000
#define HUF_ONE_MB			1000000
// per leggere l'header stimo che sia lungo al massimo 1 KB
// 4B per il magic number
// 4B per la lunghezza del nome del file originale
// da 0B a 500B per il nome del file vero e proprio (un po' esagerato ma fa lo stesso)
// 4B per il numero di simboli
// 512B per il massimo numero possibile di coppie <lunghezza_codice, simbolo>
#define HUF_HEADER_DIM			1024

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

	//! Write to file
    /*!
	  This function transfer the whole content of the _file_out vector to a file on the hard drive.
      \param filename The output filename.
    */
	void write_on_file(std::string filename);

	// Virtual functions

	//! Decompression
    /*!
	  This function fill the _file_out vector with the decompressed version of the _file_in vector
	  without taking into account memory issues.
	  This function is implemented in different ways in the subclasses (parallel or sequential).
      \param filename The input filename.
    */
	virtual void decompress(std::string filename) = 0;

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