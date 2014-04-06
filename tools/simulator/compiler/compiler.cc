
#include <iostream>
#include <getopt.h>
#include "Kernel.h"
#include "llvm/Support/raw_ostream.h"

using namespace std;
using namespace llvm;

int main(int argc, char** argv)
{
  bool assembly = false;
  string kernel_src;
  string output_path;
  char ch;

  while ((ch = getopt(argc, argv, "Shk:o:")) != -1) {
    switch (ch) {
      case 'S':
        assembly = true;
        break;
      case 'o':
        output_path = optarg;
        break;
      case 'h':
      case '?':
        printf("%s - MCU Simulator\n", argv[0]);
        printf("  -S output llvm assembly (default is bitcode)\n");
        printf("  -o <path> output path (default is kernel.bc)\n");
        return 0;
    }
  }

  if (optind >= argc) {
    printf("%s kernel_dir\n", argv[0]);
    return 1;
  }

  kernel_src = argv[optind];

  Kernel kernel(kernel_src);

  kernel.build();

  raw_ostream* ostream;
  if (output_path.empty()) {
    if (!assembly) {
      printf("Won't output bitcode to stdout\n");
      return 1;
    }

    ostream = new raw_fd_ostream(0, false);
  }
  else {
    string err;
    ostream = new raw_fd_ostream(output_path.c_str(), err, sys::fs::OpenFlags::F_None);

    if (!ostream) {
      cerr << "COuld not open output file: " << err << endl;
      return 1;
    }
  }

  kernel.writeTo(*ostream, assembly);

  delete ostream;

  return 0;
}
