#ifndef CMD_LINE_INTERFACE
#define CMD_LINE_INTERFACE

#include <string>
#include <unordered_set>
#include <tuple> 

#define ARGC_ERROR -1
#define PAR_ERROR  -2
#define FILE_ERROR -3

class CMDLineInterface{

private:

	int num_par;
	std::vector<std::string> par_vector;
	std::vector<std::string> file_vector;
	std::unordered_set<std::string> allowed_parameters;

	void init();
	void separate_par_from_files(int argc, char** argv);
	int check_par_consistency();
	int check_file_existence();

public:

	CMDLineInterface::CMDLineInterface( int argc, char** argv);
	int verify_inputs();
	void error_message(int code);

};



#endif /*CMD_LINE_INTERFACE*/