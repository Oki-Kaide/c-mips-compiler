#include <iostream>
#include <string>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "includes.hpp"

#include "c_parser.tab.hpp"

/* version info */
#define BUILD_VERSION "100"
#define LEXER_VERSION "27"

/* compiler modes */
#define MODE_COMPILE 0
#define MODE_LEX 1
#define MODE_JSON 2
#define MODE_PARSE 3
#define MODE_AST 4
#define MODE_IR 5

/* lexer functions */
void print_tokens();
void print_json_tokens();
int yylex();
int yyparse();
extern FILE* yyin;
extern FILE* yyout;
extern int yydebug;

/* function declarations */
bool has_suffix(const std::string& str, const std::string& suffix);
void print_version();
void print_help();
void debug_ast();
void print_xml_ast();
void generate_ir();
void generate_mips();

/* Will be written to by yyparse */
Node* ast_root;

int main(int argc, char const *argv[]) {
	// parse command-line arguments
	int mode = MODE_COMPILE;
	std::string infile;
	std::string outfile;
	yydebug = 0;

	// determine mode of operation
	#ifdef program_mode
		mode = program_mode;
	#else
		std::string binaryname = argv[0];
		if(has_suffix(binaryname, "c_lexer")) {
			mode = MODE_JSON;
		} else if(has_suffix(binaryname, "c_parser")) {
			mode = MODE_PARSE;
		} else if(has_suffix(binaryname, "c_compiler")) {
			mode = MODE_COMPILE;
		}
	#endif

	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
			print_version();
			return 0;
		} else if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-?") == 0) {
			print_help();
			return 0;

		} else if(strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-d") == 0) {
			yydebug = 1;

		} else if(strcmp(argv[i], "--lex") == 0) {
			mode = MODE_LEX;
		} else if(strcmp(argv[i], "--json") == 0) {
			mode = MODE_JSON;
		} else if(strcmp(argv[i], "--parse") == 0 || strcmp(argv[i], "--xml") == 0) {
			mode = MODE_PARSE;
		} else if(strcmp(argv[i], "--ast") == 0) {
			mode = MODE_AST;
		} else if(strcmp(argv[i], "--compile") == 0 || strcmp(argv[i], "-S") == 0) {
			mode = MODE_COMPILE;
		} else if(strcmp(argv[i], "--ir") == 0 || strcmp(argv[i], "-i") == 0) {
			mode = MODE_IR;

		} else if(strcmp(argv[i], "-o") == 0) {
			if(i + 1 < argc) {
				if(argv[i+1][0] != '-') {
					outfile = argv[++i]; // store outfile and skip the next argv
				} else {
					std::cerr << "Invalid: -o is missing file argument" << std::endl;
					return 1;
				}
			} else {
				std::cerr << "Invalid: -o is missing file argument" << std::endl;
				return 1;
			}

		} else if(argv[i][0] == '-') {
			std::cerr << "Invalid option: " << argv[i] << std::endl;
			return 1;

		} else { // must be infile
			infile = argv[i];
		}
	}

	// set input and output files
	if(infile != "") {
    	FILE* fh;
		if((fh = fopen(infile.c_str(), "r"))) {
			yyin = fh;
		} else {
			std::cerr << "Error: input file \"" << infile << "\" could not be opened." << std::endl;
			return 1;
		}
	}
	if(outfile != "") {
    	FILE* fh;
		if((fh = fopen(outfile.c_str(), "w"))) {
			yyout = fh;
		} else {
			std::cerr << "Error: output file \"" << outfile << "\" could not be opened." << std::endl;
			return 1;
		}
	}


	// perform action
	switch (mode) {
		case MODE_LEX:
			while(yylex());
			print_tokens();
			break;
		case MODE_JSON:
			while(yylex());
			print_json_tokens();
			break;
		case MODE_PARSE:
			yyparse();
			print_xml_ast();
			break;
		case MODE_AST:
			yyparse();
			debug_ast();
			break;
		case MODE_COMPILE:
			yyparse();
			generate_mips();
			break;
		case MODE_IR:
			yyparse();
			generate_ir();
			break;
		default:
			std::cerr << "Error: unknown mode of operation " << mode << std::endl;
			return 1;
	}

	return 0;
}

/* ********************************************* */

bool has_suffix(const std::string &str, const std::string &suffix) {
    return (str.size() >= suffix.size()) && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void print_version() {
	std::cout << "lscc - Lorenzo's C compiler" << std::endl;
	std::cout << "lscc build "BUILD_VERSION << std::endl;
	std::cout << "Lexer build "LEXER_VERSION << std::endl;
}
void print_help() {
	std::cout << "lscc - Lorenzo's C compiler" << std::endl << std::endl;
	std::cout << "Usage: lscc [OPTIONS] [inputfile] [-o outputfile]\n\n";

	std::cout << "inputfile: Path to .c file, preprocessed wih the GNU C Preprocessor\n\tif omitted, defaults to stdin\n\n";
	std::cout << "outputfile: Location where the output needs to be sent\n\tif omitted, defaults to stdout\n\n";
	std::cout << "OPTIONS:\n\n";
	std::cout << "  -v, --version    Display compiler versions\n\n";
	std::cout << "  -h, -?, --help   Display this help menu\n\n";
	std::cout << "  -d, --debug      Send verbose debug output to stderr\n\n";
	std::cout << "  --lex            Tokenise the input, format as table\n\n";
	std::cout << "  --json           Tokenise the input, format as JSON\n\n";
	std::cout << "  --ast            Parse the input into AST, format as text\n\n";
	std::cout << "  --parse, --xml   Parse the input into AST, format as XML\n\n";
	std::cout << "  -i, --ir         Compile the C code into an interm. rep.\n\n";
	std::cout << "  -S, --compile    Compile the C code into MIPS assembly\n\n";
	std::cout << "\nIf none specified, defaults to --compile" << std::endl << std::endl;
}

void debug_ast() {
	ast_root->Debug(std::cout, 0);
}

void print_xml_ast() {
	ast_root->PrintXML(std::cout, 0);
}

void generate_ir() {
	try {
		std::stringstream ss;
		dynamic_cast<ProgramRoot*>(ast_root)->CompileIR(ss);
		fprintf(yyout, "%s", ss.str().c_str());
	} catch(compile_error& e) {
		std::cerr << e.what() << std::endl;
		std::cerr << "compilation terminated." << std::endl;
	}

}

void generate_mips() {
	try {
		std::stringstream ss;
		dynamic_cast<ProgramRoot*>(ast_root)->CompileMIPS(ss);
		fprintf(yyout, "%s", ss.str().c_str());
	} catch(compile_error& e) {
		std::cerr << e.what() << std::endl;
		std::cerr << "compilation terminated." << std::endl;
	}

}
