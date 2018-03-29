#pragma once
#include <memory>
#include "Base/OutputFlags.h"
#include <unordered_map>
#include <vector>
#include "Buffer/MemoryFileBuffer.h"
#include "Export.h"
#include "Base/BaseFileStatus.h"


class SHARED_EXPORT OcrResult
{
private:
	BaseFileStatus* file = nullptr;
public:
	BaseFileStatus* File();
	void File(BaseFileStatus* file);
	std::string Name();
	OcrResult();
	~OcrResult();
};
