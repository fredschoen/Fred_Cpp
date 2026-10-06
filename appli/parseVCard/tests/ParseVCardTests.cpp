// Depuis appli/parseVCard :
// g++ -std=c++11 -Wall -Wextra tests/ParseVCardTests.cpp -o ParseVCardTests.exe
#define main parseVCardMain
#include "../ParseVCard.cpp"
#undef main

#include <windows.h>
#include <sstream>
#include <stdexcept>

static void verifier(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

static void tester() {
    const string nom = "(Resto83 SixFoursCentre poulet)";
    const string prenom = "rogert et cl\xC3\xA9ment (sylazur)";
    std::ifstream fixture("vcard-test/vcard.vcf", std::ios::binary);
    verifier(static_cast<bool>(fixture), "Fixture vcard-test/vcard.vcf introuvable");
    string ligne;
    verifier(fLireLigneVcf(fixture, ligne) && ligne == "BEGIN:VCARD", "BEGIN perdu");
    verifier(fLireLigneVcf(fixture, ligne) && ligne == "VERSION:2.1", "VERSION perdue");
    verifier(fLireLigneVcf(fixture, ligne), "Propriete N introuvable");
    fDecouperLigneVcf(ligne);
    verifier(sN1.empty() && sN2 == nom && sN3 == prenom && sN4.empty(),
             "Nom multiligne ou UTF-8 incorrect");
    verifier(fLireLigneVcf(fixture, ligne) && ligne.find("FN;") == 0, "FN absorbe par N");
    verifier(fLireLigneVcf(fixture, ligne) && ligne.find("TEL;CELL:") == 0, "TEL absorbe par FN");
    fixture.close();

    std::istringstream plis("N;ENCODING=QUOTED-PRINTABLE:;=C3= \t\r\n=A9;;\r\n"
                           "NOTE:une longue\r\n suite\r\n\tencore\r\n"
                           "END:VCARD\r\n");
    verifier(fLireLigneVcf(plis, ligne), "Lecture QP impossible");
    fDecouperLigneVcf(ligne);
    verifier(sN2 == "\xC3\xA9", "Octets UTF-8 separes par une coupure mal raccordes");
    verifier(fLireLigneVcf(plis, ligne) && ligne == "NOTE:une longuesuiteencore", "Pliage classique incorrect");
    verifier(fLireLigneVcf(plis, ligne) && ligne == "END:VCARD", "Fin de carte perdue");
    verifier(!fLireLigneVcf(plis, ligne), "Fin de fichier incorrecte");
    verifier(fTranscoderTexte("=3D=32=30 =c3=a9 =ZZ =") == "=20 \xC3\xA9 =ZZ =", "Decodage QP incorrect");
    verifier(fTranscoderTexte("Ligne1=0D=0ALigne2=0ALigne3=0DLigne4") ==
             "Ligne1 Ligne2 Ligne3 Ligne4", "Retours quoted-printable non remplaces par des espaces");
    fDecouperLigneVcf("N;CHARSET=UTF-8;ENCODING=QUOTED-PRINTABLE:;Nom=0D=0Acompose;Pr=0Aenom;;");
    verifier(sN2 == "Nom compose" && sN3 == "Pr enom", "Retours dans un champ structure non aplatis");
    fDecouperLigneVcf("N;ENCODING=QUOTED-PRINTABLE:;=41=3B=42;=43;;");
    verifier(sN2 == "A;B" && sN3 == "C", "Point-virgule encode pris pour un separateur");
    fDecouperLigneVcf("N:;=20;Texte UTF-8 \xC3\xA9;;");
    verifier(sN2 == "=20" && sN3 == "Texte UTF-8 \xC3\xA9", "Valeur non QP alteree");

    char dossier[MAX_PATH], sortie[MAX_PATH], entreeTest[MAX_PATH];
    verifier(GetTempPathA(MAX_PATH, dossier) != 0, "Dossier temporaire inaccessible");
    verifier(GetTempFileNameA(dossier, "vcf", 0, sortie) != 0, "Fichier temporaire inaccessible");
    if (!GetTempFileNameA(dossier, "vcf", 0, entreeTest)) {
        DeleteFileA(sortie);
        throw std::runtime_error("Entree temporaire inaccessible");
    }
    try {
        std::ifstream original("vcard-test/vcard.vcf", std::ios::binary);
        string contenu((std::istreambuf_iterator<char>(original)), std::istreambuf_iterator<char>());
        // Le fichier fourni est un extrait sans END:VCARD. Completer une copie.
        if (contenu.find("END:VCARD") == string::npos) contenu += "\r\nEND:VCARD\r\n";
        contenu.insert(contenu.find("END:VCARD"),
            "NOTE;CHARSET=UTF-8;ENCODING=QUOTED-PRINTABLE:Ligne1=0D=0ALigne2=0ALigne3=0DLigne4\r\n");
        contenu += "BEGIN:VCARD\r\nVERSION:2.1\r\n"
            "N;CHARSET=UTF-8;ENCODING=QUOTED-PRINTABLE:;Nom=0D=0Acompose;Pr=0Aenom;;\r\n"
            "NOTE;CHARSET=UTF-8;ENCODING=QUOTED-PRINTABLE:Autre=0D=0Afiche\r\nEND:VCARD\r\n";
        std::ofstream complet(entreeTest, std::ios::binary);
        complet << contenu;
        complet.close();
        verifier(static_cast<bool>(complet), "Ecriture de l'entree de test impossible");
        nomFicEntree = entreeTest;
        nomFicSortie = sortie;
        fVcfVersCsv();
        std::ifstream csv(sortie, std::ios::binary);
        verifier(static_cast<bool>(csv), "CSV genere introuvable");
        string entete, premiere;
        getline(csv, entete);
        getline(csv, premiere);
        verifier(premiere.find(nom + '\t' + prenom + '\t') == 0, "Conversion VCF/CSV incorrecte");
        verifier(premiere.find("Ligne1 Ligne2 Ligne3 Ligne4") != string::npos,
                 "Note multiligne non aplatie dans le CSV");
        string seconde, supplementaire;
        verifier(static_cast<bool>(getline(csv, seconde)) && seconde.find("Nom compose\tPr enom\t") == 0 &&
                 seconde.find("Autre fiche") != string::npos, "Seconde fiche multiligne incorrecte");
        verifier(!getline(csv, supplementaire), "Plus d'une ligne CSV par vCard");
        csv.close();
    } catch (...) {
        DeleteFileA(sortie);
        DeleteFileA(entreeTest);
        throw;
    }
    DeleteFileA(sortie);
    DeleteFileA(entreeTest);
}

int main() {
    std::ostringstream traces;
    std::streambuf* ancien = std::cout.rdbuf(traces.rdbuf());
    try {
        tester();
    } catch (const std::exception& erreur) {
        std::cout.rdbuf(ancien);
        std::cerr << "ECHEC : " << erreur.what() << '\n';
        return 1;
    }
    std::cout.rdbuf(ancien);
    std::cout << "Tests quoted-printable, pliage et conversion UTF-8 : OK\n";
    return 0;
}
