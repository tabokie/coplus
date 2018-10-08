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