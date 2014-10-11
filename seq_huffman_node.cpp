#include "seq_huffman_node.h"
#include <iostream>

void SeqHuffNode::increaseDepth(unsigned i) { 
	_depth += i; 
}

// funzione utile per debug
	void SeqHuffNode::to_string(){
		std::cerr << "Symb: " << getSymb() << "\tOcc: " << getOcc() << std::endl;
	}

	// Getters and setters
	int SeqHuffNode::getSymb (){ return _symb; }
	void SeqHuffNode::setSymb (int symb){ _symb = symb; }

	unsigned SeqHuffNode::getOcc (){ return _occ; }
	void SeqHuffNode::setOcc (unsigned occ){ _occ = occ; }

	unsigned SeqHuffNode::getDepth (){ return _depth; }
	void SeqHuffNode::setDepth (unsigned depth){ _depth = depth; }

	SeqHuffNode* SeqHuffNode::getLeft (){ return _left; }
	void SeqHuffNode::setLeft (SeqHuffNode* left){ _left = left; }

	SeqHuffNode* SeqHuffNode::getRight (){ return _right; }
	void SeqHuffNode::setRight (SeqHuffNode* right){ _right = right; }

	bool SeqHuffNode::isLeaf (){ return _isLeaf; }
	void SeqHuffNode::setLeaf (bool leaf){ _isLeaf = leaf; }

	bool SeqHuffNode::isRoot (){ return _isRoot; }
	void SeqHuffNode::setRoot (bool root){ _isRoot = root; }

