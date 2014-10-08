#ifndef HUFFMAN_ENGINE
#define HUFFMAN_ENGINE

#include <string>

class HuffmanEngine {

private:



public:
	HuffmanEngine();
	void compress(std::string in_file);
	void decompress(std::string in_file, std::string out_file);

	void compress_p(std::string in_file);

};




#endif /*HUFFMAN_ENGINE*/