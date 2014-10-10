#include <iostream>
#include <fstream>
#include "bitwriter.h"
#include "tbb\tbb.h"
#include "tbb\concurrent_vector.h"
#include "seq_huffman.h"
#include "huffman_utils.h"

using namespace std;
using namespace tbb;

void SeqHuffman::compress(string filename){
	// utili per ottimizzazioni
	tick_count t0, t1;

	ifstream in_f(filename, ifstream::in|ifstream::binary);
	string out_file(filename);

	// NON salta i whitespaces
	in_f.unsetf (ifstream::skipws);

	// file di input e di output
//	string out_file(filename);
	// converto l'estensione del file di output in ".bcp" 
	out_file.replace(out_file.size()-4, 4, ".bcp");
	ofstream out_f(out_file, fstream::out|fstream::binary);

	//creo un istogramma per i 256 possibili uint8_t
	t0 = tick_count::now();
	cont_t histo(256);
	for_each (istream_iterator<uint8_t>(in_f), istream_iterator<uint8_t>(), HistoIncr(histo));
	t1 = tick_count::now();
	cerr << "[SEQ] La creazione dell'istogramma ha impiegato " << (t1 - t0).seconds() << " sec" << endl;

	// creo un vettore che conterr� le foglie dell'albero di huffman, ciascuna con simbolo e occorrenze
	t0 = tick_count::now();
	LeavesVector leaves_vect;
	create_huffman_tree(histo, leaves_vect);
	t1 = tick_count::now();
	cerr << "[SEQ] La creazione dell'albero ha impiegato " << (t1 - t0).seconds() << " sec" << endl;

	leaves_vect[0]->setRoot(true);

	// creo una depthmap, esplorando tutto l'albero, per sapere a che profondit� si trovano i simboli
	// la depthmap contiene le coppie <lunghezza_simbolo, simbolo>
	DepthMap depthmap;
	depth_assign(leaves_vect[0], depthmap);

	cerr << "[SEQ] Profondita' assegnate\n";

	// ordino la depthmap per profondit� 
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

	// crea una mappa <simbolo, <codice, lunghezza_codice>> per comodit�
	CodesMap codes_map;
	for(unsigned i=0; i<codes.size(); ++i){
		codes_map.insert(CodesMapElement(codes[i].symbol, CodesMapValue(codes[i].code,codes[i].code_len)));
	}

	cout << "[SEQ] Scrittura su file..." << endl;
	t0 = tick_count::now();
	// crea il file di output
	BitWriter btw(out_f);
	uint8_t tmp;

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

	// scrivo l'output compresso
	// resetto in_f all'inizio
	in_f.clear();
	in_f.seekg(0, ios::beg);
	in_f.unsetf (ifstream::skipws);

	while(in_f.good()){
		tmp = in_f.get();
		btw.write(codes_map[tmp].first, codes_map[tmp].second);
	}
	btw.flush();
	t1 = tick_count::now();
	cerr << "[SEQ] La scrittura del file di output ha impiegato " << (t1 - t0).seconds() << " sec" << endl;

	in_f.close();
	out_f.close();
}

void SeqHuffman::decompress (){
	cerr << "ciao" << endl;
}