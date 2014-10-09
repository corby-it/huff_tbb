#ifndef HUFFMAN_UTILS_H
#define HUFFMAN_UTILS_H

#include <cstdint>
#include <vector>
#include <map>
#include <algorithm> //std::sort
#include "tbb\tbb.h"

// CLASSES

struct Triplet{
	std::uint8_t symbol;
	std::uint32_t code;
	std::uint32_t code_len;
};

class HistoIncr {
private:
	std::vector<uint32_t>& _histo; 

public:
	HistoIncr (std::vector<uint32_t>& histo) : _histo(histo) {} 

	void operator() (const std::uint8_t& val) {
		_histo[val]++;
	}
};

class HuffNode {
private:
	int _symb;
	unsigned _occ;
	unsigned _depth;
	// sarebbe meglio non usare qui i puntatori ma le reference o cose del genere credo... però per il momento va bene così
	HuffNode *_left;
	HuffNode *_right;
	bool _isLeaf;
	bool _isRoot;

public:
	HuffNode ();
	HuffNode (int symb, unsigned occ, bool is_leaf, HuffNode *left, HuffNode *right) : _symb(symb), _occ(occ), _left(left), _right(right), _isLeaf(is_leaf), _isRoot(false), _depth(0) {}
	HuffNode (int symb, unsigned occ) : _symb(symb), _occ(occ), _left(NULL), _right(NULL), _isLeaf(true), _isRoot(false), _depth(0){}

	void increaseDepth(unsigned i) { _depth += i; }

	// funzione usata dal sort
	static bool leaves_compare(HuffNode* first, HuffNode* second){
		return (first->getOcc() > second->getOcc());
	}

	// Getters and setters
	int getSymb (){ return _symb; }
	void setSymb (int symb){ _symb = symb; }

	unsigned getOcc (){ return _occ; }
	void setOcc (unsigned occ){ _occ = occ; }

	unsigned getDepth (){ return _depth; }
	void setDepth (unsigned depth){ _depth = depth; }

	HuffNode* getLeft (){ return _left; }
	void setLeft (HuffNode* left){ _left = left; }

	HuffNode* getRight (){ return _right; }
	void setRight (HuffNode* right){ _right = right; }

	bool isLeaf (){ return _isLeaf; }
	void setLeaf (bool leaf){ _isLeaf = leaf; }

	bool isRoot (){ return _isRoot; }
	void setRoot (bool root){ _isRoot = root; }
};

typedef std::vector<std::uint32_t> cont_t;
typedef cont_t::iterator iter_t;

typedef std::vector<HuffNode*> LeavesVector;

typedef std::vector<std::pair<unsigned,std::uint8_t>> DepthMap;
typedef std::pair<unsigned,std::uint8_t> DepthMapElement;

typedef std::map<std::uint8_t, std::pair<std::uint32_t,std::uint32_t>> CodesMap;
typedef std::pair<std::uint8_t, std::pair<std::uint32_t,std::uint32_t>> CodesMapElement;
typedef std::uint8_t CodesMapKey;
typedef std::pair<std::uint32_t,std::uint32_t> CodesMapValue;

// METHODS

double entropy_func (const double& entropy, const double& d) {
	if (d>0.0)
		return entropy-d*log(d)/log(2.0);
	else
		return entropy;
}

bool depth_compare(DepthMapElement first, DepthMapElement second){
	return (first.first < second.first);
}

void create_huffman_tree(cont_t histo, LeavesVector& leaves_vect){

	// creo un vettore che conterrà le foglie dell'albero di huffman, ciascuna con simbolo e occorrenze
	for (std::size_t i=0;i<histo.size();++i) {
		if(histo[i] > 0){
			leaves_vect.push_back(new HuffNode(i,histo[i]));
		}
	}

	// ordino le foglie per occorrenze, in modo da partire da quelle con probabilità più bassa
	std::sort(leaves_vect.begin(), leaves_vect.end(), HuffNode::leaves_compare);

	// creo l'albero di huffman
	while(leaves_vect.size() > 1){
		// scelgo i due nodi con probabilità minore
		HuffNode* node1;
		HuffNode* node2;
		node1 = leaves_vect.back();
		leaves_vect.pop_back();
		node2 = leaves_vect.back();
		leaves_vect.pop_back();

		unsigned tot_occ = node1->getOcc() + node2->getOcc();

		// inserisco il nodo padre nella posizione giusta in base alle probabilità
		for(unsigned i=0; i<leaves_vect.size(); ++i){
			if(leaves_vect[i]->getOcc() <= tot_occ){
				leaves_vect.insert(leaves_vect.begin()+i, new HuffNode(-1, tot_occ, false, node1, node2));
				break;
			} else if(i == leaves_vect.size()-1){
				leaves_vect.insert(leaves_vect.begin()+(i+1), new HuffNode(-1, tot_occ, false, node1, node2));
				break;
			}
		}

		// se sono arrivato alla fine creo la root
		if(leaves_vect.size() == 0) leaves_vect.push_back(new HuffNode(-1, tot_occ, false, node1, node2));
	}
}

void depth_assign(HuffNode* huff_node, DepthMap & depthmap){
	if(huff_node->isLeaf()){
		depthmap.push_back(DepthMapElement(huff_node->getDepth(), huff_node->getSymb()));
		return;
	} else {
		if(huff_node->isRoot()){
			huff_node->getLeft()->setDepth(1);
			huff_node->getRight()->setDepth(1);

			depth_assign(huff_node->getLeft(), depthmap);
			depth_assign(huff_node->getRight(), depthmap);
		} else {
			huff_node->getLeft()->setDepth(huff_node->getDepth());
			huff_node->getLeft()->increaseDepth(1);
			huff_node->getRight()->setDepth(huff_node->getDepth());
			huff_node->getRight()->increaseDepth(1);

			depth_assign(huff_node->getLeft(), depthmap);
			depth_assign(huff_node->getRight(), depthmap);
		}
	}
}

void canonical_codes(DepthMap & depthmap, std::vector<Triplet>& codes){
	Triplet curr_code;
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
//---------------------------------------------------------------------------------------------
// ----------- DEFINITIONS AND METHODS FOR PARALLEL EXECUTION----------------------------------
//---------------------------------------------------------------------------------------------

class TBBHuffNode {
private:
	tbb::atomic<int> _symb;
	tbb::atomic<unsigned> _occ;
	unsigned _depth;
	// sarebbe meglio non usare qui i puntatori ma le reference o cose del genere credo... però per il momento va bene così
	TBBHuffNode *_left;
	TBBHuffNode *_right;
	bool _isLeaf;
	bool _isRoot;

public:
	TBBHuffNode ();
	TBBHuffNode (tbb::atomic<int> symb, tbb::atomic<unsigned> occ, bool is_leaf, TBBHuffNode *left, TBBHuffNode *right) : _symb(symb), _occ(occ), _left(left), _right(right), _isLeaf(is_leaf), _isRoot(false), _depth(0) {}
	TBBHuffNode (tbb::atomic<int> symb, tbb::atomic<unsigned> occ) : _symb(symb), _occ(occ), _left(NULL), _right(NULL), _isLeaf(true), _isRoot(false), _depth(0){}

	void increaseDepth(unsigned i) { _depth += i; }

	// funzione usata dal sort
	static bool leaves_compare(TBBHuffNode* first, TBBHuffNode* second){
		return (first->getOcc() > second->getOcc());
	}


	// Getters and setters
	int getSymb (){ return _symb; }
	void setSymb (int symb){ _symb = symb; }

	unsigned getOcc (){ return _occ; }
	void setOcc (unsigned occ){ _occ = occ; }

	unsigned getDepth (){ return _depth; }
	void setDepth (unsigned depth){ _depth = depth; }

	TBBHuffNode* getLeft (){ return _left; }
	void setLeft (TBBHuffNode* left){ _left = left; }

	TBBHuffNode* getRight (){ return _right; }
	void setRight (TBBHuffNode* right){ _right = right; }

	bool isLeaf (){ return _isLeaf; }
	void setLeaf (bool leaf){ _isLeaf = leaf; }

	bool isRoot (){ return _isRoot; }
	void setRoot (bool root){ _isRoot = root; }
};


typedef std::vector<tbb::atomic<std::uint32_t>> TBBHisto;
typedef std::vector<TBBHuffNode*> TBBLeavesVector;


void create_huffman_tree_p(TBBHisto histo, TBBLeavesVector& leaves_vect){

	// creo un vettore che conterrà le foglie dell'albero di huffman, ciascuna con simbolo e occorrenze
	parallel_for(tbb::blocked_range<int>( 0, histo.size()), [&](const tbb::blocked_range<int>& range) {
		for( int i=range.begin(); i!=range.end(); ++i ){
			if(histo[i] > 0){
				tbb::atomic<int> j;
				j = i;
				leaves_vect.push_back(new TBBHuffNode(j,histo[i]));
			}
		}
	});
	//for (std::size_t i=0;i<histo.size();++i) {
	//	if(histo[i] > 0){
	//		tbb::atomic<int> j;
	//		j=i;
	//		leaves_vect.push_back(new TBBHuffNode(j,histo[i]));
	//	}
	//}

	// ordino le foglie per occorrenze, in modo da partire da quelle con probabilità più bassa
	std::sort(leaves_vect.begin(), leaves_vect.end(), TBBHuffNode::leaves_compare);

	// creo l'albero di huffman
	while(leaves_vect.size() > 1){
		// scelgo i due nodi con probabilità minore
		TBBHuffNode* node1;
		TBBHuffNode* node2;
		node1 = leaves_vect.back();
		leaves_vect.pop_back();
		node2 = leaves_vect.back();
		leaves_vect.pop_back();

		unsigned tot_occ = node1->getOcc() + node2->getOcc();

		tbb::atomic<unsigned> atomic_tot_occ;
		atomic_tot_occ = tot_occ;

		tbb::atomic<int> j;
		j = -1;

		// inserisco il nodo padre nella posizione giusta in base alle probabilità
		for(unsigned i=0; i<leaves_vect.size(); ++i){

			if(leaves_vect[i]->getOcc() <= tot_occ){
				leaves_vect.insert(leaves_vect.begin()+i, new TBBHuffNode(j, atomic_tot_occ, false, node1, node2));
				break;
			} else if(i == leaves_vect.size()-1){
				leaves_vect.insert(leaves_vect.begin()+(i+1), new TBBHuffNode(j, atomic_tot_occ, false, node1, node2));
				break;
			}
		}

		// se sono arrivato alla fine creo la root
		if(leaves_vect.size() == 0)	leaves_vect.push_back(new TBBHuffNode(j, atomic_tot_occ, false, node1, node2));
	}
}

void depth_assign_p(TBBHuffNode* tbb_huff_node, DepthMap & depthmap){
	if(tbb_huff_node->isLeaf()){
		depthmap.push_back(DepthMapElement(tbb_huff_node->getDepth(), tbb_huff_node->getSymb()));
		return;
	} else {
		if(tbb_huff_node->isRoot()){
			tbb_huff_node->getLeft()->setDepth(1);
			tbb_huff_node->getRight()->setDepth(1);

			depth_assign_p(tbb_huff_node->getLeft(), depthmap);
			depth_assign_p(tbb_huff_node->getRight(), depthmap);
		} else {
			tbb_huff_node->getLeft()->setDepth(tbb_huff_node->getDepth());
			tbb_huff_node->getLeft()->increaseDepth(1);
			tbb_huff_node->getRight()->setDepth(tbb_huff_node->getDepth());
			tbb_huff_node->getRight()->increaseDepth(1);

			depth_assign_p(tbb_huff_node->getLeft(), depthmap);
			depth_assign_p(tbb_huff_node->getRight(), depthmap);
		}
	}
}

#endif /*HUFFMAN_UTILS_H*/