#pragma region "Includes"//{
#define _CRT_SECURE_NO_WARNINGS // On permet d'utiliser les fonctions de copies de chaînes qui sont considérées non sécuritaires.

#include "structures.hpp"      // Structures de données pour la collection de films en mémoire.

#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <algorithm>
#include <span>

#include "cppitertools/range.hpp"

#include "bibliotheque_cours.hpp"
#include "verification_allocation.hpp" // Nos fonctions pour le rapport de fuites de mémoire.
#include "debogage_memoire.hpp"        // Ajout des numéros de ligne des "new" dans le rapport de fuites.  Doit être après les include du système, qui peuvent utiliser des "placement new" (non supporté par notre ajout de numéros de lignes).

using namespace std;
using namespace iter;

#pragma endregion//}

typedef uint8_t UInt8;
typedef uint16_t UInt16;

#pragma region "Fonctions de base pour lire le fichier binaire"//{

UInt8 lireUint8(istream& fichier)
{
	UInt8 valeur = 0;
	fichier.read((char*)&valeur, sizeof(valeur));
	return valeur;
}
UInt16 lireUint16(istream& fichier)
{
	UInt16 valeur = 0;
	fichier.read((char*)&valeur, sizeof(valeur));
	return valeur;
}
string lireString(istream& fichier)
{
	string texte;
	texte.resize(lireUint16(fichier));
	fichier.read((char*)&texte[0], streamsize(sizeof(texte[0])) * texte.length());
	return texte;
}

#pragma endregion//}

void ajouterActeur(ListeActeurs& listeActeurs, Acteur* acteur) {

	if (listeActeurs.nElements >= listeActeurs.capacite) {
		if (listeActeurs.capacite == 0) {
			listeActeurs.capacite = 1;
		}
		else {
			listeActeurs.capacite *= 2;
		}
		Acteur** nouvelleListe = new Acteur * [listeActeurs.capacite];
		copy(listeActeurs.elements, listeActeurs.elements + listeActeurs.nElements, nouvelleListe);
		delete[] listeActeurs.elements;
		listeActeurs.elements = nouvelleListe;
	}
	listeActeurs.elements[listeActeurs.nElements++] = acteur;
}

void ajouterFilm(ListeFilms& listeFilms, Film* film) {

	if (listeFilms.nElements >= listeFilms.capacite) {
		if (listeFilms.capacite == 0) {
			listeFilms.capacite = 1;
		}
		else {
			listeFilms.capacite *= 2;
		}
		Film** nouvelleListe = new Film * [listeFilms.capacite];
		copy(listeFilms.elements, listeFilms.elements + listeFilms.nElements, nouvelleListe);
		delete[] listeFilms.elements;
		listeFilms.elements = nouvelleListe;
	}
	listeFilms.elements[listeFilms.nElements++] = film;
}

void enleverFilm(ListeFilms& listeFilms, Film* inputFilm)
{
	for (Film* film : span(listeFilms.elements, listeFilms.nElements)) {
		if (film == inputFilm) {
			film = listeFilms.elements[listeFilms.nElements - 1];
			delete listeFilms.elements[listeFilms.nElements--];
			break;
		}
	}
}

Acteur* trouverActeur(const ListeFilms& listeFilms, const string& nomActeur)
{
	for (Film* film : span(listeFilms.elements, listeFilms.nElements)) {
		for (Acteur* acteur : span(film->acteurs.elements, film->acteurs.nElements)) {
			if (acteur->nom == nomActeur) {
				return acteur;
			}
		}
	}
	return nullptr;
}

Acteur* lireActeur(ListeFilms& listeFilms, istream& fichier)
{
	Acteur* acteur = new Acteur;

	string nomActeur = lireString(fichier);
	acteur->nom = nomActeur;
	acteur->anneeNaissance = lireUint16(fichier);
	acteur->sexe = lireUint8(fichier);
	if (trouverActeur(listeFilms, nomActeur) != nullptr) {
		cout << "Nom de l'acteur (EXISTANT): " << trouverActeur(listeFilms, nomActeur)->nom << endl;
		delete acteur;
		return trouverActeur(listeFilms, nomActeur);
	}

	ListeFilms acteurListesFilms = {};
	acteurListesFilms.capacite = 1;
	acteurListesFilms.nElements = 0;
	acteurListesFilms.elements = new Film * [acteurListesFilms.capacite];
	acteur->joueDans = acteurListesFilms;
	cout << "Nom de l'acteur (NOUVEAU): " << acteur->nom << endl;
	return acteur;
}

Film* lireFilm(ListeFilms& listeFilms, istream& fichier)
{
	Film* film = new Film;

	film->titre = lireString(fichier);
	film->realisateur = lireString(fichier);
	film->anneeSortie = lireUint16(fichier);
	film->recette = lireUint16(fichier);
	film->acteurs.capacite = lireUint8(fichier);
	film->acteurs.nElements = 0;
	film->acteurs.elements = new Acteur * [film->acteurs.capacite];

	for (int i : range(film->acteurs.capacite)) {
		ajouterActeur(film->acteurs, lireActeur(listeFilms, fichier));
		ajouterFilm(film->acteurs.elements[i]->joueDans, film);
	}
	return film;
}

ListeFilms creerListe(string nomFichier)
{
	ifstream fichier(nomFichier, ios::binary);
	fichier.exceptions(ios::failbit);

	int nElements = lireUint16(fichier);

	ListeFilms listeFilms = {};
	listeFilms.capacite = 1;
	listeFilms.nElements = 0;
	listeFilms.elements = new Film * [listeFilms.capacite];

	for (int i : range(nElements)) {
		cout << "Boucle n." << i + 1 << endl;
		ajouterFilm(listeFilms, lireFilm(listeFilms, fichier));
		cout << "Capacite listeFilms: " << listeFilms.capacite << endl;
		cout << "nElements listeFilms: " << listeFilms.nElements << endl;
	}

	return listeFilms;
}

//TODO: Une fonction pour détruire un film (relâcher toute la mémoire associée à ce film, et les acteurs qui ne jouent plus dans aucun films de la collection).  Noter qu'il faut enleve le film détruit des films dans lesquels jouent les acteurs.  Pour fins de débogage, affichez les noms des acteurs lors de leur destruction.
void detruireFilm(Film* film)
{
	for (Acteur* acteur : span(film->acteurs.elements, film->acteurs.nElements)) {
		for (Film* filmActeur : span(acteur->joueDans.elements, acteur->joueDans.nElements)) {
			if (filmActeur == film) {
				filmActeur = acteur->joueDans.elements[--acteur->joueDans.nElements];
				break;
			}
		}
		if (acteur->joueDans.nElements == 0) {
			cout << "Destruction de l'acteur: " << acteur->nom << endl;
			delete[] acteur->joueDans.elements;
			delete acteur;
		}
	}
	delete[] film->acteurs.elements;
	delete film;
}

void detruireListeFilms(ListeFilms* listeFilms)
{
	for (Film* film : span(listeFilms->elements, listeFilms->nElements)) {
		delete film;
	}
	delete listeFilms;
}

void afficherActeur(const Acteur& acteur)
{
	cout << "  " << acteur.nom << ", " << acteur.anneeNaissance << " " << acteur.sexe << endl;
}

void afficherFilm(const Film& film)
{
	cout << "Nom du film: " << film.titre << ", Acteurs:" << endl;
	for (Acteur* acteur : span(film.acteurs.elements, film.acteurs.nElements)) {
		afficherActeur(*acteur);
	}
}

void afficherListeFilms(const ListeFilms& listeFilms)
{
	static const string ligneDeSeparation = "\n";
	cout << ligneDeSeparation;
	for (Film* film : span(listeFilms.elements, listeFilms.nElements)) {
		afficherFilm(*film);
		cout << ligneDeSeparation;
	}
}

void afficherFilmographieActeur(const ListeFilms& listeFilms, const string& nomActeur)
{
	const Acteur* acteur = trouverActeur(listeFilms, nomActeur);
	if (acteur == nullptr)
		cout << "Aucun acteur de ce nom" << endl;
	else
		afficherListeFilms(acteur->joueDans);
}

int main()
{
	bibliotheque_cours::activerCouleursAnsi();  // Permet sous Windows les "ANSI escape code" pour changer de couleurs https://en.wikipedia.org/wiki/ANSI_escape_code ; les consoles Linux/Mac les supportent normalement par défaut.

	static const string ligneDeSeparation = "\n\033[35m════════════════════════════════════════\033[0m\n";

	//TODO: Chaque TODO dans cette fonction devrait se faire en 1 ou 2 lignes, en appelant les fonctions écrites.

	//TODO: La ligne suivante devrait lire le fichier binaire en allouant la mémoire nécessaire.  Devrait afficher les noms de 20 acteurs sans doublons (par l'affichage pour fins de débogage dans votre fonction lireActeur).
	ListeFilms listeFilms = creerListe("films.bin");
	cout << "CREERLISTE DONE" << endl;

	cout << ligneDeSeparation << "Le premier film de la liste est:" << endl;
	//TODO: Afficher le premier film de la liste.  Devrait être Alien.
	afficherFilm(*listeFilms.elements[0]);

	cout << ligneDeSeparation << "Les films sont:" << endl;
	//TODO: Afficher la liste des films.  Il devrait y en avoir 7.

	//TODO: Modifier l'année de naissance de Benedict Cumberbatch pour être 1976 (elle était 0 dans les données lues du fichier).  Vous ne pouvez pas supposer l'ordre des films et des acteurs dans les listes, il faut y aller par son nom.

	cout << ligneDeSeparation << "Liste des films où Benedict Cumberbatch joue sont:" << endl;
	//TODO: Afficher la liste des films où Benedict Cumberbatch joue.  Il devrait y avoir Le Hobbit et Le jeu de l'imitation.

	//TODO: Détruire et enlever le premier film de la liste (Alien).  Ceci devrait "automatiquement" (par ce que font vos fonctions) détruire les acteurs Tom Skerritt et John Hurt, mais pas Sigourney Weaver puisqu'elle joue aussi dans Avatar.

	cout << ligneDeSeparation << "Les films sont maintenant:" << endl;
	//TODO: Afficher la liste des films.

	//TODO: Faire les appels qui manquent pour avoir 0% de lignes non exécutées dans le programme (aucune ligne rouge dans la couverture de code; c'est normal que les lignes de "new" et "delete" soient jaunes).  Vous avez aussi le droit d'effacer les lignes du programmes qui ne sont pas exécutée, si finalement vous pensez qu'elle ne sont pas utiles.

	//TODO: Détruire tout avant de terminer le programme.  La bibliothèque de verification_allocation devrait afficher "Aucune fuite detectee." a la sortie du programme; il affichera "Fuite detectee:" avec la liste des blocs, s'il manque des delete.
}
