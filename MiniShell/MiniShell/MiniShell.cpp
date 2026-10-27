#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <algorithm>
#include <io.h>
#include <fcntl.h>

BOOL WINAPI CtrlHandler(DWORD type) {
	return (type == CTRL_C_EVENT) ? TRUE : FALSE;
}

bool IsCmdBuiltin(const std::string& name) {
	static const std::vector<std::string> list = {
		"assoc","cls","color","copy","date","del","dir","echo","erase","md","mkdir",
		"mklink","move","path","rd","ren","rename","rmdir","set","time","title","type","ver","vol"
	};
	return std::find(list.begin(), list.end(), name) != list.end();
}

void RunExternal(const std::string& line, const std::string& firstToken) {

	std::string commandLine = line;

	if (IsCmdBuiltin(firstToken))
		commandLine = "cmd.exe /d /s /s \"" + line + "\"";

	std::vector<char> buffer(line.begin(), line.end());
	buffer.push_back('\0');

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	BOOL ok = CreateProcessA(
		nullptr,
		buffer.data(),
		nullptr,
		nullptr,
		FALSE,
		0,
		nullptr,
		nullptr,
		&si, &pi
	);

	if (!ok) {
		DWORD err = GetLastError();
		if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
			std::cout << line << ": команда не найдена\n";
		}
		else {
			std::cout << line << ": ошибка запуска, код " << err << "\n";
		}

		return;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

std::vector<std::string> Tokenize(const std::string& line) {
	std::vector<std::string> tokens;
	std::string current;
	bool inQuotes = false;
	bool hasToken = false;

	for (char ch : line) {
		if (ch == '"') {
			inQuotes = !inQuotes;

			hasToken = true;
		}
		else if (std::isspace(static_cast<unsigned char>(ch)) && !inQuotes) {
			if (hasToken) {
				tokens.push_back(current);
				current.clear();
				hasToken = false;
			}
		}
		else {
			current.push_back(ch);
			hasToken = true;
		}
	}
	if (hasToken) tokens.push_back(current);
	return tokens;
}

std::string GetCurrentDir() {
	DWORD size = GetCurrentDirectoryA(0, nullptr);

	if (size == 0) return "";
	std::string buf(size, '\0');
	DWORD len = GetCurrentDirectoryA(size, &buf[0]);
	buf.resize(len);
	return buf;
}

std::string Trim(const std::string& s) {
	size_t b = s.find_first_not_of(" \t\r\n");
	if (b == std::string::npos) return "";
	size_t e = s.find_last_not_of(" \t\r\n");
	return s.substr(b, e - b + 1);
}


int main()
{
	setlocale(LC_ALL, "RU");

	SetConsoleCtrlHandler(CtrlHandler, TRUE);

	std::string line;

	while (true) {
		
		std::cout << "myshell> " << std::flush;
		if (!std::getline(std::cin, line)) break;

		line = Trim(line);
		if (line.empty()) continue;

		std::cout << "Вы ввели: " << line << "\n";

		auto tokens = Tokenize(line);
		if (tokens.empty()) continue;
		const std::string& cmd = tokens[0];

		if (cmd == "exit") {
			break;
		}
		else if (cmd == "help") {
			std::cout << "Команды: help, pwd, cd [путь], exit\n";
		} 
		else if (cmd == "pwd") {
			std::cout << GetCurrentDir() << "\n";
		}
		else if (cmd == "cd") {
			if (tokens.size() == 1) {
				std::cout << GetCurrentDir() << "\n";
			}
			else if (!SetCurrentDirectoryA(tokens[1].c_str())) {
				std::cout << "cd: не удалось перейти в \"" << tokens[1] << "\"\n";
			}
		}
		else {
			RunExternal(line, tokens[0]);
		}
	}
	return 0;
}

