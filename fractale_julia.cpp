#include <stdio.h>
#include <stdlib.h>
#include <opencv2/opencv.hpp>

/* Compile avec */
/* g++ -Wall fractale_julia.cpp `pkg-config --cflags --libs opencv` -std=c++11 */

/* CONSTANTES *****************************************************************/

#define IMG_W 400
#define IMG_H 400 //768
#define MAX_ITER 300    // 200
#define MAX_NORM 4        // 2

#define LIMIT_LEFT -1
#define LIMIT_RIGHT 1
#define LIMIT_TOP -1
#define LIMIT_BOTTOM 1

/* STRUCTURES GLOBALES CONVERSION HSVtoRGB ************************************/
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

void julia(cv::Mat& img) {
    struct HSV data;
    struct RGB value;

    for (int x = 0; x < IMG_W; x++) {
        for (int y = 0; y < IMG_H; y++) {
            int j = juliaDot(convert(x, y), MAX_ITER);
            data.H = j;
            data.S = 1;
            data.V = 1;
            value = HSVToRGB(data);
            cv::Vec3b color(value.R, value.G, value.B);
            //cv::Vec3b color(j, j, j);
            img.at<cv::Vec3b>(cv::Point(x, y)) = color;
        }
    }
}

/* MAIN ***********************************************************************/

int main(int argc, char * argv[]) {

	/*const int nb_values = 4;
	long double c_values[nb_values][2] = {{-1.41702285618, 0},
																				{0.285, 0.013},
																				{0.285, 0.01},
																				{0.3, 0.5},};

		int i;*/
		long double c_real, c_imaginary;
		int nb_thread, nb_sample;

		/* Vérification des arguments */
		// nb_thread, nb_sample, c_real, c_imaginary
		if (argc != 5) {
				printf("4 arguments attendus\n. argc=%c", argc);
	 			return 0;
			}

 		c_real = strtold(argv[1], NULL);
		c_imaginary = strtold(argv[2], NULL);
		nb_thread = atoi(argv[3]);
		nb_sample = atoi(argv[4]);

    // creation de l'image
    cv::Mat newImg(IMG_H, IMG_W, CV_8UC3);


    //for (i = 0; i < nb_values; i++) {
        // calcul de la fractale
        //c = new_complex(c_values[i][0], c_values[i][1]);
				c = new_complex(c_real, c_imaginary);
        julia(newImg);

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
    //}

    return 0;
}
