#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <iterator>
#include <cmath>
#include <numeric>
#include <functional>
#include <map>
#include <cstdint>
#include <string>
#include "bitreader.h"

using namespace std;

struct triplet{
	uint8_t symbol;
	uint32_t code;
	uint32_t code_len;
};

struct histo_incr {
	vector<uint32_t>& _histo; 

	histo_incr (vector<uint32_t>& histo) : _histo(histo) {} 

	void operator() (const uint8_t& val) {
		_histo[val]++;
	}
};

struct huff_node {
	int _symb;
	unsigned _occ;
	unsigned _depth;
	huff_node *_left;
	huff_node *_right;
	bool _is_leaf;
	bool _is_root;

	huff_node ();
	huff_node (int symb, unsigned occ, bool is_leaf, huff_node *left, huff_node *right) : _symb(symb), _occ(occ), _left(left), _right(right), _is_leaf(is_leaf), _is_root(false), _depth(0) {}
	huff_node (int symb, unsigned occ) : _symb(symb), _occ(occ), _left(NULL), _right(NULL), _is_leaf(true), _is_root(false), _depth(0){}
};

void depth_assign(huff_node* root, vector<pair<unsigned,uint8_t>> & depthmap);
void huffman_compress(string in_file, string out_file);
void huffman_decompress(string in_file, string out_file);
void canonical_codes(vector<pair<unsigned,uint8_t>> & depthmap, vector<triplet>& codes);

double entropy_func (const double& entropy, const double& d) {
	if (d>0.0)
		return entropy-d*log(d)/log(2.0);
	else
		return entropy;
}

typedef vector<uint32_t> cont_t;
typedef cont_t::iterator iter_t;

bool leaves_compare(huff_node* first, huff_node* second){
	return (first->_occ > second->_occ);
}

bool depth_compare(pair<unsigned,uint8_t> first, pair<unsigned,uint8_t> second){
	return (first.first < second.first);
}

int main (int argc, char *argv[]) {
	if (argc!=3) {
		cout << "Syntax error:\n\n";
		cout << "stat <input-file> <output-file>\n";
		return -1;
	}

	huffman_compress(argv[1], argv[2]);

	huffman_decompress(argv[2], argv[1]);
}

void huffman_compress(string in_file, string out_file){
	
	// file di input e di output
	ifstream in_f(in_file, ifstream::in|ifstream::binary);
	ofstream out_f(out_file, fstream::out|fstream::binary);
	// NON salta i whitespaces
	in_f.unsetf (ifstream::skipws);
	//creo un istogramma per i 256 possibili uint8_t
	cont_t histo(256);
	for_each (istream_iterator<uint8_t>(in_f), istream_iterator<uint8_t>(), histo_incr(histo));


	// creo un vettore che conterrà le foglie dell'albero di huffman, ciascuna con simbolo e occorrenze
	vector<huff_node*> leaves_vect;

	for (size_t i=0;i<histo.size();++i) {
		if(histo[i] > 0){
			leaves_vect.push_back(new huff_node(i,histo[i]));
		}
	}

	// ordino le foglie per occorrenze, in modo da partire da quelle con probabilità più bassa
	sort(leaves_vect.begin(), leaves_vect.end(), leaves_compare);

	// creo l'albero di huffman
	while(leaves_vect.size() > 1){
		// scelgo i due nodi con probabilità minore
		huff_node* node1;
		huff_node* node2;
		node1 = leaves_vect.back();
		leaves_vect.pop_back();
		node2 = leaves_vect.back();
		leaves_vect.pop_back();

		unsigned tot_occ = node1->_occ + node2->_occ;

		// inserisco il nodo padre nella posizione giusta in base alle probabilità
		for(unsigned i=0; i<leaves_vect.size(); ++i){
			if(leaves_vect[i]->_occ <= tot_occ){
				leaves_vect.insert(leaves_vect.begin()+i, new huff_node(-1, tot_occ, false, node1, node2));
				break;
			} else if(i == leaves_vect.size()-1){
				leaves_vect.insert(leaves_vect.begin()+(i+1), new huff_node(-1, tot_occ, false, node1, node2));
				break;
			}
		}

		// se sono arrivato alla fine creo la root
		if(leaves_vect.size() == 0) leaves_vect.push_back(new huff_node(-1, tot_occ, false, node1, node2));
	}
	cout << "albero creato\n";

	leaves_vect[0]->_is_root = true;

	// creo una depthmap, esplorando tutto l'albero, per sapere a che profondità si trovano i simboli
	// la depthmap contiene le coppie <lunghezza_simbolo, simbolo>
	vector<pair<unsigned,uint8_t>> depthmap;
	depth_assign(leaves_vect[0], depthmap);

	cout << "profondita' assegnate\n";

	// ordino la depthmap per profondità 
	sort(depthmap.begin(), depthmap.end(), depth_compare);

	cout << "profondita' ordinate\n\n";

	// creo i codici canonici usando la depthmap e li scrivo in codes
	vector<triplet> codes;
	canonical_codes(depthmap, codes);

	// stampa i codici canonici sulla console
	cout << "SYM\tCODE\tC_LEN\n";
	for(unsigned i=0; i<codes.size(); ++i)
		cout << (int)codes[i].symbol << "\t" << (int)codes[i].code << "\t" << (int)codes[i].code_len << "\n";

	cout << "\n";
	cout << "Ci sono " << codes.size() << " simboli" << "\n";

	// crea una mappa <simbolo, <codice, lunghezza_codice>> per comodità
	map<uint8_t, pair<uint32_t,uint32_t>> codes_map;
	for(unsigned i=0; i<codes.size(); ++i){
		codes_map.insert(pair<uint8_t, pair<uint32_t,uint32_t>>(codes[i].symbol, pair<uint32_t,uint32_t>(codes[i].code,codes[i].code_len)));
	}

	//system("pause");

	cout << "scrittura su file...\n";

	// crea il file di output
	bitwriter btw(out_f);
	uint8_t tmp;

	/* scrivo le lunghezze dei codici all'inizio del file secondo il formato (migliorabile penso...):
	 * - un magic number di 4 byte per contraddistinguere il formato: BCP1 (in hex: 42 43 50 01) 
	 * - i primi 4 byte dicono quanti simboli ci sono (n)
	 * - il primo byte è seguito dagli n simboli:
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
		btw.write(codes_map[tmp].first,codes_map[tmp].second);
	}
	btw.flush();

	in_f.close();
	out_f.close();

	cout << "fatto\n";
}

void huffman_decompress(string in_file, string out_file){

	cout << "Decompressing " << out_file << "..." << "\n";
	
	// aggiungo "decompressed" al file di output
	out_file.insert(out_file.size()-4, "_decompressed");

	// apro i file
	ifstream in_f(in_file, ifstream::in|ifstream::binary);
	ofstream out_f(out_file, fstream::out|fstream::binary);

	bitreader btr(in_f);
	bitwriter btw(out_f);
	
	// leggo il magic number
	uint32_t magic_number = btr.read(32);
	if(magic_number != 0x42435001){
		cout << "Error: unknown format, wrong magic number..." << "\n";
		return;
	}

	// leggo il numero di simboli totali
	uint32_t tot_symbols = btr.read(32);
	// creo un vettore di coppie <lunghezza_simbolo, simbolo>
	vector<pair<unsigned, uint8_t>> depthmap;

	for(unsigned i=0; i<tot_symbols; ++i)
		depthmap.push_back( pair<unsigned, uint8_t>(btr.read(8), btr.read(8)) );

	// creo i codici canonici usando la depthmap e li scrivo in codes
	vector<triplet> codes;
	canonical_codes(depthmap, codes);

	// crea una mappa <codice, <simbolo, lunghezza_codice>>
	map<uint32_t, pair<uint8_t, uint32_t>> codes_map;
	
	for(unsigned i=0; i<codes.size(); ++i)
		codes_map.insert(pair<uint32_t, pair<uint32_t, uint8_t>>(codes[i].code, pair<uint32_t, uint8_t>(codes[i].symbol, codes[i].code_len)));
	
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
		pair<uint8_t, uint32_t> tmp_sym_pair = codes_map[tmp_code];
		btw.write(tmp_sym_pair.first, 8);
	}
	btw.flush();

	// chiudo i file
	in_f.close();
	out_f.close();

	system("pause");
}

void depth_assign(huff_node* root, vector<pair<unsigned,uint8_t>> & depthmap){
	if(root->_is_leaf){
		depthmap.push_back(pair<unsigned,uint8_t>(root->_depth,root->_symb));
		return;
	} else {
		if(root->_is_root){
			root->_left->_depth = 1;
			root->_right->_depth = 1;

			depth_assign(root->_left, depthmap);
			depth_assign(root->_right, depthmap);
		} else {
			root->_left->_depth = root->_depth;
			root->_left->_depth += 1;
			root->_right->_depth = root->_depth;
			root->_right->_depth += 1;

			depth_assign(root->_left, depthmap);
			depth_assign(root->_right, depthmap);
		}
	}
}

void canonical_codes(vector<pair<unsigned,uint8_t>> & depthmap, vector<triplet>& codes){
	triplet curr_code;
	curr_code.code = 0;
	curr_code.code_len = depthmap[0].first;
	curr_code.symbol = depthmap[0].second;

	codes.push_back(curr_code);

	// calcolo i codici canonici
	for(unsigned i=1; i<depthmap.size(); ++i){
		if(depthmap[i].first > curr_code.code_len){
			curr_code.code = (curr_code.code+1)<<(depthmap[i].first - curr_code.code_len);
		} else {
			curr_code.code += 1;
		}
		curr_code.symbol = depthmap[i].second;
		curr_code.code_len = depthmap[i].first;

		codes.push_back(curr_code);
	}
}