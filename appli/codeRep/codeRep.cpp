#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

void configureConsoleEncoding() {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
}

bool isValidKey(const std::string& key) {
    if (key.empty() || key.size() > 8) {
        return false;
    }

    for (unsigned char character : key) {
        if (!std::isalnum(character)) {
            return false;
        }
    }
    return true;
}

std::vector<unsigned char> readBinaryFile(const fs::path& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("impossible d'ouvrir le fichier source");
    }

    return std::vector<unsigned char>(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>());
}

void writeBinaryFile(const fs::path& filePath, const std::vector<unsigned char>& data) {
    fs::create_directories(filePath.parent_path());
    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    if (!file) {
        throw std::runtime_error("impossible d'ouvrir le fichier cible");
    }

    file.write(reinterpret_cast<const char*>(data.data()),
               static_cast<std::streamsize>(data.size()));
    if (!file) {
        throw std::runtime_error("erreur d'ecriture du fichier cible");
    }
}

int hexValue(char character) {
    if (character >= '0' && character <= '9') {
        return character - '0';
    }
    if (character >= 'a' && character <= 'f') {
        return character - 'a' + 10;
    }
    if (character >= 'A' && character <= 'F') {
        return character - 'A' + 10;
    }
    return -1;
}

std::vector<unsigned char> codeData(const std::vector<unsigned char>& input,
                                    const std::string& key) {
    std::ostringstream output;
    output << std::hex << std::setfill('0');

    for (std::size_t index = 0; index < input.size(); ++index) {
        const unsigned char coded = static_cast<unsigned char>(
            input[index] ^ static_cast<unsigned char>(key[index % key.size()]));
        output << std::setw(2) << static_cast<int>(coded);
    }

    const std::string text = output.str();
    return std::vector<unsigned char>(text.begin(), text.end());
}

std::vector<unsigned char> decodeData(const std::vector<unsigned char>& input,
                                      const std::string& key) {
    std::string hexText;
    hexText.reserve(input.size());

    for (unsigned char character : input) {
        if (!std::isspace(character)) {
            hexText.push_back(static_cast<char>(character));
        }
    }

    if (hexText.size() % 2 != 0) {
        throw std::runtime_error("fichier code invalide : nombre impair de caracteres");
    }

    std::vector<unsigned char> output;
    output.reserve(hexText.size() / 2);

    for (std::size_t index = 0; index < hexText.size(); index += 2) {
        const int high = hexValue(hexText[index]);
        const int low = hexValue(hexText[index + 1]);
        if (high < 0 || low < 0) {
            throw std::runtime_error("fichier code invalide : caractere non hexadecimal");
        }

        const unsigned char coded = static_cast<unsigned char>((high << 4) | low);
        const unsigned char decoded = static_cast<unsigned char>(
            coded ^ static_cast<unsigned char>(key[(index / 2) % key.size()]));
        output.push_back(decoded);
    }

    return output;
}

bool transformFile(const fs::path& sourceFile,
                   const fs::path& targetFile,
                   const std::string& mode,
                   const std::string& key) {
    try {
        const std::vector<unsigned char> input = readBinaryFile(sourceFile);
        const std::vector<unsigned char> output =
            mode == "cod" ? codeData(input, key) : decodeData(input, key);
        writeBinaryFile(targetFile, output);
        std::cout << sourceFile << " -> " << targetFile << "\n";
        return true;
    } catch (const std::exception& error) {
        std::cerr << "Erreur avec " << sourceFile << " : " << error.what() << "\n";
        return false;
    }
}

int main(int argc, char* argv[]) {
    configureConsoleEncoding();

    if (argc != 4) {
        std::cerr << "Usage : codeRep <repertoire_source> <repertoire_cible> "
                  << "[cod|dec]\n";
        return 1;
    }

    const fs::path sourceRoot = fs::absolute(argv[1]).lexically_normal();
    const fs::path targetRoot = fs::absolute(argv[2]).lexically_normal();
    const std::string mode = argv[3];

    if (mode != "cod" && mode != "dec") {
        std::cerr << "Mode invalide : utiliser cod ou dec.\n";
        return 1;
    }

    std::string key;
    do {
        std::cout << "Cle de codage (1 a 8 chiffres et/ou lettres) : ";
        if (!std::getline(std::cin, key)) {
            std::cerr << "Impossible de lire la cle.\n";
            return 1;
        }
        if (!isValidKey(key)) {
            std::cerr << "Cle invalide : utiliser 1 a 8 chiffres et/ou lettres.\n";
        }
    } while (!isValidKey(key));

    std::error_code error;
    if (!fs::is_directory(sourceRoot, error)) {
        std::cerr << "Le repertoire source n'existe pas ou n'est pas un repertoire : "
                  << sourceRoot << "\n";
        return 1;
    }

    fs::create_directories(targetRoot, error);
    if (error) {
        std::cerr << "Impossible de creer le repertoire cible : "
                  << error.message() << "\n";
        return 1;
    }

    fs::recursive_directory_iterator entries(
        sourceRoot, fs::directory_options::skip_permission_denied, error);
    const fs::recursive_directory_iterator end;
    int successCount = 0;
    int errorCount = 0;

    for (; entries != end; entries.increment(error)) {
        if (error) {
            std::cerr << "Erreur de parcours : " << error.message() << "\n";
            error.clear();
            ++errorCount;
            continue;
        }

        const fs::directory_entry& entry = *entries;
        if (!entry.is_regular_file(error)) {
            error.clear();
            continue;
        }

        const fs::path relativePath = fs::relative(entry.path(), sourceRoot, error);
        if (error) {
            std::cerr << "Erreur de chemin relatif pour " << entry.path()
                      << " : " << error.message() << "\n";
            error.clear();
            ++errorCount;
            continue;
        }

        if (transformFile(entry.path(), targetRoot / relativePath, mode, key)) {
            ++successCount;
        } else {
            ++errorCount;
        }
    }

    std::cout << successCount << " fichier(s) traite(s)";
    if (errorCount > 0) {
        std::cout << ", " << errorCount << " erreur(s)";
    }
    std::cout << ".\n";

    return errorCount == 0 ? 0 : 1;
}
