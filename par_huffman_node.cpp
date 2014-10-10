#include "par_huffman_node.h"

void ParHuffNode::increaseDepth(unsigned i) { 
	_depth += i; 
}

//Getter and Setter
int ParHuffNode::getSymb (){ return _symb; }
void ParHuffNode::setSymb (int symb){ _symb = symb; }

unsigned ParHuffNode::getOcc (){ return _occ; }
void ParHuffNode::setOcc (unsigned occ){ _occ = occ; }

unsigned ParHuffNode::getDepth (){ return _depth; }
void ParHuffNode::setDepth (unsigned depth){ _depth = depth; }

ParHuffNode* ParHuffNode::getLeft (){ return _left; }
void ParHuffNode::setLeft (ParHuffNode* left){ _left = left; }

ParHuffNode* ParHuffNode::getRight (){ return _right; }
void ParHuffNode::setRight (ParHuffNode* right){ _right = right; }

bool ParHuffNode::isLeaf (){ return _isLeaf; }
void ParHuffNode::setLeaf (bool leaf){ _isLeaf = leaf; }

bool ParHuffNode::isRoot (){ return _isRoot; }
void ParHuffNode::setRoot (bool root){ _isRoot = root; }