#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

bool hasFilesExtension(const fs::path& filePath) {
	std::string extension = filePath.extension().string();
	for (char& character : extension) {
		character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
	}
	return extension == ".files";
}

bool isHidden(const fs::path& filePath) {
#ifdef _WIN32
	const DWORD attributes = GetFileAttributesW(filePath.c_str());
	return attributes != INVALID_FILE_ATTRIBUTES &&
		   (attributes & FILE_ATTRIBUTE_HIDDEN) != 0;
#else
	return filePath.filename().string().front() == '.';
#endif
}

int main(int argc, char* argv[]) {
	if (argc < 2 || argc > 3) {
		std::cerr << "Usage : hiddenFiles <repertoire> [exec]\n";
		return 1;
	}

	const fs::path root = fs::absolute(argv[1]).lexically_normal();
	const bool execution = argc == 3 && std::string(argv[2]) == "exec";

	if (argc == 3 && !execution) {
		std::cerr << "Mode invalide : utiliser exec.\n";
		return 1;
	}

	std::error_code error;
	if (!fs::is_directory(root, error)) {
		std::cerr << "Le repertoire n'existe pas ou n'est pas un repertoire : "
				  << root << "\n";
		return 1;
	}

	fs::recursive_directory_iterator entries(
		root, fs::directory_options::skip_permission_denied, error);
	const fs::recursive_directory_iterator end;
	std::vector<fs::path> matches;
	std::size_t count = 0;

	for (; entries != end; entries.increment(error)) {
		if (error) {
			std::cerr << "Erreur de parcours : " << error.message() << "\n";
			error.clear();
			continue;
		}

		const fs::directory_entry& entry = *entries;
		const bool isRegularFile = entry.is_regular_file(error);
		if (error) {
			error.clear();
			continue;
		}
		const bool isDirectory = entry.is_directory(error);
		if (error || (!isRegularFile && !isDirectory)) {
			error.clear();
			continue;
		}

		const fs::path filePath = entry.path();
		if (!hasFilesExtension(filePath) || !isHidden(filePath)) {
			continue;
		}

		std::cout << filePath << "\n";
		matches.push_back(filePath);
		++count;

		if (isDirectory) {
			entries.disable_recursion_pending();
		}
	}

	if (execution) {
		for (const fs::path& filePath : matches) {
			fs::remove_all(filePath, error);
			if (error) {
				std::cerr << "Erreur de suppression de " << filePath
						  << " : " << error.message() << "\n";
				error.clear();
			}
		}
	}

	std::cout << count << " element(s) .files cache(s)";
	if (execution) {
		std::cout << " supprime(s)";
	}
	std::cout << ".\n";
	return 0;
}