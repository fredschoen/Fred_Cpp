#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <windows.h> // Pour SetConsoleOutputCP et SetConsoleCP
#include <filesystem>
#include <vector>
#include <algorithm>

using namespace std;
namespace fs = std::filesystem;

string bottom() {

    string sRetour;
    
	// Construire le chemin du fichier
    fs::path file_path = fs::current_path() / "zParam" / "bottom.txt";

    // Ouvrir le fichier
    std::ifstream file(file_path);

    // Vérifier si le fichier est ouvert
    if (!file.is_open()) {
        std::cerr << "Erreur : Impossible d'ouvrir le fichier " << file_path << std::endl;
        return sRetour;
    }

    // Lire et afficher le contenu ligne par ligne
    std::string line;
    while (std::getline(file, line)) {
        sRetour += line + "\n";
    }

    // Fermer le fichier
    file.close();
    return sRetour;
}

string convertQuotedUrlsToLinks(string sTexte) {
    string sRetour;
    size_t positionDebut = 0;
    size_t crochetOuvrant = sTexte.find('[', positionDebut);

    while (crochetOuvrant != string::npos) {
        size_t separateur = sTexte.find(';', crochetOuvrant + 1);
        size_t crochetFermant = sTexte.find(']', separateur + 1);

        if (separateur == string::npos || crochetFermant == string::npos) {
            break;
        }

        sRetour += sTexte.substr(positionDebut, crochetOuvrant - positionDebut);
        sRetour += "--INF--a href=--QUOTE--";
        sRetour += sTexte.substr(crochetOuvrant + 1, separateur - crochetOuvrant - 1);
        sRetour += "--QUOTE-- target=--QUOTE--_blank--QUOTE----SUP--";
        sRetour += sTexte.substr(separateur + 1, crochetFermant - separateur - 1);
        sRetour += "--INF--/a--SUP--";

        positionDebut = crochetFermant + 1;
        crochetOuvrant = sTexte.find('[', positionDebut);
    }

    sRetour += sTexte.substr(positionDebut)+"\n";
    return sRetour;
}

//============================================================================
string replaceString(string sTexte, string sCode, string sDecode) {
// objectif: trouver la chaine "sCode" dans le texte "sTexte" et le remplacer par "sDecode"
//============================================================================

	//cout<<"replaceString, in  : " << sTexte << "," << sCode << "," << sDecode << "," << endl;
	std::string sRetour=sTexte;
	std::size_t found =sRetour.find(sCode);
	while(found!=std::string::npos) {
		sRetour.replace(found, sCode.length(), sDecode);
		found =sRetour.find(sCode, found + sDecode.length()); //pour eviter boucle infinie
	}
	//cout<<"replaceString, out : " << sRetour << endl;
return sRetour;
}



void transcodeToHTML(const std::string& inputFilePath, const std::string& outputFilePath,
                    const std::string& pageLib, const std::string& previousPageRef,
                    const std::string& nextPageRef, size_t documentNumber,
                    size_t documentCount) {
    std::ifstream inFile(inputFilePath);
    std::ofstream outFile(outputFilePath);

    if (!inFile.is_open()) {
        std::cerr << "Erreur : Impossible d'ouvrir le fichier d'entrée : " << inputFilePath << std::endl;
        return;
    }

    if (!outFile.is_open()) {
        std::cerr << "Erreur : Impossible de créer le fichier de sortie : " << outputFilePath << std::endl;
        return;
    }

    std::string line;
	
    // Début du document HTML avec le <head> personnalisé
    outFile << "<!DOCTYPE html>\n<html>\n<head>\n";
    outFile << "  <meta charset=\"UTF-8\" />\n";
    outFile << "  <title>" << pageLib << "</title>\n";
    outFile << "  <link href=\"data/style.css\" rel=\"stylesheet\"/>\n";
    outFile << "</head>\n<body>\n";



	// Envelopper le <h1> dans la structure de tableau
	outFile << "<table style=\"width: 100%; border-collapse: collapse; margin-bottom: 1em;\">\n";
	outFile << "  <tr>\n";
	outFile << "    <td style=\"width: 5%; text-align: center; padding: 0.5em; border: 1px solid #ccc;\">\n";
    outFile << "      <a href=\"index.html\" style=\"display: inline-block;\">\n";
	outFile << "        <img src=\"../home.svg\" alt=\"Accueil\" style=\"width: 32px; height: 32px;\">\n";
	outFile << "      </a>\n";
	outFile << "    </td>\n";
    outFile << "    <td style=\"width: 80%; text-align: center; padding: 0.5em; border: 1px solid #ccc;\">\n";
	outFile << "      <h1>" << pageLib << "</h1>\n";
	outFile << "    </td>\n";
    outFile << "    <td style=\"width: 5%; text-align: center; padding: 0.5em; border: 1px solid #ccc;\">\n";
    outFile << "      <a href=\"" << previousPageRef << "\" style=\"display: inline-block;\">\n";
	outFile << "        <img src=\"../left-arrow.svg\" alt=\"Retour\" style=\"width: 32px; height: 32px;\">\n";
	outFile << "      </a>\n";
	outFile << "    </td>\n";
    outFile << "    <td class=\"document-counter\" style=\"width: 5%; text-align: center; padding: 0.5em; border: 1px solid #ccc;\">\n";
    outFile << "      " << documentNumber << "/" << documentCount << "\n";
    outFile << "    </td>\n";
	outFile << "    <td style=\"width: 5%; text-align: center; padding: 0.5em; border: 1px solid #ccc;\">\n";
    outFile << "      <a href=\"" << nextPageRef << "\" style=\"display: inline-block;\">\n";
	outFile << "        <img src=\"../right-arrow.svg\" alt=\"Suivant\" style=\"width: 32px; height: 32px;\">\n";
	outFile << "      </a>\n";
	outFile << "    </td>\n";
	outFile << "  </tr>\n";
	outFile << "</table>\n";			
	outFile << "<br>\n";			



    while (std::getline(inFile, line)) {
        if (line.empty()) {
            continue; // Ignorer les lignes vides
        }

        line=convertQuotedUrlsToLinks(line);

        line=replaceString(line, ";", "."); //commencer par cette simplification perso, sinon le reste est KO !!
        line=replaceString(line, "&", "&amp;"); //commencer par le & , sinon le reste est KO !!
		line=replaceString(line, "'", "&apos;");
		line=replaceString(line, "\"", "&quot;");

		line=replaceString(line, "à", "&agrave;");
		line=replaceString(line, "â", "&acirc;");
		line=replaceString(line, "ä", "&auml;");
		line=replaceString(line, "ç", "&ccedil;");
		line=replaceString(line, "é", "&eacute;");
		line=replaceString(line, "è", "&egrave;");
		line=replaceString(line, "ê", "&ecirc;");
		line=replaceString(line, "ë", "&euml;");
		line=replaceString(line, "î", "&icirc;");
		line=replaceString(line, "ï", "&iuml;");
		line=replaceString(line, "ô", "&ocirc;");
		line=replaceString(line, "ö", "&ouml;");
		line=replaceString(line, "ù", "&ugrave;");
		line=replaceString(line, "û", "&ucirc;");
		line=replaceString(line, "ü", "&uuml;");
		line=replaceString(line, "ÿ", "&yuml;");
		line=replaceString(line, "À", "&Agrave;");
		line=replaceString(line, "Â", "&Acirc;");
		line=replaceString(line, "Ä", "&Auml;");
		line=replaceString(line, "Ç", "&Ccedil;");
		line=replaceString(line, "É", "&Eacute;");
		line=replaceString(line, "È", "&Egrave;");
		line=replaceString(line, "Ê", "&Ecirc;");
		line=replaceString(line, "Ë", "&Euml;");
		line=replaceString(line, "Î", "&Icirc;");
		line=replaceString(line, "Ï", "&Iuml;");
		line=replaceString(line, "Ô", "&Ocirc;");
		line=replaceString(line, "Ö", "&Ouml;");
		line=replaceString(line, "Ù", "&Ugrave;");
		line=replaceString(line, "Û", "&Ucirc;");
		line=replaceString(line, "Ü", "&Uuml;");
		line=replaceString(line, "«", "&laquo;");
		line=replaceString(line, "»", "&raquo;");
		line=replaceString(line, "<", "&lt;");
		line=replaceString(line, ">", "&gt;");

        line=replaceString(line, "--INF--", "<");
		line=replaceString(line, "--SUP--", ">");
		line=replaceString(line, "--QUOTE--", "\"");

	
	
	    // Remplacer tous les \r\n, \n, \r par un espace
       std::regex newline_regex("\\r\\n|\\n|\\r");
       std::string clearedLine = std::regex_replace(line, newline_regex, "");
	
	
        if (clearedLine == "-") {
			outFile << "<br>\n";			
            outFile << "<hr>\n";
			outFile << "<br>\n";			

        } else if (!clearedLine.empty() && clearedLine.front() == '#') {
			outFile << "<h2>" << line.substr(1) << "</h2>\n";

        } else {
			std::cout << "b clearedLine=\"" << clearedLine << "\"" << std::endl;
            outFile << "<p>" << line << "</p>\n";
        }
    }

    // Fin du document HTML
	outFile << "<table style=\"width: 100%; border-collapse: collapse; margin-bottom: 1em;\">\n";
	outFile << "  <tr>\n";
	outFile << "    <td style=\"width: 5%; text-align: center; padding: 0.5em; border: 1px solid #ccc;\">\n";
    outFile << "      <a href=\"index.html\" style=\"display: inline-block;\">\n";
	outFile << "        <img src=\"../home.svg\" alt=\"Accueil\" style=\"width: 32px; height: 32px;\">\n";
	outFile << "      </a>\n";
	outFile << "    </td>\n";
    outFile << "    <td style=\"width: 80%; text-align: center; padding: 0.5em; border: 1px solid #ccc;\">\n";
	outFile << "    </td>\n";
    outFile << "    <td style=\"width: 5%; text-align: center; padding: 0.5em; border: 1px solid #ccc;\">\n";
    outFile << "      <a href=\"" << previousPageRef << "\" style=\"display: inline-block;\">\n";
	outFile << "        <img src=\"../left-arrow.svg\" alt=\"Retour\" style=\"width: 32px; height: 32px;\">\n";
	outFile << "      </a>\n";
	outFile << "    </td>\n";
    outFile << "    <td class=\"document-counter\" style=\"width: 5%; text-align: center; padding: 0.5em; border: 1px solid #ccc;\">\n";
    outFile << "      " << documentNumber << "/" << documentCount << "\n";
    outFile << "    </td>\n";
	outFile << "    <td style=\"width: 5%; text-align: center; padding: 0.5em; border: 1px solid #ccc;\">\n";
    outFile << "      <a href=\"" << nextPageRef << "\" style=\"display: inline-block;\">\n";
	outFile << "        <img src=\"../right-arrow.svg\" alt=\"Suivant\" style=\"width: 32px; height: 32px;\">\n";
	outFile << "      </a>\n";
	outFile << "    </td>\n";
	outFile << "  </tr>\n";
	outFile << "</table>\n";			
    outFile << "</body>\n</html>";

    inFile.close();
    outFile.close();
    std::cout << "Transcodage terminé : " << outputFilePath << " généré avec succès !" << std::endl;
}

int trtDirTxt() {
    fs::path txtDirPath = fs::current_path() / "txt";
    fs::path htmlDirPath = fs::current_path() / "html";

    if (fs::exists(htmlDirPath) && fs::is_directory(htmlDirPath)) {
        for (const auto& entry : fs::directory_iterator(htmlDirPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".html") {
                std::error_code error;
                fs::remove(entry.path(), error);
                if (error) {
                    std::cerr << "Erreur : Impossible de supprimer le fichier HTML : "
                              << entry.path() << " (" << error.message() << ")" << std::endl;
                    return 0;
                }
            }
        }
    }

    fs::path indexPath = htmlDirPath / "index.html";
    std::ofstream indexFile(indexPath.string());

    if (!indexFile.is_open()) {
        std::cerr << "Erreur : Impossible de créer le fichier index de sortie : " << indexPath.string() << std::endl;
        return 0;
    }

    // Début du document index HTML avec le <head> personnalisé
    indexFile << "<!DOCTYPE html>\n<html>\n<head>\n";
    indexFile << "  <meta charset=\"UTF-8\" />\n";
    indexFile << "  <title>Liste des textes</title>\n";
    indexFile << "  <link href=\"data/style.css\" rel=\"stylesheet\"/>\n";
    indexFile << "</head>\n<body>\n";

    indexFile << "	<table style=\"width: 100%; border-collapse: collapse; margin-bottom: 1em;\">\n";
    indexFile << "		<tr>\n";
    indexFile << "		  <td style=\"width: 80%; text-align: center; padding: 0.5em; border: 1px solid #ccc;\">\n";
    indexFile << "			<h1>Liste des textes</h1>\n";
    indexFile << "		  </td>\n";
    indexFile << "		</tr>\n";
    indexFile << "	</table>\n";



    std::vector<fs::path> txtFiles;
    if (fs::exists(txtDirPath) && fs::is_directory(txtDirPath)) {
        for (const auto& entry : fs::directory_iterator(txtDirPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                txtFiles.push_back(entry.path());
            }
        }
        std::sort(txtFiles.begin(), txtFiles.end(), [](const fs::path& left, const fs::path& right) {
            return left.filename().string() < right.filename().string();
        });
    }

    std::string inputFilePath, outputFilePath;
	std::string pageRef, pageLib, temp;

    if (!txtFiles.empty()) {
        for (size_t fileIndex = 0; fileIndex < txtFiles.size(); ++fileIndex) {
                const auto& entry = txtFiles[fileIndex];
                //std::cout << entry.path().filename().string() << std::endl;
                std::cout << entry.string() << std::endl;
				
				inputFilePath=entry.string();
				outputFilePath=replaceString(entry.string(),".txt",".html");
				outputFilePath=replaceString(outputFilePath,"\\txt\\","\\html\\");
				pageLib=replaceString(entry.filename().string(),".txt","");
				temp=replaceString(pageLib," ","_");

				pageRef=replaceString(entry.filename().string(),".txt",".html");
				pageRef=replaceString(pageRef," ","_");
				const size_t previousIndex = (fileIndex == 0) ? txtFiles.size() - 1 : fileIndex - 1;
				const size_t nextIndex = (fileIndex + 1) % txtFiles.size();
				std::string previousPageRef = replaceString(txtFiles[previousIndex].filename().string(), ".txt", ".html");
				std::string nextPageRef = replaceString(txtFiles[nextIndex].filename().string(), ".txt", ".html");
				previousPageRef = replaceString(previousPageRef, " ", "_");
				nextPageRef = replaceString(nextPageRef, " ", "_");

				outputFilePath=replaceString(outputFilePath,pageLib,temp);
				pageLib=pageLib.substr(8, 99);

				indexFile << "	<p class=\"index\">\n";
				indexFile << "		<a href=\"" << pageRef << "\">" << pageLib << "</a><br>\n";
				indexFile << "	</p>\n";
                transcodeToHTML(inputFilePath, outputFilePath, pageLib, previousPageRef, nextPageRef,
                    fileIndex + 1, txtFiles.size());
        }
    } else {
        std::cerr << "Directory 'txt' does not exist." << std::endl;
    }


    // Fin du document index HTML, inclure eventuel bas de page
	indexFile << bottom();

    // Fin du document index HTML
    indexFile << "</body>\n</html>";
    indexFile.close();
    std::cout << "Transcodage terminé : " << indexPath.string() << " généré avec succès !" << std::endl;

    return 0;
}

int main() {
    // Forcer la console Windows à utiliser UTF-8
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

	trtDirTxt();

    return 0;
}
