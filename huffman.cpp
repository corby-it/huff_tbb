
//#include <fstream>

#include "huffman.h"

using namespace std;

/*
Funzione che legge il file in input, riempie un vector<uint8_t> con il contenuto del file
e restituisce il vector<uint8_t> su cui successivamente applicare la compressione
*/
void Huffman::read_file(string filename){

	// Apri file di input
	ifstream file_in(filename, ifstream::in|ifstream::binary|fstream::ate);
	// NON salta i whitespaces
	file_in.unsetf (ifstream::skipws);
	
	// Salva la dimensione del file e torna all'inizio
	_file_length = (uint32_t) file_in.tellg();
	file_in.seekg(0, ios::beg);

	// Lettura one-shot del file
	char* buffer = new char [_file_length];
	file_in.read(buffer, _file_length);
	_file_vector.assign(buffer, buffer+_file_length);

}

/*
Funzione che prende il risultato della comrpessione da un vector<uint8_t> e lo
scrive in blocco sul file di output
*/
void Huffman::write_on_file(vector<uint8_t>, string filename_out){

}
