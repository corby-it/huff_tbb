#include "bitreader.h"
#include "bitwriter.h"
#include "cmd_line_interface.h"
//#include "huffman_engine.h"
#include "tbb/tick_count.h"
#include "par_huffman.h"
#include "seq_huffman.h"

using namespace std;
using tbb::tick_count;

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

	tick_count t0p, t1p, t0s, t1s;

	CMDLineInterface shell(argc, argv);

	int code = shell.verify_inputs();
	if(code < 0){
		shell.error_message(code);
		exit(1);
	}

	/*HuffmanEngine huff;*/
	ParHuffman par_huff;
	SeqHuffman seq_huff;

	if(!shell.get_mode().compare("compression")) {

		vector<string> input_files = shell.get_files();
		for(unsigned i=0; i<input_files.size(); ++i){

			//// --- Comprimi con compressione parallela
			t0p = tick_count::now();
			par_huff.read_file(input_files[i]);
			par_huff.compress(input_files[i]);
			t1p = tick_count::now();
			cerr << "[PAR] La compressione del file " << input_files[i] << " ha impiegato " << (t1p - t0p).seconds() << " sec" << endl << endl;

			// --- Comprimi con compressione sequenziale
			t0s = tick_count::now();
			seq_huff.read_file(input_files[i]);
			seq_huff.compress(input_files[i]);
			t1s = tick_count::now();
			cerr << "[SEQ] La compressione del file " << input_files[i] << " ha impiegato " << (t1s - t0s).seconds() << " sec" << endl << endl;

		}
	}
	else{
		cout << "DECOMPRESS!" << endl;
		par_huff.decompress("prova.bcp", "prova.txt");
		seq_huff.decompress("prova.bcp", "prova.txt");
	}


	system("pause");

	exit(0);

}



