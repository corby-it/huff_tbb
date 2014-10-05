#ifndef BITWRITER_H
#define BITWRITER_H

#include <iostream>
#include <cstdint>

using namespace std;

//!  BitWriter class is used to write bit-by-bit 
/*!
  A more elaborate class description.
*/
class BitWriter {
	//! ostream.
    /*! More detailed ostream description. */
	ostream& _f;
	//! uint8_t.
    /*! More detailed uint8_t description. */
	uint8_t _buf;
	//! uint8_t.
    /*! More detailed uint8_t description. */
	uint8_t _count;

    //! write_bit function description
    /*!
      \param u an unsigned 32 bit integer
      \return void
    */
	void write_bit (uint32_t u) {
		_buf = (_buf<<1) + (u&1);
		_count++;
		if (_count==8) {
			//! Write one byte (_buf content) into the stream
			_f.write(reinterpret_cast<const char*>(&_buf),1);
			_count = 0;
		}
	}

public:
	//! A constructor.
    /*!
      A more elaborate description of the constructor.
    */
	BitWriter (ostream& f) : _f(f),_count(0) {}

	//! write function description
    /*!
      \param u an unsigned 32 bit integer.
      \param n an unsigned 8 bit integer.
      \return void
      \sa BitWriter::flush()
    */
	void write (uint32_t u, uint8_t n) {
		while (n-->0)
			write_bit(u>>n);
	}

	//! flush function description.
    /*!
      \param void
      \return void
      \sa BitWriter::write (uint32_t u, uint8_t n)
    */
	void flush() {
		while (_count>0)
			write_bit(0);
	}
};

#endif /*BITWRITER_H*/