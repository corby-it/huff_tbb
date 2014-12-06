#ifndef SEQ_HUFFMAN_UTILS_H
#define SEQ_HUFFMAN_UTILS_H

#include <cstdint>
#include <vector>
#include <map>
#include <algorithm>
#include <utility> // pair
#include "tbb/tbb.h"
#include "tbb/concurrent_vector.h"
#include "seq_huffman_node.h"

// CLASSES


typedef std::vector<std::uint32_t> cont_t;
typedef cont_t::iterator iter_t;

typedef std::vector<SeqHuffNode*> LeavesVector;


// METHODS


//! Sequential creation of the huffman tree function.
/*!
A function used to create the huffman tree given the histogram.
\param histo The histogram of the input file.
\param leaves_vect The vector containing the tree's leaves.
*/
void seq_create_huffman_tree(cont_t histo, LeavesVector& leaves_vect){

	// creo un vettore che conterrà le foglie dell'albero di huffman, ciascuna con simbolo e occorrenze
	for (std::size_t i=0;i<histo.size();++i) {
		if(histo[i] > 0){
			leaves_vect.push_back(new SeqHuffNode(i,histo[i]));
		}
	}

	// ordino le foglie per occorrenze, in modo da partire da quelle con probabilità più bassa
	std::sort(leaves_vect.begin(), leaves_vect.end(), SeqHuffNode::leaves_compare);


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

//! Sequential depth assign function.
/*!
A function used to the depth values to each node of the huffman tree.
It's a recursive function that explores the tree depth-first.
\param tbb_huff_node The current node.
\param depthmap The map that will contain the depth values.
*/
void seq_depth_assign(SeqHuffNode* huff_node, DepthMap & depthmap){
	if(huff_node->isLeaf()){
		depthmap.push_back(DepthMapElement(huff_node->getDepth(), huff_node->getSymb()));
		return;
	} else {
		if(huff_node->isRoot()){
			huff_node->getLeft()->setDepth(1);
			huff_node->getRight()->setDepth(1);

			seq_depth_assign(huff_node->getLeft(), depthmap);
			seq_depth_assign(huff_node->getRight(), depthmap);
		} else {
			huff_node->getLeft()->setDepth(huff_node->getDepth());
			huff_node->getLeft()->increaseDepth(1);
			huff_node->getRight()->setDepth(huff_node->getDepth());
			huff_node->getRight()->increaseDepth(1);

			seq_depth_assign(huff_node->getLeft(), depthmap);
			seq_depth_assign(huff_node->getRight(), depthmap);
		}
	}
}


#endif /*SEQ_HUFFMAN_UTILS_H*/