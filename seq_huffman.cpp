#include <iostream>
#include <fstream>
#include "bitwriter.h"
#include "bitreader.h"
#include "tbb/tbb.h"
#include "tbb/concurrent_vector.h"
#include "seq_huffman.h"
#include "seq_huffman_utils.h"

using namespace std;
using namespace tbb;

void SeqHuffman::compress(string filename){
	// utili per ottimizzazioni
	tick_count t0, t1;
	
	// file di output
	// converto l'estensione del file di output in ".bcp" 
	string out_file(filename);
	out_file.replace(out_file.size()-4, 4, ".bcp");
	ofstream out_f(out_file, fstream::out|fstream::binary);

	//creo un istogramma per i 256 possibili uint8_t
	vector<uint32_t> histo(256);
	t0 = tick_count::now();
	for(size_t i=0; i<_file_length; ++i)
		histo[_file_vector[i]]++;
	t1 = tick_count::now();
	cerr << "[SEQ] La creazione dell'istogramma ha impiegato " << (t1 - t0).seconds() << " sec" << endl;

	// creo un vettore che conterrà le foglie dell'albero di huffman, ciascuna con simbolo e occorrenze
	t0 = tick_count::now();
	LeavesVector leaves_vect;
	create_huffman_tree(histo, leaves_vect);
	t1 = tick_count::now();
	cerr << "[SEQ] La creazione dell'albero ha impiegato " << (t1 - t0).seconds() << " sec" << endl;

	leaves_vect[0]->setRoot(true);

	// creo una depthmap, esplorando tutto l'albero, per sapere a che profondità si trovano i simboli
	// la depthmap contiene le coppie <lunghezza_simbolo, simbolo>
	DepthMap depthmap;
	depth_assign(leaves_vect[0], depthmap);

	cerr << "[SEQ] Profondita' assegnate\n";

	// ordino la depthmap per profondità 
	sort(depthmap.begin(), depthmap.end(), sdepth_compare);

	cerr << "[SEQ] Profondita' ordinate" << endl;

	// creo i codici canonici usando la depthmap e li scrivo in codes
	vector<Triplet> codes;
	scanonical_codes(depthmap, codes);

	// ----- DEBUG stampa i codici canonici sulla console--------------------------------
	//cout << "SYM\tCODE\tC_LEN\n";
	//for(unsigned i=0; i<codes.size(); ++i)
	//	cout << (int)codes[i].symbol << "\t" << (int)codes[i].code << "\t" << (int)codes[i].code_len << endl;
	//-----------------------------------------------------------------------------------
	cout << "[SEQ] Ci sono " << codes.size() << " simboli" << endl;

	// crea una mappa <simbolo, <codice, lunghezza_codice>> per comodità
	CodesMap codes_map;
	for(unsigned i=0; i<codes.size(); ++i){
		codes_map.insert(CodesMapElement(codes[i].symbol, CodesMapValue(codes[i].code,codes[i].code_len)));
	}

	cout << "[SEQ] Scrittura su file..." << endl;
	t0 = tick_count::now();
	// crea il file di output
	BitWriter btw(out_f);

	/* scrivo le lunghezze dei codici all'inizio del file secondo il formato (migliorabile penso...):
	* - un magic number di 4 byte per contraddistinguere il formato: BCP1 (in hex: 42 43 50 01) 
	* - i primi 4 byte dicono quanti simboli ci sono (n)
	* - seguono gli n simboli:
	*		-- 1 byte per il simbolo, 1 byte per la lunghezza
	*/
	//scrivo il magic number
	btw.write(0x42435001, 32); //BCP + 1 byte versione
	// ---- qui metterei l'estensione originaria
	// scrivo il numero di simboli
	btw.write(depthmap.size(), 32);
	// scrivo tutti i simboli seguiti dalle lunghezze
	for(unsigned i=0; i<depthmap.size(); ++i){
		btw.write(depthmap[i].second, 8);
		btw.write(depthmap[i].first, 8);
	}

	// Scrittura del file di output
	for (size_t i = 0; i < _file_length; i++)
		btw.write(codes_map[_file_vector[i]].first, codes_map[_file_vector[i]].second);
	btw.flush();
	t1 = tick_count::now();
	cerr << "[SEQ] La scrittura del file di output ha impiegato " << (t1 - t0).seconds() << " sec" << endl;

	out_f.close();
}

void SeqHuffman::decompress (string in_file, string out_file){

	cout << "Decompressing " << out_file << "..." << endl;

	// aggiungo "decompressed" al file di output
	out_file.insert(out_file.size()-4, "_decompressed");

	// apro i file
	ifstream in_f(in_file, ifstream::in|ifstream::binary);
	ofstream out_f(out_file, fstream::out|fstream::binary);

	BitReader btr(in_f);
	BitWriter btw(out_f);

	// leggo il magic number
	uint32_t magic_number = btr.read(32);
	if(magic_number != 0x42435001){
		cerr << "Error: unknown format, wrong magic number..." << endl;
		exit(1);
	}

	// leggo il numero di simboli totali
	uint32_t tot_symbols = btr.read(32);
	// creo un vettore di coppie <lunghezza_simbolo, simbolo>
	DepthMap depthmap;

	for(unsigned i=0; i<tot_symbols; ++i)
		depthmap.push_back( DepthMapElement(btr.read(8), btr.read(8)) );

	// creo i codici canonici usando la depthmap e li scrivo in codes
	vector<Triplet> codes;
	scanonical_codes(depthmap, codes);

	// crea una mappa <codice, <simbolo, lunghezza_codice>>
	CodesMap codes_map;

	for(unsigned i=0; i<codes.size(); ++i)
		codes_map.insert(CodesMapElement(codes[i].code, CodesMapValue(codes[i].symbol, codes[i].code_len)));

	// leggo il file compresso e scrivo l'output
	while(in_f.good()){
		uint32_t tmp_code = 0;
		uint32_t tmp_code_len = depthmap[0].first;
		tmp_code = btr.read(depthmap[0].first);

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
		CodesMapValue tmp_sym_pair = codes_map[tmp_code];
		btw.write(tmp_sym_pair.first, 8);
	}
	btw.flush();

	// chiudo i file
	in_f.close();
	out_f.close();
}
