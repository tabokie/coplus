#include <gtest/gtest.h>

#include "threading.h"
#include <iostream>

TEST(SingleThreadTest, CreateSingleThread){
	int callbackValue = -1;
	Thread newThread(function<()>([&]{callbackValue = 0;}));
	newThread.join();
	EXPECT_EQ(callbackValue, 0);
}
TEST(SingleThreadTest, SingleThreadReturnValue){
	int retValue = -1;
	Thread newThread(function<int()>([]{return 0;}));
	newThread.join();
	EXPECT_EQ(newThread.fetch(), 0);
}
