#include <iostream>
#include <fstream>
#include "bitwriter.h"
#include "bitreader.h"
#include "tbb/tbb.h"
#include "tbb/concurrent_vector.h"
#include "seq_huffman.h"
#include "seq_huffman_utils.h"
#include <utility> // pair

using namespace std;
using namespace tbb;

void SeqHuffman::create_histo(vector<uint32_t>& histo, uint64_t chunk_dim){
	for(size_t i=0; i<chunk_dim; ++i)
		histo[_file_in[i]]++;
}

map<std::uint8_t, pair<uint32_t,uint32_t>> SeqHuffman::create_code_map(vector<uint32_t>& histo){
	tick_count t0, t1;
	t0 = tick_count::now();
	LeavesVector leaves_vect;
	seq_create_huffman_tree(histo, leaves_vect);
	t1 = tick_count::now();
	//cerr << "[SEQ] La creazione dell'albero ha impiegato " << (t1 - t0).seconds() << " sec" << endl;

	leaves_vect[0]->setRoot(true);
	DepthMap depthmap;
	seq_depth_assign(leaves_vect[0], depthmap);

	// ordino la depthmap per profondità 
	sort(depthmap.begin(), depthmap.end(), seq_depth_compare);

	// creo i codici canonici usando la depthmap e li scrivo in codes
	vector<SeqTriplet> codes;
	seq_canonical_codes(depthmap, codes);

	// crea una mappa <simbolo, <codice, lunghezza_codice>> per comodità
	map<uint8_t, pair<uint32_t,uint32_t>> codes_map;
	for(unsigned i=0; i<codes.size(); ++i){
		codes_map.insert(pair<uint8_t,pair<uint32_t,uint32_t>>(codes[i].symbol, pair<uint32_t,uint32_t>(codes[i].code,codes[i].code_len)));
	}

	return codes_map;
}

BitWriter SeqHuffman::write_header(std::map<std::uint8_t, std::pair<std::uint32_t,std::uint32_t>> codes_map){
	// utili per ottimizzazione
	tick_count t0, t1;
	t0 = tick_count::now();
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
	sort(depthmap.begin(), depthmap.end(), seq_depth_compare);

	// scrivo PRIMA IL SIMBOLO POI LA LUNGHEZZA
	for(size_t i=0; i<depthmap.size(); ++i){
		btw.write(depthmap[i].second, 8); // simbolo
		btw.write(depthmap[i].first, 8);  // lunghezza
	}
	return btw;
}

void SeqHuffman::write_chunks_compressed(std::uint64_t available_ram, std::uint64_t macrochunk_dim, std::map<std::uint8_t, std::pair<std::uint32_t,std::uint32_t>> codes_map, BitWriter& btw){

	uint64_t num_microchunk = 1 + (macrochunk_dim*8-1)/available_ram;
	if(num_microchunk==0)
		num_microchunk=1;
	num_microchunk++; // overprovisioning
	//cerr << "\nNumero di microchunks: " << num_microchunk << endl;
	uint64_t microchunk_dim = macrochunk_dim/num_microchunk; 
	//cerr << "Dimensione di un microchunk: " << microchunk_dim/1000000 << " MB" << endl;

	for (size_t i=0; i < num_microchunk; ++i) {
		vector<pair<uint32_t, uint32_t>> buffer_map(microchunk_dim);
		pair<uint32_t,uint32_t> element;
		for( int r=i*microchunk_dim; r<(microchunk_dim*(i+1)); ++r ){
			element = codes_map[_file_in[r]];
			buffer_map[r-i*microchunk_dim].first = element.first;
			buffer_map[r-i*microchunk_dim].second = element.second;
		}

		for (size_t j = 0; j < microchunk_dim; j++)
			btw.write(buffer_map[j].first, buffer_map[j].second);
		buffer_map.clear();
	}
	// Legge la parte del file che viene tagliata dall'approssimazione nella divisione in chunks
	pair<uint32_t,uint32_t> element;
	for (size_t i=num_microchunk*microchunk_dim; i < macrochunk_dim; i++){
		element = codes_map[_file_in[i]];
		btw.write(element.first, element.second);
	}
}

void SeqHuffman::decompress (string filename){

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
	// In realtà legge coppie <simbolo, lunghezza_codice>, non viceversa!!! come fa a funzionare???
	DepthMap depthmap;
	for(unsigned i=0; i<tot_symbols; ++i)
		depthmap.push_back(DepthMapElement(btr.read(8), btr.read(8)));

	// creo i codici canonici usando la depthmap e li scrivo in codes
	vector<SeqTriplet> codes;
	seq_canonical_codes(depthmap, codes);

	// crea una mappa <codice, <simbolo, lunghezza_codice>>
	map<uint32_t, pair<uint8_t,uint32_t>> codes_map; 
	for(unsigned i=0; i<codes.size(); ++i)
		codes_map.insert(pair<uint32_t, pair<uint8_t,uint32_t>>(codes[i].code,pair<uint8_t,uint32_t>(codes[i].symbol, codes[i].code_len)));

	// leggo il file compresso e scrivo l'output
	while(btr.good()){
		uint32_t tmp_code = 0;
		uint32_t tmp_code_len = depthmap[0].first;
		tmp_code = btr.read(depthmap[0].first);
		// cerco il codice nella mappa, se non c'è un codice come quello che ho appena letto
		// o se c'è ma ha lunghezza diversa da quella corrente, leggo un altro bit e lo
		// aggiungo al codice per cercare di nuovo.
		// questo modo mi sa che è super inefficiente... sicuramente si può fare di meglio
		while(codes_map.find(tmp_code) == codes_map.end() || (codes_map.find(tmp_code) != codes_map.end() && tmp_code_len != codes_map[tmp_code].second )){
			tmp_code = (tmp_code << 1) | (1 & btr.read_bit());
			tmp_code_len++;
		}

		// se esco dal while ho trovato un codice, lo leggo, e lo scrivo sul file di output
		pair<uint32_t,uint32_t> tmp_sym_pair = codes_map[tmp_code];
		btw.write(tmp_sym_pair.first, 8); 
	}
	btw.flush();

}
