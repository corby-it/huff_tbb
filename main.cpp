#include "bitreader.h"
#include "bitwriter.h"
#include "cmd_line_interface.h"
#include "huffman_engine.h"

using namespace std;


int main (int argc, char *argv[]) {


	/*
	Come provare se funziona:
		Compressione -> huffman_tbb.exe -c prova.txt
						oppure huffman_tbb.exe --compress prova.txt
		Decompressione -> huffman_tbb.exe -d prova.bcp
						oppure huffman_tbb.exe --decompress prova.bcp,

	In teoria dovrebbe dare errore se si sbagliano i parametri, se il file di input non esiste,
	se ci sono troppi pochi argomenti ecc ecc 

	I file di prova sono in Debug, quindi per provare se va bisogna entrare in Debug
	*/
	CMDLineInterface shell(argc, argv);


	int code = shell.verify_inputs();
	if(code < 0){
		shell.error_message(code);
		exit(1);
	}

	HuffmanEngine huff;

	if(!shell.get_mode().compare("compression")) {

		vector<string> input_files = shell.get_files();
		for(unsigned i=0; i<input_files.size(); ++i)
			huff.compress(input_files[i]);
	}
	else
		//cout << "DECOMPRESS!" << endl;
		huff.decompress("prova.bcp", "prova.txt");


	exit(0);


}



