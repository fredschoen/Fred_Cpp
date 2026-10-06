#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
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

struct FileInfo {
    fs::path path;
    std::uintmax_t size = 0;
    fs::file_time_type modificationTime;
};

std::string relativeName(const fs::path& path) {
    return path.generic_string();
}
struct CaseInsensitiveLess {
    bool operator()(const std::string& left, const std::string& right) const {
        return std::lexicographical_compare(
            left.begin(), left.end(), right.begin(), right.end(),
            [](unsigned char leftCharacter, unsigned char rightCharacter) {
                return std::tolower(leftCharacter) < std::tolower(rightCharacter);
            });
    }
};

bool isDesktopIni(const fs::path& path) {
    std::string filename = path.filename().string();
    std::transform(filename.begin(), filename.end(), filename.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::tolower(character));
                   });
    return filename == "desktop.ini";
}

using FileMap = std::map<std::string, FileInfo, CaseInsensitiveLess>;
using DirectorySet = std::set<std::string, CaseInsensitiveLess>;

FileMap listFiles(const fs::path& root) {
    FileMap files;

    if (!fs::exists(root) || !fs::is_directory(root)) {
        return files;
    }

    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file() || isDesktopIni(entry.path())) {
            continue;
        }

        const fs::path relativePath = fs::relative(entry.path(), root);
        files.emplace(relativeName(relativePath), FileInfo{
            entry.path(),
            entry.file_size(),
            entry.last_write_time()
        });
    }

    return files;
}

DirectorySet listDirectories(const fs::path& root) {
    DirectorySet directories;

    if (!fs::exists(root) || !fs::is_directory(root)) {
        return directories;
    }

    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_directory()) {
            continue;
        }

        directories.insert(relativeName(fs::relative(entry.path(), root)));
    }

    return directories;
}

std::string nowAsText() {
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm localTime{};
#ifdef _WIN32
DirectorySet allNames;
#else
    localtime_r(&now, &localTime);
#endif

    std::ostringstream result;
    result << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return result.str();
}

std::string fileTimeAsText(const fs::file_time_type& fileTime) {
    const auto systemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        fileTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    const std::time_t time = std::chrono::system_clock::to_time_t(systemTime);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif

    std::ostringstream result;
    result << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return result.str();
}

bool sameModificationTime(const fs::file_time_type& source,
                          const fs::file_time_type& target) {
    const auto difference = source >= target ? source - target : target - source;
    return difference <= std::chrono::seconds(2);
}

void writeLog(std::ofstream& log, const std::string& action,
              const std::string& relativePath, bool execution) {
    const std::string message = nowAsText() + " | "
        + (execution ? "EXEC" : "SIMU") + " | " + action + " | " + relativePath;
    log << message << '\n';
    //if (execution) {
        std::cout << message << std::endl;
    //}
}

bool copySourceFile(const FileInfo& source, const fs::path& target) {
    try {
        fs::create_directories(target.parent_path());
        fs::copy_file(source.path, target, fs::copy_options::overwrite_existing);
        fs::last_write_time(target, source.modificationTime);
        return true;
    } catch (const fs::filesystem_error& error) {
        std::cerr << "Erreur de copie de " << source.path << " vers "
                  << target << " : " << error.what() << '\n';
        return false;
    }
}

bool removeTargetFile(const fs::path& target) {
    try {
        return fs::remove(target);
    } catch (const fs::filesystem_error& error) {
        std::cerr << "Erreur de suppression de " << target << " : "
                  << error.what() << '\n';
        return false;
    }
}

bool removeTargetDirectory(const fs::path& target) {
    try {
        return fs::remove(target);
    } catch (const fs::filesystem_error& error) {
        std::cerr << "Erreur de suppression du repertoire " << target << " : "
                  << error.what() << '\n';
        return false;
    }
}

int main(int argc, char* argv[]) {
    configureConsoleEncoding();

    if (argc < 3 || argc > 4) {
        std::cerr << "Usage : iterativeSave <repertoire_source> <repertoire_cible> [simu|exec]\n";
        return 1;
    }

    const fs::path sourceRoot = fs::absolute(argv[1]).lexically_normal();
    const fs::path targetRoot = fs::absolute(argv[2]).lexically_normal();
    const std::string mode = argc == 4 ? argv[3] : "simu";
    const bool execution = mode == "exec";

    if (!execution && mode != "simu") {
        std::cerr << "Mode invalide : utiliser simu ou exec.\n";
        return 1;
    }

    if (execution) {
        std::cout << "Mode execution : confirmer le traitement (o/n) : " << std::flush;
        std::string confirmation;
        std::getline(std::cin, confirmation);
        if (confirmation != "o" && confirmation != "O") {
            std::cout << "Traitement annule.\n";
            return 0;
        }
    }

    if (!fs::exists(sourceRoot) || !fs::is_directory(sourceRoot)) {
        std::cerr << "Le repertoire source n'existe pas ou n'est pas un repertoire : "
                  << sourceRoot << '\n';
        return 1;
    }
    if (execution) {
        try {
            fs::create_directories(targetRoot);
        } catch (const fs::filesystem_error& error) {
            std::cerr << "Impossible de creer le repertoire cible : "
                      << error.what() << '\n';
            return 1;
        }
    }

    std::ofstream log("iterativeSave.log", std::ios::trunc);
    if (!log) {
        std::cerr << "Impossible d'ouvrir iterativeSave.log.\n";
        return 1;
    }

    const FileMap sourceFiles = listFiles(sourceRoot);
    const FileMap targetFiles = listFiles(targetRoot);
    const DirectorySet sourceDirectories = listDirectories(sourceRoot);
    const DirectorySet targetDirectories = listDirectories(targetRoot);
    std::set<std::string> allNames;
    for (const auto& file : sourceFiles) {
        allNames.insert(file.first);
    }
    for (const auto& file : targetFiles) {
        allNames.insert(file.first);
    }

    int differenceCount = 0;

    for (const std::string& name : sourceDirectories) {
        if (targetDirectories.find(name) != targetDirectories.end()) {
            continue;
        }

        differenceCount++;
        writeLog(log, "NOUVEAU", name + "/", execution);
        if (execution) {
            try {
                fs::create_directories(targetRoot / fs::path(name));
            } catch (const fs::filesystem_error& error) {
                std::cerr << "Erreur de creation du repertoire "
                          << targetRoot / fs::path(name) << " : "
                          << error.what() << '\n';
                return 1;
            }
        }
    }

    for (const std::string& name : allNames) {
        const auto source = sourceFiles.find(name);
        const auto target = targetFiles.find(name);
        const fs::path targetPath = targetRoot / fs::path(name);

        if (source != sourceFiles.end() && target == targetFiles.end()) {
            differenceCount++;
            writeLog(log, "NOUVEAU", name, execution);
            if (execution && !copySourceFile(source->second, targetPath)) {
                return 1;
            }
        } else if (source == sourceFiles.end() && target != targetFiles.end()) {
            differenceCount++;
            writeLog(log, "SUPPRIME", name, execution);
            if (execution && !removeTargetFile(targetPath)) {
                return 1;
            }
        } else if (source->second.size != target->second.size ||
                   !sameModificationTime(source->second.modificationTime,
                                         target->second.modificationTime)) {
            differenceCount++;
            std::string reason;
            if (source->second.size != target->second.size) {
                reason = "taille " + std::to_string(source->second.size)
                    + " / " + std::to_string(target->second.size);
            }
            if (!sameModificationTime(source->second.modificationTime,
                                      target->second.modificationTime)) {
                if (!reason.empty()) {
                    reason += "; ";
                }
                reason += "date modif "
                    + fileTimeAsText(source->second.modificationTime)
                    + " / " + fileTimeAsText(target->second.modificationTime);
            }
            writeLog(log, "MODIFIE (" + reason + ")", name, execution);
            if (execution && !copySourceFile(source->second, targetPath)) {
                return 1;
            }
        }
    }

    for (auto directory = targetDirectories.rbegin();
         directory != targetDirectories.rend(); ++directory) {
        if (sourceDirectories.find(*directory) != sourceDirectories.end()) {
            continue;
        }

        differenceCount++;
        writeLog(log, "SUPPRIME", *directory + "/", execution);
        if (execution && !removeTargetDirectory(targetRoot / fs::path(*directory))) {
            return 1;
        }
    }

    std::cout << "Nombre de differences : " << differenceCount << '\n';
    return 0;
}