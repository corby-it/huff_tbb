#ifndef PAR_HUFFMAN_NODE
#define PAR_HUFFMAN_NODE

#include "tbb/tbb.h"
#include "tbb/concurrent_vector.h"

//! ParHuffNode class, huffman tree node.
/*!
This class represents a node in the huffman binary tree creation process.
It uses TBB's parallel functions.
*/
class ParHuffNode {
private:
	//! The symbol represented by the node.
	tbb::atomic<int> _symb;
	//! The total occurences of the symbol in the input file (intended as its probability).
	tbb::atomic<unsigned> _occ;
	//! The node's depth in the huffman tree.
	tbb::atomic<unsigned> _depth;
	//! The node's left child
	ParHuffNode *_left;
	//! The node's right child
	ParHuffNode *_right;
	//! A boolean, true if the node is a leaf. 
	bool _isLeaf;
	//! A boolean, true if the node is the root of the tree. 
	bool _isRoot;

public:

	//! Empty Constructor.
	/*!
	Creates and empty huffman tree node.
	*/
	ParHuffNode ();

	//! Constructor with fields.
	/*!
	Creates an huffman tree node given the parameters
	\param symb The symbol represented by the node.
	\param occ The number of occurences of the symbol (intended as its probability).
	\param is_leaf A boolean used to represent a leaf in the huffman tree.
	\param left A pointer to its left child
	\param right A pointer to its rigth child
	*/
	ParHuffNode (tbb::atomic<int> symb, tbb::atomic<unsigned> occ, bool is_leaf, ParHuffNode *left, ParHuffNode *right) : _symb(symb), _occ(occ), _left(left), _right(right), _isLeaf(is_leaf), _isRoot(false){_depth=0;}

	//! Constructor with fields.
	/*!
	Creates an huffman tree node given SOME of the parameters.
	The other parameters are initialized with default values.
	\param symb The symbol represented by the node.
	\param occ The number of occurences of the symbol (intended as its probability).
	*/
	ParHuffNode (tbb::atomic<int> symb, tbb::atomic<unsigned> occ) : _symb(symb), _occ(occ), _left(NULL), _right(NULL), _isLeaf(true), _isRoot(false){_depth=0;}

	//! Increase depth function.
	/*!
	Increases the depth value by i.
	\param i The increase value.
	*/
	void increaseDepth(unsigned i);

	//! Leaves compare function.
	/*!
	A compare function used for sorting. The comparison is based on the number of occurences (the symbol's probability)
	\param first The first node to be compared.
	\param second The second node to be compared.
	*/
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