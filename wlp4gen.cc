#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <stack>
#include <fstream>
#include <map>

using std::cout;
using std::cin;
using std::cerr;
using std::endl;
using std::string;
using std::istringstream;
using std::vector;
using std::stack;
using std::ifstream;
using std::map;

stack<string>nt_tran;
stack<string>t_tran;
vector<string>terminal;
vector<string>non_terminal;
map<string, string>labels;
map<string, int>locations;
int counter = 0;
int loops = 0;
int conds = 0;

void push(int reg) {

	cout<<"sw $"<<reg<<", -4($30)"<<endl;
	cout<<"sub $30, $30, $4"<<endl;
	counter-=4;
}

void pop(int reg) {

	counter+=4;
	cout<<"lw $"<<reg<<", "<<counter<<"($29)"<<endl;
	cout<<"add $30, $30, $4"<<endl;

}

string checkValid(string a, string op,string b) {
	if (a == "int" && b == "int") return "int";
	if ((a == "int" || b == "int") && op == "PLUS") return "int*";
	if (a == "int*" && op == "MINUS") return b=="int*"? "int" : "int*";
	cerr<<"ERROR when do "<<a<<" "<<op<<" "<<b<<endl;
	exit(0);
}

class Node {
	string name;
	string line;
	string val = "";
	int type;
	vector<Node *>children;

	public:
	Node(string name, int type):name(name), type(type){}

	void read() {
		if (type) {
			getline(cin, line);

			istringstream iss(line);
			iss>>val;
			iss>>val;
		} else {
			getline(cin, line);

			istringstream iss(line);
			string newName;
			iss >> newName;
			while (iss>>newName) {
				int newType = 0;
				for (auto& s : terminal) {
					if (s == newName) newType = 1;
				}

				Node *newNode = new Node(newName, newType);
				children.push_back(newNode);
			}

			for (unsigned i = 0; i < children.size(); i++) {
				children.at(i)->read();
			}
		}

	}

	void print() {
		cout<<line<<endl;
		for (auto n : children) n->print();
	}

	string getName() {
		return name;
	}

	string getVal() {
		string result = val;

		if (name == "expr" || name == "term") {
			return children[0]->getVal();
		} else if (name == "factor" || name == "lvalue" || name == "dcl") {
			return children[children.size() != 1]->getVal();
		}
		for (unsigned i = 0; i < children.size(); i++) result+=children[i]->getVal();
		return result;
	}

	string getType() {
		string type = "";
		if (name == "ID") {
			
			for (auto p : labels) if (p.first == val) type = p.second;
			return type;

		} else if (name == "dcl") {

			return children[1]->getType();

		} else if (name == "NUM" || name == "INT") {
			
			return "int";

		} else if (name == "NULL") {
			
			return "int*";
		
		} else if (name == "lvalue") {
		
			switch (children.size()) {
				case 1:								// lvalue ID
					return children[0]->getType();
				case 2:								// lvalue STAR factor
					type = children[1]->getType();
					if (type == "" || type.back() != '*') {
						cerr<<"ERROR cannot dereference non-pointer "<<children[1]->getVal()<<endl;
						exit(0);
					}
					type.pop_back();
					return type;
				case 3:								// lvalue LPAREN lvalue RPAREN
					return children[1]->getType();
			}
		
		} else if (name == "factor") {
		
			if (children[0]->getName() == "ID" || 
				children[0]->getName() == "NUM") {
				return children[0]->getType();
			} else if (children[0]->getName() == "NULL") {
				return "int*";
			} else if (children[0]->getName() == "LPAREN") {
				return children[1]->getType();
			} else if (children[0]->getName() == "AMP") {
				return children[1]->getType()+"*";
			} else if (children[0]->getName() == "STAR") {
				type = children[1]->getType();
				type.pop_back();
				return type;
			} else {								// int array
				if (children[3]->getType() != "int") {
					cerr<<"ERROR: array must be int"<<endl;
					exit(0);
				}
				return "int*";
			}
		
		} else if (name == "term" || name == "expr") {

			if (children.size() == 1) {
				return children[0]->getType();
			} else {
				return checkValid(children[0]->getType(), children[1]->getName(), children[2]->getType());
			}

		}
		return "error";
	}

	void check() {
		if (name == "main" && (children[5]->getType() != "int" ||children[11]->getType() != "int")) {
			cerr<<"ERROR WRONG MAIN TYPE"<<endl;
			exit(0);
		}

		if (name == "procedure" && children[9]->getType() != "int") {
			cerr<<"ERROR WRONG MAIN TYPE"<<endl;
			exit(0);
		}

		if (name == "expr" || name == "term") {
			getType();
		}

		if (name == "statement") {
			switch(children.size()) {
				case 4:
					if (children[0]->getType() != children[2]->getType()) {
						cerr<<"ERROR WRONG TYPE ASSIGNMENT"<<endl;
						exit(0);
					}
					break;
				case 5:
					if ((children[0]->getName() == "PRINTLN" && children[2]->getType() != "int")
						||(children[0]->getName() == "DELETE" && children[3]->getType() != "int*")) {
						cerr<<"ERROR WHEN PRINTLN OR DELETE"<<endl;	
					}
					break;
				case 7:
					children[5]->check();
					break;
				case 11:
					children[5]->check();
					children[9]->check();
			}
		}

		if (name == "test") {
			if (children[0]->getType() != children[2]->getType()) {
				cerr<<"ERROR WHEN COMPARING"<<endl;
				exit(0);
			}
		}

		if (name == "dcls" && children.size() == 5 && 
				children[1]->getType() != children[3]->getType()) {
			cerr<<"ERROR WHEN ASSIGNMENT"<<endl;
		}

		for (unsigned i = 0; i < children.size(); i++) {
			children[i]->check();
		}
	}


	void buildMIPS(int reg) {
		if (name == "dcls") {												/* dcls */

			if (children.size()) {
				children[0]->buildMIPS(0);
				children[1]->buildMIPS(0);
				children[3]->buildMIPS(0);
				
				cout<<"sw $3, "<<locations.find(children[1]->getVal())->second<<"($29)"<<endl;
			}

		} else if (name == "dcl") {											/* dcl */
			string type = children.at(0)->getVal();
			string value = children.at(1)->getVal();
			if (labels.find(value) == labels.end()) {
				labels.insert(std::pair<string, string>(value, type));
				locations.insert(std::pair<string, int>(value, counter));

				push(reg);

			} else {
				cerr<<"ERROR redefined variable "<<value<<endl;
				exit(0);
			}
		} else if (name == "ID") {											/* ID */
			if (labels.find(val) == labels.end()) {
				cerr<<"ERROR not define variable "<<val<<endl;
				exit(0);
			}

			int destLoca = locations.find(val)->second;

			cout<<"lw $3, "<<destLoca<<"($29)"<<endl;

		} else if (name == "NUM") {											/* NUM */

			cout<<"lis $3"<<endl;
			cout<<".word "<<val<<endl;

		} else if (name == "lvalue") {										/* lvalue */

			int offset = 0;
			switch (children.size()) {
				case 2:
					children[1]->buildMIPS(0);
					break;
				default:
					offset = locations.find(getVal())->second;
					cout<<"lis $3"<<endl;
					cout<<".word "<<offset<<endl;
					cout<<"add $3, $3, $29"<<endl;
			}

		} else if (name == "main") {										/* main */

			cout<<";; main params"<<endl;
			children[3]->buildMIPS(1);
			children[5]->buildMIPS(2);


			cout<<endl<<";; dcls"<<endl;
			children[8]->buildMIPS(0);
			cout<<endl<<";; statemenst"<<endl;
			children[9]->buildMIPS(0);
			cout<<endl<<";; return "<<endl;
			children[11]->buildMIPS(0);

			cout<<endl;

		} else if (name == "statements") {									/* stmts */

			if (children.size()) {
				children[0]->buildMIPS(0);
				children[1]->buildMIPS(0);
			}

		} else if (name == "statement") {									/* statement */

			if (children[0]->getName() == "PRINTLN") {
				children[2]->buildMIPS(0);
	
				cout<<"add $1, $3, $0"<<endl;
				cout<<"lis $10"<<endl;
				cout<<".word print"<<endl;
				cout<<"jalr $10"<<endl;
			} else if (children[0]->getName() == "lvalue") {

				children[2]->buildMIPS(0);

				push(3);

				children[0]->buildMIPS(0);

				pop(5);

				cout<<"sw $5, 0($3)"<<endl;

			} else if (children[0]->getName() == "WHILE") {
				loops ++;
				int temp = loops;

				cout<<"stwl"<<temp<<":"<<endl;

				children[2]->buildMIPS(0);
				
				cout<<"beq $3, $0, endwl"<<temp<<endl;

				children[5]->buildMIPS(0);

				cout<<"beq $0, $0, stwl"<<temp<<endl;

				cout<<"endwl"<<temp<<":"<<endl;

			} else if (children[0]->getName() == "IF") {
				conds ++;
				int temp = conds;

				children[2]->buildMIPS(0);

				cout<<"bne $3, $0, stthen"<<temp<<endl;

				children[9]->buildMIPS(0);

				cout<<"beq $0, $0, endthen"<<temp<<endl;

				cout<<"stthen"<<temp<<":"<<endl;

				children[5]->buildMIPS(0);

				cout<<"endthen"<<temp<<":"<<endl;

			}

		} else if (name == "expr") {										/* expr */
			
			if (children.size() == 1)
				children[0]->buildMIPS(0);
			else {
				children[0]->buildMIPS(0);

				push(3);

				children[2]->buildMIPS(0);

				pop(5);

				string operation = (children[1]->getName()=="PLUS")? "add":"sub";

				// MIPS add or sub
				cout<<operation<<" "<<"$3, $5, $3"<<endl;

			}

		} else if (name == "term") {										/* term */
			
			if (children.size() == 1) 
				children[0]->buildMIPS(0);
			else {

				children[0]->buildMIPS(0);

				push(3);

				children[2]->buildMIPS(0);

				pop(5);

				string operation1 = (children[1]->getName()=="STAR")? "mult":"div";
				string operation2 = (children[1]->getName()=="PCT")? "mfhi":"mflo";

				// MIPS instruction
				cout<<operation1<<" "<<"$5, $3"<<endl;
				cout<<operation2<<" "<<"$3"<<endl;

			}

		} else if (name == "factor") {										/* factor */
			if (children[0]->getName() == "ID" ||
				children[0]->getName() == "NUM") {
				children[0]->buildMIPS(0);
			} else if (children[0]->getName() == "LPAREN") {
				children[1]->buildMIPS(0);
			} else if (children[0]->getName() == "STAR") {
				children[1]->buildMIPS(0);
				cout<<"lw $3, 0($3)"<<endl;
			} else if (children[0]->getName() == "NULL") {
				cout<<"add $3, $0, $11"<<endl;
			} else if (children[0]->getName() == "AMP") {
				children[1]->buildMIPS(0);
			}

				
		} else if (name == "test") {										/* test */
			children[0]->buildMIPS(0);

			push(3);

			children[2]->buildMIPS(0);

			pop(5);

			if (children[1]->getName() == "LT") {
				cout<<"slt $3, $5, $3"<<endl;
			} else if (children[1]->getName() == "GT") {
				cout<<"slt $3, $3, $5"<<endl;
			} else if (children[1]->getName() == "LE") {
				cout<<"slt $3, $3, $5"<<endl;
				cout<<"sub $3, $11, $3"<<endl;
			} else if (children[1]->getName() == "GE") {
				cout<<"slt $3, $5, $3"<<endl;
				cout<<"sub $3, $11, $3"<<endl;
			} else if (children[1]->getName() == "NE") {
				cout<<"slt $6, $3, $5"<<endl;
				cout<<"slt $7, $5, $3"<<endl;
				cout<<"add $3, $6, $7"<<endl;
			} else if (children[1]->getName() == "EQ") {
				cout<<"slt $6, $3, $5"<<endl;
				cout<<"slt $7, $5, $3"<<endl;
				cout<<"add $3, $6, $7"<<endl;
				cout<<"sub $3, $11, $3"<<endl;
			}

		} else {															/* else .. */
			for (unsigned i = 0; i < children.size(); i++) {
				children[i]->buildMIPS(0);
			}
		}

	}

	~Node() {
		for (auto& n : children) delete n;
	}

};

void printTab() {
	cerr<<"wain"<<endl;
	for (auto p : labels) cerr<<p.first<<" "<<p.second<<endl;
}

void generateMips(Node *nd) {

	// for println
	cout<<".import print"<<endl;

	// $4 = 4, $11 = 11
	cout<<"lis $4"<<endl;
	cout<<".word 4"<<endl<<endl;;
	cout<<"lis $11"<<endl;
	cout<<".word 1"<<endl<<endl;;

	cout<<"sub $29, $30, $4"<<endl;

	// store $31 to stack
	cout<<"sw $31, -4($30)"<<endl;
	cout<<"sub $30, $30, $4"<<endl;
	counter-=4;

	nd->buildMIPS(0);
	nd->check();

	// load first to $3
	counter+=4;
	cout<<"lw $31, 0($29)"<<endl;

	cout<<endl<<"add $30, $29, $4"<<endl;
	cout<<"jr $31"<<endl;
}

int main () {
	int n;
	string line;
	vector<string>rules;
	vector<string>transit;
	vector<string>parse;
	vector<int>output;
	string start;

	ifstream fin;
	fin.open("wlp4.lr1");

	/* terminal symbols */
	fin >> n;
	while (n > 0) {
		string t;
		fin >>t;
		terminal.push_back(t);
		n--;
	}

	/* non-terminal symbols */
	fin >> n;
	while (n > 0) {
		string t;
		fin >>t;
		non_terminal.push_back(t);
		n--;
	}

	/* start symbol */
	fin >> start;

	Node *nd = new Node(start, 0);
	nd->read();
	generateMips(nd);
	delete nd;
}
