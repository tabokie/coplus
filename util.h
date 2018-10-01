#pragma once

class NoCopy{
 private:
 	NoCopy(const NoCopy& rhs) = delete;
 	NoCopy(NoCopy rhs) = delete;
 public:
 	virtual ~NoCopy() { }
};

class NoMove: public NoCopy{
 private:
 	NoMove(NoMove&& rhs) = delete;
 public:
 	virtual ~NoMove() { }
};