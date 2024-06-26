#include <Zut/ZxNative.h>
#include <Wx32/APIs.h>


namespace Zqf::Zut::ZxNative::Sys
{
	auto InitConsoleEncoding() -> void
	{
		::SetConsoleCP(CP_UTF8);       // input code page
		::SetConsoleOutputCP(CP_UTF8); // output code page
	}
}


namespace Zqf::Zut::ZxNative::Str
{
	static auto WideStrViewToU16StrView(const std::wstring_view& wsStr) -> std::u16string_view
	{
		return { reinterpret_cast<const char16_t*>(wsStr.data()), wsStr.size() };
	}

	static auto U16StrViewToWideStrView(const std::u16string_view& u16Str) -> std::wstring_view
	{
		return { reinterpret_cast<const wchar_t*>(u16Str.data()), u16Str.size() };
	}

	static auto WideStrToU16Str(Wx32::WideStr_t&& rfWideStr) -> U16Str_t
	{
		return
		{
			WideStrViewToU16StrView(rfWideStr.first),
			std::unique_ptr<char16_t[]>{ reinterpret_cast<char16_t*>(rfWideStr.second.release()) }
		};
	}

	auto CvtSafe(const std::string_view msStr, const CodePage eCodePage) -> U16Str_t
	{
		return WideStrToU16Str(Wx32::Utils::StrCvtSafe(msStr, eCodePage));
	}

	auto CvtSafe(const std::u16string_view u16Str, const CodePage eCodePage) -> U8Str_t
	{
		return Wx32::Utils::StrCvtSafe(U16StrViewToWideStrView(u16Str), eCodePage);
	}

	auto CvtForce(const std::string_view msStr, const CodePage eCodePage) -> U16Str_t
	{
		return WideStrToU16Str(Wx32::Utils::StrCvtForce(msStr, eCodePage));
	}

	auto CvtForce(const std::u16string_view u16Str, const CodePage eCodePage) -> U8Str_t
	{
		return Wx32::Utils::StrCvtForce(U16StrViewToWideStrView(u16Str), eCodePage);
	}

	auto CvtForce(const std::string_view msStr, std::span<char16_t> spBuffer, const CodePage eCodePage) -> std::u16string_view
	{
		return WideStrViewToU16StrView(Wx32::Utils::StrCvtForce(msStr, std::span<wchar_t>{ reinterpret_cast<wchar_t*>(spBuffer.data()), spBuffer.size() }, eCodePage));
	}

	auto CvtForce(const std::u16string_view u16Str, std::span<char> spBuffer, const CodePage eCodePage) -> std::string_view
	{
		return Wx32::Utils::StrCvtForce(U16StrViewToWideStrView(u16Str), spBuffer, eCodePage);
	}

	auto Cmpni(const std::string_view msStr0, const std::string_view msStr1, size_t nMaxCount) -> size_t
	{
		return static_cast<size_t>(::_strnicmp(msStr0.data(), msStr1.data(), nMaxCount));
	}
}


namespace Zqf::Zut::ZxNative::File
{
	auto Open(const std::string_view msPath, OpenMod eMode) -> std::optional<SysID>
	{
		DWORD access{}, attributes{};
		switch (eMode)
		{
		case Zut::OpenMod::ReadSafe:
			access = GENERIC_READ;
			attributes = OPEN_EXISTING;
			break;
		case Zut::OpenMod::WriteSafe:
			access = GENERIC_WRITE;
			attributes = CREATE_NEW;
			break;
		case Zut::OpenMod::WriteForce:
			access = GENERIC_WRITE;
			attributes = CREATE_ALWAYS;
			break;
		}

		return Wx32::API::CreateFileU8(msPath, { attributes, access, FILE_SHARE_READ });
	}

	auto Close(SysID hFile) -> bool
	{
		return Wx32::API::CloseHandle(hFile);
	}

	auto Flush(SysID hFile) -> bool
	{
		return Wx32::API::FlushFileBuffers(hFile);
	}

	auto GetSize(SysID hFile) -> std::optional<uint64_t>
	{
		return Wx32::API::GetFileSizeEx(hFile);
	}

	auto GetPtr(SysID hFile) -> std::optional<uint64_t>
	{
		return Wx32::API::SetFilePointerEx(hFile, 0, Wx32::MoveWayEnum::Cur);
	}

	auto SetPtr(SysID hFile, uint64_t nOffset, MoveWay eWay) -> std::optional<uint64_t>
	{
		return Wx32::API::SetFilePointerEx(hFile, nOffset, eWay);
	}

	auto Read(SysID hFile, const std::span<uint8_t> spBuffer) -> std::optional<size_t>
	{
		return Wx32::API::ReadFile(hFile, spBuffer, nullptr);
	}

	auto Write(SysID hFile, const std::span<const uint8_t> spBuffer) -> std::optional<size_t>
	{
		return Wx32::API::WriteFile(hFile, spBuffer, nullptr);
	}
}


namespace Zqf::Zut::ZxNative::Fs
{
	auto Fs::WalkCreate(const std::string_view msPath) -> std::optional<SysID>
	{
		WIN32_FIND_DATAW find_data;
		HANDLE hFind = ::FindFirstFileW(Wx32::Utils::ApiStrCvt(msPath).first.data(), &find_data);
		return (hFind == INVALID_HANDLE_VALUE) ? std::nullopt : std::optional<SysID>{ hFind };
	}

	auto Fs::WalkClose(SysID hFind) -> bool
	{
		return ::FindClose(hFind) != FALSE;
	}

	auto WalkNextFile(SysID hFind, std::span<char> spBuffer) -> std::optional<std::string_view>
	{
		WIN32_FIND_DATAW find_data;
		while (::FindNextFileW(hFind, &find_data))
		{
			if (find_data.cFileName[0] == L'.')
			{
				if (find_data.cFileName[1] == L'\0')
				{
					continue;
				}
				else if (find_data.cFileName[1] == L'.')
				{
					if (find_data.cFileName[2] == L'\0')
					{
						continue;
					}
				}
			}

			if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				return Wx32::Utils::ApiStrCvt(find_data.cFileName, spBuffer);
			}
		}

		return std::nullopt;
	}

	auto WalkNextDir(SysID hFind, std::span<char> spBuffer) -> std::optional<std::string_view>
	{
		WIN32_FIND_DATAW find_data;
		while (::FindNextFileW(hFind, &find_data))
		{
			if (find_data.cFileName[0] == L'.')
			{
				if (find_data.cFileName[1] == L'\0')
				{
					continue;
				}
				else if (find_data.cFileName[1] == L'.')
				{
					if (find_data.cFileName[2] == L'\0')
					{
						continue;
					}
				}
			}

			if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				auto dir = Wx32::Utils::ApiStrCvt(find_data.cFileName, spBuffer);
				spBuffer.data()[dir.size() + 0] = '/';
				spBuffer.data()[dir.size() + 1] = '\0';
				return std::string_view{ spBuffer.data(), dir.size() + 1 };
			}
		}

		return std::nullopt;
	}

	auto Fs::SelfDir() -> U8Str_t
	{
		return Wx32::Kernel32::GetCurrentDirectoryU8();
	}

	auto Fs::SelfPath() -> U8Str_t
	{
		return Wx32::Kernel32::GetModuleFileNameU8(nullptr);
	}

	auto Fs::Exist(const std::string_view msPath) -> bool
	{
		return Wx32::Kernel32::GetFileAttributesU8(msPath) == INVALID_FILE_ATTRIBUTES ? false : true;
	}

	auto MakeDir(const std::string_view msPath) -> bool
	{
		return Wx32::Kernel32::CreateDirectoryU8(msPath, nullptr);
	}

	auto MakeDirs(const std::string_view msPath) -> bool
	{
		if (msPath.empty()) { return false; }
		if (msPath.back() != '/') { return false; }

		auto wide_str = Wx32::Utils::ApiStrCvt(msPath);
		wchar_t* path_cstr = wide_str.second.get();
		const wchar_t* path_cstr_org = path_cstr;

		while (*path_cstr++)
		{
			if (*path_cstr != L'/') { continue; }

			wchar_t tmp = *path_cstr;
			*path_cstr = {};
			if (::GetFileAttributesW(path_cstr_org) == INVALID_FILE_ATTRIBUTES)
			{
				::CreateDirectoryW(path_cstr_org, nullptr);
			}

			*path_cstr = tmp;
			path_cstr++;
		}

		return true;
	}

	auto Fs::RemoveFile(const std::string_view msPath) -> bool
	{
		return Wx32::Kernel32::DeleteFileU8(msPath);
	}

	auto Fs::CopyFile(const std::string_view msExistPath, const std::string_view msNewPath, bool isFailIfExists) -> bool
	{
		return Wx32::Kernel32::CopyFileU8(msExistPath, msNewPath, isFailIfExists);
	}

	auto Fs::GetFileSize(const std::string_view msPath) -> std::optional<uint64_t>
	{
		WIN32_FIND_DATAW find_data;
		const auto hfind = ::FindFirstFileW(Wx32::Utils::ApiStrCvt(msPath).first.data(), &find_data);
		if (hfind != INVALID_HANDLE_VALUE) { ::FindClose(hfind); }
		else { return std::nullopt; }
		uint64_t size_l = static_cast<uint64_t>(find_data.nFileSizeLow);
		uint64_t size_h = static_cast<uint64_t>(find_data.nFileSizeHigh);
		return static_cast<size_t>(size_l | (size_h << 32));
	}
}