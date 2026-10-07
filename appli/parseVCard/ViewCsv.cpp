// Lecteur CSV pour la console Windows.
// Compilation : g++ -std=c++11 -Wall -Wextra ViewCsv.cpp -o ViewCsv.exe
// Usage : ViewCsv.exe [fichier.csv]

#include <windows.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Les tabulations sont le separateur utilise par ParseVCard.cpp.
static char detecterSeparateur(const std::string& ligne) {
    const char candidats[] = {'\t', ';', ','};
    size_t compteurs[] = {0, 0, 0};
    bool entreGuillemets = false;
    for (size_t i = 0; i < ligne.size(); ++i) {
        if (ligne[i] == '"') {
            if (entreGuillemets && i + 1 < ligne.size() && ligne[i + 1] == '"') {
                ++i;
            } else {
                entreGuillemets = !entreGuillemets;
            }
        } else if (!entreGuillemets) {
            for (size_t j = 0; j < 3; ++j) {
                if (ligne[i] == candidats[j]) ++compteurs[j];
            }
        }
    }
    size_t meilleur = 0;
    for (size_t j = 1; j < 3; ++j) {
        if (compteurs[j] > compteurs[meilleur]) meilleur = j;
    }
    return candidats[meilleur];
}

static std::vector<std::string> decouper(const std::string& ligne, char separateur) {
    std::vector<std::string> colonnes;
    std::string valeur;
    bool entreGuillemets = false;
    for (size_t i = 0; i < ligne.size(); ++i) {
        const char c = ligne[i];
        if (c == '"' && (entreGuillemets || valeur.empty())) {
            if (entreGuillemets && i + 1 < ligne.size() && ligne[i + 1] == '"') {
                valeur += '"';
                ++i;
            } else {
                entreGuillemets = !entreGuillemets;
            }
        } else if (c == separateur && !entreGuillemets) {
            colonnes.push_back(valeur);
            valeur.clear();
        } else {
            valeur += c;
        }
    }
    colonnes.push_back(valeur); // Conserver aussi la derniere colonne vide.
    return colonnes;
}

// Eviter qu'une tabulation ou un caractere de controle change la mise en page.
static std::string texteEcran(std::string texte) {
    for (size_t i = 0; i < texte.size(); ++i) {
        if (static_cast<unsigned char>(texte[i]) < 32) texte[i] = ' ';
    }
    return texte;
}

// Le CSV reste en UTF-8 en memoire et sur disque. Seul l'affichage est
// converti en UTF-16 pour la console Windows, quelle que soit sa page de codes.
static std::wstring texteConsole(const std::string& texte, size_t largeur) {
    const std::string nettoye = texteEcran(texte);
    if (nettoye.empty()) return std::wstring();
    const int taille = MultiByteToWideChar(CP_UTF8, 0, nettoye.data(),
                                          static_cast<int>(nettoye.size()), NULL, 0);
    if (!taille) return L"[Erreur UTF-8]";
    std::wstring visible(static_cast<size_t>(taille), L' ');
    MultiByteToWideChar(CP_UTF8, 0, nettoye.data(), static_cast<int>(nettoye.size()),
                        &visible[0], taille);
    if (visible.size() > largeur) {
        size_t garder = largeur >= 3 ? largeur - 3 : largeur;
        // Ne pas couper une paire de substituts (emoji, caracteres hors BMP).
        if (garder > 0 && visible[garder - 1] >= 0xD800 && visible[garder - 1] <= 0xDBFF) {
            --garder;
        }
        visible.resize(garder);
        if (largeur >= 3) visible += L"...";
    }
    return visible;
}

static void ecrireConsole(HANDLE sortie, const std::string& texte, size_t largeur,
                          bool nouvelleLigne) {
    std::wstring visible = texteConsole(texte, largeur);
    if (nouvelleLigne) visible += L"\r\n";
    DWORD ecrits;
    if (!visible.empty()) {
        WriteConsoleW(sortie, visible.data(), static_cast<DWORD>(visible.size()), &ecrits, NULL);
    }
}

static std::string versUtf8(const std::wstring& texte) {
    if (texte.empty()) return "";
    const int taille = WideCharToMultiByte(CP_UTF8, 0, texte.data(),
        static_cast<int>(texte.size()), NULL, 0, NULL, NULL);
    std::string resultat(static_cast<size_t>(taille), '\0');
    if (taille) WideCharToMultiByte(CP_UTF8, 0, texte.data(),
        static_cast<int>(texte.size()), &resultat[0], taille, NULL, NULL);
    return resultat;
}

static std::wstring sansCasse(const std::string& texte) {
    std::wstring resultat = texteConsole(texte, texte.size());
    if (!resultat.empty()) CharLowerBuffW(&resultat[0], static_cast<DWORD>(resultat.size()));
    return resultat;
}

static bool contientRecherche(const std::vector<std::string>& valeurs,
                              const std::wstring& recherche) {
    for (size_t i = 0; i < valeurs.size(); ++i) {
        if (sansCasse(valeurs[i]).find(recherche) != std::wstring::npos) return true;
    }
    return false;
}

// Parcours circulaire des donnees, sans jamais rechercher dans l'entete.
static bool trouverLigne(const std::vector<std::vector<std::string> >& lignes,
                         size_t& numero, const std::string& recherche, bool suivant) {
    const size_t nombre = lignes.size() - 1;
    if (!nombre || recherche.empty()) return false;
    const std::wstring motif = sansCasse(recherche);
    const size_t depart = numero > 0 ? numero - 1 : 0;
    for (size_t pas = 1; pas <= nombre; ++pas) {
        const size_t candidat = suivant ? (depart + pas) % nombre + 1
            : (depart + nombre - pas) % nombre + 1;
        if (contientRecherche(lignes[candidat], motif)) {
            numero = candidat;
            return true;
        }
    }
    return false;
}

static size_t trouverChamp(const std::vector<std::string>& entete, std::string nom) {
    const size_t debut = nom.find_first_not_of(" \t");
    if (debut == std::string::npos) return entete.size();
    nom = nom.substr(debut, nom.find_last_not_of(" \t") - debut + 1);
    const std::wstring demande = sansCasse(nom);
    size_t trouve = entete.size();
    for (size_t i = 0; i < entete.size(); ++i) {
        if (sansCasse(entete[i]) == demande) {
            if (trouve != entete.size()) return entete.size(); // Intitule ambigu.
            trouve = i;
        }
    }
    return trouve;
}

static std::string encoderChamp(const std::string& valeur, char separateur) {
    if (valeur.find(separateur) == std::string::npos &&
        valeur.find_first_of("\"\r\n") == std::string::npos) return valeur;
    std::string resultat = "\"";
    for (size_t i = 0; i < valeur.size(); ++i) {
        if (valeur[i] == '"') resultat += '"';
        resultat += valeur[i];
    }
    return resultat + '"';
}

static void modifierChamp(std::vector<std::vector<std::string> >& lignes,
                          std::vector<std::string>& brutes, size_t numero,
                          size_t colonne, const std::string& valeur, char separateur) {
    if (numero == 0 || numero >= lignes.size() || colonne >= lignes[0].size()) return;
    std::vector<std::string>& valeurs = lignes[numero];
    if (valeurs.size() <= colonne) valeurs.resize(colonne + 1);
    valeurs[colonne] = valeur;
    const std::string& original = brutes[numero];
    std::string fin;
    if (!original.empty() && original.back() == '\n') {
        fin = original.size() >= 2 && original[original.size() - 2] == '\r' ? "\r\n" : "\n";
    }
    std::string nouvelle;
    for (size_t i = 0; i < valeurs.size(); ++i) {
        if (i) nouvelle += separateur;
        nouvelle += encoderChamp(valeurs[i], separateur);
    }
    brutes[numero] = nouvelle + fin;
}

// Garder les lignes originales preserve les guillemets, colonnes masquees,
// encodage et fins de ligne lors de la sauvegarde.
static bool sauverCsv(const std::string& fichier, const std::vector<std::string>& brutes,
                      std::string& message) {
    char chemin[MAX_PATH];
    char* nom = NULL;
    const DWORD longueur = GetFullPathNameA(fichier.c_str(), MAX_PATH, chemin, &nom);
    if (!longueur || longueur >= MAX_PATH || !nom) {
        message = "Erreur : chemin de sauvegarde invalide.";
        return false;
    }
    const std::string dossier(chemin, nom - chemin);
    char temporaire[MAX_PATH];
    if (!GetTempFileNameA(dossier.c_str(), "csv", 0, temporaire)) {
        message = "Erreur : impossible de creer le fichier temporaire.";
        return false;
    }
    std::ofstream sortie(temporaire, std::ios::binary | std::ios::trunc);
    for (size_t i = 0; i < brutes.size() && sortie; ++i) {
        sortie.write(brutes[i].data(), static_cast<std::streamsize>(brutes[i].size()));
    }
    sortie.close();
    if (!sortie) {
        DeleteFileA(temporaire);
        message = "Erreur d'ecriture : fichier d'origine conserve.";
        return false;
    }
    // Remplacer seulement une fois toutes les donnees ecrites et le fichier ferme.
    if (!MoveFileExA(temporaire, chemin, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        const DWORD erreur = GetLastError();
        DeleteFileA(temporaire);
        std::ostringstream texte;
        texte << "Erreur de sauvegarde Windows " << erreur << " : original conserve.";
        message = texte.str();
        return false;
    }
    message = "CSV sauvegarde.";
    return true;
}

static void supprimerLigne(std::vector<std::vector<std::string> >& lignes,
                           std::vector<std::string>& brutes, size_t& numero) {
    if (numero == 0 || numero >= lignes.size()) return;
    lignes.erase(lignes.begin() + numero);
    brutes.erase(brutes.begin() + numero);
    if (numero >= lignes.size()) numero = lignes.size() - 1;
}

static bool copierLigne(const std::vector<std::vector<std::string> >& lignes,
                         size_t numero, std::string& message) {
    if (numero == 0 || numero >= lignes.size()) {
        message = "Aucune ligne a copier.";
        return false;
    }
    // Copier les colonnes visibles avec leur valeur complete, en Unicode.
    std::wstring texte;
    const std::vector<std::string>& entete = lignes.front();
    const std::vector<std::string>& valeurs = lignes[numero];
    for (size_t i = 0; i < entete.size(); ++i) {
        if (entete[i].find_first_not_of(" \t\r\n") == std::string::npos) continue;
        std::string champ = entete[i] + " : ";
        if (i < valeurs.size()) champ += valeurs[i];
        texte += texteConsole(champ, champ.size()) + L"\r\n";
    }
    HGLOBAL bloc = GlobalAlloc(GMEM_MOVEABLE, (texte.size() + 1) * sizeof(wchar_t));
    if (!bloc) {
        message = "Erreur : memoire insuffisante pour copier.";
        return false;
    }
    wchar_t* destination = static_cast<wchar_t*>(GlobalLock(bloc));
    if (!destination) {
        GlobalFree(bloc);
        message = "Erreur : impossible de preparer la copie.";
        return false;
    }
    std::copy(texte.begin(), texte.end(), destination);
    destination[texte.size()] = L'\0';
    GlobalUnlock(bloc);
    if (!OpenClipboard(GetConsoleWindow())) {
        GlobalFree(bloc);
        message = "Erreur : presse-papiers indisponible.";
        return false;
    }
    const bool copie = EmptyClipboard() && SetClipboardData(CF_UNICODETEXT, bloc) != NULL;
    CloseClipboard();
    // Windows devient proprietaire du bloc seulement si la copie a reussi.
    if (!copie) GlobalFree(bloc);
    message = copie ? "Ligne copiee dans le presse-papiers."
                    : "Erreur : impossible de copier dans le presse-papiers.";
    return copie;
}

static void afficher(HANDLE sortie, const std::string& fichier,
                     const std::vector<std::vector<std::string> >& lignes,
                     size_t numero, const std::string& message,
                     const std::string& recherche = "") {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(sortie, &info)) return;
    const COORD debut = {0, 0};
    DWORD ecrits;
    const DWORD taille = static_cast<DWORD>(info.dwSize.X) * info.dwSize.Y;
    FillConsoleOutputCharacterA(sortie, ' ', taille, debut, &ecrits);
    FillConsoleOutputAttribute(sortie, info.wAttributes, taille, debut, &ecrits);
    SetConsoleCursorPosition(sortie, debut);
    SMALL_RECT fenetre = info.srWindow;
    fenetre.Bottom -= fenetre.Top;
    fenetre.Top = 0;
    SetConsoleWindowInfo(sortie, TRUE, &fenetre);

    // Une seule ligne de console par colonne, sans retour automatique.
    const size_t largeur = static_cast<size_t>(info.dwSize.X > 1 ? info.dwSize.X - 1 : 1);
    const auto ecrireLigne = [sortie, largeur](const std::string& texte) {
        ecrireConsole(sortie, texte, largeur, true);
    };

    ecrireLigne("Fichier : " + fichier);
    ecrireLigne("Droite : suivante | Gauche : precedente");
    ecrireLigne("Pg Down/Pg Up : +/-10 lignes | Debut : premiere | Fin : derniere");
    ecrireLigne("Suppr : supprimer | Ctrl+S : sauver | Ctrl+C ou 99 : quitter");
    ecrireLigne("C : copier | F : rechercher | M : modifier | Echap : navigation normale");
    if (!recherche.empty()) ecrireLigne("Recherche : " + recherche + " (Droite/Gauche : +/-1, Pg Down/Pg Up : +/-10 resultats)");
    ecrireLigne("");

    const std::vector<std::string>& entete = lignes.front();
    const std::vector<std::string>& valeurs = lignes[numero];
    size_t nombreAffiche = 0;
    for (size_t i = 0; numero != 0 && i < entete.size(); ++i) {
        if (entete[i].find_first_not_of(" \t\r\n") == std::string::npos) continue;
        std::ostringstream texte;
        texte << entete[i] << " : ";
        if (i < valeurs.size()) texte << valeurs[i];
        ecrireLigne(texte.str());
        ++nombreAffiche;
    }
    if (numero == 0) {
        ecrireLigne("Aucune ligne de donnees.");
        ++nombreAffiche;
    }
    // Afficher la saisie et la confirmation en entier, meme pour une valeur longue.
    size_t lignesMessage = 0;
    if (!message.empty()) {
        ecrireLigne("");
        ++lignesMessage;
        const std::wstring texte = texteConsole(message, message.size());
        for (size_t debutTexte = 0; debutTexte < texte.size();) {
            size_t longueur = std::min(largeur, texte.size() - debutTexte);
            if (longueur > 1 && debutTexte + longueur < texte.size() &&
                texte[debutTexte + longueur - 1] >= 0xD800 &&
                texte[debutTexte + longueur - 1] <= 0xDBFF) --longueur;
            const std::wstring morceau = texte.substr(debutTexte, longueur) + L"\r\n";
            WriteConsoleW(sortie, morceau.data(), static_cast<DWORD>(morceau.size()), &ecrits, NULL);
            debutTexte += longueur;
            ++lignesMessage;
        }
    }
    std::cout.flush();

    // Placer le compteur sous les colonnes, au bas de la fenetre si possible.
    const size_t bas = std::max(nombreAffiche + lignesMessage + (recherche.empty() ? 7 : 8),
                               static_cast<size_t>(fenetre.Bottom));
    const COORD pied = {0, static_cast<SHORT>(std::min(bas, static_cast<size_t>(info.dwSize.Y - 1)))};
    SetConsoleCursorPosition(sortie, pied);
    std::ostringstream compteur;
    compteur << numero << '/' << lignes.size() - 1;
    ecrireConsole(sortie, compteur.str(), largeur, false);
}

class ModeConsole {
public:
    explicit ModeConsole(HANDLE entree) : entree_(entree), actif_(false), mode_(0) {
        if (GetConsoleMode(entree_, &mode_)) {
            // Recevoir Ctrl+C comme une touche et empecher la selection de bloquer la lecture.
            actif_ = SetConsoleMode(entree_, (mode_ | ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT)
                & ~(ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_QUICK_EDIT_MODE)) != 0;
        }
    }
    ~ModeConsole() {
        if (actif_) SetConsoleMode(entree_, mode_);
    }
    bool actif() const { return actif_; }
private:
    HANDLE entree_;
    bool actif_;
    DWORD mode_;
};

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage : ViewCsv.exe [fichier.csv]\n";
        return 1;
    }
    const std::string fichier = argc == 2 ? argv[1] : "vcard-prod/vcardNEW.csv";
    std::ifstream entree(fichier.c_str(), std::ios::binary);
    if (!entree) {
        std::cerr << "Impossible d'ouvrir le fichier : " << fichier << '\n';
        return 1;
    }

    std::vector<std::vector<std::string> > lignes;
    std::vector<std::string> brutes;
    std::string ligne;
    char separateur = '\t';
    while (std::getline(entree, ligne)) {
        brutes.push_back(ligne + (entree.eof() ? "" : "\n"));
        if (!ligne.empty() && ligne.back() == '\r') ligne.pop_back();
        if (lignes.empty()) {
            if (ligne.compare(0, 3, "\xEF\xBB\xBF") == 0) ligne.erase(0, 3);
            separateur = detecterSeparateur(ligne);
        }
        lignes.push_back(decouper(ligne, separateur));
    }
    if (entree.bad()) {
        std::cerr << "Erreur de lecture : " << fichier << '\n';
        return 1;
    }
    entree.close();
    if (lignes.empty()) {
        std::cout << "Fichier vide : " << fichier << '\n';
        return 0;
    }

    const HANDLE clavier = GetStdHandle(STD_INPUT_HANDLE);
    const HANDLE ecran = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD modeSortie;
    ModeConsole console(clavier);
    if (!console.actif() || !GetConsoleMode(ecran, &modeSortie)) {
        std::cerr << "Ce programme doit etre lance dans une console Windows interactive.\n";
        return 1;
    }

    // L'indice 0 reste reserve a l'entete, jamais presentee comme une fiche.
    size_t numero = lignes.size() > 1 ? 1 : 0;
    enum Mode { NORMAL, RECHERCHE, CHAMP, VALEUR, CONFIRMER_MODIFICATION, CONFIRMER_SUPPRESSION };
    Mode mode = NORMAL;
    bool neufSaisi = false;
    bool modifie = false;
    size_t colonne = 0;
    std::string message, recherche, nouvelleValeur;
    std::wstring saisie;
    const auto redessiner = [&]() {
        afficher(ecran, fichier, lignes, numero, message, recherche);
    };
    const auto actualiserSaisie = [&]() {
        if (mode == RECHERCHE) message = "Rechercher (vide : tout afficher) : ";
        else if (mode == CHAMP) message = "Nom du champ a modifier : ";
        else message = "Nouvelle valeur de " + lignes[0][colonne] + " : ";
        message += versUtf8(saisie);
        redessiner();
    };
    redessiner();
    for (;;) {
        INPUT_RECORD evenement;
        DWORD lus;
        if (!ReadConsoleInputW(clavier, &evenement, 1, &lus) || lus == 0) {
            std::cerr << "Erreur de lecture du clavier.\n";
            return 1;
        }
        if (evenement.EventType == WINDOW_BUFFER_SIZE_EVENT) {
            redessiner();
            continue;
        }
        if (evenement.EventType != KEY_EVENT || !evenement.Event.KeyEvent.bKeyDown) continue;
        const KEY_EVENT_RECORD& touche = evenement.Event.KeyEvent;
        const bool controle = (touche.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
        if ((touche.wVirtualKeyCode == 'C' && controle) || touche.uChar.UnicodeChar == 3) {
            std::cout << "\nFin de ViewCsv.\n";
            return 0;
        }
        if (mode == RECHERCHE || mode == CHAMP || mode == VALEUR) {
            if (touche.wVirtualKeyCode == VK_ESCAPE) {
                mode = NORMAL;
                saisie.clear();
                message = "Saisie annulee.";
                redessiner();
            } else if (touche.wVirtualKeyCode == VK_RETURN) {
                const std::string texte = versUtf8(saisie);
                saisie.clear();
                if (mode == RECHERCHE) {
                    recherche = texte;
                    mode = NORMAL;
                    if (recherche.empty()) message = "Navigation normale.";
                    else message = trouverLigne(lignes, numero, recherche, true)
                        ? "Recherche active. Droite/Gauche : resultats." : "Aucun resultat.";
                    redessiner();
                } else if (mode == CHAMP) {
                    colonne = trouverChamp(lignes[0], texte);
                    if (colonne == lignes[0].size()) {
                        message = "Champ inconnu ou ambigu. Saisir son intitule : ";
                        redessiner();
                    } else {
                        mode = VALEUR;
                        actualiserSaisie();
                    }
                } else {
                    nouvelleValeur = texte;
                    mode = CONFIRMER_MODIFICATION;
                    message = "Modifier " + lignes[0][colonne] + " = \"" + nouvelleValeur + "\" ? O/N (Echap : annuler)";
                    redessiner();
                }
            } else if (touche.wVirtualKeyCode == VK_BACK) {
                if (!saisie.empty()) {
                    const wchar_t dernier = saisie.back();
                    saisie.pop_back();
                    if (dernier >= 0xDC00 && dernier <= 0xDFFF && !saisie.empty() &&
                        saisie.back() >= 0xD800 && saisie.back() <= 0xDBFF) saisie.pop_back();
                }
                actualiserSaisie();
            } else if (touche.uChar.UnicodeChar >= 32) {
                saisie.append(touche.wRepeatCount ? touche.wRepeatCount : 1, touche.uChar.UnicodeChar);
                actualiserSaisie();
            }
            continue;
        }
        if (mode == CONFIRMER_SUPPRESSION || mode == CONFIRMER_MODIFICATION) {
            if (touche.wVirtualKeyCode == 'O' && !controle) {
                if (mode == CONFIRMER_SUPPRESSION) {
                    supprimerLigne(lignes, brutes, numero);
                    message = "Ligne supprimee en memoire. Ctrl+S pour sauver.";
                } else {
                    modifierChamp(lignes, brutes, numero, colonne, nouvelleValeur, separateur);
                    message = "Champ modifie en memoire. Ctrl+S pour sauver.";
                }
                modifie = true;
            } else if (touche.wVirtualKeyCode == 'N' || touche.wVirtualKeyCode == VK_ESCAPE) {
                message = "Operation annulee.";
            } else {
                continue;
            }
            mode = NORMAL;
            redessiner();
            continue;
        }
        // Deux frappes consecutives sur 9 quittent, sauf pendant une saisie.
        if (touche.uChar.UnicodeChar == L'9') {
            if (neufSaisi) {
                std::cout << "\nFin de ViewCsv.\n";
                return 0;
            }
            neufSaisi = true;
            continue;
        }
        if (touche.wVirtualKeyCode == VK_SHIFT || touche.wVirtualKeyCode == VK_CONTROL ||
            touche.wVirtualKeyCode == VK_MENU) continue;
        neufSaisi = false;
        if (touche.wVirtualKeyCode == 'C' && !controle) {
            copierLigne(lignes, numero, message);
            redessiner();
            continue;
        }
        if (touche.wVirtualKeyCode == 'S' && controle) {
            if (sauverCsv(fichier, brutes, message)) modifie = false;
            redessiner();
            continue;
        }
        if (touche.wVirtualKeyCode == VK_DELETE && numero != 0) {
            mode = CONFIRMER_SUPPRESSION;
            message = "Supprimer cette ligne ? O : oui / N ou Echap : annuler";
            redessiner();
            continue;
        }
        if (!controle && (touche.wVirtualKeyCode == 'F' || touche.wVirtualKeyCode == 'M')) {
            if (touche.wVirtualKeyCode == 'M' && numero == 0) {
                message = "Aucune ligne a modifier.";
                redessiner();
                continue;
            }
            mode = touche.wVirtualKeyCode == 'F' ? RECHERCHE : CHAMP;
            saisie.clear();
            actualiserSaisie();
            continue;
        }
        if (touche.wVirtualKeyCode == VK_ESCAPE) {
            recherche.clear();
            message = "Navigation normale.";
            redessiner();
            continue;
        }
        const size_t precedent = numero;
        const size_t pas = (touche.wVirtualKeyCode == VK_NEXT || touche.wVirtualKeyCode == VK_PRIOR) ? 10 : 1;
        bool navigation = true;
        bool trouve = true;
        switch (touche.wVirtualKeyCode) {
            case VK_RIGHT:
            case VK_NEXT:
                if (!recherche.empty()) {
                    for (size_t i = 0; i < pas && trouve; ++i) {
                        trouve = trouverLigne(lignes, numero, recherche, true);
                    }
                } else numero += std::min(pas, lignes.size() - 1 - numero);
                break;
            case VK_LEFT:
            case VK_PRIOR:
                if (!recherche.empty()) {
                    for (size_t i = 0; i < pas && trouve; ++i) {
                        trouve = trouverLigne(lignes, numero, recherche, false);
                    }
                } else if (numero > 1) numero -= std::min(pas, numero - 1);
                break;
            case VK_HOME:
                if (!recherche.empty()) {
                    size_t candidat = lignes.size() - 1;
                    trouve = trouverLigne(lignes, candidat, recherche, true);
                    if (trouve) numero = candidat;
                } else numero = lignes.size() > 1 ? 1 : 0;
                break;
            case VK_END:
                if (!recherche.empty()) {
                    size_t candidat = lignes.size() > 1 ? 1 : 0;
                    trouve = trouverLigne(lignes, candidat, recherche, false);
                    if (trouve) numero = candidat;
                } else numero = lignes.size() - 1;
                break;
            default: navigation = false; break;
        }
        if (navigation && (!recherche.empty() || numero != precedent)) {
            message = trouve ? (modifie ? "Modifications non sauvegardees." : "") : "Aucun resultat.";
            redessiner();
        }
    }
}
