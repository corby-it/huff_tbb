#include "huffman_engine.h"
#include "huffman_utils.h"
#include "bitreader.h"
#include "bitwriter.h"

#include "tbb/tbb.h"
#include <fstream>


using namespace std;

HuffmanEngine::HuffmanEngine (){


}

void HuffmanEngine::compress(string in_file){
	
	// file di input e di output
	ifstream in_f(in_file, ifstream::in|ifstream::binary);
	string out_file(in_file);
	// converto l'estensione del file di output in ".bcp" 
	out_file.replace(out_file.size()-4, 4, ".bcp");
	ofstream out_f(out_file, fstream::out|fstream::binary);
	// NON salta i whitespaces
	in_f.unsetf (ifstream::skipws);

	//creo un istogramma per i 256 possibili uint8_t
	cont_t histo(256);
	for_each (istream_iterator<uint8_t>(in_f), istream_iterator<uint8_t>(), HistoIncr(histo));


	// creo un vettore che conterrà le foglie dell'albero di huffman, ciascuna con simbolo e occorrenze
	LeavesVector leaves_vect;
	create_huffman_tree(histo, leaves_vect);
	cout << "albero creato\n";

	leaves_vect[0]->setRoot(true);

	// creo una depthmap, esplorando tutto l'albero, per sapere a che profondità si trovano i simboli
	// la depthmap contiene le coppie <lunghezza_simbolo, simbolo>
	DepthMap depthmap;
	depth_assign(leaves_vect[0], depthmap);

	cout << "profondita' assegnate\n";

	// ordino la depthmap per profondità 
	sort(depthmap.begin(), depthmap.end(), depth_compare);

	cout << "profondita' ordinate\n\n";

	// creo i codici canonici usando la depthmap e li scrivo in codes
	vector<Triplet> codes;
	canonical_codes(depthmap, codes);

	// stampa i codici canonici sulla console
	cout << "SYM\tCODE\tC_LEN\n";
	for(unsigned i=0; i<codes.size(); ++i)
		cout << (int)codes[i].symbol << "\t" << (int)codes[i].code << "\t" << (int)codes[i].code_len << "\n";

	cout << "\n";
	cout << "Ci sono " << codes.size() << " simboli" << "\n";

	// crea una mappa <simbolo, <codice, lunghezza_codice>> per comodità
	CodesMap codes_map;
	for(unsigned i=0; i<codes.size(); ++i){
		codes_map.insert(CodesMapElement(codes[i].symbol, CodesMapValue(codes[i].code,codes[i].code_len)));
	}

	//system("pause");

	cout << "scrittura su file...\n";

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
	btw.write(0x42435001, 32);
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

	in_f.close();
	out_f.close();

	cout << "fatto\n";
}



void HuffmanEngine::decompress(string in_file, string out_file){

	cout << "Decompressing " << out_file << "..." << "\n";
	
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
		cout << "Error: unknown format, wrong magic number..." << "\n";
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
	canonical_codes(depthmap, codes);

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