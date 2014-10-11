#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <vector>
#include <cstdint>
#include <string>
#include <fstream>

// CLASSE ASTRATTA

class Huffman {

private:

public:
	std::uint32_t _file_length;
	std::vector<uint8_t> _file_vector;
	std::vector<uint8_t> _file_compressed;

	/*
	Funzione che legge il file in input, riempie un vector<uint8_t> con il contenuto del file
	e restituisce il vector<uint8_t> su cui successivamente applicare la compressione
	*/
	void read_file(std::string filename_in);

	/*
	Funzione che prende il risultato della comrpessione da un vector<uint8_t> e lo
	scrive in blocco sul file di output
	*/
	void write_on_file(std::vector<uint8_t>, std::string filename_out);

	// Funzione virtual da implementare nelle classi reali
	virtual void compress(std::string filename) = 0;

	// Funzione virtual da implementare nelle classi reali
	virtual void decompress(std::string in_file, std::string out_file) = 0;
};

#endif /*HUFFMAN_H*/