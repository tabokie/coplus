// #pragma once

#include <random>
#include <string>

namespace coplus{

namespace { // isolated implementation

class Random{
	std::mt19937 gen_;
	std::uniform_real_distribution<double> double_dist_;
 public:
 	Random(){Shuffle();}
	void Shuffle(void){
		gen_.seed(std::random_device()());
	}
	void SetSeed(int seed){
		gen_.seed(seed);
	}
	int UInt(int max){
		return static_cast<int>(double_dist_(gen_) * max);
	}
	int Int(int max){ // [-max, max]
		return (double_dist_(gen_) * 2 - 1) * max;
	}
	float UFloat(void){
		return double_dist_(gen_);
	}
	float Float(void){
		return (double_dist_(gen_) * 2 - 1);
	}
	bool Bool(void){
		return double_dist_(gen_) > 0.5f;
	}
	std::string NumericString(int digit){
		std::string ret;
		// 5 digit a time
		while(ret.size() < digit){
			ret += std::to_string( static_cast<unsigned int>((UFloat()*9+1.0f) * 10000));
		}
		return ret.substr(0, digit);
	}
};

}

Random corand;

}

