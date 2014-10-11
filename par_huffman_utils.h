#ifndef PAR_HUFFMAN_UTILS
#define PAR_HUFFMAN_UTILS

#include <cstdint>
#include <vector>
#include <map>
#include <algorithm> //std::sort
#include "tbb/tbb.h"
#include "tbb/concurrent_vector.h"
#include "par_huffman_node.h"

//---------------------------------------------------------------------------------------------
// ----------- DEFINITIONS AND METHODS FOR PARALLEL EXECUTION----------------------------------
//---------------------------------------------------------------------------------------------
typedef std::vector<std::pair<unsigned,std::uint8_t>> DepthMap;
typedef std::pair<unsigned,std::uint8_t> DepthMapElement;
typedef std::map<std::uint8_t, std::pair<std::uint32_t,std::uint32_t>> CodesMap;
typedef std::pair<std::uint8_t, std::pair<std::uint32_t,std::uint32_t>> CodesMapElement;
typedef std::uint8_t CodesMapKey;
typedef std::pair<std::uint32_t,std::uint32_t> CodesMapValue;

struct Triplet{
	std::uint8_t symbol;
	std::uint32_t code;
	std::uint32_t code_len;
};

bool depth_compare(DepthMapElement first, DepthMapElement second){
	return (first.first < second.first);
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

typedef std::vector<tbb::atomic<std::uint32_t>> TBBHisto; // futuro: provare concurrent_vector
typedef tbb::concurrent_vector<ParHuffNode*> TBBLeavesVector;


void create_huffman_tree_p(TBBHisto histo, TBBLeavesVector& leaves_vect){

	// creo un vettore che conterr� le foglie dell'albero di huffman, ciascuna con simbolo e occorrenze
	parallel_for(tbb::blocked_range<int>( 0, histo.size()), [&](const tbb::blocked_range<int>& range) {
		for( int i=range.begin(); i!=range.end(); ++i ){
			if(histo[i] > 0){
				tbb::atomic<int> j;
				j = i;
				leaves_vect.push_back(new ParHuffNode(j,histo[i]));
			}
		}
	});

	// ordino le foglie per occorrenze, in modo da partire da quelle con probabilit� pi� bassa
	std::sort(leaves_vect.begin(), leaves_vect.end(), ParHuffNode::leaves_compare);

	//std::cerr << "Arrivo fino a qui: punto 2" << std::endl;

	/* Dal momento che i concurrent vector non supportano pop_back() n� insert,
	copio i dati in un vector per il pezzo critico, poi li rimetto nel concurrent_vector
	NB: vec � un normalissimo std::vector											*/
	std::vector<ParHuffNode*> vec;
	vec.assign(leaves_vect.begin(), leaves_vect.end());

	// Creazione dell'albero utilizzando std::vector anzich� tbb::concurrent_vector
	while(vec.size() > 1){
		// scelgo i due nodi con probabilit� minore
		ParHuffNode* node1;
		ParHuffNode* node2;
		node1 = vec.back();
		
		vec.pop_back(); 
		node2 = vec.back();
		vec.pop_back(); 

		unsigned tot_occ = node1->getOcc() + node2->getOcc();

		// Il template tbb::atomic non ha un costruttore in fase di dichiarazione
		// Occorre prima dichiarare, poi inizializzare (vedi anche sotto)
		tbb::atomic<unsigned> atomic_tot_occ;
		atomic_tot_occ = tot_occ;

		tbb::atomic<int> j;
		j = -1;

		// inserisco il nodo padre nella posizione giusta in base alle probabilit�
		for(unsigned i=0; i<vec.size(); ++i){

			if(vec[i]->getOcc() <= tot_occ){
				vec.insert(vec.begin()+i, new ParHuffNode(j, atomic_tot_occ, false, node1, node2));
				break;
			} else if(i == vec.size()-1){
				vec.insert(vec.begin()+(i+1), new ParHuffNode(j, atomic_tot_occ, false, node1, node2)); 
				break;
			}
		}

		// se sono arrivato alla fine creo la root
		if(vec.size() == 0)	vec.push_back(new ParHuffNode(j, atomic_tot_occ, false, node1, node2));
	}
	// Creato l'albero, rimetto i valori nel concurrent_vector originario
	leaves_vect.clear();
	leaves_vect.assign(vec.begin(), vec.end());

	//std::cerr << "Arrivo fino a qui: punto 3" << std::endl;
}

void depth_assign_p(ParHuffNode* tbb_huff_node, DepthMap & depthmap){
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

#endif /*PAR_HUFFMAN_UTILS*/