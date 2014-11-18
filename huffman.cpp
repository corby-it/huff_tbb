#include "huffman.h"
#include <iostream>

using namespace std;

void Huffman::init(std::string filename){
	// setta original filename
	_original_filename = filename;
	// setto output filename
	_output_filename = _original_filename;
	_output_filename.replace(_output_filename.size()-4, 4, ".bcp");
}

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
	_file_length = (uint64_t) file_in.tellg();
	file_in.seekg(0, ios::beg);

	// Lettura one-shot del file
	cout << "Dimensione del file: " << (float)_file_length/1000000000 << endl;

	char* buffer = new char [_file_length];
	file_in.read(buffer, _file_length);
	_file_in.assign(buffer, buffer+_file_length);
	delete[] buffer;
	file_in.close();

}

void Huffman::read_file(ifstream& file_in, uint64_t beg_pos, uint64_t chunk_dim){

	// setta original filename
	//_original_filename = filename;

	// setto output filename
	//_output_filename = _original_filename;
	//_output_filename.replace(_output_filename.size()-4, 4, ".bcp");

	// Apri file di input
	//ifstream file_in(filename, ifstream::in|ifstream::binary);
	//cout << "Apro file in input" << endl;
	// NON salta i whitespaces
	//file_in.unsetf (ifstream::skipws);

	char* buffer = new char [chunk_dim];
	file_in.seekg(beg_pos); // posizione iniziale = inizio del chunk attuale
	file_in.read(buffer, chunk_dim);
	_file_in.assign(buffer, buffer+chunk_dim);
	delete[] buffer;
	//file_in.close();
}

/*
Funzione che prende il risultato della comrpessione da un vector<uint8_t> e lo
scrive in blocco sul file di output
*/
void Huffman::write_on_file (string filename){

	ofstream outf(_output_filename, fstream::out|fstream::binary);
	outf.write(reinterpret_cast<char*>(&_file_out[0]), _file_out.size());

	outf.close();
}

