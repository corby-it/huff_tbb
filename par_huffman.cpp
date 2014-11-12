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

void ParHuffman::compress(string filename){
	// utili per ottimizzazione
	tick_count t0, t1;

	// setto output filename
	_output_filename = _original_filename;
	_output_filename.replace(_output_filename.size()-4, 4, ".bcp");

	// Creazione dell'istogramma in parallelo con parallel_reduce
	t0 = tick_count::now();
	TBBHistoReduce tbbhr;
	parallel_reduce(blocked_range<uint8_t*>(_file_in.data(),_file_in.data()+_file_length), tbbhr);
	t1 = tick_count::now();
	cerr << "[PAR] La creazione dell'istogramma ha impiegato " << (t1 - t0).seconds() << " sec" << endl;

	// creo un vettore che conterrà le foglie dell'albero di huffman, ciascuna con simbolo e occorrenze
	t0 = tick_count::now();
	TBBLeavesVector leaves_vect;
	par_create_huffman_tree(tbbhr._histo, leaves_vect);
	//par_create_huffman_tree(histo, leaves_vect);
	t1 = tick_count::now();
	cerr << "[PAR] La creazione dell'albero ha impiegato " << (t1 - t0).seconds() << " sec" << endl;

	leaves_vect[0]->setRoot(true);

	// creo una depthmap, esplorando tutto l'albero, per sapere a che profondità si trovano i simboli
	// la depthmap contiene le coppie <lunghezza_simbolo, simbolo>
	DepthMap depthmap;
	par_depth_assign(leaves_vect[0], depthmap);

	cerr << "[PAR] Profondita' assegnate" << endl;

	// ordino la depthmap per profondità 
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

	// crea una mappa <simbolo, <codice, lunghezza_codice>> per comodità
	map<uint8_t, pair<uint32_t,uint32_t>> codes_map;
	for(unsigned i=0; i<codes.size(); ++i){
		codes_map.insert(pair<uint8_t,pair<uint32_t,uint32_t>>(codes[i].symbol, pair<uint32_t,uint32_t>(codes[i].code,codes[i].code_len)));
	}

	cerr << "[PAR] Scrittura su file";
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
	btw.write(_original_filename.size(), 32);
	// Scrivo il nome del file originale, per recuperarlo in decompressione
	for(size_t i=0; i<_original_filename.size(); ++i)
		btw.write(_original_filename[i], 8);

	// scrivo il numero di simboli
	btw.write(depthmap.size(), 32);
	// scrivo tutti i simboli seguiti dalle lunghezze
	for(unsigned i=0; i<depthmap.size(); ++i){
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
	
	int num_chunk = (_file_length*8)/status.ullAvailPhys; //cout << "\nNumero chunks: " << num_chunk << endl;
	if(num_chunk==0)
		num_chunk=1;
	cout << "\nNumero chunks: " << num_chunk << endl;
	size_t chunk_dim = _file_length/num_chunk; //cerr << "Chunk dim: " << chunk_dim << endl;

	for (size_t i=0; i < num_chunk; ++i) {
		//cerr << "Inizia il ciclo: " << i << endl;
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
	// Legge la parte del file che viene tagliata dall'approssimazione chunk_dim = _file_length/8;
	pair<uint32_t,uint32_t> element;
	//cerr << "8*chunk_dim: " << 8*chunk_dim << " file_len :" << _file_length << endl;
	for (size_t i=8*chunk_dim; i < _file_length; i++){
		//cerr << "Scrivo un byte avanzato " << i << endl;
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

	for(unsigned i=0; i<tot_symbols; ++i)
		depthmap.push_back(DepthMapElement(btr.read(8), btr.read(8)));

	// creo i codici canonici usando la depthmap e li scrivo in codes
	vector<ParTriplet> codes;
	par_canonical_codes(depthmap, codes);

	// crea una mappa <codice, <simbolo, lunghezza_codice>>
	map<uint32_t, pair<uint8_t,uint32_t>> codes_map; 
	for(unsigned i=0; i<codes.size(); ++i)
		codes_map.insert(pair<uint32_t, pair<uint8_t,uint32_t>>(codes[i].code,pair<uint8_t,uint32_t>(codes[i].symbol, codes[i].code_len)));

	// tentativo di decompressione parallela (righe seguenti)
	uint32_t bitreader_idx = btr.tell_index();
	cerr << endl << "Indice BITREADER: " << bitreader_idx << endl;
	//parallel_for(blocked_range<int>( bitreader_idx, _file_length, 10000), [&](const blocked_range<int>& range) {
	//	pair<uint32_t,uint32_t> element;
	//	for( int i=range.begin(); i!=range.end(); ++i ){	

	//	}
	//});

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