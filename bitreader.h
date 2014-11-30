#ifndef BITREADER_H
#define BITREADER_H

#include <iostream>
#include <vector>
#include <cstdint>

//!  BitReader class is used to read bit-by-bit
/*!
  This class is responsible for the bit-by-bit reading operations during the
  compression/decompression process.
  It is designed to operate on a uint8_t vector, not a file (ifstream).
*/
class BitReader {
	//! Input vector.
    /*! Input vector used for reading operations. */
	std::vector<std::uint8_t>& _f;
	//! One byte buffer
    /*! A one byte buffer used for reading operations. */
	std::uint8_t _buf;
	//! Counter
    /*! A counter used to manage reading opreations inside the buffer. */
	std::uint8_t _count;
	//! Index
    /*! An index used to manage reading operations inside the input vector. */
	std::uint32_t _index;



public:
	//! Constructor.
    /*!
      A constructor that takes the input vector that will be used for reading oeprations.
	  This constructor also initializes other useful internal variables.
	  \param f The input vector from which the data will be read.
    */
	BitReader (std::vector<std::uint8_t>& f) : _f(f), _count(0), _index(0) {}

	//! Read bits function
    /*!
	  This function allows the user to read a desired amount of bits from the input vector.
	  The return value is of uint32_t type, so it's not possible to read more than 32 bits at a time. 
      \param n The number of bits to be read.
      \return The n-bits read from the input vector.
    */
	std::uint32_t read (std::uint32_t n) {
		std::uint32_t u = 0;
		while (n-->0)
			u = (u<<1) | read_bit();
		return u;
	}

	//! Read bit function
    /*!
	  This function has no parameters because it reads only one bit from the input vector.
      \return A uin32_t containing the bit read from the input vector.
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

	//! Read bytes function
    /*!
	  This function allows the user to read a desired amount of bytes from the input vector.
	  The result is given in another vector<uint8_t>.
      \param n The number of bytes to be read.
      \return The vector<uint8_t> containing the n-bytes reda from the input vector.
    */
	std::vector<std::uint8_t> read_n_bytes( std::uint8_t n){
		std::vector<std::uint8_t> tmp;
		
		tmp.insert(tmp.begin(), _f.begin()+_index, _f.begin()+_index+n); 
		_index += n;
		return tmp;
	}

	//! Is good function
    /*!
	  This function tells if the bit reader is still good for reading data. 
	  It returns true if there is more data to be read, false if it is pointing to the
	  last element in the input vector.
      \return A boolean telling if the bit reader is good or not.
    */
	bool good(){ 
		return (_index < _f.size());
	}

	//! Tell index function
    /*!
	  This function returns the current position of the input vector from which the bit reader
	  is reading. It is similar to the ifstream's tellg() function.
      \return The current position inside the input vector.
    */
	std::uint32_t tell_index(){
		return _index;
	}

	//! Seek index function
    /*!
	  This function sets the position in the input vector from which the bit reader
	  will read. It is similar to the ifstream's seekg() function.
      \param idx The new index position in a uint32_t.
    */
	void seek_index(std::uint32_t idx){
		_index = idx;
	}

	//! reset index function
    /*!
	  This function resets the position in the input vector from which the bit reader
	  will read to 0. The bit reader will start reading again from the beginning of the
	  input vector
    */
	void reset_index(){
		_index = 0;
	}
};

#endif /* BITREADER_H */