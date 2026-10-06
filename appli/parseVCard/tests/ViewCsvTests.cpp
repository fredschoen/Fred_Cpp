// Depuis appli/parseVCard :
// g++ -std=c++11 -Wall -Wextra tests/ViewCsvTests.cpp -o ViewCsvTests.exe
#define main viewCsvMain
#include "../ViewCsv.cpp"
#undef main
#include <stdexcept>

static void verifier(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

static void tester() {
    std::vector<std::string> brutes;
    brutes.push_back("Nom;Note;;Ville\r\n");
    brutes.push_back("Alice;\"Rencontre; lundi\";cache;Paris\r\n");
    brutes.push_back("B\xC3\xA9" "atrice;Bonjour;secret;Lyon\n");
    brutes.push_back("ALICE;Autre;;Nice");
    std::vector<std::vector<std::string> > lignes;
    for (size_t i = 0; i < brutes.size(); ++i) {
        std::string ligne = brutes[i];
        while (!ligne.empty() && (ligne.back() == '\r' || ligne.back() == '\n')) ligne.pop_back();
        lignes.push_back(decouper(ligne, ';'));
    }
    size_t numero = 1;
    verifier(trouverLigne(lignes, numero, "alice", true) && numero == 3, "Resultat suivant incorrect");
    verifier(trouverLigne(lignes, numero, "alice", true) && numero == 1, "Bouclage suivant incorrect");
    verifier(trouverLigne(lignes, numero, "alice", false) && numero == 3, "Bouclage precedent incorrect");
    verifier(trouverLigne(lignes, numero, "B\xC3\x89" "ATRICE", true) && numero == 2, "Recherche UTF-8/casse incorrecte");
    verifier(!trouverLigne(lignes, numero, "absent", true) && numero == 2, "Recherche absente deplace la fiche");
    verifier(!trouverLigne(lignes, numero, "Nom", true), "Recherche inclut l'entete");
    verifier(trouverChamp(lignes[0], " note ") == 1, "Selection de champ incorrecte");
    verifier(trouverChamp(lignes[0], "") == lignes[0].size(), "Champ vide accepte");
    verifier(trouverChamp(std::vector<std::string>{"Nom", "NOM"}, "nom") == 2, "Intitule ambigu accepte");
    const std::vector<std::string> originales = brutes;
    const std::string valeur = "Cl\xC3\xA9ment; dit \"bonjour\"";
    modifierChamp(lignes, brutes, 1, 1, valeur, ';');
    verifier(lignes[1][1] == valeur, "Valeur memoire incorrecte");
    verifier(decouper(brutes[1].substr(0, brutes[1].size() - 2), ';') == lignes[1], "Guillemets/separateurs mal encodes");
    verifier(lignes[1][2] == "cache", "Colonne masquee perdue");
    verifier(brutes[0] == originales[0] && brutes[2] == originales[2] && brutes[3] == originales[3], "Lignes intactes alterees");
    verifier(brutes[1].substr(brutes[1].size() - 2) == "\r\n", "CRLF perdu");
    modifierChamp(lignes, brutes, 2, 1, "", ';');
    verifier(lignes[2][1].empty() && brutes[2].back() == '\n', "Valeur vide ou LF perdu");
    modifierChamp(lignes, brutes, 3, 3, "Toulon", ';');
    verifier(brutes[3].back() != '\n', "Fin de ligne ajoutee");
    const std::string entete = brutes[0];
    modifierChamp(lignes, brutes, 0, 0, "interdit", ';');
    verifier(brutes[0] == entete, "Entete modifiee");
    verifier(versUtf8(L"\u00E9\u00E8\u00E7") == "\xC3\xA9\xC3\xA8\xC3\xA7", "Saisie Unicode incorrecte");

    char dossier[MAX_PATH], fichier[MAX_PATH];
    verifier(GetTempPathA(MAX_PATH, dossier) != 0 && GetTempFileNameA(dossier, "csv", 0, fichier) != 0,
             "Fichier temporaire inaccessible");
    try {
        std::string message;
        verifier(sauverCsv(fichier, brutes, message), "Sauvegarde impossible");
        std::ifstream entree(fichier, std::ios::binary);
        const std::string contenu((std::istreambuf_iterator<char>(entree)), std::istreambuf_iterator<char>());
        entree.close();
        verifier(contenu == brutes[0] + brutes[1] + brutes[2] + brutes[3], "Sauvegarde ne conserve pas les donnees");
    } catch (...) {
        DeleteFileA(fichier);
        throw;
    }
    DeleteFileA(fichier);
    numero = 1;
    supprimerLigne(lignes, brutes, numero);
    verifier(trouverLigne(lignes, numero, "alice", true) && numero == 2, "Recherche apres suppression incorrecte");
    const std::vector<std::vector<std::string> > sansDonnees(1, lignes[0]);
    numero = 0;
    verifier(!trouverLigne(sansDonnees, numero, "alice", true), "Recherche sans donnees incorrecte");
}

int main() {
    try {
        tester();
    } catch (const std::exception& erreur) {
        std::cerr << "ECHEC : " << erreur.what() << '\n';
        return 1;
    }
    std::cout << "Tests recherche, modification CSV et UTF-8 : OK\n";
    return 0;
}
