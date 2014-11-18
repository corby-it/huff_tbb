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

	system("cls");

	SYSTEM_INFO info_sistema;
	GetSystemInfo(&info_sistema);

	// Check number of available cores
	int numeroCPU = info_sistema.dwNumberOfProcessors;
	cout << "Total cores available: " << numeroCPU  << endl;

	// Check available memory
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	cerr << "Total RAM installed: " << (float)status.ullTotalPhys/1000000 << " MB" << endl;
	cerr << "Total RAM available: " << (float)status.ullAvailPhys/1000000 << " MB" << endl << endl;

	tick_count t0p, t1p, t0s, t1s;
	tick_count t01s, t02s, t01p, t02p;

	// Check inputs from console
	CMDLineInterface shell(argc, argv);
	int code = shell.verify_inputs();
	if(code < 0){
		shell.error_message(code);
		exit(1);
	}
	// Get list of input files
	vector<string> input_files = shell.get_files();

	if(!shell.get_mode().compare("compression")) {
		if(shell.is_parallel()){

			// Object for parallel compression
			ParHuffman par_huff;

			// Check file dimension - chunking is needed?
			ifstream file_in(input_files[0], ifstream::in|ifstream::binary|fstream::ate);
			// Whitespaces are accepted
			file_in.unsetf (ifstream::skipws);
			// Check file length
			uint64_t file_len = (uint64_t) file_in.tellg();
			uint64_t MAX_LEN = ten_MB; 
			uint64_t num_macrochunks = 1;
			if(file_len > MAX_LEN) 
				num_macrochunks = 1 + (file_len-1)/ MAX_LEN;
			cerr << "Number of macrochunks: " << num_macrochunks << endl;
			uint64_t macrochunk_dim = file_len / num_macrochunks;
			cerr << "Dimension of macrochunks: " << (float)macrochunk_dim/1000000 << " MB"<< endl << endl;

			//Initialize parallel object
			par_huff.init(input_files[0]);

			TBBHistoReduce tbbhr;

			// For each macrochunk -> read and histo
			tick_count th1, th2;
			th1 = tick_count::now();
			for(uint64_t k=0; k < num_macrochunks; ++k) {
				par_huff.read_file(file_in, k*macrochunk_dim, macrochunk_dim);
				par_huff.create_histo(tbbhr, macrochunk_dim);
				cerr << "\rHistogram computation: " << ((100*k)/num_macrochunks) << "%";
			}
			th2 = tick_count::now();
			if(num_macrochunks==1) cerr << "\rHistogram computation: 100%";
			cerr << endl << "Time for all sub-histograms: " << (th2-th1).seconds() << " sec" << endl;

			// For each exceeding byte -> read and histo
			if(num_macrochunks*macrochunk_dim < file_len){ 
				par_huff.read_file(file_in, num_macrochunks*macrochunk_dim, file_len-num_macrochunks*macrochunk_dim);
				par_huff.create_histo(tbbhr, (file_len - num_macrochunks*macrochunk_dim));
			}

			// Create map <symbol, <code, len_code>>
			map<uint8_t, pair<uint32_t,uint32_t>> codes_map = par_huff.create_code_map(tbbhr);

			// Write file header
			BitWriter btw = par_huff.write_header(codes_map);

			//status.dwLength = sizeof(status);
			//GlobalMemoryStatusEx(&status);
			uint64_t available_ram = status.ullAvailPhys;

			cerr << endl << endl << "Chunk dimension: " << macrochunk_dim/1000000 << " MB" << " (x8: "<< (macrochunk_dim*8)/1000000 << " MB)" << endl;

			ofstream output_file(par_huff._output_filename, fstream::out|fstream::binary);
			cerr << "Output filename: " << par_huff._output_filename << endl;

			// Write compressed file chunk-by-chunk
			for(uint64_t k=0; k < num_macrochunks; ++k) {
				par_huff.read_file(file_in, k*macrochunk_dim, macrochunk_dim);
				par_huff.write_chunks_compressed(available_ram, macrochunk_dim, codes_map, btw);
				output_file.write(reinterpret_cast<char*>(&par_huff._file_out[0]), par_huff._file_out.size());
				par_huff._file_out.clear();
				cerr << "\rWrite compressed file: " << ((100*k)/num_macrochunks) << "%";
			}
			if(num_macrochunks==1) cerr << "\rWrite compressed file: 100%";
			// Write exceeding byte
			if(num_macrochunks*macrochunk_dim < file_len){ 
				cerr << endl << "Byte exceeding are written..." << endl;
				par_huff.read_file(file_in, num_macrochunks*macrochunk_dim, file_len-num_macrochunks*macrochunk_dim);
				par_huff.write_chunks_compressed(available_ram, file_len-(num_macrochunks*macrochunk_dim), codes_map, btw);
			}
			btw.flush();

			// Write on HDD
			if(par_huff._file_out.size() != 0)
				output_file.write(reinterpret_cast<char*>(&par_huff._file_out[0]), par_huff._file_out.size());
			output_file.close();
			file_in.close();
			cerr << endl;


		} else { 
			// Comprimi con compressione sequenziale
			SeqHuffman seq_huff;
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
		SeqHuffman seq_huff;
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



