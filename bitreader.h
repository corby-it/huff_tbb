#ifndef BITREADER_H
#define BITREADER_H

#include <iostream>
#include <vector>
#include <cstdint>

//!  BitReader class is used to read bit-by-bit
/*!
  A more elaborate class description.
*/
class BitReader {
	//! istream.
    /*! More detailed istream description. */
	std::vector<std::uint8_t>& _f;
	//! An uint8_t.
    /*! More detailed uint8_t description. */
	std::uint8_t _buf;
	//! An uint8_t.
    /*! More detailed uint8_t description. */
	std::uint8_t _count;
	//! An uint8_t.
    /*! More detailed uint8_t description. */
	std::uint32_t _index;



public:
	//! A constructor.
    /*!
      A more elaborate description of the constructor.
    */
	BitReader (std::vector<std::uint8_t>& f) : _f(f), _count(0), _index(0) {}

	//! read function description
    /*!
      \param n an unsigned 8 bit integer
      \return unsigned 32 bit integer
    */
	std::uint32_t read (std::uint32_t n) {
		std::uint32_t u = 0;
		while (n-->0)
			u = (u<<1) | read_bit();
		return u;
	}

	//! read_bit function description
    /*!
      \param void
      \return unsigned 32 bit integer
    */
	std::uint32_t read_bit() {
		if (_count==0) {
			//! Extract one byte from the stream, stores it in _buf
			_buf = _f[_index];
			_index++;
			//_f.read(reinterpret_cast<char*>(&_buf),1);
			_count = 8;
		}
		return (_buf>>--_count)&1;
	}

	std::vector<std::uint8_t> read_n_bytes( std::uint8_t n){
		std::vector<std::uint8_t> tmp;
		
		tmp.insert(tmp.begin(), _f.begin()+_index, _f.begin()+_index+n); 
		_index += n;
		return tmp;
	}

	bool good(){ 
		return (_index < _f.size());
	}

	std::uint32_t tell_index(){
		return _index;
	}

	void seek_index(std::uint32_t idx){
		_index = idx;
	}

	void reset_index(){
		_index = 0;
	}
};

#endif /* BITREADER_H */