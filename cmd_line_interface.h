#ifndef CMD_LINE_INTERFACE
#define CMD_LINE_INTERFACE

#include <string>
#include <unordered_set>
#include <tuple> 

#define ARGC_ERROR -1
#define PAR_ERROR  -2
#define FILE_ERROR -3
#define MODE_ERROR -4

//!  CMDLineInterface models the command line interface with the user.
/*!
  CMDLineInterface models the command line interface with the user.
  This class is given argc and argv from the main function, 
  and handles input validation, parsing of the arguments,
  display of error/usage messages.
  It also prepares the environment in order to call the compression algorithm
  with the right parameters.
*/
class CMDLineInterface{

private:

	//! Number of parameters given to the interface (argc-1)
	int num_par;
	//! Vector that contains all the parameters given
	std::vector<std::string> par_vector;
	//! Vector that contains all the filenames given
	std::vector<std::string> file_vector;
	//! Set of parameters allowed by the program
	std::unordered_set<std::string> allowed_parameters;

	//! Initialization of allowed parameters
	void init();
	//! Parameters and filenames are taken from argv and put in appropriate vectors
	void separate_par_from_files(int argc, char** argv);
	//! Check if all parameters are allowed
	int check_par_consistency(void);
	//! Check if all the files actually exist
	int check_file_existence(void);
	//! Print usage message
	void usage_message(void);

public:
	//! Constructor
    /*!
	  CMDLineInterface object models the command line interface with the user.
      \param argc number of parameters.
      \param argv array of parameters.
    */
	CMDLineInterface::CMDLineInterface( int argc, char** argv);

	//! Verification of input given to the interface
    /*!
	  All inputs given to the interface are verified.
	  For every parameter given, the interface checks if is allowed.
	  For every filename given, the interface checks if the file exists in the current directory.
      \param void
      \return int return code
    */
	int verify_inputs(void);

	//! Print on console error and usage messages
    /*!
	  Depending on the error code given to the function, the appropriate message is displayed on console.
      \param code an int representing the type of error
      \return void
	*/
	void error_message(int code);

	//! Ask the interface if the current mode is compression or decompression
    /*!
	  If the user gave as parameters either "-c" or "--compress", the mode is compression.
	  If the user gave as parameters either "-d" or "--decompress", the mode is decompression.
      \return std::string mode
	*/
	std::string get_mode(void);

};



#endif /*CMD_LINE_INTERFACE*/