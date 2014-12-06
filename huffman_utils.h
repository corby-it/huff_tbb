#ifndef HUFFMAN_UTILS_H
#define HUFFMAN_UTILS_H

#include <vector>
#include <utility> //pair
#include <cstdint>

// Constants
#define HUF_MAGIC_NUMBER	0x42435001

#define HUF_ONE_GB			1000000000
#define HUF_ONE_HUNDRED_MB	100000000
#define HUF_TEN_MB			10000000
#define HUF_ONE_MB			1000000
// per leggere l'header stimo che sia lungo al massimo 1 KB
// 4B per il magic number
// 4B per la lunghezza del nome del file originale
// da 0B a 500B per il nome del file vero e proprio (un po' esagerato ma fa lo stesso)
// 4B per il numero di simboli
// 512B per il massimo numero possibile di coppie <lunghezza_codice, simbolo>
#define HUF_HEADER_DIM			1024


//! Element of a DepthMap
typedef std::pair<std::uint32_t,std::uint32_t> DepthMapElement;
//! A vector used to store <length, code> pairs
typedef std::vector<std::pair<std::uint32_t,std::uint32_t>> DepthMap;


//! Triplet struct
/*!
A struct used to hold the three important values related to the a symbols in the
huffman compression process
*/
struct Triplet{
	//! The symbol itself
	std::uint8_t symbol;
	//! The code assigned to the symbol
	std::uint32_t code;
	//! The code's length
	std::uint32_t code_len;
};


//! Depth compare function.
/*!
A function used to compare elements of a depthmap. Used for sorting.
\param first The first DepthMapElement to be compared.
\param second The second DepthMapElement to be compared.
*/
static bool depth_compare(DepthMapElement first, DepthMapElement second){
	// Se le lunghezze dei simboli sono diverse, confronta quelle
	if(first.first != second.first)
		return (first.first < second.first);
	// in caso contrario confronta i simboli
	else
		return (first.second < second.second);
}


//! Canonical codes function.
/*!
A function used to compute huffman's canonical codes.
\param depthmap The input depthmap, a structure containing the symbols and their depth in the tree.
\param codes The output vector containing the resulting triplets.
*/
static void canonical_codes(DepthMap & depthmap, std::vector<Triplet>& codes){
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

#endif //HUFFMAN_UTILS_H