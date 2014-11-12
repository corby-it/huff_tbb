#include "bitreader.h"
#include "bitwriter.h"
#include "cmd_line_interface.h"
#include "tbb/tick_count.h"

#include "par_huffman.h"
#include "seq_huffman.h"
#include <windows.h>

using namespace std;
using tbb::tick_count;

int main (int argc, char *argv[]) {

	SYSTEM_INFO info_sistema;
	GetSystemInfo(&info_sistema);

	int numeroCPU = info_sistema.dwNumberOfProcessors;
	cout << "Num cores: " << numeroCPU  << endl << endl;

	MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    cout << "Total RAM installed: " << (float)status.ullTotalPhys/1000000000 << endl;
	cout << "Total RAM available: " << (float)status.ullAvailPhys/1000000000 << endl;


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
			for(unsigned i=0; i<input_files.size(); ++i){
				// --- Comprimi con compressione parallela
				t0p = tick_count::now();
				par_huff.read_file(input_files[i]);
				par_huff.compress(input_files[i]);
				t01p = tick_count::now();
				par_huff.write_on_file();
				t02p = tick_count::now();
				cerr << "[PAR] Il trasferimento del buffer su HDD ha impiegato " << (t02p - t01p).seconds() << " sec" << endl << endl;
				t1p = tick_count::now();
				cerr << "[PAR] La compressione del file " << input_files[i] << " ha impiegato " << (t1p - t0p).seconds() << " sec" << endl << endl;
			}
		} else {
			for(unsigned i=0; i<input_files.size(); ++i){
				// Comprimi con compressione sequenziale
				t0s = tick_count::now();
				seq_huff.read_file(input_files[i]);
				seq_huff.compress(input_files[i]);
				t01s = tick_count::now();
				seq_huff.write_on_file();
				t02s = tick_count::now();
				cerr << "[SEQ] Il trasferimento del buffer su HDD ha impiegato " << (t02s - t01s).seconds() << " sec" << endl << endl;
				t1s = tick_count::now();
				cerr << "[SEQ] La compressione del file " << input_files[i] << " ha impiegato " << (t1s - t0s).seconds() << " sec" << endl << endl;
				
			}
		}
			
	}
	else{
		for(unsigned i=0; i<input_files.size(); ++i){
			// --- Decomprimi con decompressione parallela
			t0p = tick_count::now();

			par_huff.read_file(input_files[i]);
			par_huff.decompress(input_files[i]);
			par_huff.write_on_file();

			t1p = tick_count::now();
			cerr << "[PAR] La decompressione del file " << input_files[i] << " ha impiegato " << (t1p - t0p).seconds() << " sec" << endl << endl;

			// --- Decomprimi con decompressione sequenziale
			t0s = tick_count::now();
			
			seq_huff.read_file(input_files[i]);
			seq_huff.decompress(input_files[i]);
			seq_huff.write_on_file();

			t1s = tick_count::now();
			cerr << "[SEQ] La decompressione del file " << input_files[i] << " ha impiegato " << (t1s - t0s).seconds() << " sec" << endl << endl;
		}
	}


	system("pause");

	exit(0);

}



