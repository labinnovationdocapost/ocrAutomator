#pragma once
#include "FileStatus.h"

struct FileSum
{
	FileSum() : count(0) {};
	void operator()(BaseFileStatus* fs)
	{
		if (fs->isEnd)
		{
			sum += fs->ellapsed;
			count++;
		}
	}
	boost::posix_time::ptime::time_duration_type sum;
	int count;
};
