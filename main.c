
/**
 * Copyright 2012 Eric Tremblay TIandSE@gmail.com.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/*********************************************************************************************************************
 *
 * Algorithmes de remplacements des pages et treads.
 *
 *
 * Ce logiciel est une Simulation des algorithmes de remplacements des pages
 * Chaque algorithme s'execute sur son propre thraead et le main affiche ensuite leurs resultat dans l'ordre suivant.
 *
 * – Optimal
 * – Horloge
 * – LRU
 *
 * Ce logicle compile et s'execute sur les machiens Raon1 et Chicorre des laboratoire de l'UQAM
 *
 * Dans le cadre du cours : INF3172 - Principes des systemes d'exploitation
 * par                    : Eric Tremblay
 * UQAM
 * 18 decembre 2012
 **********************************************************************************************************************/

typedef struct _data *Data;
typedef struct _resultats *Resultats;

enum boolean {
    faux, vrai
};

/**********************************************************************************************************************
 * Structure servant aux transfert des donnees entre les threads et le main().
 * pointeur : tableau   : int* : pointeur vers les pages
 * var      : nb_page   : int  : le nombre de page du tableau
 * var      : nb_cadre  : int  : il s'agit du nombre de cadre, donc de la taill des int* tableaux, cadre et R
 * var      : nb_defaut : int  : defaut de page ( une valeur retour utilisee par les 3 aglorithmes )
 * pointeur : cadre     : int* : un pointeur vers les cadres
 * pointeur : R         : int* : pointeur vers les BIT de page referencee.
 **********************************************************************************************************************/
struct _data {
    int *tableau;
    int nb_page;
    int nb_cadre;
    int nb_defaut;
    int *cadre;
    int *R;

};

/**
 * Structure servant de conteneur de retour pour les resultats des Threads executants les algorithmes Horloge et LRU.
 * var : nb_defaut : int  : nombre de defaut de page
 * var : nb_cadre  : int  : nombre de cadre
 * var : cadre     : *int : pointeur vers les cadres
 * var : R         : *int : pointeur vers les BIT de reference( utilise par l'algorithme Horloge )
 */
struct _resultats {
    int nb_defaut;
    int nb_cadre;
    int *cadre;
    int *R;

};

/**********************************************************************************************************************
 * Fonction qui retourne l'indice de la premiere itaration de i dans le tableau.
 * param : i : int : l'entier dont on cherche l'indice
 * param : taille_tableau : int : la taille du tableau
 * param : tableau : int[] : un tableau d'entier sur lequel effectuer la recherche
 * retour : indice : int : l'indice ou se retrouve i
 * erreur : retourne -1 si l'indice n'est pas trouve.
 **********************************************************************************************************************/
int trouver_indice(int i, int taille_tableau, int tableau[]) {

    int j;
    int indice;

    enum boolean trouve;

    trouve = faux;

    indice = -1;
    j = 0;


    while (!trouve && j < taille_tableau) {

        if (tableau[j] == i) {

            indice = j;
            trouve = vrai;

        }

        ++j;

    }

    return indice;

}

/**********************************************************************************************************************
 * Constructeur de la structure Data
 * param : data : Data : Struicture a construire
 * param : nb_cadre : int : nombre de cadre associee a la structure
 * param : R        : int[] : tableau des BIT de referencement
 * param : cadre    : int[] : tableaux contenant les cadres
 **********************************************************************************************************************/
Data Creation_Data(char * nb_cadre, int taille, int *tableau) {

    Data resultat = NULL;

    resultat = (Data) malloc(sizeof ( struct _data));

    if (resultat == NULL) {
        fprintf(stderr, "Creation_Data: Erreur d'allocation");
        exit(-1);
    }

    resultat->nb_page = taille;
    resultat->nb_cadre = atoi(nb_cadre);
    resultat->tableau = tableau;


    return resultat;
}

/**********************************************************************************************************************
 * Recupere les donnees en entree et les places dans une structure de type Data
 * param  : nb_Cadre : char * : pointeur ver la chaine de caracter correspondant au nombre de cadre
 * retour : data     : Data   : strucutre contenant les donees recuperees
 * erreur : souleve une erreur s'il y a un probleme d'allocation.
 **********************************************************************************************************************/
Data remplir_tableau_page(char * nb_cadre) {

    Data data = NULL;
    char delimiteur[] = " ";
    char *resultat = NULL;
    int retour;
    int *temp = NULL;
    int *tableau_pages = NULL;
    int nb_page;

    retour = 0;

    int i;

    while (scanf("%d", &i) == 1) {

        temp = realloc(tableau_pages, sizeof (int) * (retour + 1));

        if (temp == NULL) {

            fprintf(stderr, "Erreur : remplir_tableau_page() : probleme d'allocation de memoire.");
            exit(-1);
        } else {

            tableau_pages = temp;

        }

        tableau_pages[retour] = i;

        ++retour;
        fflush(stdin);
        //printf("%d, ",i);

    }

    //printf("%d\n", retour);
    data = Creation_Data(nb_cadre, retour, tableau_pages);

    return data;

}

/**********************************************************************************************************************
 * Fonction qui permet de savoir si un entier existe dans un tableau.
 * param : page : int : l'entier recherche
 * param : taille_tableau : int[] : le tableau dans lequel effectuer la recherche
 * retour : trouve : boolean : vrai si trouve sinon faux.
 **********************************************************************************************************************/
enum boolean exist(int page, int taille_tableau, int tableau[]) {

    int i;
    enum boolean trouve;

    trouve = faux;

    for (i = 0; i < taille_tableau; ++i) {


        if (!trouve && page == tableau[i]) {

            trouve = vrai;

        }

    }

    return trouve;

}

/**********************************************************************************************************************
 * Fonction permettant a l'algorithme optimal  de 'voir' les pages a venir
 * param : indice : int : l'indice de depart
 * param : data : Data : la structure contenant la liste des pages et les cadres
 * param : tab_cadre : int : le tableau contenant les cadres.
 * retour : indice_retour : int : indice de la page avec le plus loin defaut de page ( dans tab_cadre )
 **********************************************************************************************************************/
int plus_loin_defaut_page(int indice, Data data, int tab_cadre[]) {

    int i;
    int j;
    int compteur;
    int indice_retour;
    enum boolean trouve;

    trouve = faux;

    i = 0;
    indice_retour = 0;
    //verifie s'il existe une page qui ne provoquera pas de defaut de page dans l'avenir, si oui on l'ecrase.
    while (!trouve && i < data->nb_cadre) {
        j = indice;
        compteur = 0;
        while (j < data->nb_page) {

            if (tab_cadre[i] == data->tableau[j]) {

                ++compteur;

            }
            //printf("%d\n", compteur);
            ++j;

        }

        if (compteur == 0) {//si le compteur  == 0 alors la page du cadre i ne va pas creer de defaut de page alors on la remplacera

            trouve = vrai;
            indice_retour = i;

        }

        ++i;
    }

    if (!trouve) {
        i = indice;
        while (i < data->nb_page) {

            if (exist(data->tableau[i], data->nb_cadre, tab_cadre)) {
                indice_retour = trouver_indice(data->tableau[i], data->nb_cadre, tab_cadre);
                //printf("indice retour = %d\n", indice_retour);
            }

            ++i;
        }

    }
    return indice_retour;

}

/**********************************************************************************************************************
 * Implementation de l'algorithme de pagination Optimal
 * Cet algorithme determine le nombre de defaut de page minimum.
 * param : dat : void* : pointeur vers une structure(Data) contenant les donnees necessaires
 * a l'execution de l'algorithme.
 * retour : nb_defaut_page : int : le nombre de defaut de page minimum
 * la valeur de retour sera place dans la structure dat.
 **********************************************************************************************************************/
void* optimal(void * dat) {

    int i;
    int j;
    int nb_cadre;

    int nb_defaut_page;
    int indice;
    Data data = NULL;
    data = dat;

    nb_defaut_page = 0;
    nb_cadre = data->nb_cadre;

    int tab_cadre[nb_cadre];


    for (i = 0; i < nb_cadre; ++i) {

        tab_cadre[i] = -1;
        //++nb_defaut_page;

    }

    i = 0;

    for (; i < data->nb_page; ++i) {//parcourir toute la liste des pages


        if (!exist(data->tableau[i], nb_cadre, tab_cadre)) {//s'il y a un defaut de page

            ++nb_defaut_page;

            indice = plus_loin_defaut_page(i + 1, data, tab_cadre);
            tab_cadre[indice] = data->tableau[i];

        }

    }


    data->nb_defaut = nb_defaut_page;

    pthread_exit(dat);

}

/**********************************************************************************************************************
 * Implementation de l'algorithme de pagination Horloge
 * param : dat : void* : pointeur vers une structure(Data) contenant les donnees necessaires
 * a l'execution de l'algorithme.
 * retour : nb_defaut_page : int : le nombre de defaut de page minimum
 * retour : tab_cadre : int* : un pointeur vers les cadres
 * retour : matrice_R : int* : un pointeur vers les BIT de referencement
 * pthread_exit : les valeurs de retour seront placees dans la structure dat.
 **********************************************************************************************************************/
void* horloge(void * dat) {

    int i;
    int *tab_cadre;
    int *matrice_R;
    int nb_defaut_page;
    int indice;
    int nb_cadre;


    int aiguille;
    int iterateur;

    Data data = NULL;
    data = dat;
    Resultats resultats = NULL;

    resultats = malloc(sizeof (Resultats));


    nb_cadre = data->nb_cadre;

    tab_cadre = malloc(sizeof (int) * nb_cadre);
    matrice_R = malloc(sizeof (int) * nb_cadre);

    nb_defaut_page = 0;
    aiguille = 0;
    iterateur = 0;
    indice = 0;


    //remplissage des cadre vides.
    for (i = 0; i < nb_cadre; ++i) {

        tab_cadre[i] = -1;
        matrice_R[i] = 0;
        //++nb_defaut_page;
        ++iterateur;

    }

    i = 0;
    iterateur = 0;
    int h;
    while (iterateur < data->nb_page) {//tout la liste des page sera traitee


        if (!exist(data->tableau[iterateur], nb_cadre, tab_cadre)) {//defaut de page
            ++nb_defaut_page;


            while (matrice_R[aiguille] != 0) {//l'aiguille est deplacee et recherche un R == 0

                matrice_R[aiguille] = 0; //si R == 1 on le met a zero
                ++aiguille;

                if (!(aiguille < nb_cadre)) {//la liste est circulaire
                    aiguille -= nb_cadre;
                }

            }

            tab_cadre[aiguille] = data->tableau [iterateur];
            matrice_R[aiguille] = 1;
            ++aiguille;
            if (!(aiguille < nb_cadre)) {//la liste est circulaire
                aiguille -= nb_cadre;
            }

        } else {//si la page est deja dans un cadre

            indice = trouver_indice(data->tableau[iterateur], nb_cadre, tab_cadre);

            if (matrice_R[indice] == 0) {//son bit R sera mis a 1 s'il etait a 0

                matrice_R[indice] = 1;

            }

        }
        ++iterateur;
    }

    data->nb_defaut = nb_defaut_page;
    data->cadre = tab_cadre;
    data->R = matrice_R;


    resultats->nb_defaut = nb_defaut_page;
    resultats->nb_cadre = data->nb_cadre;

    resultats->cadre = malloc(sizeof (int) * (data->nb_cadre));
    resultats->R = malloc(sizeof (int) * (data->nb_cadre));

    resultats->cadre = data->cadre;
    resultats->R = data->R;

    pthread_exit(resultats);

}

/**********************************************************************************************************************
 * Implementation de l'algorithme de pagination LRU
 * param : dat : void* : pointeur vers une structure(Data) contenant les donnees necessaires
 * a l'execution de l'algorithme.
 * retour : nb_defaut_page : int : le nombre de defaut de page minimum
 * retour : tab_cadre : int* : un pointeur vers les cadres
 * retour : matrice_R : int* : un pointeur vers les BIT de referencement
 * pthread_exit : les valeurs de retour seront placees dans la structure dat.
 **********************************************************************************************************************/
void* lru(void* dat) {

    int i;
    int j;
    int temp;
    int *tab_cadre;
    int nb_defaut_page;
    int nb_cadre;
    int indice;

    Data data = NULL;
    Resultats resultats = NULL;
    resultats = malloc(sizeof (Resultats));
    data = dat;

    nb_cadre = data->nb_cadre;
    int file[nb_cadre];
    indice = 0;
    tab_cadre = malloc(sizeof (int) * nb_cadre);


    //remplissage des cadre vides.
    for (i = 0; i < nb_cadre; ++i) {

        tab_cadre[i] = data->tableau[i];
        file[i] = data->tableau[i];
        ++nb_defaut_page;

    }

    temp = file[0]; //garde la page la plus ancienne en memoire


    for (i = 0; i < data->nb_page; ++i) {//parcours toute la liste des pages


        if (exist(data->tableau[i], nb_cadre, tab_cadre)) {

            indice = trouver_indice(data->tableau[i], nb_cadre, file);
            temp = file[indice];

            for (j = indice; j < nb_cadre - 1; ++j) {//decale les page de -1 position

                file[j] = file[j + 1];

            }

            file[nb_cadre - 1] = data->tableau[i];

            temp = file[0]; //garde la page la plus ancienne en memoire

        } else {
            int p;

            for (j = 0; j < nb_cadre - 1; ++j) {//decale les page de -1 position

                file[j] = file[j + 1];

            }

            file[nb_cadre - 1] = data->tableau[i];
            indice = trouver_indice(temp, nb_cadre, tab_cadre);
            tab_cadre[indice] = data->tableau[i];
            ++nb_defaut_page;

            temp = file[0]; //garde la page la plus ancienne en memoire

        }

    }

    data->nb_defaut = nb_defaut_page;
    data->cadre = tab_cadre;

    //data->nb_defaut = nb_defaut_page;
    data->cadre = tab_cadre;


    resultats->nb_defaut = nb_defaut_page;
    resultats->nb_cadre = nb_cadre;
    resultats->cadre = malloc(sizeof (int) * (nb_cadre));
    resultats->cadre = data->cadre;


    pthread_exit(resultats);

}

int main(int argc, char *argv[]) {

    int i;
    int nb_cadre;
    int liste_algo;
    int liste_cadre[atoi(argv[1])];

    Data data = NULL;
    int resultat_optimal;
    Resultats resultat_horloge = NULL;
    Resultats resultat_LRU = NULL;

    void* ptr_retour_opt;

    pthread_t th_opt, th_clock, th_lru;

    data = (Data) malloc(sizeof ( struct _data));
    data = remplir_tableau_page(argv[1]);


    if (pthread_create(&th_opt, NULL, optimal, (void*) data) < 0) {
        fprintf(stderr, "erreur: pthread_create\n");
        exit(1);
    }

    if (pthread_create(&th_clock, NULL, horloge, (void*) data) < 0) {
        fprintf(stderr, "erreur: pthread_create\n");
        exit(1);
    }

    if (pthread_create(&th_lru, NULL, lru, (void*) data) < 0) {
        fprintf(stderr, "erreur: pthread_create\n");
        exit(1);
    }

    printf("Programme de simulation des algorithmes de remplacement des pages\n\n");
    printf("#Nombre de defaut des pages (NDP)\n");

    if (!pthread_join(th_opt, &ptr_retour_opt)) {
        printf("NDP OPTIMAL : %d\n", ((Data) ptr_retour_opt)->nb_defaut);
    } else {
        fprintf(stderr, "ERREUR : pthread_join : th_opt\n");
    }
    if (!pthread_join(th_clock, &ptr_retour_opt)) {
        resultat_horloge = (Resultats) ptr_retour_opt;
        printf("NDP HORLOGE : %d\n", resultat_horloge->nb_defaut);
    } else {
        fprintf(stderr, "ERREUR : pthread_join : th_opt\n");
    }

    if (!pthread_join(th_lru, &ptr_retour_opt)) {
        resultat_LRU = (Resultats) ptr_retour_opt;
        printf("NDP LRU : %d\n", resultat_LRU->nb_defaut);
    } else {
        fprintf(stderr, "ERREUR : pthread_join : th_opt\n");
    }

    printf("#Contenus des cadres de %d a %d\n", 1, resultat_horloge->nb_cadre);
    printf("#HORLOGE :");

    for (i = 0; i < resultat_horloge->nb_cadre; ++i) {

        printf(" %d%c ", resultat_horloge->cadre[i], (resultat_horloge->R[i] == 1 ? 'R' : ' '));
    }

    printf("\n");
    printf("#LRU :");

    for (i = 0; i < resultat_LRU->nb_cadre; ++i) {

        printf(" %d ", resultat_LRU->cadre[i]);

    }

    printf("\n");
    exit(0);
}
