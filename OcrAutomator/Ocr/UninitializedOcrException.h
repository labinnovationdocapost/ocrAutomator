#pragma once

#include <string>

class UninitializedOcrException
{
protected:
	std::string mMessage = "";
public:
	std::string message() const
	{
		return mMessage;
	}

	void message(std::string message)
	{
		mMessage = message;
	}

	UninitializedOcrException() {}
	UninitializedOcrException(std::string message) : mMessage(message) {}
};