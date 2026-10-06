//============================================================================
// Name        : ParseVCard.cpp
// Author      : F SCHOEN
// Version     : 41212a
// Copyright   : F SCHOEN
// Description : conversion aller et retour Vcf/Csv
//============================================================================
// fonctions:
// void fCsvVersHtml(void)
//    la 1ère ligne conitent 'texteCherche'
//============================================================================

#include <iostream>     // std::cout
#include <fstream>      // std::ifstream

using namespace std;

string nomFicEntree="";
string nomFicSortie="";

string nom1="liensHTML";

string texteCherche="";
string colonne1="";
string colonne2="";

//============================================================================
void fCsvVersHtml(void) {
	//============================================================================
	std::cout << "exec fCsvVersHtml" << endl;

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

	int posTab=0;
	int posFin=0;
	int posTexteCherche=0;
	int numLigne=0;


	ficSortie	<< "<html>"<< endl;
	ficSortie	<< "<body>"<< endl;
	ficSortie	<< "<table border='1'>"<< endl;

	// la 1ère ligne conitent 'texteCherche'
	getline(ficEntree, ligneEntree); 
	numLigne+=1;
	posFin=ligneEntree.size();
	if (posFin>2) {
			posTab = ligneEntree.find("\t");
			texteCherche=ligneEntree.substr(posTab+1,posFin);
			cout << "fCsvVersHtml, texteCherche=" << texteCherche << endl;
	} else {
		texteCherche="azertyuiop1234567890"; //cas impossible
	}
	cout << "fCsvVersHtml, texteCherche=<" << texteCherche  << ">" << endl;
	
	//suite
	string colonne2="";
	string libelle="";
	
	while(getline(ficEntree, ligneEntree)) {

		numLigne+=1;
		posFin=ligneEntree.size();
		if (posFin>2) {

			posTab = ligneEntree.find("\t");
			colonne1=ligneEntree.substr(0,posTab);
			colonne2=ligneEntree.substr(posTab+1,posFin);
			cout << "fCsvVersHtml, colonne1=" << colonne1 << endl;
			cout << "fCsvVersHtml, colonne2=" << colonne2 << endl;

			libelle=colonne2;
			cout << "fCsvVersHtml, libelle=" << libelle << endl;

			ficSortie	<< "<tr>" << endl;
			ficSortie	<< "<td width='10'>" << endl;
			ficSortie	<< numLigne << endl;
			ficSortie	<< "</td>" << endl;
			ficSortie	<< "<td width='400'>" << endl;
			ficSortie	<< colonne1 << endl;
			ficSortie	<< "</td>" << endl;
			ficSortie	<< "<td>" << endl;
			ficSortie	<< "<a href='" << colonne2  << "' target='fenetre F1'>" << libelle << "</a>" << "<br>" << endl;
			ficSortie	<< "</td>"<< endl;
			ficSortie	<< "</tr>"<< endl;
		
		}
	}

	ficSortie	<< "</table>"<< endl;
	ficSortie	<< "</body>"<< endl;
	ficSortie	<< "</html>"<< endl;

	ficEntree.close();
	ficSortie.close();
	cout << "fCsvVersHtml, nomFicEntree=" << nomFicEntree << " nomFicSortie=" << nomFicSortie << " : OK !!!!!!!!!" << endl;
	return;
}


//============================================================================
string fChoixNomFic() {
//============================================================================
    int val=0;
    string sNomFic="";

    cout << "choix du nom des fichiers vcf et csv'" << std::endl
			<< "1 : '" << nom1 << ".*'" << std::endl
			<< "2.autre nom " << std::endl
			<< "9.sortie " << std::endl
			;

    cin >> val;

    	if (val == 1) {
    		sNomFic=nom1;
    	} else if (val == 2) {
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

    int val=0;

    while (val<9) {

    	sNomFic = fChoixNomFic();

    	if (sNomFic.length()>0) {
			cout << "1 csv->html " << sNomFic << std::endl
				 << "9.sortie" << std::endl
				 ;
			cin >> val;
    	}
    	else { // sortir
			val=9;
    	}
		string prefixeNomFichier;
		prefixeNomFichier="./data/";
    	if (val == 1) {
        	nomFicEntree= prefixeNomFichier + sNomFic+".csv";
        	nomFicSortie= prefixeNomFichier + sNomFic+".fCsvVersHtml.html";
        	fCsvVersHtml();
    	}
    }

    cout << "CIAO FRED" << endl;

    return EXIT_SUCCESS;
}
