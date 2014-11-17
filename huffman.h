#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <vector>
#include <cstdint>
#include <string>
#include <sstream>
#include <fstream>

// CLASSE ASTRATTA

#define HUF_MAGIC_NUMBER 0x42435001

class Huffman {

private:

public:
	std::uint64_t _file_length;
	std::vector<uint8_t> _file_in;
	std::vector<uint8_t> _file_out;

	std::string _original_filename;
	std::string _output_filename;

	/*
	Funzione che legge il file in input, riempie un vector<uint8_t> con il contenuto del file
	e restituisce il vector<uint8_t> su cui successivamente applicare la compressione
	*/
	void read_file(std::string filename_in);

	void read_file(std::ifstream& file_in, std::uint64_t beg_pos, std::uint64_t chunk_dim);

	/*
	Funzione che prende il risultato della comrpessione da un vector<uint8_t> e lo
	scrive in blocco sul file di output
	*/
	void write_on_file(std::string filename);

	// Funzione virtual da implementare nelle classi reali
	virtual void compress(std::string filename) = 0;

	// Funzione virtual da implementare nelle classi reali
	// virtual void decompress(std::string in_file, std::string out_file) = 0;
	virtual void decompress(std::string filename) = 0;
};

#endif /*HUFFMAN_H*/