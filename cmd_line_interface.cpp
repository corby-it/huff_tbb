#include <array>
#include <iostream>

#include "dirent.h"
#include "dirent.cpp"
#include "cmd_line_interface.h"

using namespace std;

// COSTRUTTORE
CMDLineInterface::CMDLineInterface( int argc, char** argv){
	init();
	separate_par_from_files(argc, argv);
}



int CMDLineInterface::verify_inputs() {

	if( (check_par_consistency()) < 0)
		return check_par_consistency();

	else if (check_file_existence() < 0)
		return check_file_existence();

	return 1;
}



// Inizializza la lista di parametri consentiti
void CMDLineInterface::init(){
	array<string,10> myarray = {"-c","--compress", "-d", "--decompress", "-p", "--parallel",
		"-t", "--timer", 	"-v", "--verbose"};
	allowed_parameters.insert(myarray.begin(), myarray.end());
}



// Stampa su console la sintassi del programma
void CMDLineInterface::error_message(int code){
	switch (code) {
	case ARGC_ERROR:
		cout << "Error: parameters and files needed!" << endl;
	case PAR_ERROR:
		cout << "Looks like you made some syntax error." << endl;
		cout << "	Use: huffman_tbb.exe <mode> [options] <file>" << endl;
		cout << "	<mode>: -c (--compress), -d (--decompress)" << endl;
		cout << "	[options]: -p (--parallel), -t (--timer), -v (--verbose)" << endl;
		cout << "	<file>: filename1 filename2 ... filenameN" << endl;
		break;
	case FILE_ERROR:
		cout << "Looks like you gave in input some non-existent file." << endl;
		cout << "Please check you input files again!" << endl;
		break;
	default:
		break;
	}
}



// Check parameters consistency
int CMDLineInterface::check_par_consistency() {

	// Check if some parameter is provided
	if(!par_vector.size())
		return ARGC_ERROR;

	// Check if all the parameters provided are allowed
	for (vector<string>::iterator it = par_vector.begin(); it != par_vector.end(); ++it)
		if(allowed_parameters.find (*it)==allowed_parameters.end())
			return PAR_ERROR;

	return 1;
}



// Check that all input files actually exist
int CMDLineInterface::check_file_existence() {

	// At least one file is needed
	if(!file_vector.size())
		return ARGC_ERROR;

	unordered_set<string> files_listed;
	
	char* dirname = ".";
	DIR *dir;
    struct dirent *ent;
                
    /* Open directory stream */
    dir = opendir (dirname);
    if (dir != NULL) {

        /* List all files within the directory */
        while ((ent = readdir (dir)) != NULL) {
            switch (ent->d_type) {
            case DT_REG:
				files_listed.insert(ent->d_name);
                break;
            default: 
				;  
            }
        }

        closedir (dir);

    } else {
        /* Could not open directory */
        printf ("Cannot open directory %s\n", dirname);
        exit (EXIT_FAILURE);
    }

	// Control that all files in the file vector actually exist in the current directory
	for (vector<string>::iterator it = file_vector.begin(); it != file_vector.end(); ++it)
		if(files_listed.find (*it)==files_listed.end())
			return FILE_ERROR;
	
	return 1;
}



// Separa i parametri dai file in input (supponendo non ci siano file che iniziano con '-')
void CMDLineInterface::separate_par_from_files(int argc, char** argv){
	num_par = argc-1;
	for(int i=1; i<= num_par; ++i){
		if(string(argv[i]).at(0)=='-')
			par_vector.push_back(argv[i]);
		else
			file_vector.push_back(argv[i]);
	}
}

