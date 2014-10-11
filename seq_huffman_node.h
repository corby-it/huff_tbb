#ifndef SEQ_HUFFMAN_NODE
#define SEQ_HUFFMAN_NODE

#include "tbb/tbb.h"
#include "tbb/concurrent_vector.h"

class SeqHuffNode {
private:
	int _symb;
	unsigned _occ;
	unsigned _depth;
	// sarebbe meglio non usare qui i puntatori ma le reference o cose del genere credo... però per il momento va bene così
	SeqHuffNode *_left;
	SeqHuffNode *_right;
	bool _isLeaf;
	bool _isRoot;

public:
	SeqHuffNode ();
	SeqHuffNode (int symb, unsigned occ, bool is_leaf, SeqHuffNode *left, SeqHuffNode *right) : _symb(symb), _occ(occ), _left(left), _right(right), _isLeaf(is_leaf), _isRoot(false), _depth(0) {}
	SeqHuffNode (int symb, unsigned occ) : _symb(symb), _occ(occ), _left(NULL), _right(NULL), _isLeaf(true), _isRoot(false), _depth(0){}

	void increaseDepth(unsigned i);

	// funzione usata dal sort
	static bool leaves_compare(SeqHuffNode* first, SeqHuffNode* second){
		return (first->getOcc() > second->getOcc());
	}

	// funzione utile per debug
	void to_string();

	// Getters and setters
	int getSymb ();
	void setSymb (int symb);

	unsigned getOcc ();
	void setOcc (unsigned occ);

	unsigned getDepth ();
	void setDepth (unsigned depth);

	SeqHuffNode* getLeft ();
	void setLeft (SeqHuffNode* left);

	SeqHuffNode* getRight ();
	void setRight (SeqHuffNode* right);

	bool isLeaf ();
	void setLeaf (bool leaf);

	bool isRoot ();
	void setRoot (bool root);
};

#endif /*SEQ_HUFFMAN_NODE*/