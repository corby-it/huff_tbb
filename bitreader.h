#ifndef BITREADER_H
#define BITREADER_H

#include <iostream>
#include <cstdint>

//!  BitReader class is used to read bit-by-bit
/*!
  A more elaborate class description.
*/
class BitReader {
	//! istream.
    /*! More detailed istream description. */
	std::istream& _f;
	//! An uint8_t.
    /*! More detailed uint8_t description. */
	std::uint8_t _buf;
	//! An uint8_t.
    /*! More detailed uint8_t description. */
	std::uint8_t _count;



public:
	//! A constructor.
    /*!
      A more elaborate description of the constructor.
    */
	BitReader (std::istream& f) : _f(f),_count(0) {}

	//! read function description
    /*!
      \param n an unsigned 8 bit integer
      \return unsigned 32 bit integer
    */
	std::uint32_t read (std::uint8_t n) {
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
			_f.read(reinterpret_cast<char*>(&_buf),1);
			_count = 8;
		}
		return (_buf>>--_count)&1;
	}
};

#endif /* BITREADER_H */