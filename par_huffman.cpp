#include <iostream>
#include <fstream>
#include "par_huffman.h"
#include "par_huffman_utils.h"
#include "bitwriter.h"
#include "bitreader.h"
#include "tbb/tbb.h"
#include "tbb/concurrent_vector.h"
#include <string>
#include <utility>

using namespace std;
using namespace tbb;

void ParHuffman::compress_chunked(string filename){
	// Utility
	tick_count tt1, tt2;
	tt1 = tick_count::now();

	// Check for chunking 
	ifstream file_in(filename, ifstream::in|ifstream::binary|fstream::ate);
	// Whitespaces are accepted
	file_in.unsetf (ifstream::skipws);
	// Check file length
	uint64_t file_len = (uint64_t) file_in.tellg();
	uint64_t MAX_LEN = HUF_TEN_MB; 
	uint64_t num_macrochunks = 1;
	if(file_len > MAX_LEN) 
		num_macrochunks = 1 + (file_len-1)/ MAX_LEN;
	uint64_t macrochunk_dim = file_len / num_macrochunks;

	//Initialize parallel object
	init(filename);

	// Global histogram
	TBBHistoReduce tbbhr;

	// For each macrochunk -> read and histo
	tick_count th1, th2;
	th1 = tick_count::now();
	for(uint64_t k=0; k < num_macrochunks; ++k) {
		read_file(file_in, k*macrochunk_dim, macrochunk_dim);
		create_histo(tbbhr, macrochunk_dim);
		cerr << "\rHuffman computation: " << ((100*(k+1))/num_macrochunks) << "%";
	}
	th2 = tick_count::now();
	if(num_macrochunks==1) cerr << "\rHuffman computation: 100%";
	//cerr << endl << "Time for all sub-histograms: " << (th2-th1).seconds() << " sec" << endl;

	// For each exceeding byte -> read and histo
	if(num_macrochunks*macrochunk_dim < file_len){ 
		read_file(file_in, num_macrochunks*macrochunk_dim, file_len-num_macrochunks*macrochunk_dim);
		create_histo(tbbhr, (file_len - num_macrochunks*macrochunk_dim));
	}

	// Create map <symbol, <code, len_code>>
	map<uint8_t, pair<uint32_t,uint32_t>> codes_map = create_code_map(tbbhr);

	// Write file header
	BitWriter btw = write_header(codes_map);

	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	uint64_t available_ram = status.ullAvailPhys;

	ofstream output_file(_output_filename, fstream::out|fstream::binary);
	cerr << endl << "Output filename: " << _output_filename << endl;

	// Write compressed file chunk-by-chunk
	tick_count tw1, tw2;
	tw1 = tick_count::now();
	for(uint64_t k=0; k < num_macrochunks; ++k) {
		read_file(file_in, k*macrochunk_dim, macrochunk_dim);
		write_chunks_compressed(available_ram, macrochunk_dim, codes_map, btw);
		output_file.write(reinterpret_cast<char*>(&_file_out[0]), _file_out.size());
		_file_out.clear();
		cerr << "\rWrite compressed file: " << ((100*(k+1))/num_macrochunks) << "%";
	}
	if(num_macrochunks==1) cerr << "\rWrite compressed file: 100%";
	// Write exceeding byte
	if(num_macrochunks*macrochunk_dim < file_len){ 
		read_file(file_in, num_macrochunks*macrochunk_dim, file_len-num_macrochunks*macrochunk_dim);
		write_chunks_compressed(available_ram, file_len-(num_macrochunks*macrochunk_dim), codes_map, btw);
		output_file.write(reinterpret_cast<char*>(&_file_out[0]), _file_out.size());
		_file_out.clear();
	}
	btw.flush();
	tw2 = tick_count::now();
	//cerr << endl << "Time for all writing (buffer): " << (tw2-tw1).seconds() << " sec" << endl;

	// Write on HDD
	tick_count twhd1, twhd2;
	twhd1 = tick_count::now();
	if(_file_out.size() != 0)
		output_file.write(reinterpret_cast<char*>(&_file_out[0]), _file_out.size());
	output_file.close();
	file_in.close();
	twhd2 = tick_count::now();
	//cerr << "Time for all writing (Hard Disk): " << (twhd2-twhd1).seconds() << " sec" << endl;
	cerr << endl;

	tt2 = tick_count::now();
	cerr <<  "Total time for compression: " << (tt2-tt1).seconds() << " sec" << endl << endl;
}

void ParHuffman::write_chunks_compressed(uint64_t available_ram, uint64_t macrochunk_dim, map<uint8_t, pair<uint32_t,uint32_t>> codes_map, BitWriter& btw){

	uint64_t num_microchunk = 1 + (macrochunk_dim*8-1)/available_ram;
	if(num_microchunk==0)
		num_microchunk=1;
	num_microchunk++; // overprovisioning
	//cerr << "\nNumero di microchunks: " << num_microchunk << endl;
	uint64_t microchunk_dim = macrochunk_dim/num_microchunk; 
	//cerr << "Dimensione di un microchunk: " << microchunk_dim/1000000 << " MB" << endl;

	for (size_t i=0; i < num_microchunk; ++i) {
		vector<pair<uint32_t, uint32_t>> buffer_map(microchunk_dim);
		parallel_for(blocked_range<int>(i*microchunk_dim, microchunk_dim*(i+1),10000), [&](const blocked_range<int>& range) {
			pair<uint32_t,uint32_t> element;
			for( int r=range.begin(); r!=range.end(); ++r ){
				element = codes_map[_file_in[r]];
				buffer_map[r-i*microchunk_dim].first = element.first;
				buffer_map[r-i*microchunk_dim].second = element.second;
			}

		});
		for (size_t j = 0; j < microchunk_dim; j++)
			btw.write(buffer_map[j].first, buffer_map[j].second);
		buffer_map.clear();
	}
	// Legge la parte del file che viene tagliata dall'approssimazione nella divisione in chunks
	pair<uint32_t,uint32_t> element;
	//cerr << "Scrivo un byte avanzato, infatti (num_microchunk*microchunk_dim)=" << num_microchunk*microchunk_dim << " < file_len=" << macrochunk_dim << endl;
	for (size_t i=num_microchunk*microchunk_dim; i < macrochunk_dim; i++){
		element = codes_map[_file_in[i]];
		btw.write(element.first, element.second);
	}
}

BitWriter ParHuffman::write_header(map<uint8_t, pair<uint32_t,uint32_t>> codes_map){

	// utili per ottimizzazione
	tick_count t0, t1;
	t0 = tick_count::now();
	// crea il file di output
	BitWriter btw(_file_out);

	/* scrivo le lunghezze dei codici all'inizio del file secondo il formato (migliorabile penso...):
	* - un magic number di 4 byte per contraddistinguere il formato: BCP1 (in hex: 42 43 50 01) 
	* - 4 byte per la lunghezza del nome del file originale (m)
	* - seguono gli m caratteri (1 byte l'uno) del nome del file
	* - 4 byte dicono quanti simboli ci sono (n)
	* - seguono gli n simboli:
	*		-- 1 byte per il simbolo, 1 byte per la lunghezza
	*/
	//scrivo il magic number
	btw.write(HUF_MAGIC_NUMBER, 32);
	// scrivo la dimensione del nome del file originale
	btw.write((uint32_t)_original_filename.size(), 32);
	// Scrivo il nome del file originale, per recuperarlo in decompressione
	for(size_t i=0; i<_original_filename.size(); ++i)
		btw.write(_original_filename[i], 8);

	// scrivo il numero di simboli
	btw.write((uint32_t)codes_map.size(), 32);

	// creo un'altra struttura ordinata per scrivere i simboli in ordine, dal più corto al più lungo
	// la depthmap contiene le coppie <lunghezza, simbolo>
	DepthMap depthmap;
	for(uint32_t i=0; i<256; ++i){
		if(codes_map.find(i) != codes_map.end()){
			DepthMapElement tmp;
			tmp.first = codes_map[i].second;
			tmp.second = i;
			depthmap.push_back(tmp);
		}
	}
	sort(depthmap.begin(), depthmap.end(), par_depth_compare);

	// scrivo PRIMA IL SIMBOLO POI LA LUNGHEZZA
	for(size_t i=0; i<depthmap.size(); ++i){
		btw.write(depthmap[i].second, 8); // simbolo
		btw.write(depthmap[i].first, 8);  // lunghezza
	}
	return btw;
}

void ParHuffman::create_histo(TBBHistoReduce& tbbhr, uint64_t chunk_dim){
	// Creazione dell'istogramma in parallelo con parallel_reduce
	parallel_reduce(blocked_range<uint8_t*>(_file_in.data(),_file_in.data()+chunk_dim), tbbhr);
}

map<uint8_t, pair<uint32_t,uint32_t>> ParHuffman::create_code_map(TBBHistoReduce& tbbhr){
	// utili per ottimizzazione
	tick_count t0, t1;

	// creo un vettore che conterrà le foglie dell'albero di huffman, ciascuna con simbolo e occorrenze
	t0 = tick_count::now();
	TBBLeavesVector leaves_vect;
	par_create_huffman_tree(tbbhr._histo, leaves_vect);
	//par_create_huffman_tree(histo, leaves_vect);
	t1 = tick_count::now();
	//cerr << endl << "[PAR] La creazione dell'albero ha impiegato " << (t1 - t0).seconds() << " sec" << endl;

	leaves_vect[0]->setRoot(true);

	// creo una depthmap, esplorando tutto l'albero, per sapere a che profondità si trovano i simboli
	// la depthmap contiene le coppie <lunghezza_simbolo, simbolo>
	DepthMap depthmap;
	par_depth_assign(leaves_vect[0], depthmap);

	// ordino la depthmap per profondità 
	sort(depthmap.begin(), depthmap.end(), par_depth_compare);

	// creo i codici canonici usando la depthmap e li scrivo in codes
	vector<ParTriplet> codes;
	par_canonical_codes(depthmap, codes);

	// crea una mappa <simbolo, <codice, lunghezza_codice>> per comodità
	map<uint8_t, pair<uint32_t,uint32_t>> codes_map;
	for(unsigned i=0; i<codes.size(); ++i){
		codes_map.insert(pair<uint8_t,pair<uint32_t,uint32_t>>(codes[i].symbol, pair<uint32_t,uint32_t>(codes[i].code,codes[i].code_len)));
	}
	return codes_map;

}

void ParHuffman::decompress_chunked (string filename) {

}

void ParHuffman::decompress (string filename){

	BitReader btr(_file_in);
	BitWriter btw(_file_out);

	// leggo il magic number
	uint32_t magic_number = btr.read(32);
	if(magic_number != HUF_MAGIC_NUMBER){
		cerr << "Error: unknown format, wrong magic number..." << endl;
		exit(1);
	}

	// leggo la lunghezza del nome del file
	uint32_t fname_length = btr.read(32);
	// leggo il nome del file originale e setto output filename
	vector<uint8_t> fname = btr.read_n_bytes(fname_length);
	_output_filename.assign(fname.begin(), fname.end());

	// leggo il numero di simboli totali
	uint32_t tot_symbols = btr.read(32);

	// creo un vettore di coppie <lunghezza_simbolo, simbolo>
	DepthMap depthmap;
	for(unsigned i=0; i<tot_symbols; ++i){
		uint8_t code_len = btr.read(8);
		uint8_t symb = btr.read(8);
		depthmap.push_back(DepthMapElement(code_len, symb));
	}


	// debug -- stampo depthmap
	cerr << endl;
	for(int i=0; i<depthmap.size(); ++i)
		cerr << depthmap[i].first << "\t" << depthmap[i].second << endl;

	// creo i codici canonici usando la depthmap e li scrivo in codes
	vector<ParTriplet> codes;
	par_canonical_codes(depthmap, codes);

	// crea una mappa <codice, <simbolo, lunghezza_codice>>
	map<uint32_t, pair<uint8_t,uint32_t>> codes_map; 
	for(unsigned i=0; i<codes.size(); ++i)
		codes_map.insert(pair<uint32_t, pair<uint8_t,uint32_t>>(codes[i].code,pair<uint8_t,uint32_t>(codes[i].symbol, codes[i].code_len)));

	// leggo il file compresso e scrivo l'output
	while(btr.good()){
		uint32_t tmp_code_len = depthmap[0].first;
		uint32_t tmp_code = btr.read(depthmap[0].first);

		// cerco il codice nella mappa, se non c'è un codice come quello che ho appena letto
		// o se c'è ma ha lunghezza diversa da quella corrente, leggo un altro bit e lo
		// aggiungo al codice per cercare di nuovo.
		// questo modo mi sa che è super inefficiente... sicuramente si può fare di meglio
		while(codes_map.find(tmp_code) == codes_map.end() ||
			(codes_map.find(tmp_code) != codes_map.end() && tmp_code_len != codes_map[tmp_code].second )){

				tmp_code = (tmp_code << 1) | (1 & btr.read_bit());
				tmp_code_len++;
		}

		// se esco dal while ho trovato un codice, lo leggo, e lo scrivo sul file di output
		pair<uint32_t,uint32_t> tmp_sym_pair = codes_map[tmp_code];
		btw.write(tmp_sym_pair.first, 8);
	}
	btw.flush();

}