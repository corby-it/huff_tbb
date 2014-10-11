#ifndef HUFFMAN_UTILS_H
#define HUFFMAN_UTILS_H

#include <cstdint>
#include <vector>
#include <map>
#include <algorithm> //std::sort
#include "tbb\tbb.h"
#include "tbb/concurrent_vector.h"
#include "seq_huffman_node.h"

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

typedef std::vector<std::uint32_t> cont_t;
typedef cont_t::iterator iter_t;

typedef std::vector<SeqHuffNode*> LeavesVector;

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

bool sdepth_compare(DepthMapElement first, DepthMapElement second){
	return (first.first < second.first);
}

void create_huffman_tree(cont_t histo, LeavesVector& leaves_vect){

	// creo un vettore che conterrà le foglie dell'albero di huffman, ciascuna con simbolo e occorrenze
	for (std::size_t i=0;i<histo.size();++i) {
		if(histo[i] > 0){
			leaves_vect.push_back(new SeqHuffNode(i,histo[i]));
		}
	}

	// ordino le foglie per occorrenze, in modo da partire da quelle con probabilità più bassa
	std::sort(leaves_vect.begin(), leaves_vect.end(), SeqHuffNode::leaves_compare);
	//// ---------------solo per debug -------------------
	//for(size_t i=0; i<leaves_vect.size(); ++i)
	//	leaves_vect[i]->to_string();
	//// -------------------------------------------------

	// creo l'albero di huffman
	while(leaves_vect.size() > 1){
		// scelgo i due nodi con probabilità minore
		SeqHuffNode* node1;
		SeqHuffNode* node2;
		node1 = leaves_vect.back();
		leaves_vect.pop_back();
		node2 = leaves_vect.back();
		leaves_vect.pop_back();

		unsigned tot_occ = node1->getOcc() + node2->getOcc();

		// inserisco il nodo padre nella posizione giusta in base alle probabilità
		for(unsigned i=0; i<leaves_vect.size(); ++i){
			if(leaves_vect[i]->getOcc() <= tot_occ){
				leaves_vect.insert(leaves_vect.begin()+i, new SeqHuffNode(-1, tot_occ, false, node1, node2));
				break;
			} else if(i == leaves_vect.size()-1){
				leaves_vect.insert(leaves_vect.begin()+(i+1), new SeqHuffNode(-1, tot_occ, false, node1, node2));
				break;
			}
		}

		// se sono arrivato alla fine creo la root
		if(leaves_vect.size() == 0) leaves_vect.push_back(new SeqHuffNode(-1, tot_occ, false, node1, node2));
	}
}

void depth_assign(SeqHuffNode* huff_node, DepthMap & depthmap){
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

void scanonical_codes(DepthMap & depthmap, std::vector<Triplet>& codes){
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

#endif /*HUFFMAN_UTILS_H*/