#include <iostream>
#include <fstream>

#include "par_huffman.h"
#include "par_huffman_utils.h"

#include "bitwriter.h"
#include "bitreader.h"

#include "tbb/tbb.h"
#include "tbb/concurrent_vector.h"

#include <string>
#include <utility> // pair

using namespace std;
using namespace tbb;

void ParHuffman::write_chunks_compressed(uint64_t available_ram, uint64_t macrochunk_dim, map<uint8_t, pair<uint32_t,uint32_t>> codes_map, BitWriter& btw){
// ----- RIPARTIZIONE DINAMICA
	uint64_t num_microchunk = 1 + (macrochunk_dim*8-1)/available_ram;
	if(num_microchunk==0)
		num_microchunk=1;
	num_microchunk++; // overprovisioning
	cerr << "\nNumero di microchunks: " << num_microchunk << endl;
	uint64_t microchunk_dim = macrochunk_dim/num_microchunk; 
	cerr << "Dimensione di un microchunk: " << microchunk_dim/1000000 << " MB" << endl;
	// ------ FINE RIPARTIZIONE DINAMICA

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
	cerr << "Scrivo un byte avanzato, infatti (num_microchunk*microchunk_dim)=" << num_microchunk*microchunk_dim << " < file_len=" << macrochunk_dim << endl;
	for (size_t i=num_microchunk*microchunk_dim; i < macrochunk_dim; i++){
		element = codes_map[_file_in[i]];
		btw.write(element.first, element.second);
	}
	/*btw.flush();*/

}

BitWriter ParHuffman::write_header(map<uint8_t, pair<uint32_t,uint32_t>> codes_map){

	// utili per ottimizzazione
	tick_count t0, t1;
	cerr << "[PAR] Scrittura su file" << endl;
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

	// creo un'altra struttura ordinata per scrivere i simboli in ordine, dal pi� corto al pi� lungo
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
	//// scrivo PRIMA LA LUNGHEZZA POI IL SIMBOLO
	//for(size_t i=0; i<depthmap.size(); ++i){
	//	btw.write(depthmap[i].first, 8); // lunghezza
	//	btw.write(depthmap[i].second, 8);  // simbolo
	//}

	//for(uint32_t i=0; i<256; ++i){
	//	if(codes_map.find(i) != codes_map.end()){
	//		btw.write(i, 8);
	//		btw.write(codes_map[i].second, 8);
	//	}
	//}

	return btw;
}

void ParHuffman::create_histo(TBBHistoReduce& tbbhr, uint64_t chunk_dim){
	// utili per ottimizzazione
	tick_count t0, t1;
	// Creazione dell'istogramma in parallelo con parallel_reduce
	t0 = tick_count::now();
	parallel_reduce(blocked_range<uint8_t*>(_file_in.data(),_file_in.data()+chunk_dim), tbbhr);
	t1 = tick_count::now();
	cerr << "[PAR] La creazione dell'istogramma ha impiegato " << (t1 - t0).seconds() << " sec" << endl;
}

map<uint8_t, pair<uint32_t,uint32_t>> ParHuffman::create_code_map(TBBHistoReduce& tbbhr){

	// utili per ottimizzazione
	tick_count t0, t1;

	// creo un vettore che conterr� le foglie dell'albero di huffman, ciascuna con simbolo e occorrenze
	t0 = tick_count::now();
	TBBLeavesVector leaves_vect;
	par_create_huffman_tree(tbbhr._histo, leaves_vect);
	//par_create_huffman_tree(histo, leaves_vect);
	t1 = tick_count::now();
	cerr << "[PAR] La creazione dell'albero ha impiegato " << (t1 - t0).seconds() << " sec" << endl;

	leaves_vect[0]->setRoot(true);

	// creo una depthmap, esplorando tutto l'albero, per sapere a che profondit� si trovano i simboli
	// la depthmap contiene le coppie <lunghezza_simbolo, simbolo>
	DepthMap depthmap;
	par_depth_assign(leaves_vect[0], depthmap);

	cerr << "[PAR] Profondita' assegnate" << endl;

	// ordino la depthmap per profondit� 
	sort(depthmap.begin(), depthmap.end(), par_depth_compare);

	cerr << "[PAR] Profondita' ordinate" << endl;

	// creo i codici canonici usando la depthmap e li scrivo in codes
	vector<ParTriplet> codes;
	par_canonical_codes(depthmap, codes);

	cerr << "[PAR] Ci sono " << codes.size() << " simboli" << endl;

	// crea una mappa <simbolo, <codice, lunghezza_codice>> per comodit�
	map<uint8_t, pair<uint32_t,uint32_t>> codes_map;
	for(unsigned i=0; i<codes.size(); ++i){
		codes_map.insert(pair<uint8_t,pair<uint32_t,uint32_t>>(codes[i].symbol, pair<uint32_t,uint32_t>(codes[i].code,codes[i].code_len)));
	}

	return codes_map;

}

void ParHuffman::compress(string filename){
	// utili per ottimizzazione
	tick_count t0, t1;

	// Creazione dell'istogramma in parallelo con parallel_reduce
	t0 = tick_count::now();
	TBBHistoReduce tbbhr;
	parallel_reduce(blocked_range<uint8_t*>(_file_in.data(),_file_in.data()+_file_length), tbbhr);
	t1 = tick_count::now();
	cerr << "[PAR] La creazione dell'istogramma ha impiegato " << (t1 - t0).seconds() << " sec" << endl;

	// creo un vettore che conterr� le foglie dell'albero di huffman, ciascuna con simbolo e occorrenze
	t0 = tick_count::now();
	TBBLeavesVector leaves_vect;
	par_create_huffman_tree(tbbhr._histo, leaves_vect);
	//par_create_huffman_tree(histo, leaves_vect);
	t1 = tick_count::now();
	cerr << "[PAR] La creazione dell'albero ha impiegato " << (t1 - t0).seconds() << " sec" << endl;

	leaves_vect[0]->setRoot(true);

	// creo una depthmap, esplorando tutto l'albero, per sapere a che profondit� si trovano i simboli
	// la depthmap contiene le coppie <lunghezza_simbolo, simbolo>
	DepthMap depthmap;
	par_depth_assign(leaves_vect[0], depthmap);

	cerr << "[PAR] Profondita' assegnate" << endl;

	// ordino la depthmap per profondit� 
	sort(depthmap.begin(), depthmap.end(), par_depth_compare);

	cerr << "[PAR] Profondita' ordinate" << endl;

	// creo i codici canonici usando la depthmap e li scrivo in codes
	vector<ParTriplet> codes;
	par_canonical_codes(depthmap, codes);

	// ----- DEBUG stampa i codici canonici sulla console--------------------------------
	/*cout << "SYM\tCODE\tC_LEN\n";
	for(unsigned i=0; i<codes.size(); ++i)
	cout << (int)codes[i].symbol << "\t" << (int)codes[i].code << "\t" << (int)codes[i].code_len << endl;*/
	//-----------------------------------------------------------------------------------
	cerr << "[PAR] Ci sono " << codes.size() << " simboli" << endl;

	// crea una mappa <simbolo, <codice, lunghezza_codice>> per comodit�
	map<uint8_t, pair<uint32_t,uint32_t>> codes_map;
	for(unsigned i=0; i<codes.size(); ++i){
		codes_map.insert(pair<uint8_t,pair<uint32_t,uint32_t>>(codes[i].symbol, pair<uint32_t,uint32_t>(codes[i].code,codes[i].code_len)));
	}

	cerr << "[PAR] Scrittura su file" << endl;
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
	btw.write((uint32_t)depthmap.size(), 32);
	// scrivo tutti i simboli seguiti dalle lunghezze
	for(uint32_t i=0; i<depthmap.size(); ++i){
		btw.write(depthmap[i].second, 8);
		btw.write(depthmap[i].first, 8);
	}

	//// Scrittura del vettore di output
	//cerr << "Dimensione del pair: " << sizeof(pair<uint32_t, uint32_t>) << endl;
	//vector<pair<uint32_t, uint32_t>> buffer_map(_file_length);
	//cerr << "Dimensione di buffer_map " << buffer_map.size()*sizeof(pair<uint32_t, uint32_t>) << endl;
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);

	cerr << endl << "Dimensione file: " << _file_length/1000000 << " MB" << endl;
	cerr << "Dimensione x 8 : " << (_file_length*8)/1000000 << " MB" << endl;
	uint64_t available_ram = status.ullAvailPhys;
	cerr << "RAM disponibile: " << available_ram/1000000 << " MB" << endl;

	// ----- RIPARTIZIONE DINAMICA
	uint64_t num_chunk = 1 + (_file_length*8-1)/available_ram;
	if(num_chunk==0)
		num_chunk=1;
	num_chunk++; // overprovisioning
	cerr << "\nNumero di chunks: " << num_chunk << endl;
	uint64_t chunk_dim = _file_length/num_chunk; 
	cerr << "Dimensione di un chunk: " << chunk_dim/1000000 << " MB" << endl;
	// ------ FINE RIPARTIZIONE DINAMICA

	for (size_t i=0; i < num_chunk; ++i) {
		vector<pair<uint32_t, uint32_t>> buffer_map(chunk_dim);
		parallel_for(blocked_range<int>(i*chunk_dim, chunk_dim*(i+1),10000), [&](const blocked_range<int>& range) {
			pair<uint32_t,uint32_t> element;
			for( int r=range.begin(); r!=range.end(); ++r ){
				element = codes_map[_file_in[r]];
				buffer_map[r-i*chunk_dim].first = element.first;
				buffer_map[r-i*chunk_dim].second = element.second;
			}

		});
		for (size_t j = 0; j < chunk_dim; j++)
			btw.write(buffer_map[j].first, buffer_map[j].second);
		buffer_map.clear();
	}
	// Legge la parte del file che viene tagliata dall'approssimazione nella divisione in chunks
	pair<uint32_t,uint32_t> element;
	//cerr << "Scrivo un byte avanzato " << i << endl;
	//cerr << "8*chunk_dim: " << 8*chunk_dim << " file_len :" << _file_length << endl;
	for (size_t i=num_chunk*chunk_dim; i < _file_length; i++){
		element = codes_map[_file_in[i]];
		btw.write(element.first, element.second);
	}
	btw.flush();

	t1 = tick_count::now();
	cerr << endl << "[PAR] La scrittura del file di output su buffer ha impiegato " << (t1 - t0).seconds() << " sec" << endl;

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

		// cerco il codice nella mappa, se non c'� un codice come quello che ho appena letto
		// o se c'� ma ha lunghezza diversa da quella corrente, leggo un altro bit e lo
		// aggiungo al codice per cercare di nuovo.
		// questo modo mi sa che � super inefficiente... sicuramente si pu� fare di meglio
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