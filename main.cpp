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

	system("cls");

	SYSTEM_INFO info_sistema;
	GetSystemInfo(&info_sistema);

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
		for(int num_files=0;num_files < input_files.size();++num_files){

			if(shell.is_parallel()){ //PARALLEL COMPRESSION

				cout << "Parallel Compressing " << input_files[num_files] << "..." << endl;

				ParHuffman par_huff;
				par_huff.compress_chunked(input_files[num_files]);
				
			} else { //SEQUENTIAL COMPRESSION

				cout << "Sequential Compressing " << input_files[num_files] << "..." << endl;

				SeqHuffman seq_huff;
				seq_huff.compress_chunked(input_files[num_files]);
			}
		}
	} else {// DECOMPRESS
		SeqHuffman seq_huff;

		//seq_huff.read_file(input_files[0]);
		//seq_huff.decompress(input_files[0]);
		//seq_huff.write_on_file(input_files[0]);

		seq_huff.decompress_chunked(input_files[0]);

	}

	system("pause");

	return(0);

}



