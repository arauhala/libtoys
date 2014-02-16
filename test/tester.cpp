/*
 * tester.cpp
 *
 *  Created on: Jul 6, 2011
 *      Author: arau
 */

#ifdef __clang__
char* gets(char*);
#endif

#include "tester.h"
#include <string.h>
#include <sys/stat.h>

//checkl_t checkl;
using namespace std;

report_output::report_output(test_tool& tool, linemod_t mod)
: tool_(tool), mod_(mod) {
	if (tool_.line_.str().size()) {
		tool_<<"\n";
	}
}
report_output::~report_output() {
	if (tool_.line_.str().size()) {
		(*this)<<"\n";
	}
}

test_tool::test_tool(const string& test, bool& ok, bool verbose)
:	name_(test),
	expfile_(test + "_exp.txt"),
	outfile_(test + "_out.txt"),
	recexpfile_(test + "_rec_exp.txt"),
	recoutfile_(test + "_rec_out.txt"),
	exp_(expfile_),
	out_(outfile_),
	rec_(recoutfile_),
	line_(),
	linenumber_(),
	verbose_(verbose),
	cleanline_(),
	fail_(),
	ok_(ok),
	time_(),
	records_() {
	cout<<test<<".. ";
	frozen_ = bool(exp_);
	if (!frozen_) {
		verbose_ = true;
		cout<<"test not yet frozen";
	}
	if (verbose_) { cout<<endl<<endl; cleanline_ = true; }
	std::ifstream rec(recexpfile_);
	if (rec) {
		while (rec) {
			int keys = 0;
			rec>>keys;
			record_entry e;
			for (int i = 0; i < keys; ++i) {
				std::string key;
				rec>>key;
				e.first.insert(key);
			}
			e.first.insert("run:exp");
			rec>>e.second;
			records_.push_back(e);
		}
	}
}

test_tool::~test_tool() {
	int ms = time_.ms();
	cout<<ms<<"ms.. ";
	if (verbose_) cout<<endl;

	rec_.close();
	out_.close();

	if (records_.size() == 0) remove(recoutfile_.c_str());
	if (frozen_ && !fail_) {
		cout<<"ok. "<<endl;
		remove(outfile_.c_str());
	} else {
		if (frozen_) cout<<"failed. ";
		while (true) {
			if (frozen_) cout<<"re[f]reeze";
			else cout<<" [f]reeze";
			cout<<" or view [d]iff? ";
			cout.flush();
			string answer;
			cin>>answer;
			if (answer == "f") {
				cout<<endl;
				int err = rename(outfile_.c_str(), expfile_.c_str());
				if (!err) {
					if (records_.size() > 0) {
						err = rename(recoutfile_.c_str(), recexpfile_.c_str());
					} else {
						remove(recexpfile_.c_str());
					}
					if (!err) {
						cout<<"exp file created."<<endl;
					}
				}
				if (err) {
					cout<<"failed creating exp file."<<endl;
				}
				break;
			} else if (answer == "d") {
				std::ostringstream cmd;
				cmd<<"meld ";
				cmd<<expfile_<<" ";
				cmd<<outfile_;
				int rv = system(cmd.str().c_str());
				if (rv) cout<<"no meld?\n";
				cout<<"\n";
			} else {
				break;
			}
		}
		cout<<endl;
	}
	ok_ &= !fail_;
}

test_tool& test_tool::operator<<(linemod_t mod) {
	string l = line_.str();
	line_.str("");
	linenumber_++;
	out_ << l << endl;
	string e;
	bool fail = false;
	if (mod == faill ||
	    (frozen_ && (!getline(exp_, e)
	    		 || (mod == checkl && l != e)))) {
		fail = fail_ = true;
	}

	if (verbose_ || fail || mod == reportl) {
		if (!cleanline_) { cout<<endl; cleanline_ = true; };
		cout<<"[";
		cout<<(linenumber_/1000)%10;
		cout<<(linenumber_/100)%10;
		cout<<(linenumber_/10)%10;
		cout<<(linenumber_%10);
		cout<<"] ";
		switch (mod) {
			case reportl:  cout<<"R "; break;
			case faill: cout<<"F "; break;
			default: cout<<"  "; break;
		}
		cout<<l<<endl;
		if (mod == checkl && fail) {
			cout<<" [!=]    "<<e<<endl;
		}
	}
	return *this;
}

std::string test_tool::file_path(const std::string& name) const {
	std::string dir = name_ + "/";
	mkdir(dir.c_str(), 0777);
	std::string rv = dir + name;
	return rv;
}

void test_tool::record(const std::set<std::string>& tags, double value) {
	rec_<<tags.size()<<" ";
	for (const std::string& tag : tags) {
		rec_<<tag<<" ";
	}
	rec_<<value<<"\n";
	records_.push_back(record_entry(tags, value));
	records_.back().first.insert("run:out");
}

test_runner::test_runner(const char* testname)
: testname_(testname), tests_() {}

void test_runner::add(const char* name, const std::set<std::string>& tags, testfunc func) {
	tests_.push_back(test_entry(name, tags, func));
	std::string ntag( name );
	while (true) {
		tests_.back().tags_.insert(ntag);
		size_t sep = ntag.find_last_of("/");
		if (sep == std::string::npos) break;
		ntag = ntag.substr(0, sep);
	}
}

bool test_runner::run(const std::set<std::string>& keys, bool verbose) {
	bool ok = true;
	map(keys, [&](const test_entry& e) {
		std::string test;
		test += "test/";
		test += e.name_;
		test_tool t(test, ok, verbose);
		(*e.func_)(t);
	});
	return ok;
}

std::string test_runner::expfilepath(const std::string& testcase) {
	return sup()<<"test/"<<testcase<<"_exp.txt";
}

int test_runner::exec(int argc, char** argv) {
	int rv = 0;
	bool list = false, print = false, verbose = false, path = false;
	set<string> keys;
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-l") == 0) {
			list = true;
		} else if (strcmp(argv[i], "-p") == 0) {
			print = true;
		} else if (strcmp(argv[i], "--path") == 0) {
			path = true;
		} else if (strcmp(argv[i], "-v") == 0) {
		    verbose = true;
		} else {
			keys.insert(argv[i]);
		}
	}
	if (list) {
		map(keys, [](const test_entry& t) {
			cout.width(50);
//			cout.setf(std::_S_left, std::_S_left);
			cout<<t.name_<<"{";
			bool first = true;
			for (const string& k : t.tags_) {
				if (t.name_.substr(0, k.size()) != k) {
					if (!first) cout<<", ";
					cout<<k; first = false;
				}
			}
			cout<<"}\n";
		});
	} else if (print) {
		map(keys, [&](const test_entry& t) {
			cout<<t.name_<<": ";
			std::ifstream in(expfilepath(t.name_));
			if (in) {
				std::cout<<"\n";
				int c;
				while (in && (c = in.get()) >= 0) {
					std::cout<<char(c);;
				}
			} else {
				cout<<"(not defined)\n";
			}
		});
	} else if (path) {
		map(keys, [&](const test_entry& t) {
			cout<<expfilepath(t.name_)<<"\n";
		});
	} else {
		cout<<testname_<<"..\n\n";

		time_sentry ms;
		bool ok = run(keys, verbose);
		cout<<"\n";
		cout<<ms.ms()<<"ms.. ";

		if (ok) {
			cout<<"done.\n";
		} else {
			cout<<"failed.\n";
			rv = -1;
		}
	}
	return rv;
}

table::table(const std::vector<std::string>& xlabels,
	  const std::vector<std::string>& ylabels,
	  const std::string& yprefix)
: 	xlabels_(xlabels),
	ylabels_(ylabels),
	yprefix_(yprefix),
	values_() {
	values_.resize(xlabels_.size() * ylabels_.size());
}
const std::vector<std::string>& table::xlabels() const {
	return xlabels_;
}
const std::vector<std::string>& table::ylabels() const {
	return ylabels_;
}
table table::operator-(const table& t) const {
	if (xlabels_ != t.xlabels_
	 || ylabels_ != t.ylabels_
	 || yprefix_ != t.yprefix_) {
		throw std::runtime_error("incompatible tables for operator-");
	}
	table rv(xlabels_, ylabels_, yprefix_);
	rv.values_ = values_;
	for (size_t i = 0; i < values_.size(); ++i) {
		rv.values_[i] -= t.values_[i];
	}
	return rv;
}
double& table::at(int x, int y) {
	return values_[y * xlabels_.size() + x];
}
const double& table::at(int x, int y) const {
	return values_[y * xlabels_.size() + x];
}
double& table::at(const std::string& x, const std::string& y) {
	size_t xi = 0, yi = 0;
	for( ;xi < xlabels_.size(); xi++) {
		if (xlabels_[xi] == x) break;
	}
	for( ;yi < ylabels_.size(); yi++) {
		if (ylabels_[yi] == y) break;
	}
	return at(xi, yi);
}
table_format table::formatted(int precision) const {
	return table_format(*this, precision);
}
const table& table::format(std::ostream& o, int precision) const {
	std::ostringstream buf;
	buf.setf(std::ios::fixed,std::ios::floatfield);
	buf.precision(precision);
	buf.setf(std::ios::left, std::ios::adjustfield);

	buf.width(12);
	buf<<yprefix_;
	for (size_t x = 0; x < xlabels_.size(); ++x) {
		buf.width(12);
		buf<<xlabels_[x];
	}
	buf<<std::endl;
	for (size_t y = 0; y < ylabels_.size(); ++y) {
		buf.width(12);
		buf<<ylabels_[y];
		for (size_t x = 0; x < xlabels_.size(); ++x) {
			buf.width(12);
			buf<<at(x, y);
		}
		buf<<std::endl;
	}
	o<<buf.str();
	return *this;
}
const table& table::operator>>(std::ostream& o) const {
	return format(o);
}
std::string table::to_plot(int xscale, int height, double minbase) const {
	std::ostringstream buf;
	buf.setf(std::ios::fixed,std::ios::floatfield);
	buf.setf(std::ios::left, std::ios::adjustfield);

	double max = 0, min = minbase;
	for (size_t y = 0; y < ylabels_.size(); ++y) {
		for (size_t x = 0; x < xlabels_.size(); ++x) {
			max = std::max(max, at(x, y));
			min = std::min(min, at(x, y));
		}
	}
	double vheight = (max - min)*1.1;

	buf.precision(1);
	size_t lsize = 4, l = 0;
	for (size_t i = 0; i < ylabels_.size(); i++) {
		size_t begin = xscale*i;
		while (l < begin) {l++; buf<<' '; }
		if (l == begin) {
			int sz = std::min(size_t(lsize), ylabels_[i].size());
			if (l + sz > xscale*ylabels_.size()) break;
			buf<<ylabels_[i];
			buf<<' ';
			l += sz+1;
		}
	}
	buf<<'\n';

	buf.precision(3);
	for (int y = height -1; y >= 0; y--) {
		for (size_t i = 0; i < ylabels_.size(); i++) {
			char c = '.';
			for (size_t j = 0; j < xlabels_.size(); j++) {
				double value = at(j, i);
				double relat = (value - min) / vheight;
				int discat = int(relat * height);
				if (discat == y) {
					c = char(j + 'A');
				}
			}
			for (int j = 0; j < xscale; ++j) buf<<c;
		}
		buf<<"  ";
		buf<<(((double(y))*vheight)/height+min);
		buf<<'\n';
	}
	buf<<"\n";
	for (size_t j = 0; j < xlabels_.size(); j++) {
		buf<<char(j+'A')<<"   "<<xlabels_[j]<<"\n";
	}

	return buf.str();
}

std::string table::to_latex_pgf_plot(const std::string& quatity_label) const {
	std::ostringstream buf;
	buf<<"\\begin{tikzpicture}\n";
	buf<<"\\begin{axis}[\n";
	buf<<"grid=major,\n";
	buf<<"xlabel="<<yprefix_<<",\n";
	buf<<"ylabel="<<quatity_label<<"]\n";

	for (size_t x = 0; x < xlabels_.size(); ++x) {
		buf<<"\\addplot coordinates {\n";
		for (size_t y = 0; y < ylabels_.size(); ++y) {
			buf<<"("<<ylabels_[y]<<","<<at(x, y)<<")";
		}
		buf<<"};\n";
		std::string xl(xlabels_[x]);
		for (size_t i = 0; i < xl.size(); ++i) {
			if (xl[i] == '_') xl[i] = ' ';
		}
		buf<<"\\addlegendentry{"<<xl<<"}\n";
	}
	buf<<"\\end{axis}\n";
	buf<<"\\end{tikzpicture}\n";
	return buf.str();
}

std::string table::to_latex_pgf_doc(const std::string& quatity_label) const {
	std::ostringstream buf;
	buf<<"\\documentclass[a4paper]{article}\n"
	   <<"\\usepackage{pgfplots}\n"
	   <<"\\pgfplotsset{compat=1.5}\n"
	   <<"\\begin{document}\n";
	buf<<to_latex_pgf_plot(quatity_label);
	buf<<"\\end{document}\n";
	return buf.str();
}

std::ostream& operator<<(std::ostream& o, const table& t) {
	t>>o;
	return o;
}

std::ostream& operator<<(std::ostream& o, const table_format& t) {
	t.t_.format(o, t.precision_);
	return o;
}

bool subset(const std::set<std::string>& set,
				   const std::set<std::string>& subset) {
	for (auto s : subset) {
		if (set.count(s) == 0) return false;
	}
	return true;
}

bool cuttail(const std::string& path,
					const std::string& head,
					std::string& tail) {
	if (path.substr(0, head.size()) == head) {
		tail = path.substr(head.size());
		return true;
	}
	return false;
}

bool cuttail(const std::set<std::string>& paths,
					const std::string& head,
					std::string& tail) {
	for (auto p : paths) {
		if (cuttail(p, head, tail)) return true;
	}
	return false;
}
