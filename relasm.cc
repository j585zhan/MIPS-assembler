#include "kind.h"
#include "lexer.h"
#include <vector>
#include <string>
#include <iostream>
#include <cstdio>
#include <map>
// Use only the neeeded aspects of each namespace
using std::string;
using std::vector;
using std::endl;
using std::cerr;
using std::cin;
using std::map;
using std::pair;
using std::getline;
using ASM::Token;
using ASM::Lexer;


int main(int argc, char* argv[]){
	// Nested vector representing lines of Tokens
	// Needs to be used here to cleanup in the case
	// of an exception
	vector< vector<Token*> > tokLines;
	map<string, int>label;
	map<string, int>::iterator lit;
	vector<int> reloTable;

	try{
		// Create a MIPS recognizer to tokenize
		// the input lines
		Lexer lexer;
		// Tokenize each line of the input
		string line;
		while(getline(cin,line)){
			tokLines.push_back(lexer.scan(line));
		}

		// Iterate over the lines of tokens and print them
		// to standard error
		vector<vector<Token*> >::iterator it;

		// extract label
		int pc = 0xc;
		int numLabel = 0;
		for(it = tokLines.begin(); it != tokLines.end(); ++it, pc+=4){
			vector<Token*>::iterator it2;
			it2 = it->begin();

			while (it2 != it->end() && (*it2)->getKind() == ASM::LABEL) {
				string newlabel = (*it2)->getLexeme().substr(0, (*it2)->getLexeme().size() - 1);
				if (label.find(newlabel) != label.end()) {
					throw string("ERROR: label already exist");
				}
				label.insert(pair<string, int>(newlabel, pc));
				it2++;
			}

			if (it2 == it->end()) {
				pc-=4;
			}

		}

		pc = 0xc;
		// extract relocated
		for(it = tokLines.begin(); it != tokLines.end(); ++it, pc+=4){
			vector<Token*>::iterator it2;
			it2 = it->begin();

			// jump over all labels
			while (it2 != it->end() && (*it2)->getKind() == ASM::LABEL) it2++;

			// check if only contain labels
			if (it2 == it->end()) {
				pc-=4;
				continue;
			}

			if ((*it2)->getKind() == ASM::DOTWORD) {

				// only instruction
				if (++it2 == it->end()) {
					throw string("ERROR: Need an integer after directive .word");
				}


				if ((*it2)->getKind() == ASM::ID && label.find((*it2)->getLexeme()) != label.end()) {
					reloTable.emplace_back(pc);
					numLabel++;
				}
			}

		}

		// header

		// beq $0, $0, 2
		putchar(0x10);
		putchar(0x00);
		putchar(0x00);
		putchar(0x02);

		// file length (pc)
		pc += numLabel*8;
		putchar((pc>>24)&0xff);
		putchar((pc>>16)&0xff);
		putchar((pc>>8)&0xff);
		putchar(pc&0xff);

		// code + hdr (pc-numLabel*8)
		pc -= numLabel*8;
		putchar((pc>>24)&0xff);
		putchar((pc>>16)&0xff);
		putchar((pc>>8)&0xff);
		putchar(pc&0xff);

		pc = 0xc;
		// do instruction
		for(it = tokLines.begin(); it != tokLines.end(); ++it){
			pc+=4;
			vector<Token*>::iterator it2;
			it2 = it->begin();

			// jump over all labels
			while (it2 != it->end() && (*it2)->getKind() == ASM::LABEL) it2++;

			// check if only contain labels
			if (it2 == it->end()) {
				pc-=4;
				continue;
			}

			int word;
			// .WORD
			if ((*it2)->getKind() == ASM::DOTWORD) {

				// only instruction
				if (++it2 == it->end()) {
					throw string("ERROR: Need an integer after directive .word");
				}


				if ((*it2)->getKind() != ASM::INT && (*it2)->getKind() != ASM::HEXINT) {
					if ((*it2)->getKind() == ASM::ID && label.find((*it2)->getLexeme()) != label.end()) {
						word = label[(*it2)->getLexeme()];
					} else {
						throw string("ERROR: No such label");
					}
				} else {
					word = (*it2)->toInt();
				}
			} else if ((*it2)->getKind() == ASM::ID) {

				string instruction = (*it2)->getLexeme();
				if (++it2 == it->end() || (*it2)->getKind() != ASM::REGISTER) {
					throw string("ERROR: need a register after instruction");
				}

				if (instruction == "jr" || instruction == "jalr") {

					word = (*it2)->toInt();
					word <<= 21;
					word |= 8;

					if (instruction == "jalr") word+=1;

				} else if (instruction == "add" || instruction == "sub"
						|| instruction == "slt" || instruction == "sltu") {
					int ins, d, s, t;

					d = (*it2)->toInt();

					// check comma
					if (++it2 == it->end() || (*it2)->getKind() != ASM::COMMA) {
						throw string("ERROR: need a comma after register");
					}

					// check register
					if (++it2 == it->end() || (*it2)->getKind() != ASM::REGISTER) {
						throw string("ERROR: need a register after instruction");
					}

					s = (*it2)->toInt();

					// check comma
					if (++it2 == it->end() || (*it2)->getKind() != ASM::COMMA) {
						throw string("ERROR: need a comma after register");
					}

					// check register
					if (++it2 == it->end() || (*it2)->getKind() != ASM::REGISTER) {
						throw string("ERROR: need a register after instruction");
					}

					t = (*it2)->toInt();

					if (instruction == "add") {
						ins =32;
					} else if (instruction == "sub") {
						ins = 34;
					} else if (instruction == "slt") {
						ins = 42;
					} else {
						ins = 43;
					}

					word = s<<21|t<<16|d<<11|ins;

				} else if (instruction == "beq" || instruction == "bne"){
					int ins, s, t, i;

					s = (*it2)->toInt();

					// check comma
					if (++it2 == it->end() || (*it2)->getKind() != ASM::COMMA) {
						throw string("ERROR: need a comma after register");
					}

					// check register
					if (++it2 == it->end() || (*it2)->getKind() != ASM::REGISTER) {
						throw string("ERROR: need a register after instruction");
					}

					t = (*it2)->toInt();

					// check comma
					if (++it2 == it->end() || (*it2)->getKind() != ASM::COMMA) {
						throw string("ERROR: need a comma after register");
					}

					// check number
					if (++it2 == it->end()) {
						throw string("ERROR: need a number after instruction");
					}

					if ((*it2)->getKind() != ASM::INT && (*it2)->getKind() != ASM::HEXINT) {
						if ((*it2)->getKind() == ASM::ID && label.find((*it2)->getLexeme()) != label.end()) {
							i = (label[(*it2)->getLexeme()] - pc) / 4;

							if (i < -32768 || i > 32767) {
								throw string("ERROR: Branch offset out of range");
							}

						} else {
							throw string("ERROR: No such label");
						}
					} else {
						i = (*it2)->toInt();

						if (((*it2)->getKind() == ASM::INT  && (i < -32768 || i > 32767))
								||	((*it2)->getKind() == ASM::HEXINT  && (i < 0 || i > 0xffff))) {
							throw string("ERROR: Branch offset out of range");
						}

					}

					i &= ((1<<16) - 1);

					if (instruction == "beq") {
						ins = 4;
					} else {
						ins = 5;
					}

					word = ins<<26|s<<21|t<<16|i;

				} else if (instruction == "lis" || instruction == "mfhi" || instruction == "mflo"){
					int d = (*it2)->toInt();

					if (instruction == "lis") {
						word = 20;
					} else if (instruction == "mfhi") {
						word = 16;
					} else {
						word = 18;
					}

					word |= (d<<11);

				} else if (instruction == "mult" || instruction == "multu" 
						|| instruction == "div" || instruction == "divu"){
					int s, t;
					s = (*it2)->toInt();

					// check comma
					if (++it2 == it->end() || (*it2)->getKind() != ASM::COMMA) {
						throw string("ERROR: need a comma after register");
					}

					// check register
					if (++it2 == it->end() || (*it2)->getKind() != ASM::REGISTER) {
						throw string("ERROR: need a register after instruction");
					}

					t = (*it2)->toInt();
					if (instruction == "mult") {
						word = 24;
					} else if (instruction == "multu") {
						word = 25;
					} else if (instruction == "div") {
						word = 26;
					} else {
						word = 27;
					}

					word |= (s<<21|t<<16);

				} else if (instruction == "lw" || instruction == "sw") {
					int t, i, s;

					t = (*it2)->toInt();
					// check comma
					if (++it2 == it->end() || (*it2)->getKind() != ASM::COMMA) {
						throw string("ERROR: need a comma after register");
					}

					// check number
					if (++it2 == it->end() || ((*it2)->getKind() != ASM::INT && (*it2)->getKind() != ASM::HEXINT)) {
						throw string("ERROR: need a number after instruction");
					} else {
						i = (*it2)->toInt();

						if (((*it2)->getKind() == ASM::INT  && (i < -32768 || i > 32767))
								||	((*it2)->getKind() == ASM::HEXINT  && (i < 0 || i > 0xffff))) {
							throw string("ERROR: Branch offset out of range");
						}
					}
					i &= ((1<<16) - 1);

					// check lparen
					if (++it2 == it->end() || (*it2)->getKind() != ASM::LPAREN) {
						throw string("ERROR: need a lparen after number");
					}

					// check register
					if (++it2 == it->end() || (*it2)->getKind() != ASM::REGISTER) {
						throw string("ERROR: need a register after instruction");
					}

					s = (*it2)->toInt();

					// check rparen
					if (++it2 == it->end() || (*it2)->getKind() != ASM::RPAREN) {
						throw string("ERROR: need a rparen after register");
					}

					if (instruction == "lw") {
						word = 35<<26;
					} else {
						word = 43<<26;
					}

					word |= (s<<21|t<<16|i);

				} else {
					throw string("ERROR: Unknown directive ");
				}
			} else {
				throw string("ERROR: Unknown directive ");
			}

			int mask = 0xff;
			int ab = word&mask;
			word>>=8;
			int cd = word&mask;
			word>>=8;
			int ef = word&mask;
			word>>=8;
			int gh = word&mask;
			putchar(gh);
			putchar(ef);
			putchar(cd);
			putchar(ab);


			if (++it2 != it->end()) {
				throw string("ERROR: Expecting end of line, but there's more stuff.");
			}

			//for(it2 = it->begin(); it2 != it->end(); ++it2){
			//  cerr << "  Token: "  << *(*it2) << "    ";
			//}
		}

	} catch(const string& msg){
		// If an exception occurs print the message and end the program
		cerr << msg << endl;
	}
	for (vector<int>::iterator sit = reloTable.begin(); sit != reloTable.end(); sit++) {
		int reloTab = *sit;
		putchar(00);
		putchar(00);
		putchar(00);
		putchar(01);
		putchar((reloTab>>24) & 0xff);
		putchar((reloTab>>16) & 0xff);
		putchar((reloTab>>8) & 0xff);
		putchar(reloTab & 0xff);
	}

	// Delete the Tokens that have been made
	vector<vector<Token*> >::iterator it;
	for(it = tokLines.begin(); it != tokLines.end(); ++it){
		vector<Token*>::iterator it2;
		for(it2 = it->begin(); it2 != it->end(); ++it2){
			delete *it2;
		}
	}
}
