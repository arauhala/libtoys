/*
 * testsuite.h
 *
 *  Created on: Jul 5, 2011
 *      Author: arau
 */

#ifndef TESTER_H
#define TESTER_H
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <time.h>
#include <sys/time.h>

enum linemod_t {
	checkl = 0,
	ignorel,
	reportl,
	faill,
};

class time_sentry {
private:
	timeval begin_;

public:
	time_sentry() : begin_() {
		reset();
	}
	void reset() {
		gettimeofday(&begin_, 0);
	}

	size_t us() {
		timeval end;
		gettimeofday(&end, 0);
		long secdiff = end.tv_sec - begin_.tv_sec;
		long usecdiff = end.tv_usec - begin_.tv_usec;
		usecdiff += 1000000 * secdiff;
		return usecdiff;
	}
	size_t ms() {
		return us() / 1000;
	}
};

typedef std::pair<std::set<std::string>, double> record_entry;
/*
class IReporter<T> {
	public:
		virtual void feed(record_entry& e) = 0;
		virtual T report() = 0;
};*/

struct average {
	typedef double return_type;
	double sum_;
	int n_;
	average() : sum_(), n_() {}
	inline void feed(const record_entry& e) {
		sum_ += e.second;
		n_++;
	}
	inline double report() {
		return sum_ / n_;
	}
};

template <typename T, int N>
struct multiplied {
	typedef double return_type;
	T t_;
	multiplied(const T& t = T()) : t_(t) {}
	inline void feed(const record_entry& e) {
		t_.feed(e);
	}
	inline double report() {
		return t_.report() * N;
	}
};

template <typename T, typename RV = typename T::type>
struct filtered {
	typedef RV return_type;
	const std::string& str_;
	T v_;
	filtered(const std::string& str, const T& v)
	: str_(str), v_(v) {}
	inline void feed(const record_entry& e) {
		if (e.first.count(str_)) {
			v_.feed(e);
		}
	}
	inline RV report() {
		return v_.report();
	}
};

class test_tool;

class report_output {
	private:
		test_tool& tool_;
		linemod_t mod_;
	public:
		report_output(test_tool& tool, linemod_t mod);
		~report_output();

		template <typename T>
		report_output& operator<<(const T& c);

};

class test_tool {

	private:

		friend class report_output;

		std::string name_;
		std::string expfile_;
		std::string outfile_;
		std::string recexpfile_;
		std::string recoutfile_;
		std::ifstream exp_;
		std::ofstream out_;
		std::ofstream rec_; // record file
		std::ostringstream line_;

		int linenumber_;
		bool verbose_;
		bool cleanline_;
		bool frozen_;
		bool fail_;
		bool& ok_;
		time_sentry time_;

		std::vector<record_entry> records_;

	public:

		test_tool(const std::string& test, bool& ok, bool verbose = false);
		~test_tool();

		test_tool& operator<<(linemod_t mod);

		std::string file_path(const std::string& name) const;

		void record(const std::set<std::string>& tags, double value);

		template <typename R, typename T = typename R::return_type>
		T report(R r) const {
			for (const record_entry& e : records_) {
				r.feed(e);
			}
			return r.report();
		}

		report_output ignored() {
			return report_output(*this, ignorel);
		}

		report_output reported() {
			return report_output(*this, reportl);
		}

		report_output failed() {
			return report_output(*this, faill);
		}

		template <typename T>
		test_tool& operator<<(const T& c) {
			std::ostringstream buf;
			buf<<c;
			std::string s( buf.str() );
			for (size_t i = 0; i < s.size(); ++i) {
				if (s[i] == '\n') {
					(*this)<<checkl;
				} else {
					line_<<s[i];
				}
			}
			return *this;
		}

};

template <typename T>
report_output& report_output::operator<<(const T& c) {
	std::ostringstream buf;
	buf<<c;
	std::string s( buf.str() );
	for (size_t i = 0; i < s.size(); ++i) {
		if (s[i] == '\n') {
			tool_<<mod_;
		} else {
			tool_.line_<<s[i];
		}
	}
	return *this;
}

typedef void (*testfunc)(test_tool& t);

struct test_entry {
	std::string name_;
	std::set<std::string> tags_;
	testfunc func_;
	test_entry(const char* name,
			  const std::set<std::string>& tags,
			  testfunc func)
	: name_(name),
	  tags_(tags),
	  func_(func) {}
};

class test_runner {
	public:
		test_runner(const char* testname);
		void add(const char* name, const std::set<std::string>& tags, testfunc func);
		template <typename F>
		void map(const std::set<std::string>& keys, F f) {
			for (const test_entry& e : tests_) {
				bool run = true;
				for (std::string key : keys) {
					if (e.tags_.count(key) == 0) {
						run = false;
						break;
					}
				}
				if (run) f(e);
			}
		}
		bool run(const std::set<std::string>& keys, bool verbose = false);
		std::string expfilepath(const std::string& testcase);
		int exec(int argc, char** argv);

	private:
		std::string testname_;
		std::vector<test_entry> tests_;
};

// mostly syntatctic sugar, fast way for constructing string
class sup {
private:
	std::ostringstream str_;
public:
	inline sup() : str_() {}
	template <typename T>
	inline sup& operator<<(T t) {
		str_<<t;
		return *this;
	}
	inline operator std::string () {
		return str_.str();
	}
};

// helps to create tag sets
inline std::set<std::string> operator+(const std::set<std::string>& set, const std::string& t) {
	std::set<std::string> rv;
	for (auto m : set) rv.insert(m);
	rv.insert(t);
	return rv;
}

class table;

struct table_format {
	const table& t_;
	int precision_;
	table_format(const table& t, int precision) : t_(t), precision_(precision) {}
};

class table {
	private:
		std::vector<std::string> xlabels_;
		std::vector<std::string> ylabels_;
		std::string yprefix_;
		std::vector<double> values_;
	public:
		table(const std::vector<std::string>& xlabels,
			  const std::vector<std::string>& ylabels,
			  const std::string& yprefix);
		const std::vector<std::string>& xlabels() const;
		const std::vector<std::string>& ylabels() const;
		table operator-(const table& table) const;
		double& at(int x, int y);
		const double& at(int x, int y) const;
		double& at(const std::string& x, const std::string& y);
		table_format formatted(int precision) const;
		const table& format(std::ostream& o, int precision = 3) const;
		const table& operator>>(std::ostream& o) const;
		std::string to_plot(int xscale, int height, double minbase = 0) const;
		std::string to_latex_pgf_plot(const std::string& quatity_label) const;
		std::string to_latex_pgf_doc(const std::string& quatity_label) const;
};

std::ostream& operator<<(std::ostream& o, const table& t);

std::ostream& operator<<(std::ostream& o, const table_format& t);

bool subset(const std::set<std::string>& set,
			const std::set<std::string>& subset);

bool cuttail(const std::string& path,
			 const std::string& head,
			 std::string& tail);

bool cuttail(const std::set<std::string>& paths,
			 const std::string& head,
			 std::string& tail);

template <typename T>
class to_table {
	public:
		typedef table return_type;
	private:
		std::set<std::string> filter_;
		std::string xprefix_;
		std::string yprefix_;
		std::map<std::set<std::string>, T> entries_;
		std::vector<std::string> xlabels_;
		std::vector<std::string> ylabels_;
	public:
		to_table(const std::set<std::string>& filter,
				const std::string& xprefix,
				const std::string& yprefix)
		: filter_(filter), xprefix_(xprefix), yprefix_(yprefix), entries_(), xlabels_(), ylabels_() {}
		void feed(const record_entry& e) {
			if (subset(e.first, filter_)) {
				std::string xlabel;
				std::string ylabel;
				if (cuttail(e.first, xprefix_, xlabel)) {
					if (cuttail(e.first, yprefix_, ylabel)) {
						std::set<std::string> key = {xprefix_+xlabel, yprefix_+ylabel};
						entries_[key].feed(e);
						if (std::find(xlabels_.begin(), xlabels_.end(), xlabel)
						    == xlabels_.end()) {
							xlabels_.push_back(xlabel);
						}
						if (std::find(ylabels_.begin(), ylabels_.end(), ylabel)
						    == ylabels_.end()) {
							ylabels_.push_back(ylabel);
						}
					}
				}
			}
		}
		table report() {
			table t(xlabels_, ylabels_, yprefix_);
			for (auto x : t.xlabels()) {
				std::string fx = xprefix_;
				fx += x;
				for (auto y : t.ylabels()) {
					std::string fy = yprefix_;
					fy += y;
					for (auto r : entries_) {
						if (r.first.count(fx) && r.first.count(fy)) {
							t.at(x, y) = r.second.report();
							break;
						}
					}
				}
			}
			return t;
		}


};


#endif
