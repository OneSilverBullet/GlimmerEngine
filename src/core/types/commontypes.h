#pragma once
#include <DirectXMath.h>

using namespace DirectX;


class DWParam
{
public:
	DWParam(float v): Float(v){}
	DWParam(uint32_t v):Uint(v){}
	DWParam(int v): Int(v){}

	void operator=(float v) { Float = v; }
	void operator=(uint32_t v) { Uint = v; }
	void operator=(int v) { Int = v; }

	union {
		float Float;
		uint32_t Uint;
		int Int;
	};
};

std::wstring StringToWstring(std::string wstr)
{
	std::wstring res;
	int len = MultiByteToWideChar(CP_ACP, 0, wstr.c_str(), wstr.size(), nullptr, 0);
	if (len < 0) {
		return res;
	}
	wchar_t* buffer = new wchar_t[len + 1];
	if (buffer == nullptr) {
		return res;
	}
	MultiByteToWideChar(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len);
	buffer[len] = '\0';
	res.append(buffer);
	delete[] buffer;
	return res;
}

std::string WStringToString(const std::wstring& wstr) {
	std::vector<char> buffer(wstr.size() * 4); // UTF-8 characters can take up to 4 bytes
	wcstombs(buffer.data(), wstr.c_str(), buffer.size());
	return std::string(buffer.data());
}

std::vector<std::string> ListFilesInDirectory(const std::string& path) {
	std::string searchPath = path + "\\*";
	std::vector<std::string> results;
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile(searchPath.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		return results;
	}
	else {
		do {
			if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				results.push_back(findFileData.cFileName);
			}
		} while (FindNextFile(hFind, &findFileData) != 0);
		FindClose(hFind);
	}
}



