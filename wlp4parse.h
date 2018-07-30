#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <stack>
#include <fstream>
#include <map>

using std::cout;
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
			line = t_tran.top();

			istringstream iss(line);
			iss>>val;
			iss>>val;
			t_tran.pop();
		} else {
			line = nt_tran.top();
			nt_tran.pop();

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

			for (int i = children.size()-1; i >= 0; i--) {
				children.at(i)->read();
			}
		}

	}

	void print() {
		cout<<line<<endl;
		for (auto n : children) n->print();
	}

	string getVal() {
		string result = val;
		for (unsigned i = 0; i < children.size(); i++) result+=children[i]->getVal();
		return result;
	}

	void buildTab() {
		if(name == "dcl") {
			string type = children.at(0)->getVal();
			string value = children.at(1)->getVal();
			if (labels.find(value) == labels.end())
				labels.insert(std::pair<string, string>(value, type));
			else {
				cerr<<"ERROR redefined variable "<<value<<endl;
				exit(0);
			}
		} else {
			for (unsigned i = 0; i < children.size(); i++) {
				children[i]->buildTab();
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

void parsing(istringstream &sin){
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

	/* derive rules */
	fin >> n;
	getline(fin, line);
	for (int i = 0; i < n; i++) {
		getline(fin, line);
		rules.push_back(line);
	}

	/* total # of state*/
	fin >> n;

	/* transitions */
	fin >> n;
	getline(fin, line);
	for (int i = 0; i < n; i++) {
		getline(fin, line);
		transit.push_back(line);
	}


	string token;
	string value;
	string derive = "BOF ";
	t_tran.push("BOF BOF");
	while (sin >> token) {
		sin >> value;
		derive = derive + token + " ";
		string line = token + " " + value;
		t_tran.push(line);
	}
	derive = derive + "EOF";
	t_tran.push("EOF EOF");

	/* push stack */
	int currState = 0;
	unsigned currPos = 0;

	istringstream css(derive);
	while (css >> token) {
		int error = 1;
		parse.push_back(token);
		for (auto& i : terminal) {
			if (i == token) {
				error = 0;
			}
		}
		if (error) {
			cerr<<"ERROR at "<<parse.size()-1<<endl;
			exit(0);
		}
	}

	if (parse.size() == 0) return;

	stack<string>tokens;
	stack<int>states;
	string instruction;
	int number;
	string currToken = parse.at(0);

	do {
		int find = 0;
		for (vector<string>::iterator it = transit.begin(); it != transit.end(); it++) {
			istringstream rss(*it);
			int sta;
			string inp;
			rss >> sta;
			rss >> inp;

			if (sta == currState && inp == currToken) {
				find = 1;
				rss >> instruction;
				rss >> number;
				break;
			}
		}

		if (!find) {
			cerr<<"ERROR at "<<currPos<<endl;
			exit(0);
		}

		if (instruction == "shift") {
			tokens.push(currToken);
			states.push(currState);
			currState = number;
			currPos++;
			if (currPos == parse.size()) break;
			currToken = parse.at(currPos);
		} else if (instruction == "reduce") {
			output.push_back(number);
			istringstream nss(rules.at(number));
			string newState;
			int lastState = currState;
			nss >> newState;

			stack<string>rule;
			while (nss >> token) rule.push(token);

			while (!(rule.empty() || tokens.empty()) && rule.top() == tokens.top()){
				rule.pop();
				tokens.pop();
				lastState = states.top();
				states.pop();
			}

			currState = lastState;
			currToken = newState;
			currPos--;
		} else {
			cerr<<"ERROR at "<<currPos<<endl;
			exit(0);
		}

	} while (!(tokens.size() == 1 && tokens.top() == start));

	string result = "";
	while (!tokens.empty()) {
		string temp = tokens.top();
		tokens.pop();
		result = temp +" " +  result;
	}
	result = start + " " + result.substr(0, result.size()-1);

	int error = 1;
	for (auto& i : rules) {
		if (result == i) {
			for (auto p : output) {
				nt_tran.push(rules.at(p));
			}
			nt_tran.push(result);
			error = 0;
		}
	}
	if (error) {
		cerr<<"ERROR at "<<currPos<<endl;
		exit(0);
	}

	Node *nd = new Node("start", 0);
	nd->read();
	nd->buildTab();
	printTab();
	delete nd;
}
