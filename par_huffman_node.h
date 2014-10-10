#ifndef PAR_HUFFMAN_NODE
#define PAR_HUFFMAN_NODE

#include "tbb\tbb.h"
#include "tbb/concurrent_vector.h"

class ParHuffNode {
private:
	tbb::atomic<int> _symb;
	tbb::atomic<unsigned> _occ;
	tbb::atomic<unsigned> _depth;
	// sarebbe meglio non usare qui i puntatori ma le reference o cose del genere credo... però per il momento va bene così
	ParHuffNode *_left;
	ParHuffNode *_right;
	bool _isLeaf;
	bool _isRoot;

public:
	ParHuffNode ();
	ParHuffNode (tbb::atomic<int> symb, tbb::atomic<unsigned> occ, bool is_leaf, ParHuffNode *left, ParHuffNode *right) : _symb(symb), _occ(occ), _left(left), _right(right), _isLeaf(is_leaf), _isRoot(false){_depth=0;}
	ParHuffNode (tbb::atomic<int> symb, tbb::atomic<unsigned> occ) : _symb(symb), _occ(occ), _left(NULL), _right(NULL), _isLeaf(true), _isRoot(false){_depth=0;}

	void increaseDepth(unsigned i);

	// funzione usata dal sort (static deve stare nell'header)
	static bool leaves_compare(ParHuffNode* first, ParHuffNode* second){
	return (first->getOcc() > second->getOcc());
}

	// Getters and setters
	int getSymb ();
	void setSymb (int symb);

	unsigned getOcc ();
	void setOcc (unsigned occ);

	unsigned getDepth ();
	void setDepth (unsigned depth);

	ParHuffNode* getLeft ();
	void setLeft (ParHuffNode* left);

	ParHuffNode* getRight ();
	void setRight (ParHuffNode* right);

	bool isLeaf ();
	void setLeaf (bool leaf);

	bool isRoot ();
	void setRoot (bool root);
};


#endif /*PAR_HUFFMAN_NODE*/