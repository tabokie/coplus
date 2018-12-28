#pragma once

class NoCopy{
 private:
 	NoCopy(const NoCopy& rhs) = delete;
 public:
 	NoCopy() = default;
 	virtual ~NoCopy() { }
};

class NoMove: public NoCopy{
 private:
 	NoMove(NoMove&& rhs) = delete;
 public:
 	NoMove() = default;
 	virtual ~NoMove() { }
};

#include <vector>
#include <iostream>
#include <string>

struct StringUtil {
	static int split(std::string& str, std::vector<std::string>& ret) {
		int cur = 0;
		int to = 0;
		int count = ret.size();
		while((cur = str.find_first_not_of(' ', cur)) != std::string::npos) {
			to = str.find_first_of(' ', cur); // find space
			if(to == std::string::npos) to = str.size();
			if(to > cur) {
				ret.push_back(str.substr(cur, to - cur));
			}
			cur = to + 1;
		}
		return ret.size() - count;
	}
	static bool starts_with(std::string& a, std::string b) {
		if(a.size() < b.size()) return false;
		return a.compare(0, b.size(), b) == 0;
	}

};
