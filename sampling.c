#include <stdio.h>
#include <stdlib.h>

/* CONSTANTES *****************************************************************/

#define IMG_W 400
#define IMG_H 400

/* VARIABLES GLOBALES *********************************************************/

typedef struct sample
{
	int xStart;
	int xEnd;
  int yStart;
	int yEnd;
	int processed;
} sample;

sample *matSamples;  // Matrice des échantillons
int nbSample;					// nb d'échantillons

/* Prototype de fonction de traitement ****************************************/
void* julia(void* _p) {
	sample workSample;  // échantillon de travail
	int processing;


	/* Lecture matrice pour extraction échantillon ****************************/
	for(int i = 0; i < nbSample; i++) {
		processing = 0;
		// verrouillage de la matrice des échantillons
		if(matSamples[i].processed == 0) {
			workSample = matSamples[i];
			matSamples[i].processed = 1;
			processing = 1;
		}
		// deverrouillage lecture de la Matrice pour laisser l'accès aux autres threads
		// Calcul si workSample est renseigné, sinon on passe à la boucle suivante
		if (processing == 1) {
			//calcul
			printf("Processing sample %d\n", workSample.xStart);
		}
	}
	return 0;
}

/* MAIN ***********************************************************************/
/* Calcul des échantillons : un échantillon = une colonne plus ou moins haute */
int main(int argc, char * argv[]) {

  int i, size, rest;

  if (argc != 2) {
      printf("Usage : <nb échantillons>\n");
      return 0;
    }

  nbSample = atoi(argv[1]);
  matSamples = (sample *) malloc(nbSample * sizeof(sample));


	if(nbSample > IMG_W) {
		printf("Nombre d'échantillons > colonnes max de l'image. Écrétage à %d\n", IMG_W);
		nbSample = IMG_W;
	}
	size = IMG_W / nbSample;
	rest = IMG_W % nbSample;
  if(rest == 0) {
    // découpage en nbSample morceaux de taille égale
    for(i = 0; i < nbSample; i++ ) {
      matSamples[i].xStart = i * size;
      matSamples[i].xEnd = matSamples[i].xStart + (size -1);
			matSamples[i].yStart = 0;
			matSamples[i].yEnd = IMG_H;
			matSamples[i].processed = 0;
    }
  }
  else {
    // découpage en nbSample-1 morceaux taille égale...
    for(i = 0; i < nbSample; i++ ) {
			if(i < rest) {
				matSamples[i].xStart = i * (size + 1); // on réparti le restant sur le n premier pixels
				matSamples[i].xEnd = matSamples[i].xStart + (size);
			}
			else {
				matSamples[i].xStart = matSamples[i-1].xEnd + 1;
	      matSamples[i].xEnd = matSamples[i].xStart + (size -1);
			}
			matSamples[i].yStart = 0;
			matSamples[i].yEnd = IMG_H;
			matSamples[i].processed = 0;
    }
  }

  /* affichage des échantillons
  for(i = 0; i < nbSample; i++){
    printf("Echantillon %d : xStart : %d, xEnd : %d\n", i, matSamples[i].xStart, matSamples[i].xEnd);
  }*/

	julia(NULL);

  free(matSamples);

  return 0;
}
