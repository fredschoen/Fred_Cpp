//============================================================================
// Name        : ParseVCard.cpp
// Author      : F SCHOEN
// Version     : 41212a
// Copyright   : F SCHOEN
// Description : conversion aller et retour Vcf/Csv
//============================================================================
// fonctions:
// string fChoixNomFic()
// void fCsvVersVcf(void)
// void fVcfVersCsv(void)
// void fVcfVersTxt(void)
// void fDecouperLigneCsv(string ligneEntree)
// void fDecouperLigneVcf(string ligneEntree)
// void fSeparerElements(string sousLigne)
// string fTranscoderTexte(string sTexte)
// string fTranscoderToutesOccurrences(string sTexte, string sCode, string sDecode)
//============================================================================

#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <cctype>
#include <string>

using namespace std;

string sElement1="";
string sElement2="";
string sElement3="";
string sElement4="";
string sElement5="";
string sElement6="";
string sElement7="";

string sN1="";
string sN2="";
string sN3="";
string sN4="";

string sTC="";
string sTH="";
string sTW="";
string sEH="";
string sEW="";

string sO1="";
string sO2="";
string sO3="";

string sAH1="";
string sAH2="";
string sAH3="";
string sAH4="";
string sAH5="";
string sAH6="";
string sAH7="";

string sAW1="";
string sAW2="";
string sAW3="";
string sAW4="";
string sAW5="";
string sAW6="";
string sAW7="";

string sTI="";
string sNT="";

string nomFicEntree="";
string nomFicSortie="";

string nom1="vcard";
string nom2="test";
string nom3="adr_col";

string separateurAttributsValeurs = ":";
string separateurElements = ";";
string prefixeColonneNum = ".";


//============================================================================
string fTranscoderToutesOccurrences(string sTexte, string sCode, string sDecode) {
// objectif: trouver la chaine "sCode" dans le texte "sTexte" et le remplacer par "sDecode"
//============================================================================

	//	cout<<"fTranscoderToutesOccurrences, in  : " << sTexte << "," << sCode << "," << sDecode << "," << endl;
	std::string sRetour=sTexte;
	std::size_t found =sRetour.find(sCode);
	while(found!=std::string::npos) {
		sRetour.replace(found, sCode.length(), sDecode);
		found =sRetour.find(sCode);
	}
//	cout<<"fTranscoderTexte, out : " << sRetour << endl;
return sRetour;
}

//============================================================================
string fTranscoderDebutLigne(string sTexte, string sCode, string sDecode) {
// objectif: trouver la chaine "sCode" dans le texte "sTexte" et le remplacer par "sDecode"
//============================================================================

	//	cout<<"fTranscoderDebutLigne, in  : " << sTexte << "," << sCode << "," << sDecode << "," << endl;
	std::string sRetour=sTexte;
	std::size_t found =sRetour.find(sCode);
	if(found==0) {
		sRetour.replace(found, sCode.length(), sDecode);
		cout<<"fTranscoderDebutLigne,0 sRetour=" << sRetour << endl;
	}
return sRetour;
}


//============================================================================
static int fChiffreHexadecimal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static bool fEstQuotedPrintable(const string& ligne) {
    string attributs = ligne.substr(0, ligne.find(':'));
    for (size_t i = 0; i < attributs.size(); ++i) {
        attributs[i] = static_cast<char>(toupper(static_cast<unsigned char>(attributs[i])));
    }
    return attributs.find(";ENCODING=QUOTED-PRINTABLE") != string::npos;
}

// Lire une propriete complete, meme si elle occupe plusieurs lignes physiques.
static bool fLireLigneVcf(std::istream& entree, string& ligne) {
    if (!getline(entree, ligne)) return false;
    if (!ligne.empty() && ligne.back() == '\r') ligne.pop_back();
    const bool quotedPrintable = fEstQuotedPrintable(ligne);
    for (;;) {
        if (quotedPrintable) {
            const size_t fin = ligne.find_last_not_of(" \t");
            if (fin != string::npos) ligne.resize(fin + 1);
            if (!ligne.empty() && ligne.back() == '=' && entree.peek() != std::char_traits<char>::eof()) {
                ligne.pop_back(); // Retirer la coupure souple, pas un caractere de la valeur.
                string suite;
                if (!getline(entree, suite)) return true;
                if (!suite.empty() && suite.back() == '\r') suite.pop_back();
                ligne += suite;
                continue;
            }
        }
        // Pliage vCard classique : retirer uniquement l'espace/tabulation de pliage.
        if (entree.peek() == ' ' || entree.peek() == '\t') {
            string suite;
            if (!getline(entree, suite)) return true;
            if (!suite.empty() && suite.back() == '\r') suite.pop_back();
            ligne += suite.substr(1);
            continue;
        }
        return true;
    }
}

string fTranscoderTexte(string sTexte) {
    // Decoder les octets quoted-printable, sans convertir ni alterer le UTF-8.
    // Une seule passe : =3D=32=30 devient =20 et ne doit pas etre redecode.
    string resultat;
    for (size_t i = 0; i < sTexte.size(); ++i) {
        if (sTexte[i] == '=' && i + 2 < sTexte.size()) {
            const int haut = fChiffreHexadecimal(sTexte[i + 1]);
            const int bas = fChiffreHexadecimal(sTexte[i + 2]);
            if (haut >= 0 && bas >= 0) {
                resultat += static_cast<char>((haut << 4) | bas);
                i += 2;
                continue;
            }
        }
        resultat += sTexte[i];
    }
    // Les retours decodes (=0D/=0A) ne doivent pas creer de nouvelles lignes CSV.
    // Une paire CRLF devient un seul espace ; les coupures souples ont deja
    // ete retirees par fLireLigneVcf avant le decodage.
    string uneLigne;
    for (size_t i = 0; i < resultat.size(); ++i) {
        if (resultat[i] == '\r' || resultat[i] == '\n') {
            if (resultat[i] == '\r' && i + 1 < resultat.size() && resultat[i + 1] == '\n') ++i;
            uneLigne += ' ';
        } else {
            uneLigne += resultat[i];
        }
    }
    return uneLigne;
}

//============================================================================
void fSeparerElements(string sousLigne) {
	//============================================================================
    sElement1 = "";
    sElement2 = "";
    sElement3 = "";
    sElement4 = "";
    sElement5 = "";
    sElement6 = "";
    sElement7 = "";

	size_t pos = 0;
	int numElement=0;
    
	sElement1 = sousLigne; //par défaut, il n'y a qu'un élément

	//plusieurs éléments ?
	while ((pos = sousLigne.find(separateurElements)) != std::string::npos) {
		cout << "fSeparerElements sousLigne<" << sousLigne << ">" << endl;
	    numElement+=1;
	    if (numElement==1) {
		    sElement1 = sousLigne.substr(0, pos);
				sElement2 = sousLigne.substr(pos+ separateurElements.length(), sousLigne.length());
	    } else if (numElement==2) {
		    sElement2 = sousLigne.substr(0, pos);
		    sElement3 = sousLigne.substr(pos+ separateurElements.length(), sousLigne.length());
	    } else if (numElement==3) {
		    sElement3 = sousLigne.substr(0, pos);
		    sElement4 = sousLigne.substr(pos+ separateurElements.length(), sousLigne.length());
	    }else if (numElement==4) {
		    sElement4 = sousLigne.substr(0, pos);
		    sElement5 = sousLigne.substr(pos+ separateurElements.length(), sousLigne.length());
	    }else if (numElement==5) {
		    sElement5 = sousLigne.substr(0, pos);
		    sElement6 = sousLigne.substr(pos+ separateurElements.length(), sousLigne.length());
	    }else if (numElement==6) {
		    sElement6 = sousLigne.substr(0, pos);
		    sElement7 = sousLigne.substr(pos+ separateurElements.length(), sousLigne.length());
	    }else if (numElement==7) {
		    sElement7 = sousLigne;
	    }
	    sousLigne.erase(0, pos + separateurElements.length());
		cout << "fSeparerElements sElementx<" 
		<< sElement1 << ',' << sElement2 << ',' << sElement3 << ',' 
		<< sElement4 << ',' << sElement5 << ',' << sElement6 << ',' 
		<< sElement7 << ">" << endl;
	}
    
		cout << "fSeparerElements fin      <" 
		<< sElement1 << ',' << sElement2 << ',' << sElement3 << ',' 
		<< sElement4 << ',' << sElement5 << ',' << sElement6 << ',' 
		<< sElement7 << ">" << endl;
return;
}
//============================================================================
void fDecouperLigneVcf(string ligneEntree) {
	//============================================================================

	size_t pos = 0;
	std::string tabAttribut="";
	std::string tabValeur="";
	
	string attribut1="";
	string attribut2="";

	string valeur1="";
	string valeur2="";
	string valeur3="";
	string valeur4="";
	string valeur5="";
	string valeur6="";
	string valeur7="";

	// cas bizarres

	ligneEntree =fTranscoderDebutLigne(ligneEntree, ";CHARSET=UTF-8;ENCODING=QUOTED-PRINTABLE","");
//	ligneEntree =fTranscoderDebutLigne(ligneEntree, "ADR;HOME;", "ADR;HOME:");
//	ligneEntree =fTranscoderDebutLigne(ligneEntree, "ADR;WORK;", "ADR;WORK:");
//	ligneEntree =fTranscoderDebutLigne(ligneEntree, "NOTE;", "NOTE:");
//	ligneEntree =fTranscoderDebutLigne(ligneEntree, "TITLE;", "TITLE:");
//	ligneEntree =fTranscoderDebutLigne(ligneEntree, "N;", "N:");
//	ligneEntree =fTranscoderDebutLigne(ligneEntree, "FN;", "FN:");
//	ligneEntree =fTranscoderDebutLigne(ligneEntree, "ORG;", "ORG:");

	//autres cas bizarre ?
	std::size_t found =ligneEntree.find(separateurAttributsValeurs);
	if(found==std::string::npos) {
		cout << "---ATTENTION--- ligne sans ':' : " 
		<<ligneEntree
		<< endl;
		return;
	}

	// decoupage de la ligne: element 1
	pos = ligneEntree.find(separateurAttributsValeurs);
	tabAttribut = ligneEntree.substr(0, pos);

	// Majuscules
	for (auto & c: tabAttribut) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));

	// effacer les attributs dans "ligneEntree"
	ligneEntree.erase(0, pos + separateurAttributsValeurs.length());
	// dernier element=le reste de la ligne: element 2
	const bool quotedPrintable = fEstQuotedPrintable(tabAttribut);
	tabValeur = quotedPrintable ? fTranscoderTexte(ligneEntree) : ligneEntree;

	//attribut de la ligne
	fSeparerElements(tabAttribut);
	attribut1=sElement1;
	attribut2=sElement2;

	//valeurs de la ligne
	// Separer avant de decoder : =3B est une valeur, pas un separateur structurel.
	fSeparerElements(ligneEntree);
	valeur1=sElement1;
	valeur2=sElement2;
	valeur3=sElement3;
	valeur4=sElement4;
	valeur5=sElement5;
	valeur6=sElement6;
	valeur7=sElement7;
	if (quotedPrintable) {
		valeur1 = fTranscoderTexte(valeur1);
		valeur2 = fTranscoderTexte(valeur2);
		valeur3 = fTranscoderTexte(valeur3);
		valeur4 = fTranscoderTexte(valeur4);
		valeur5 = fTranscoderTexte(valeur5);
		valeur6 = fTranscoderTexte(valeur6);
		valeur7 = fTranscoderTexte(valeur7);
	}

	//charger la bonne colonne
	if (attribut1.compare("N")==0) {
		sN1=valeur1;
		sN2=valeur2;
		sN3=valeur3;
		sN4=valeur4;

	} else if (attribut1.compare("TEL")==0){
		if (attribut2.compare("CELL")==0){
		       sTC=tabValeur;
	    } else if (attribut2.compare("HOME")==0){
	    	   sTH=tabValeur;
	    } else sTW=tabValeur;

	} else if (attribut1.compare("EMAIL")==0){
		if (attribut2.compare("HOME")==0){
		       sEH=tabValeur;
	    } else if (attribut2.compare("WORK")==0){
	    	   sEW=tabValeur;
	    } 

	} else if (attribut1.compare("ADR")==0){

		if (attribut2.compare("HOME")==0){
			sAH1=valeur1;
			sAH2=valeur2;
			sAH3=valeur3;
			sAH4=valeur4;
			sAH5=valeur5;
			sAH6=valeur6;
			sAH7=valeur7;
	    }

		if (attribut2.compare("WORK")==0){
			sAW1=valeur1;
			sAW2=valeur2;
			sAW3=valeur3;
			sAW4=valeur4;
			sAW5=valeur5;
			sAW6=valeur6;
			sAW7=valeur7;
	    }

	} else if (attribut1.compare("ORG")==0){
		sO1=valeur1;
		sO2=valeur2;
		sO3=valeur3;
	} else if (attribut1.compare("TITLE")==0){
		sTI=tabValeur;
	} else if (attribut1.compare("NOTE")==0){
		sNT=tabValeur;
	} else {
		//RAZ element2
		tabValeur="";
		if (tabAttribut.compare("BEGIN")==0) {
		   cout << "-----------------------------------------------" << endl;
 	    }
	}

	//display
	if (tabValeur.length()>0){
		  cout << " -> fDecouperLigneVcf: "
		  << " sN2:" 		  << sN2
		  << ";sN3:" 		  << sN3
		  << ";sN1:"          << sN1
		  << ";sN4:" 		  << sN4
		  << ";sTC:" 		  << sTC
		  << ";sTH:" 		  << sTH
		  << ";sTW:" 		  << sTW
		  << ";sEH:"		  << sEH
		  << ";sEW:"		  << sEW
		  << ";sO1:"		<< sO1
		  << ";sO2:"		<< sO2
		  << ";sO3:"		<< sO3
		  << ";sTI:" 		  << sTI
		  << ";sNT:" 		  << sNT

		  << ";sAH1:"		<< sAH1
		  << ";sAH2:"		<< sAH2
		  << ";sAH3:"		<< sAH3
		  << ";sAH4:"		<< sAH4
		  << ";sAH5:"		<< sAH5
		  << ";sAH6:"		<< sAH6
		  << ";sAH7:"		<< sAH7

		  << ";sAW1:"		<< sAW1
		  << ";sAW2:"		<< sAW2
		  << ";sAW3:"		<< sAW3
		  << ";sAW4:"		<< sAW4
		  << ";sAW5:"		<< sAW5
		  << ";sAW6:"		<< sAW6
		  << ";sAW7:"		<< sAW7

		  << endl
		  ;
	}
}

//============================================================================
void fDecouperLigneCsv(string ligneEntree) {
	//============================================================================
	std::string separateurColonnes = "";
	separateurColonnes += '\t';
	
	size_t pos = 0;

	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sN1 = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());

	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sN2 = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sN3 = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sN4 = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sTC = ligneEntree.substr(0, pos);
	sTC =fTranscoderDebutLigne(sTC, prefixeColonneNum, "");

	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sTH = ligneEntree.substr(0, pos);
	sTH =fTranscoderDebutLigne(sTH, prefixeColonneNum, "");
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sTW = ligneEntree.substr(0, pos);
	sTW =fTranscoderDebutLigne(sTW, prefixeColonneNum, "");
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sEH = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sEW = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sO1 = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sO2 = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sO3 = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sTI = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());

	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sNT = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sAH1 = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sAH2 = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sAH3 = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sAH4 = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sAH5 = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sAH6 = ligneEntree.substr(0, pos);
	sAH6 =fTranscoderDebutLigne(sAH6, prefixeColonneNum, "");
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sAH7 = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sAW1 = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sAW2 = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sAW3 = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sAW4 = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sAW5 = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sAW6 = ligneEntree.substr(0, pos);
	sAW6 =fTranscoderDebutLigne(sAW6, prefixeColonneNum, "");
	ligneEntree.erase(0, pos + separateurColonnes.length());


	//chercher + charger + couper
	pos = ligneEntree.find(separateurColonnes);
	sAW7 = ligneEntree.substr(0, pos);
	ligneEntree.erase(0, pos + separateurColonnes.length());

	cout << " -> fDecouperLigneCsv: "
	<< " sN2:" 		  << sN2
	<< ";sN3:" 		  << sN3
	<< ";sN1:"        << sN1
	<< ";sN4:" 		  << sN4
	<< ";sTC:" 		  << sTC
	<< ";sTH:" 		  << sTH
	<< ";sTW:" 		  << sTW
	<< ";sEH:"		  << sEH
	<< ";sEW:"		  << sEW
	<< endl
	<< ";sO1:"		<< sO1
	<< ";sO2:"		<< sO2
	<< ";sO3:"		<< sO3
	<< ";sTI:" 		  << sTI
	<< ";sNT:" 		  << sNT
	<< endl

	<< ";sAH1:"		<< sAH1
	<< ";sAH2:"		<< sAH2
	<< ";sAH3:"		<< sAH3
	<< ";sAH4:"		<< sAH4
	<< ";sAH5:"		<< sAH5
	<< ";sAH6:"		<< sAH6
	<< ";sAH7:"		<< sAH7
	<< endl

	<< ";sAW1:"		<< sAW1
	<< ";sAW2:"		<< sAW2
	<< ";sAW3:"		<< sAW3
	<< ";sAW4:"		<< sAW4
	<< ";sAW5:"		<< sAW5
	<< ";sAW6:"		<< sAW6
	<< ";sAW7:"		<< sAW7

	<< endl;


}

//============================================================================
void fCsvVersVcf(void) {
	//============================================================================
  std::cout << "exec fCsvVersVcf" << endl;

  cout << "nomFicEntree=[" << nomFicEntree << "]" << endl;
  cout << "nomFicSortie=[" << nomFicSortie << "]"<< endl;

  std::ifstream ficEntree (nomFicEntree); // prendre un fichier ANSI (binary ne change pas le pb UTF8)
  std::ofstream ficSortie (nomFicSortie);

  //std::ifstream ficEntree (nomFicEntree, std::ios::binary); (binary ne change pas le pb UTF8)
  string ligneEntree="";

  if  (!ficEntree){
      cout << "ERREUR: Impossible d'ouvrir ficEntree " << nomFicEntree<< endl;
      return;
  }
  if  (!ficSortie){
      cout << "ERREUR: Impossible d'ouvrir ficSortie " << nomFicSortie<< endl;
      ficEntree.close();
      return;
  }

  getline(ficEntree, ligneEntree); //ne pas traiter la ligne d'entete

  while(getline(ficEntree, ligneEntree)) {
	  if (ligneEntree.length()>5) { // moins de 5 car <=> n'importe quoi dans la ligne!!
		  fDecouperLigneCsv(ligneEntree);
		  ficSortie	<< "BEGIN:VCARD"<< endl;
		  ficSortie	<< "VERSION:2.1"<< endl;
		  ficSortie	<< "N:" << sN1 << ";" << sN2 << ";" << sN3 << ";" << sN4 << ";" << endl;
		  ficSortie	<< "TEL;CELL:" << sTC<< endl;
		  ficSortie	<< "TEL;HOME:" << sTH<< endl;
		  ficSortie	<< "TEL;WORK:" << sTW<< endl;
		  ficSortie	<< "EMAIL;HOME:" << sEH<< endl;
		  ficSortie	<< "EMAIL;WORK:" << sEW<< endl;
		  ficSortie	<< "ADR;HOME:" << sAH1 << ";" << sAH2  << ";" << sAH3  << ";" << sAH4  << ";" << sAH5  << ";" << sAH6 << ";" << sAH7 << endl;
		  ficSortie	<< "ADR;WORK:" << sAW1 << ";" << sAW2  << ";" << sAW3  << ";" << sAW4  << ";" << sAW5  << ";" << sAW6 << ";" << sAW7 << endl;
		  ficSortie	<< "ORG:" << sO1 << ";" << sO2<< ";" << sO2<< endl;
		  ficSortie	<< "TITLE:" << sTI<< endl;
		  ficSortie	<< "NOTE:" << sNT<< endl;
		  ficSortie	<< "END:VCARD"<< endl;
	  }
      cout << "fCsvVersVcf, nomFicEntree=" << nomFicEntree << " nomFicSortie=" << nomFicSortie << " : OK !!!!!!!!!" << endl;
  }

  ficEntree.close();
  ficSortie.close();
  return;
}

//============================================================================
void fCsvVersTxt(void) {
	//============================================================================
	std::cout << "exec fCsvVersTxt" << endl;

	cout << "nomFicEntree=[" << nomFicEntree << "]" << endl;
	cout << "nomFicSortie=[" << nomFicSortie << "]"<< endl;

	std::ifstream ficEntree (nomFicEntree); // prendre un fichier ANSI (binary ne change pas le pb UTF8)
	std::ofstream ficSortie (nomFicSortie);

	//std::ifstream ficEntree (nomFicEntree, std::ios::binary); (binary ne change pas le pb UTF8)
	string ligneEntree="";

	if  (!ficEntree){
		cout << "ERREUR: Impossible d'ouvrir ficEntree " << nomFicEntree<< endl;
		return;
	}
	if  (!ficSortie){
		cout << "ERREUR: Impossible d'ouvrir ficSortie " << nomFicSortie<< endl;
		ficEntree.close();
		return;
	}
	string ligneSortie;
	string oldO1="";

	getline(ficEntree, ligneEntree); //ne pas traiter la ligne d'entete du classeur

	while(getline(ficEntree, ligneEntree)) {
		if (ligneEntree.length()>5) { // moins de 5 car <=> n'importe quoi dans la ligne!!
			fDecouperLigneCsv(ligneEntree);

			//ligne d'entete du ORG1
			if(sO1!=oldO1) {
				ligneSortie = "___________________________ " + sO1 + sO1 + sO1 + sO1 + sO1 + sO1 + sO1 + sO1 + sO1;
				ficSortie << ligneSortie << endl;
				oldO1=sO1;
			}

			ligneSortie="";
			if(sN2.length()>0) ligneSortie += sN2 + " ";
			if(sN3.length()>0) ligneSortie += sN3 + " ";
			if(sN1.length()>0) ligneSortie += sN1 + " "; //c'est le titre -> après nom et prénom dans le carnet
			if(sN4.length()>0) ligneSortie += sN4 + " ";
			ligneSortie += "/ ";
			if(sTC.length()>0) ligneSortie += sTC + " - ";
			if(sTH.length()>0) ligneSortie += sTH + " - ";
			if(sTW.length()>0) ligneSortie += sTW + " ";
			ligneSortie += "/ ";
			if(sEH.length()>0) ligneSortie += sEH + " - ";
			if(sEW.length()>0) ligneSortie += sEW + " ";
			ligneSortie += "/ ";
			if(sAH1.length()>0) ligneSortie += sAH1 + " ";
			if(sAH2.length()>0) ligneSortie += sAH2 + " ";
			if(sAH3.length()>0) ligneSortie += sAH3 + " ";
			if(sAH4.length()>0) ligneSortie += sAH4 + " ";
			if(sAH5.length()>0) ligneSortie += sAH5 + " ";
			if(sAH6.length()>0) ligneSortie += sAH6 + " ";
			if(sAH7.length()>0) ligneSortie += sAH7 + " ";
			ligneSortie += "/ ";
			if(sAW1.length()>0) ligneSortie += sAW1 + " ";
			if(sAW2.length()>0) ligneSortie += sAW2 + " ";
			if(sAW3.length()>0) ligneSortie += sAW3 + " ";
			if(sAW4.length()>0) ligneSortie += sAW4 + " ";
			if(sAW5.length()>0) ligneSortie += sAW5 + " ";
			if(sAW6.length()>0) ligneSortie += sAW6 + " ";
			if(sAW7.length()>0) ligneSortie += sAW7 + " ";
			ligneSortie += "/ ";
			//aaa bbb fff www: sera dans la ligne d'entete : if(sO1.length()>0) ligneSortie += sO1 + " ";
			if(sO2.length()>0) ligneSortie += sO2 + " ";
			if(sO3.length()>0) ligneSortie += sO3 + " ";
			ligneSortie += "/ ";
			if(sTI.length()>0) ligneSortie += sTI + " ";
			ligneSortie += "/ ";
			if(sNT.length()>0) ligneSortie += sNT;
			
			ligneSortie=fTranscoderToutesOccurrences(ligneSortie, " - /", " /");
			ligneSortie=fTranscoderToutesOccurrences(ligneSortie, " / /", " /");

			ficSortie << ligneSortie << endl;
			cout <<"ligneSortie=<" <<ligneSortie << ">" << endl;

	  }
      cout << "fCsvVersVcf, nomFicEntree=" << nomFicEntree << " nomFicSortie=" << nomFicSortie << " : OK !!!!!!!!!" << endl;

  }

  ficEntree.close();
  ficSortie.close();
  return;
}

//============================================================================
void fVcfVersCsv(void) {
	//============================================================================
  std::cout << "exec fVcfVersCsv" << endl;

  cout << "nomFicEntree=[" << nomFicEntree << "]" << endl;
  cout << "nomFicSortie=[" << nomFicSortie << "]"<< endl;


  std::ifstream ficEntree (nomFicEntree); // prendre un fichier ANSI (binary ne change pas le pb UTF8)
  std::ofstream ficSortie (nomFicSortie);

  //std::ifstream ficEntree (nomFicEntree, std::ios::binary); (binary ne change pas le pb UTF8)



// traitement
  if  (ficSortie){
	  ficSortie
	  << "N2" << '\t'
	  << "N3" << '\t'
	  << "N1" << '\t'
	  << "N4" << '\t'

	  << "TEL_CELL"<< '\t'
	  << "TEL_HOME"<< '\t'
	  << "TEL_WORK"<< '\t'

	  << "EMAIL_H"<< '\t'
	  << "EMAIL_W"<< '\t'

	  << "ORG1"<< '\t'
	  << "ORG2"<< '\t'
	  << "ORG3"<< '\t'

	  << "TIT"<< '\t'
	  << "NOTE"<< '\t'

	  << "ADH1"<< '\t'
	  << "ADH2"<< '\t'
	  << "ADH3"<< '\t'
	  << "ADH4"<< '\t'
	  << "ADH5"<< '\t'
	  << "ADH6"<< '\t'
	  << "ADH7"<< '\t'

	  << "ADW1"<< '\t'
	  << "ADW2"<< '\t'
	  << "ADW3"<< '\t'
	  << "ADW4"<< '\t'
	  << "ADW5"<< '\t'
	  << "ADW6"<< '\t'
	  << "ADW7"<< '\t'
	  << endl
	  ;

	  if  (ficEntree){
	      cout << "d�but>>>>>>>>>>>>>>>>>>>" << endl;
		  string ligneEntree; //Une variable pour stocker les lignes lues
		  int nLigne=0;
		  //Tant qu'on n'est pas � la fin, on lit
		  // while(getline(ficEntree, ligneEntree, '\r'))
		  while(fLireLigneVcf(ficEntree, ligneEntree)) {
			  nLigne+=1;
			  cout << "getligne " << nLigne << "=" << ligneEntree<< endl;
			  fDecouperLigneVcf(ligneEntree);

			  // en fin de card on ecrit
			  // Majuscule
			  for (auto & c: ligneEntree) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));

			  if (  ligneEntree.compare("END:VCARD")==0   )
			  {
				ficSortie
				<< sN2 << '\t'
				<< sN3 << '\t'
				<< sN1 << '\t'
				<< sN4 << '\t'
				<< prefixeColonneNum << sTC<< '\t'
				<< prefixeColonneNum << sTH<< '\t'
				<< prefixeColonneNum << sTW<< '\t'
				<< sEH<< '\t'
				<< sEW<< '\t'
				<< sO1<< '\t'
				<< sO2<< '\t'
				<< sO3<< '\t'
				<< sTI<< '\t'
				<< sNT<< '\t'

				<< sAH1<< '\t'
				<< sAH2<< '\t'
				<< sAH3<< '\t'
				<< sAH4<< '\t'
				<< sAH5<< '\t'
				<< prefixeColonneNum << sAH6<< '\t'
				<< sAH7<< '\t'

				<< sAW1<< '\t'
				<< sAW2<< '\t'
				<< sAW3<< '\t'
				<< sAW4<< '\t'
				<< sAW5<< '\t'
				<< prefixeColonneNum << sAW6<< '\t'
				<< sAW7<< '\t'

				<< endl
				;

				sN1="";
				sN2="";
				sN3="";
				sN4="";
				sTC="";
				sTH="";
				sTW="";
				sEH="";
				sEW="";
				sO1="";
				sO2="";
				sO3="";
				sAH1="";
				sAH2="";
				sAH3="";
				sAH4="";
				sAH5="";
				sAH6="";
				sAH7="";

				sAW1="";
				sAW2="";
				sAW3="";
				sAW4="";
				sAW5="";
				sAW6="";
				sAW7="";

				sTI="";
				sNT="";

			  }

		  }
	      cout << "fVcfVersCsv, nomFicEntree=" << nomFicEntree << " nomFicSortie=" << nomFicSortie << " : OK !!!!!!!!!" << endl;
	  }
	  else {
	      cout << "ERREUR: Impossible d'ouvrir " << nomFicEntree<< endl;
	  }
  }
  else {
      cout << "ERREUR: Impossible d'ouvrir " << nomFicSortie << endl;
  }
  ficEntree.close();
  return;
}

//============================================================================
string fChoixNomFic() {
//============================================================================
    int val=0;
    string sNomFic="";

    cout << "choix du nom des fichiers vcf et csv'" << std::endl
			<< "1 : '" << nom1 << ".*'" << std::endl
			<< "2.: '" << nom2 << ".*'" << std::endl
			<< "3.: '" << nom3 << ".*'" << std::endl
			<< "4.autre nom " << std::endl
			<< "9.sortie " << std::endl
			;

    cin >> val;

    	if (val == 1) {
    		sNomFic=nom1;
    	} else if (val == 2) {
        	sNomFic=nom2;
    	} else if (val == 3) {
        	sNomFic=nom3;
    	} else if (val == 4) {
    	    cout << "quels autres fichiers ? " << std::endl;
    		cin >> sNomFic;
    	} else if (val == 9) {
        	sNomFic="";
    	}

	return sNomFic;

}

//============================================================================
int main(void) {
//============================================================================
    std::cout << "SALUT FRED (2024 12 12)"
			  << std::endl;

    string sNomFic = "";
	string prefixeNomFichier="";

    int val=0;
    int env=0;

    while (val<9) {

    	sNomFic = fChoixNomFic();
		prefixeNomFichier="./data/";

    	if (sNomFic.length()>0) {
			cout << "1 prod "  << std::endl
				 << "2.test "  << std::endl
				 << "9.sortie" << std::endl
				 ;
			cin >> env;
    	}
    	else { // sortir
			env=9;
    	}

    	if (env == 1) {
			prefixeNomFichier="./vcard-prod/";
		} else if (env == 2) {
			prefixeNomFichier="./vcard-test/";
    	}

    	if (sNomFic.length()>0) {
			cout << "1 vcf->csv " << sNomFic << std::endl
				 << "2.csv->vcf " << sNomFic << std::endl
				 << "3.csv->txt " << sNomFic << std::endl
				 << "9.sortie" << std::endl
				 ;
			cin >> val;
    	}
    	else { // sortir
			val=9;
    	}

    	if (val == 1) {
        	nomFicEntree= prefixeNomFichier + sNomFic+".vcf";
        	nomFicSortie= prefixeNomFichier + sNomFic+".fVcfVersCsv.csv";
        	fVcfVersCsv();
    	} else if (val == 2) {
        	nomFicEntree= prefixeNomFichier + sNomFic+".csv";
        	nomFicSortie= prefixeNomFichier + sNomFic+".fCsvVersVcf.vcf";
        	fCsvVersVcf();
    	} else if (val == 3) {
        	nomFicEntree= prefixeNomFichier + sNomFic+".csv";
        	nomFicSortie= prefixeNomFichier + sNomFic+".fCsvVersTxt.txt";
        	fCsvVersTxt();
    	}
    }

    cout << "CIAO FRED" << endl;

    return EXIT_SUCCESS;
}
