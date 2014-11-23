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

				// Utility
				tick_count tt1, tt2;
				tt1 = tick_count::now();

				// Check for chunking 
				ifstream file_in(input_files[num_files], ifstream::in|ifstream::binary|fstream::ate);
				// Whitespaces are accepted
				file_in.unsetf (ifstream::skipws);
				// Check file length
				uint64_t file_len = (uint64_t) file_in.tellg();
				uint64_t MAX_LEN = ten_MB; 
				uint64_t num_macrochunks = 1;
				if(file_len > MAX_LEN) 
					num_macrochunks = 1 + (file_len-1)/ MAX_LEN;
				uint64_t macrochunk_dim = file_len / num_macrochunks;

				// Object for parallel compression
				ParHuffman par_huff;

				//Initialize parallel object
				par_huff.init(input_files[num_files]);

				// Global histogram
				TBBHistoReduce tbbhr;

				// For each macrochunk -> read and histo
				tick_count th1, th2;
				th1 = tick_count::now();
				for(uint64_t k=0; k < num_macrochunks; ++k) {
					par_huff.read_file(file_in, k*macrochunk_dim, macrochunk_dim);
					par_huff.create_histo(tbbhr, macrochunk_dim);
					cerr << "\rHuffman computation: " << ((100*(k+1))/num_macrochunks) << "%";
				}
				th2 = tick_count::now();
				if(num_macrochunks==1) cerr << "\rHuffman computation: 100%";
				//cerr << endl << "Time for all sub-histograms: " << (th2-th1).seconds() << " sec" << endl;

				// For each exceeding byte -> read and histo
				if(num_macrochunks*macrochunk_dim < file_len){ 
					par_huff.read_file(file_in, num_macrochunks*macrochunk_dim, file_len-num_macrochunks*macrochunk_dim);
					par_huff.create_histo(tbbhr, (file_len - num_macrochunks*macrochunk_dim));
				}

				// Create map <symbol, <code, len_code>>
				map<uint8_t, pair<uint32_t,uint32_t>> codes_map = par_huff.create_code_map(tbbhr);

				// Write file header
				BitWriter btw = par_huff.write_header(codes_map);

				MEMORYSTATUSEX status;
				status.dwLength = sizeof(status);
				GlobalMemoryStatusEx(&status);
				uint64_t available_ram = status.ullAvailPhys;

				ofstream output_file(par_huff._output_filename, fstream::out|fstream::binary);
				cerr << endl << "Output filename: " << par_huff._output_filename << endl;

				// Write compressed file chunk-by-chunk
				tick_count tw1, tw2;
				tw1 = tick_count::now();
				for(uint64_t k=0; k < num_macrochunks; ++k) {
					par_huff.read_file(file_in, k*macrochunk_dim, macrochunk_dim);
					par_huff.write_chunks_compressed(available_ram, macrochunk_dim, codes_map, btw);
					output_file.write(reinterpret_cast<char*>(&par_huff._file_out[0]), par_huff._file_out.size());
					par_huff._file_out.clear();
					cerr << "\rWrite compressed file: " << ((100*(k+1))/num_macrochunks) << "%";
				}
				if(num_macrochunks==1) cerr << "\rWrite compressed file: 100%";
				// Write exceeding byte
				if(num_macrochunks*macrochunk_dim < file_len){ 
					par_huff.read_file(file_in, num_macrochunks*macrochunk_dim, file_len-num_macrochunks*macrochunk_dim);
					par_huff.write_chunks_compressed(available_ram, file_len-(num_macrochunks*macrochunk_dim), codes_map, btw);
					output_file.write(reinterpret_cast<char*>(&par_huff._file_out[0]), par_huff._file_out.size());
					par_huff._file_out.clear();
				}
				btw.flush();
				tw2 = tick_count::now();
				//cerr << endl << "Time for all writing (buffer): " << (tw2-tw1).seconds() << " sec" << endl;

				// Write on HDD
				tick_count twhd1, twhd2;
				twhd1 = tick_count::now();
				if(par_huff._file_out.size() != 0)
					output_file.write(reinterpret_cast<char*>(&par_huff._file_out[0]), par_huff._file_out.size());
				output_file.close();
				file_in.close();
				twhd2 = tick_count::now();
				//cerr << "Time for all writing (Hard Disk): " << (twhd2-twhd1).seconds() << " sec" << endl;
				cerr << endl;

				tt2 = tick_count::now();
				cerr <<  "Total time for compression: " << (tt2-tt1).seconds() << " sec" << endl << endl;

			} else { //SEQUENTIAL COMPRESSION

				cout << "Sequential Compressing " << input_files[num_files] << "..." << endl;

				// Utility
				tick_count tt1, tt2;
				tt1 = tick_count::now();

				// Check for chunking 
				ifstream file_in(input_files[num_files], ifstream::in|ifstream::binary|fstream::ate);
				// Whitespaces are accepted
				file_in.unsetf (ifstream::skipws);
				// Check file length
				uint64_t file_len = (uint64_t) file_in.tellg();
				uint64_t MAX_LEN = ten_MB; 
				uint64_t num_macrochunks = 1;
				if(file_len > MAX_LEN) 
					num_macrochunks = 1 + (file_len-1)/ MAX_LEN;
				uint64_t macrochunk_dim = file_len / num_macrochunks;

				// Object for sequential compression
				SeqHuffman seq_huff;

				//Initialize sequential object
				seq_huff.init(input_files[num_files]);

				// Global histogram
				vector<uint32_t> histo(256);

				// For each macrochunk -> read and histo
				tick_count th1, th2;
				th1 = tick_count::now();
				for(uint64_t k=0; k < num_macrochunks; ++k) {
					seq_huff.read_file(file_in, k*macrochunk_dim, macrochunk_dim);
					seq_huff.create_histo(histo, macrochunk_dim);
					cerr << "\rHuffman computation: " << ((100*(k+1))/num_macrochunks) << "%";
				}
				th2 = tick_count::now();
				if(num_macrochunks==1) cerr << "\rHistogram computation: 100%";
				//cerr << endl << "Time for all sub-histograms: " << (th2-th1).seconds() << " sec" << endl << endl;

				// For each exceeding byte -> read and histo
				if(num_macrochunks*macrochunk_dim < file_len){ 
					seq_huff.read_file(file_in, num_macrochunks*macrochunk_dim, file_len-num_macrochunks*macrochunk_dim);
					seq_huff.create_histo(histo, (file_len - num_macrochunks*macrochunk_dim));
				}

				// Create map <symbol, <code, len_code>>
				map<uint8_t, pair<uint32_t,uint32_t>> codes_map = seq_huff.create_code_map(histo);

				// Write file header
				BitWriter btw = seq_huff.write_header(codes_map);

				MEMORYSTATUSEX status;
				status.dwLength = sizeof(status);
				GlobalMemoryStatusEx(&status);
				uint64_t available_ram = status.ullAvailPhys;

				ofstream output_file(seq_huff._output_filename, fstream::out|fstream::binary);
				cerr << endl << "Output filename: " << seq_huff._output_filename << endl;

				// Write compressed file chunk-by-chunk
				tick_count tw1, tw2;
				tw1 = tick_count::now();
				for(uint64_t k=0; k < num_macrochunks; ++k) {
					seq_huff.read_file(file_in, k*macrochunk_dim, macrochunk_dim);
					seq_huff.write_chunks_compressed(available_ram, macrochunk_dim, codes_map, btw);
					output_file.write(reinterpret_cast<char*>(&seq_huff._file_out[0]), seq_huff._file_out.size());
					seq_huff._file_out.clear();
					cerr << "\rWrite compressed file: " << ((100*(k+1))/num_macrochunks) << "%";
				}
				if(num_macrochunks==1) cerr << "\rWrite compressed file: 100%";
				// Write exceeding byte
				if(num_macrochunks*macrochunk_dim < file_len){ 
					seq_huff.read_file(file_in, num_macrochunks*macrochunk_dim, file_len-num_macrochunks*macrochunk_dim);
					seq_huff.write_chunks_compressed(available_ram, file_len-(num_macrochunks*macrochunk_dim), codes_map, btw);
					output_file.write(reinterpret_cast<char*>(&seq_huff._file_out[0]), seq_huff._file_out.size());
					seq_huff._file_out.clear();
				}
				btw.flush();
				tw2 = tick_count::now();
				//cerr << endl << "Time for all writing (buffer): " << (tw2-tw1).seconds() << " sec" << endl;

				// Write on HDD
				tick_count twhd1, twhd2;
				twhd1 = tick_count::now();
				if(seq_huff._file_out.size() != 0)
					output_file.write(reinterpret_cast<char*>(&seq_huff._file_out[0]), seq_huff._file_out.size());
				output_file.close();
				file_in.close();
				twhd2 = tick_count::now();
				//cerr << "Time for all writing (Hard Disk): " << (twhd2-twhd1).seconds() << " sec" << endl;
				cerr << endl;

				tt2 = tick_count::now();
				cerr << "Total time for compression: " <<  (tt2 - tt1).seconds() << " sec" << endl << endl;

			}
		}
	} else {// DECOMPRESS
		SeqHuffman seq_huff;

		//par_huff.read_file(input_files[0]);
		//par_huff.decompress(input_files[0]);
		//par_huff.write_on_file();

		//seq_huff.read_file(input_files[0]);
		//seq_huff.decompress(input_files[0]);
		//seq_huff.write_on_file(input_files[0]);

		seq_huff.decompress_chunked(input_files[0]);

		//cerr << "[SEQ] La decompressione del file " << input_files[0] << " ha impiegato " << (t1s - t0s).seconds() << " sec" << endl << endl;
	}

	system("pause");

	return(0);

}



