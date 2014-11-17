#include "bitreader.h"
#include "bitwriter.h"
#include "cmd_line_interface.h"
#include "tbb/tick_count.h"

#include "par_huffman.h"
#include "seq_huffman.h"
#include <windows.h>

using namespace std;
using tbb::tick_count;

#define one_GB			1000000000
#define one_hundred_MB	100000000
#define ten_MB			10000000
#define one_MB			1000000 

int main (int argc, char *argv[]) {

	SYSTEM_INFO info_sistema;
	GetSystemInfo(&info_sistema);

	// Check number of available cores
	int numeroCPU = info_sistema.dwNumberOfProcessors;
	cout << "Total cores available: " << numeroCPU  << endl << endl;

	// Check available memory
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	cerr << "Total RAM installed: " << (float)status.ullTotalPhys/1000000 << " MB" << endl;
	cerr << "Total RAM available: " << (float)status.ullAvailPhys/1000000 << " MB" << endl;

	tick_count t0p, t1p, t0s, t1s;
	tick_count t01s, t02s, t01p, t02p;

	CMDLineInterface shell(argc, argv);

	int code = shell.verify_inputs();
	if(code < 0){
		shell.error_message(code);
		exit(1);
	}

	ParHuffman par_huff;
	SeqHuffman seq_huff;

	vector<string> input_files = shell.get_files();

	if(!shell.get_mode().compare("compression")) {
		if(shell.is_parallel()){
			// Check file dimension - chunking is needed?
			ifstream file_in(input_files[0], ifstream::in|ifstream::binary|fstream::ate);
			// NON salta i whitespaces
			file_in.unsetf (ifstream::skipws);
			// Salva la dimensione del file e torna all'inizio
			uint64_t file_len = (uint64_t) file_in.tellg();
			//file_in.close();
			// setta original filename
			par_huff._original_filename = input_files[0];
			// setto output filename
			par_huff._output_filename = par_huff._original_filename;
			par_huff._output_filename.replace(par_huff._output_filename.size()-4, 4, ".bcp");

			uint64_t MAX_LEN = ten_MB; 
			uint64_t num_macrochunks = 1;
			if(file_len > MAX_LEN) 
				num_macrochunks = 1 + (file_len-1)/ MAX_LEN;
			cerr << "num macrochunks: " << num_macrochunks << endl;
			uint64_t macrochunk_dim = file_len / num_macrochunks;

			TBBHistoReduce tbbhr;
			//ifstream file_in(input_files[0], ifstream::in|ifstream::binary);
			//file_in.unsetf (ifstream::skipws);

			for(uint64_t k=0; k < num_macrochunks; ++k) {
				par_huff.read_file(file_in, k*macrochunk_dim, macrochunk_dim);
				par_huff.create_histo(tbbhr, macrochunk_dim);
			}
			if(num_macrochunks*macrochunk_dim < file_len){ // byte avanzati
				par_huff.read_file(file_in, num_macrochunks*macrochunk_dim, file_len);
				par_huff.create_histo(tbbhr, (file_len - num_macrochunks*macrochunk_dim));
			}
			//Stampa di debug dell'istogramma complessivo
			/*for(int i=0; i<256; ++i)
			if(tbbhr._histo[i]!=0)
			cerr << "Byte: " << i << " Occ: " << tbbhr._histo[i] << endl;*/

			// Crea la mappa <simbolo, <codice, len_codice>>
			map<uint8_t, pair<uint32_t,uint32_t>> codes_map = par_huff.create_code_map(tbbhr);

			// Scrittura unica dell'header del file
			BitWriter btw = par_huff.write_header(codes_map);

			status.dwLength = sizeof(status);
			GlobalMemoryStatusEx(&status);

			cerr << endl << "Dimensione chunk: " << macrochunk_dim/1000000 << " MB" << endl;
			cerr << "Dimensione x 8 : " << (macrochunk_dim*8)/1000000 << " MB" << endl;
			uint64_t available_ram = status.ullAvailPhys;
			cerr << "RAM disponibile: " << available_ram/1000000 << " MB" << endl;

			ofstream output_file(par_huff._output_filename, fstream::out|fstream::binary);
			cerr << "Nome file di output: " << par_huff._output_filename << endl;

			// Scrittura del file compresso chunk-by-chunk
			for(uint64_t k=0; k < num_macrochunks; ++k) {
				cerr << endl << "Scrivo il macrochunk numero: " << k << endl;
				par_huff.read_file(file_in, k*macrochunk_dim, macrochunk_dim);
				par_huff.write_chunks_compressed(available_ram, macrochunk_dim, codes_map, btw);
				output_file.write(reinterpret_cast<char*>(&par_huff._file_out[0]), par_huff._file_out.size());
				par_huff._file_out.clear();
			}
			if(num_macrochunks*macrochunk_dim < file_len){ // byte avanzati
				cerr << endl << "Scrivo i byte avanzati dalla divisione in macrochunk" << endl;
				par_huff.read_file(file_in, num_macrochunks*macrochunk_dim, file_len);
				par_huff.write_chunks_compressed(available_ram, file_len-(num_macrochunks*macrochunk_dim), codes_map, btw);
			}
			btw.flush();
			if(par_huff._file_out.size() != 0)
				output_file.write(reinterpret_cast<char*>(&par_huff._file_out[0]), par_huff._file_out.size());
			output_file.close();
			file_in.close();


		} else { // compressione sequenziale
			// Comprimi con compressione sequenziale
			t0s = tick_count::now();
			seq_huff.read_file(input_files[0]);
			seq_huff.compress(input_files[0]);
			t01s = tick_count::now();
			seq_huff.write_on_file(input_files[0]);
			t02s = tick_count::now();
			cerr << "[SEQ] Il trasferimento del buffer su HDD ha impiegato " << (t02s - t01s).seconds() << " sec" << endl << endl;
			t1s = tick_count::now();
			cerr << "[SEQ] La compressione del file " << input_files[0] << " ha impiegato " << (t1s - t0s).seconds() << " sec" << endl << endl;

		}
	} else {// DECOMPRESS
		t0s = tick_count::now();

		//par_huff.read_file(input_files[0]);
		//par_huff.decompress(input_files[0]);
		//par_huff.write_on_file();

		seq_huff.read_file(input_files[0]);
		seq_huff.decompress(input_files[0]);
		seq_huff.write_on_file(input_files[0]);

		t1s = tick_count::now();
		cerr << "[SEQ] La decompressione del file " << input_files[0] << " ha impiegato " << (t1s - t0s).seconds() << " sec" << endl << endl;
	}








	//if(!shell.get_mode().compare("compression")) {

	//	if(shell.is_parallel()){
	//		for(unsigned i=0; i<input_files.size(); ++i){
	//			// --- Comprimi con compressione parallela
	//			t0p = tick_count::now();
	//			par_huff.read_file(input_files[i]);
	//			par_huff.compress(input_files[i]);
	//			t01p = tick_count::now();
	//			par_huff.write_on_file();
	//			t02p = tick_count::now();
	//			cerr << "[PAR] Il trasferimento del buffer su HDD ha impiegato " << (t02p - t01p).seconds() << " sec" << endl << endl;
	//			t1p = tick_count::now();
	//			cerr << "[PAR] La compressione del file " << input_files[i] << " ha impiegato " << (t1p - t0p).seconds() << " sec" << endl << endl;
	//		}
	//	} else {
	//		for(unsigned i=0; i<input_files.size(); ++i){
	//			// Comprimi con compressione sequenziale
	//			t0s = tick_count::now();
	//			seq_huff.read_file(input_files[i]);
	//			seq_huff.compress(input_files[i]);
	//			t01s = tick_count::now();
	//			seq_huff.write_on_file();
	//			t02s = tick_count::now();
	//			cerr << "[SEQ] Il trasferimento del buffer su HDD ha impiegato " << (t02s - t01s).seconds() << " sec" << endl << endl;
	//			t1s = tick_count::now();
	//			cerr << "[SEQ] La compressione del file " << input_files[i] << " ha impiegato " << (t1s - t0s).seconds() << " sec" << endl << endl;
	//			
	//		}
	//	}
	//		
	//}
	//else{
	//	for(unsigned i=0; i<input_files.size(); ++i){
	//		// --- Decomprimi con decompressione parallela
	//		t0p = tick_count::now();

	//		par_huff.read_file(input_files[i]);
	//		par_huff.decompress(input_files[i]);
	//		par_huff.write_on_file();

	//		t1p = tick_count::now();
	//		cerr << "[PAR] La decompressione del file " << input_files[i] << " ha impiegato " << (t1p - t0p).seconds() << " sec" << endl << endl;

	//		// --- Decomprimi con decompressione sequenziale
	//		t0s = tick_count::now();
	//		
	//		seq_huff.read_file(input_files[i]);
	//		seq_huff.decompress(input_files[i]);
	//		seq_huff.write_on_file();

	//		t1s = tick_count::now();
	//		cerr << "[SEQ] La decompressione del file " << input_files[i] << " ha impiegato " << (t1s - t0s).seconds() << " sec" << endl << endl;
	//	}
	//}


	system("pause");

	return(0);

}



