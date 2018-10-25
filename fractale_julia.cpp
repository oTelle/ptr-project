#include <stdio.h>
#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include <pthread.h>
#include <semaphore.h>

/* Compile avec */
/* g++ -Wall fractale_julia.cpp `pkg-config --cflags --libs opencv` -lpthread -lm -std=c++11*/

/* CONSTANTES *****************************************************************/

#define IMG_W 400
#define IMG_H 400 //768
#define MAX_ITER 300    // 200
#define MAX_NORM 4        // 2

#define LIMIT_LEFT -1
#define LIMIT_RIGHT 1
#define LIMIT_TOP -1
#define LIMIT_BOTTOM 1

/* VARIABLES GLOBALES *********************************************************/
// Type échantillon
typedef struct sample
{
	int xStart;
	int xEnd;
  int yStart;
	int yEnd;
	int processed;
} sample;

// Matrice des échantillons
sample *matSamples;

int nbSample;					// nb d'échantillons

/* Conversion HSVtoRGB */
struct RGB
{
	unsigned char R;
	unsigned char G;
	unsigned char B;
};

struct HSV
{
	double H;
	double S;
	double V;
};

/* SÉMAPHORES ANONYMES */
sem_t semRead;	// une seule lecture à la fois

// creation de l'image
cv::Mat newImg(IMG_H, IMG_W, CV_8UC3);


/* MANIPULER LES NOMBRES COMPLEXES ********************************************/

typedef struct {
    long double real;
    long double imag;
} complex;
complex c; // GLOBALE

complex new_complex(long double real, long double imag) {
    complex c;
    c.real = real;
    c.imag = imag;
    return c;
}

complex add_complex(complex a, complex b) {
    a.real += b.real;
    a.imag += b.imag;
    return a;
}

complex mult_complex(complex a, complex b) {
    complex m;
    m.real = a.real * b.real - a.imag * b.imag;
    m.imag = a.real * b.imag + a.imag * b.real;
    return m;
}

long double module_complex(complex c) {
    return c.real * c.real + c.imag * c.imag;
}

/* GESTION DES COULEURS *******************************************************/

struct RGB HSVToRGB(struct HSV hsv) {
	double r = 0, g = 0, b = 0;

	if (hsv.S == 0)
	{
		r = hsv.V;
		g = hsv.V;
		b = hsv.V;
	}
	else
	{
		int i;
		double f, p, q, t;

		if (hsv.H == 360)
			hsv.H = 0;
		else
			hsv.H = hsv.H / 60;

		i = (int)trunc(hsv.H);
		f = hsv.H - i;

		p = hsv.V * (1.0 - hsv.S);
		q = hsv.V * (1.0 - (hsv.S * f));
		t = hsv.V * (1.0 - (hsv.S * (1.0 - f)));

		switch (i)
		{
		case 0:
			r = hsv.V;
			g = t;
			b = p;
			break;

		case 1:
			r = q;
			g = hsv.V;
			b = p;
			break;

		case 2:
			r = p;
			g = hsv.V;
			b = t;
			break;

		case 3:
			r = p;
			g = q;
			b = hsv.V;
			break;

		case 4:
			r = t;
			g = p;
			b = hsv.V;
			break;

		default:
			r = hsv.V;
			g = p;
			b = q;
			break;
		}

	}

	struct RGB rgb;
	rgb.R = r * 255;
	rgb.G = g * 255;
	rgb.B = b * 255;

	return rgb;
}

/* FRACTALE DE JULIA *****************************************************/

complex convert(int x, int y) {
   return new_complex(
        ((long double) x / IMG_W * (LIMIT_RIGHT - LIMIT_LEFT)) + LIMIT_LEFT,
        ((long double) y / IMG_H * (LIMIT_BOTTOM - LIMIT_TOP)) + LIMIT_TOP );
}

int juliaDot(complex z, int iter) {
	int i;
    for (i = 0; i < iter; i++) {
        z = add_complex(mult_complex(z, z), c);
        long double norm = module_complex(z);
        if (norm > MAX_NORM) {
            break;
        }
    }
    return i * 255 / iter; // on met i dans l'intervalle 0 à 255
}

void* julia(void* _p) {
    struct HSV data;
    struct RGB value;
		//sample workSample;   //échantillon de travail
		int processing;

		/* Lecture matrice pour extraction échantillon ****************************/
		for(int i = 0; i < nbSample; i++) {
			processing = 0;
			// verrouillage de la matrice des échantillons
			sem_wait(&semRead);  // VERIFIER SI LE VERROUILLAGE EST BIEN PLACE
			if(matSamples[i].processed == 0) {
				//workSample = matSamples[i];
				matSamples[i].processed = 1;
				processing = 1;
			}
			// deverrouillage lecture de la Matrice pour laisser l'accès aux autres threads
			sem_post(&semRead);

			if (processing == 1) {
				/* Début calcul fractale sur échantillon */
				for (int x = matSamples[i].xStart; x <= matSamples[i].xEnd; x++) {
		        for (int y = matSamples[i].yStart; y <= matSamples[i].yEnd; y++) {
		            int j = juliaDot(convert(x, y), MAX_ITER);
		            data.H = j; // pour le moment 0-255 à convertir en 0-1 ?
		            data.S = 1;
		            data.V = 1;
		            value = HSVToRGB(data);
		            cv::Vec3b color(value.R, value.G, value.B);
		            newImg.at<cv::Vec3b>(cv::Point(x, y)) = color;
		        }
		    }
				/* fin calcul fractale */
			}
		}

	return NULL;
}

/* MAIN ***********************************************************************/
int main(int argc, char * argv[]) {

	/*const int nb_values = 4;
	long double c_values[nb_values][2] = {{-1.41702285618, 0},
																				{0.285, 0.013},
																				{0.285, 0.01},
																				{0.3, 0.5},};

		int i;*/
		long double cReal, cImaginary;
		int nbThread, i, size, rest;
		pthread_t* idThread;

		/* Vérification des arguments */
		// nbThread, nbSample, cReal, cImaginary
		if (argc != 5) {
				printf("Usage : <C réel> <C imaginaire> <nb threads> <nb échantillons>\n");
	 			return 0;
			}

		// Voir la commande opencv commandlineparser ?
 		cReal = strtold(argv[1], NULL);
		cImaginary = strtold(argv[2], NULL);
		nbThread = atoi(argv[3]);
		nbSample = atoi(argv[4]);

		/* INITIALISATION THREADS, SEMAPHORES ET ECHANTILLONS *********************/
		idThread = (pthread_t*) malloc(nbThread * sizeof(pthread_t));
		sem_init(&semRead, 0, 1);

		c = new_complex(cReal, cImaginary);

		/* Construbction de la matrice des échantillons / taille image *************/
		if(nbSample > IMG_W) {
			printf("Nombre d'échantillons > colonnes max de l'image. Écrétage à %d\n", IMG_W);
			nbSample = IMG_W;
		}
		matSamples  = (sample *) malloc(nbSample * sizeof(sample));
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

		/* CRÉATION DES THREADS ***************************************************/
		for(i = 0; i < nbThread; i++) {
	      pthread_create(&idThread[i], NULL, julia, NULL);
	  }

		for(i = 0; i < nbThread; i++) {
	    	pthread_join(idThread[i], 0);
	  }

    // interaction avec l'utilisateur
    char key = -1; // -1 indique qu'aucune touche est enfoncée

    // on attend 30ms une saisie clavier, key prend la valeur de la touche
    // si aucune touche est enfoncée, au bout de 30ms on exécute quand même
    // la boucle avec key = -1, l'image est mise à jour
    while( (key = cvWaitKey(30)) ) {
        if (key == 'q')
            break;
        imshow("image", newImg); // met à jour l'image
    }
    char name[15];
    sprintf(name, "image.bmp");
    imwrite(name, newImg); // sauve une copie de l'image
    cvDestroyWindow("image"); // ferme la fenêtre

		/* NETTOYAGE **************************************************************/
		sem_destroy(&semRead);

		free(idThread);
		free(matSamples);

    return 0;
}
