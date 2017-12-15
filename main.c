/*
Program oblicza szybka transformate Fouriera
oraz transformacje odwrotna. Dane wejsciowe
i wyjsciowe sa zapisane w plikach.
*/

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
/* Funkcja strcmp uzywana w funkcji readArgs()*/
#include <string.h>
/* Sin i cos uzywane w funkcji fft()*/
#include <math.h>
/* biblioteka FFTW */
#include <fftw3.h>
/* dla free() malloc() i exit() */
#include <stdlib.h>


// STA£E

/* Stala uzywana w funkcji fft(); */
#define PI 3.14159

#define ACCURACY 0.001

/* Stale uzywane do okreslania wartosci zmiennej is_scaled */
#define FALSE 0
#define TRUE 1

/* Stale uzywane do okreslania wartosci zmiennej mode */
#define FFT 0
#define IFFT 1
#define FFTW 2
#define IFFTW 3


// TYPY

/* Typ danych przeznaczony do reprezentacji liczb zespolonych */
typedef struct zespolona {
	double re, im;
} complex;


// DEKLARACJE

/*Funkcja wczytuje argumenty linii polecen;
argc - liczba argumentow;
argv[] - argumenty;
szInput - plik wejsciowy;.\fftw_lib
szOutput - plik wyjsciowy;
is_scaled - parametr skalowania;
mode - parametr trybu obliczen;
brak wartosci zwracanej.*/
void readArgs(int argc, char *argv[], char **szInput, char **szOutput, int *is_scaled, int *mode);

/*Funkcja zlicza dane wejsciowe; informacja o ilosc idanych wejsciowych jest potrzebna do stworzenia odpowiednio duzej tablicy, w ktorej przechowywane beda dane odczytane z pliku wejsciowego*/
int countSamples(const char *szInput, int mode);

/* Funkcja wczytuje dane z pliku wejsciowego i zapisuje je w tablicy o elementach zespolonych;
szInput - plik wejsciowy;
numOfSamples - liczba danych wejsciowych, dlugosc tablicy na dane wejsciowe;
Funkcja zwraca adres tablicy, do ktoej zostaly wczytane dane z pliku wejsciowego.*/
complex *getInputData(char *szInput, const int numOfSamples);

/*Funkcja, na podstawie wartosci mode, wywoluje odpowiednia funkcje odpowiedzialna za obliczenia, ktorej przekazuje wartosci samples, numOfSamples oraz is_scaled.*/
complex *compute(int mode, complex samples[], int numOfSamples, int is_scaled);

/*Funckja oblicza transforamte Fouriera z wykorzystaniem rozwiazania wlasnego
Funkcja zwraca adres tablicy, do ktoej zostal zapisany rezultat obliczen */
complex *fft(complex samples[], int numOfSamples);

/*Funckja oblicza odwrotna transforamte Fouriera z wykorzystaniem rozwiazania wlasnego
Funkcja zwraca adres tablicy, do ktoej zostal zapisany rezultat obliczen */
complex *ifft(complex samples[], int numOfSamples, int is_scaled);

/*Funkcja oblicza transforamte Fouriera lub odwrotna transformate Fouriera (w zaleznosci od wartosci zmiennej mode) z wykorzystaniem biblitoeki FFTW
Funkcja zwraca adres tablicy, do ktoej zostal zapisany rezultat obliczen */
complex *fftw(complex samples[], int numOfSamples, int mode, int is_scaled);

complex *correct(complex results[], int numOfSamples);

/*Funkcja zapisuje rezultat obliczen do pliku*/
void saveResult(const char *szOutput, complex *fftresults, int numOfSamples);


int main(int argc, char *argv[])
{
	/* zmienne na nazwy plikow wejsciowego i wyjsciowego */
	char *szInput = NULL;
	char *szOutput = NULL;
	int is_scaled = FALSE;
	int mode = -1;

	/* Odczytanie argumentow linii polecen */
	readArgs(argc, argv, &szInput, &szOutput, &is_scaled, &mode);

	/* Zliczenie elementow w pliku wejsciowym */
	int numOfSamples = countSamples(szInput, mode);

	/* Wczytanie danych z pliku wejsciowego */
	complex *samples = getInputData(szInput, numOfSamples);

	/* Obliczenie fft lub ifft */
	complex *results = compute(mode, samples, numOfSamples, is_scaled);

	/* Zapisanie rezultatu do pliku */
	saveResult(szOutput, results, numOfSamples);

	/* zwolnienie pamieci zajmowanej przez zmienne programu */
	free(samples);
	free(results);

	//system("pause");
	
	return 0;
}

void readArgs(int argc, char *argv[], char **szInput, char **szOutput, int *is_scaled, int *mode)
{
	if (argc < 6)
	{
		fprintf(stderr, "Brakuje jednego z argumentow!\n");
		exit(1);
	}

	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			switch (argv[i][1])
			{
			case 'i':
				*szInput = &argv[i + 1][0];
				break;
			case 'o':
				*szOutput = &argv[i + 1][0];
				break;
			case 's':
				*is_scaled = TRUE;
				break;
			case 'm':
				if (!strcmp("fft", &argv[i + 1][0]))
					*mode = FFT;
				else if (!strcmp("ifft", &argv[i + 1][0]))
					*mode = IFFT;
				else if (!strcmp("fftw", &argv[i + 1][0]))
					*mode = FFTW;
				else if (!strcmp("ifftw", &argv[i + 1][0]))
					*mode = IFFTW;
				else
				{
					fprintf(stderr, "Nieznany parametr!\n");
					exit(1);
				}
				break;
			default:
				fprintf(stderr, "Nieznany parametr!\n");
				exit(1);
				break;
			}
		}
	}

	if (*szInput == NULL || *szOutput == NULL)
	{
		fprintf(stderr, "Brak pliku wejsciowego lub wyjsciowego!\n");
		exit(1);
	}
}

int countSamples(const char *szInput, int mode)
{
	FILE *ifname;
	ifname = fopen(szInput, "r");

	if (ifname == NULL)
	{
		fprintf(stderr, "Plik wejsciowy nie istnieje!\n");
		exit(1);
	}

	/* Zlicz probki */
	int numOfSamples = 0;
	double tmp1;
	double tmp2;
	int successfullyRead = 0;
	while ((successfullyRead = fscanf(ifname, "%lf%lfj", &tmp1, &tmp2)) != -1)
	{
		if (successfullyRead != 2)
		{
			fprintf(stderr, "Nieprawidlowy format danych wejsciowych!\n");
			exit(1);
		}

		++numOfSamples;
	}
	fclose(ifname);

	/* sprawdz czy plik jest pusty */
	if (numOfSamples == 0)
	{
		fprintf(stderr, "Plik wejscioy jest pusty!\n");
		exit(1);
	}

	/*W przypadku obliczania fft lub ifft za pomoca wlasnego
	rozwiazania, liczba probek musi byc potega dwojki;
	ograniczenie wynika ze specyfiki wykorzystanego algorytmu.
	Funkcje biblioteki FFTW wykorzystuja algorytm znoszacy
	te ograniczenia, stad w przypadku ich wyboru, warunek nie
	jest sprawdzany.*/
	if (mode == FFT || mode == IFFT)
	{
		if ((numOfSamples & (numOfSamples - 1)) != 0)
		{
			fprintf(stderr, "Liczba probek musi byc rowna potedze dwojki!\n");
			exit(1);
		}
	}

	return numOfSamples;
}

complex *getInputData(char *szInput, const int numOfSamples)
{
	/* otworz plik wejsciowy */
	FILE *ifname;
	ifname = fopen(szInput, "r");

	if (ifname == NULL)
	{
		fprintf(stderr, "Problem z otwarciem pliku wejsciowego!\n");
		exit(1);
	}

	/* przygotuj tablice na probki */
	complex *samples = (complex *)malloc(sizeof(complex)*numOfSamples);

	/* Pobierz probki */
	int successfullyRead = 0;
	rewind(ifname); // przejscie na poczatek pliku
	for (int i = 0; i < numOfSamples; ++i)
	{
		successfullyRead = fscanf(ifname, "%lf%lfj", &samples[i].re, &samples[i].im);
		if (successfullyRead != 2)
		{
			fprintf(stderr, "Nieprawidlowy format danych wejsciowych2!\n");
			free(samples);
			exit(1);
		}
	}

	fclose(ifname);

	return samples;
}

complex *compute(int mode, complex samples[], int numOfSamples, int is_scaled)
{
	complex *results = NULL;
	switch (mode)
	{
	case FFT:
		results = fft(samples, numOfSamples);
		break;
	case IFFT:
		results = ifft(samples, numOfSamples, is_scaled);
		break;
	case FFTW:
		results = fftw(samples, numOfSamples, mode, is_scaled);
		break;
	case IFFTW:
		results = fftw(samples, numOfSamples, mode, is_scaled);
		break;
	default:

		fprintf(stderr, "Nieprawidlowy parametr trybu!\n");
		free(samples);
		exit(1);

		break;
	}
	return results;
}

/* Function recursively split the samples all the way until
we have only one sample left. And then [] roots to each
levelcombining the values until we arrive[] our frequency bins.
numOfSamples is number of samples in samples[] array */
complex *fft(complex samples[], int numOfSamples)
{
	/* Execute the end of the recursive even/odd splits
	once we only have one sample */
	if (numOfSamples == 1) return samples;

	/* Split the samples into even and odd subsums */

	/* Find half the total number of samples */
	int halfOfNumOfSamples = numOfSamples / 2;

	/* Declare an even and odd complex arrays*/
	complex *Xeven = (complex *)malloc(sizeof(complex)*halfOfNumOfSamples);
	complex *Xodd = (complex *)malloc(sizeof(complex)*halfOfNumOfSamples);

	/* Input the even and odd samples into respective arrays */
	for (int i = 0; i < halfOfNumOfSamples; i++)
	{
		Xeven[i].re = samples[2 * i].re;
		Xeven[i].im = samples[2 * i].im;
		Xodd[i].re = samples[2 * i + 1].re;
		Xodd[i].im = samples[2 * i + 1].im;
	}

	/* Perform the recursive FFT operation on the even side */
	complex *Feven = fft(Xeven, halfOfNumOfSamples);

	/* Perform the recursive FFT operation on the odd side */
	complex *Fodd = fft(Xodd, halfOfNumOfSamples);

	/* ========= Recursion ends here ========= */

	/* Declare array of frequency bins */
	complex *freqBins = (complex *)malloc(sizeof(complex)*numOfSamples);

	/* Combine the values found */
	complex cmplxExponential;
	for (int k = 0; k < numOfSamples / 2; k++)
	{
		/* For each split set, multiply a k-dependent
		complex number by the odd subsum*/
		cmplxExponential.re = cos(-2 * PI * k / (float)numOfSamples) * Fodd[k].re - sin(-2 * PI * k / (float)numOfSamples) * Fodd[k].im;
		cmplxExponential.im = cos(-2 * PI * k / (float)numOfSamples) * Fodd[k].im + sin(-2 * PI * k / (float)numOfSamples) * Fodd[k].re;
		freqBins[k].re = Feven[k].re + cmplxExponential.re;
		freqBins[k].im = Feven[k].im + cmplxExponential.im;

		/* Everytime we add pi, exponential changes sign*/
		freqBins[k + numOfSamples / 2].re = Feven[k].re - cmplxExponential.re;
		freqBins[k + numOfSamples / 2].im = Feven[k].im - cmplxExponential.im;
	}

	free(Xodd);
	free(Xeven);

	if (numOfSamples != 2)
	{
		free(Fodd);
		free(Feven);
	}

	return freqBins;
}

// vide sprawozdanie
complex *ifft(complex samples[], int numOfSamples, int is_scaled)
{
	complex *samples_temp = (complex *)malloc(sizeof(complex)*numOfSamples);
	for (int i = 0; i < numOfSamples; i++)
	{
		samples_temp[i].re = samples[i].re;
		samples_temp[i].im = -(samples[i].im);
	}

	complex *ifftResults = fft(samples_temp, numOfSamples);

	ifftResults = correct(ifftResults, numOfSamples);

	for (int i = 0; i < numOfSamples; i++)
	{
		samples_temp[i].re = ifftResults[i].re;
		samples_temp[i].im = -(ifftResults[i].im);
	}

	if (is_scaled == TRUE)
	{
		for (int i = 0; i < numOfSamples; i++)
		{
			ifftResults[i].re = samples_temp[i].re / numOfSamples;
			ifftResults[i].im = samples_temp[i].im / numOfSamples;
		}
	}

	free(samples_temp);

	return ifftResults;
}

/* Funkcja zaokragla wartosci odlegle od zera
o wartosc ACCURACY; funkcja ta zostala napisana
z uwagi na format zapisu do pliku wyjsciowego
i nie ma istotnego wplywu na samo dzialanie programu.*/
complex *correct(complex results[], int numOfSamples)
{
	complex *resultsAfterCorrection = (complex *)malloc(sizeof(complex)*numOfSamples);
	for (int i = 0; i < numOfSamples; i++)
	{
		if (results[i].re < ACCURACY && results[i].re > -ACCURACY)
			resultsAfterCorrection[i].re = 0;
		else
			resultsAfterCorrection[i].re = results[i].re;

		if (results[i].im < ACCURACY && results[i].im > -ACCURACY)
			resultsAfterCorrection[i].im = 0;
		else
			resultsAfterCorrection[i].im = results[i].im;
	}

	free(results);

	return resultsAfterCorrection;
}

complex *fftw(complex samples[], int numOfSamples, int mode, int is_scaled)
{
	/* zmienne na dane wejsciowe i wyjsciowe */
	fftw_complex *in, *out;
	/* zawiera informacje potrzebne do obliczenia FFT lub IFFT */
	fftw_plan p;

	/* alokacja pamieci dla zmiennych na dane wejsciowe i wyjsciowe */
	in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * numOfSamples);
	out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * numOfSamples);

	// wypelnia tablice danych wejsciowych tymi samymi danymi, ktore dostala moja funkcja fft()
	for (int i = 0; i < numOfSamples; i++)
	{
		in[i][0] = samples[i].re;
		in[i][1] = samples[i].im;
	}

	/* okreslenie parametrow planu obliczenia FFT */
	if (mode == FFTW)
		p = fftw_plan_dft_1d(numOfSamples, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
	if (mode == IFFTW)
		p = fftw_plan_dft_1d(numOfSamples, in, out, FFTW_BACKWARD, FFTW_ESTIMATE);
	/* wykonanie obliczen dla FFT */
	fftw_execute(p);

	complex *results = (complex *)malloc(sizeof(complex)*numOfSamples);
	for (int i = 0; i < numOfSamples; i++)
	{
		results[i].re = out[i][0];
		results[i].im = out[i][1];
	}

	if (mode == IFFTW && is_scaled == TRUE)
	{
		for (int i = 0; i < numOfSamples; i++)
		{
			results[i].re = results[i].re / numOfSamples;
			results[i].im = results[i].im / numOfSamples;
		}
	}

	/* zwolnienie pamieci zajmowanej przez zmienne biblioteki FFTW */
	fftw_destroy_plan(p);
	fftw_free(in); fftw_free(out);

	return results;
}

/* zapisuje wynik fft() do pliku */
void saveResult(const char *szOutput, complex *results, int numOfSamples)
{
	FILE *ofname;
	ofname = fopen(szOutput, "w");

	if (ofname == NULL)
	{
		fprintf(stderr, "Problem z otwarciem pliku wyjsciowego!\n");
		free(results);
		exit(1);
	}

	double re, im;
	for (int i = 0; i < numOfSamples; i++)
	{
		if (results[i].re < 0.001 && results[i].re > -0.001)
			re = 0;
		else
			re = results[i].re;

		if (results[i].im < 0.001 && results[i].im > -0.001)
			im = 0;
		else
			im = results[i].im;

		fprintf(ofname, "%.3f%+.3fj\n", re, im);
	}

	fclose(ofname);

	return;
}