#include "handle.h"
#include <string>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "windows.h"

namespace HDL
{
namespace VirtualMemory
{
	static std::string GetFormattedErrorString(DWORD _errorCode)
	{
		LPVOID buf;
		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			_errorCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPSTR)&buf,
			0,
			NULL
		);
		std::string errorString = (char*)buf;
		LocalFree(buf);

		return errorString;
	}

	size_t GetPageSize()
	{
		struct PageSizeInitializer
		{
			size_t m_value;

			PageSizeInitializer()
			{
				SYSTEM_INFO info = {};
				GetSystemInfo(&info);

				m_value = info.dwPageSize;
			}
		} static pageSize;

		return pageSize.m_value;
	}

	void* Reserve(size_t _size)
	{
		auto address = VirtualAlloc(
			nullptr, // lpAddress
			_size,
			MEM_RESERVE,
			PAGE_READWRITE
		);

		HDL_ASSERT(address != nullptr, GetFormattedErrorString(GetLastError()).c_str());

		return address;
	}

	void Release(void* _address, size_t _size)
	{
		(void)_size; // When using MEM_RELEASE, the passed size must be 0. The entire region that was reserved will be released.

		auto success = VirtualFree(
			_address,
			0,
			MEM_RELEASE
		);

		HDL_ASSERT(success, GetFormattedErrorString(GetLastError()).c_str());
	}

	bool Commit(void* _address, size_t _size)
	{
		auto address = VirtualAlloc(
			_address,
			_size,
			MEM_COMMIT,
			PAGE_READWRITE
		);

		HDL_ASSERT(address != nullptr, GetFormattedErrorString(GetLastError()).c_str());
		return address != nullptr;
	}

	void Decommit(void* _address, size_t _size)
	{
		auto success = VirtualFree(
			_address,
			_size,
			MEM_DECOMMIT
		);

		HDL_ASSERT(success, GetFormattedErrorString(GetLastError()).c_str());
	}
}
}
