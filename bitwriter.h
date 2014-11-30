#ifndef BITWRITER_H
#define BITWRITER_H

#include <iostream>
#include <vector>
#include <cstdint>

//!  BitWriter class is used to write bit-by-bit 
/*!
  This class is responsible for the bit-by-bit writing operations during the
  compression/decompression process.
  It is designed to operate on a uint8_t vector, not a file (ofstream).*/
class BitWriter {
	//! Output vector.
    /*! Output vector used for writing operations. */
	std::vector<std::uint8_t>& _f;
	//! One byte buffer
    /*! A one byte buffer used for reading operations. */
	std::uint8_t _buf;
	//! Counter
    /*! A counter used to manage writing opreations inside the buffer. */
	std::uint8_t _count;
	//! Index
    /*! An index used to manage writing operations inside the output vector. */
	std::uint32_t _index;

	//! Write bit function
	/*!
	This function is used to write only one bit to the output vector
	\param u A uint32_t containing the bit (only the least significant bit will be considered).
	*/
	void write_bit (std::uint32_t u) {
		_buf = (_buf<<1) + (u&1);
		_count++;
		if (_count==8) {
			//! Write one byte (_buf content) into the stream
			_f.push_back(_buf);
			_index++;
			//_f.write(reinterpret_cast<const char*>(&_buf),1);
			_count = 0;
		}
	}

public:
	//! Constructor.
	/*!
	  A constructor that takes the output vector that will be used for writing oeprations.
	  This constructor also initializes other useful internal variables.
	  \param f The output vector on which the data will be written.
	*/
	BitWriter (std::vector<std::uint8_t>& f) : _f(f), _count(0), _index(0) {}

	//! Write function
	/*!
	This function writes a varible using only the number of bits specified in the parameters.
	\param u The uint32_t that will be written to the output vector.
	\param n The number of bits that will be used for the writing operation.
	\return void
	\sa BitWriter::flush()
	*/
	void write (std::uint32_t u, std::uint8_t n) {
		while (n-->0)
			write_bit(u>>n);
	}

	//! Flush function.
	/*!
	This function is used to write to the output vector the data that is still in the buffer.
	It is necessary to call thi function as the last write operation, in order to prevent the
	loss of data that would occur, caused by the loss of the buffer's content.
	\sa BitWriter::write (uint32_t u, uint8_t n)
	*/
	void flush() {
		while (_count>0)
			write_bit(0);
	}

	//! Tell index function
    /*!
	  This function returns the current position of the output vector from which the bit writer
	  is writing. It is similar to the ofstream's tellp() function.
      \return The current position inside the output vector.
    */
	std::uint32_t tell_index(){
		return _index;
	}

	//! Seek index function
    /*!
	  This function sets the position in the output vector from which the bit writer
	  will write. It is similar to the ifstream's seekp() function.
      \param idx The new index position in a uint32_t.
    */
	void seek_index(std::uint32_t idx){
		_index = idx;
	}

	//! reset index function
    /*!
	  This function resets the position in the output vector from which the bit writer
	  will write to 0. The bit writer will start writing again from the beginning of the
	  output vector
    */
	void reset_index(){
		_index = 0;
	}
};

#endif /*BITWRITER_H*/