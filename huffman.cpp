#include "huffman.h"
#include <algorithm>
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



BitWriter Huffman::write_header(CodeVector& codes_map){

	// crea il file di output
	BitWriter btw(_file_out);

	//scrivo il magic number
	btw.write(HUF_MAGIC_NUMBER, 32);
	// scrivo la dimensione del nome del file originale
	btw.write((uint32_t)_original_filename.size(), 32);
	// Scrivo il nome del file originale, per recuperarlo in decompressione
	for(size_t i=0; i<_original_filename.size(); ++i)
		btw.write(_original_filename[i], 8);

	// scrivo il numero di simboli
	btw.write((uint32_t)codes_map.num_symbols, 32);

	// creo un'altra struttura ordinata per scrivere i simboli in ordine, dal più corto al più lungo
	// la depthmap contiene le coppie <lunghezza, simbolo>
	DepthMap depthmap;
	for(uint32_t i=0; i<256; ++i){
		if(codes_map.presence_vector[i]==true){
			DepthMapElement tmp;
			tmp.first = codes_map.codes_vector[i].second;
			tmp.second = i;
			depthmap.push_back(tmp);
		}
	}
	sort(depthmap.begin(), depthmap.end(), depth_compare);

	// scrivo PRIMA IL SIMBOLO POI LA LUNGHEZZA
	for(size_t i=0; i<depthmap.size(); ++i){
		btw.write(depthmap[i].second, 8); // simbolo
		btw.write(depthmap[i].first, 8);  // lunghezza
	}
	return btw;
}
