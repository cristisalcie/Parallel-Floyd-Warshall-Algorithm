
===============================================================================


					Salcie Ioan-Cristian, 341C1
				Tema 1 si 2, Structuri Multiprocesor, 2021


===============================================================================


1. Structura temei:

Tema este alcatuita din 5 directoare:

- MPI_Floyd / OpenMP_Floyd / Pthreads_Floyd / MPI_OpenMP_Floyd
/ MPI_Pthreads_Floyd (fiecare contin 6 fisiere si 3 directoare)

	-> Makefile			- compileaza, ruleaza tema
	-> checker.py		- verifica tema

	-> utils.h			- contine functii si librarii utile pentru toata tema
						- va fi inclusa in toate celelalte fisiere .h

	-> main.c			- contine rezolvarea problemei

	-> generator.c 		- generator de teste

	-> serial.c			- varianta seriala a problemei pentru generarea
						testelor de referinta

	-> tests			- director ce contine testele
	-> refs				- director ce contine rezultatele corecte ale testelor
	-> results			- director ce va contine rezultatele testelor din
						main.c


===============================================================================


2. Structura teste
		Testele vor fi numite test_xyz.out unde xyz este un numar intreg si
	sunt localizate in directorul tests.
	
		Rezultatele vor fi numite result_xyz.out unde xyz este un numar intreg
	si sunt localizate in directorul results.
	
		Referintele (rezultatele corecte) vor fi numite ref_xyz.out unde xyz
	este un numar intreg si sunt localizate in directorul refs.
	
		Testele contin pe prima linie numarul noduri (adica numarul de linii
	(respectiv coloane) din matricea de adiacenta a grafului). Dupa care sunt
	valorile din matrice una dupa alta. Acestea au fost generate folosind
	fisierul "generator.c".

		Din motive de spatiu nu pot urca teste foarte mari pentru ca ar
	depasi limita de 2MB pentru incarcare a arhivei. Insa se pot genera usor
	folosind "generator.c". Nu uita sa schimbi si in checker numarul de teste.


===============================================================================


2. Rulare tema
	python3 checker.py


===============================================================================


3. Detalii despre implementari:
		Pentru reprezentarea grafurilor se foloseste varianta cu matrice de
	adiacenta. Mai exact este o matrice de NxN unde N este numarul de
	noduri, iar fiecare valoare i,j din matrice este distanta de la nodul i
	la nodul j. Pentru a reprezenta faptul ca nu exista o distanta intre
	doua noduri putem pune o valoare foarte mare (spre infinit).
	Pentru rezolvarea problemei se folosesc 3 bucle for:
	
		- prima bucla este pasul, cu alte cuvinte, k este nodul intermediar
		dintre distanta cea mai scurta de la sursa la destinatie
		
		- cele 2 bucle interioare (i, j) itereaza prin matrice astfel ca
		formula din interiorul celor 3 bucle poate fi formulata in cuvinte
		astfel: Daca distanta de la sursa la destinatie ce trece prin nodul
		intermediar k este mai mica decat distanta directa curenta de la i
		la j atunci actualizeaza distanta de la i la j cu distanta ce trece
		prin nodul intermediar k. 

		Pentru paralelizare, daca ar fi sa paralelizam prima bucla am observa
	ca mai multe thread-uri vor putea ajunge sa incerce sa citeasca/scrie pe
	celula i, j in acelasi timp. Astfel ar fi necesar un mutex pe formula
	ceea ce il face un program serial.
	In plus, pentru a fi corect rezultatul nu putem imparti nodul intermediar
	la cate un worker pentru a fi calculat si trimis inapoi deoarece pentru a
	putea calcula nodul intermediar k = [0, N] trebuie sa stim rezultatul de
	la k - 1.
		In schimb, putem paraleliza oricare dintre buclele interioare.
	

	a) MPI / Pthreads / OpenMP (ca probleme separate)
			Paralelizam bucla de la mijloc astfel impartind liniile matricei
		de adiacenta intre workeri pentru a fi calculate. Am ales aceasta bucla
		aici pentru a avea continuitate in memorie facand sincronizarea de la
		finalul buclelor interioare mai usor de citit. De asemenea, ar trebui
		sa fie mai putine cache miss-uri.
			Acea sincronizare este necesara intrucat algorimul necesita
		rezultatele matricei de adiacenta de la pasul anterior k - 1. (Vezi [1]
		bibliografie pentru exemplu)
    b) MPI-OpenMP / MPI-Pthreads
			In plus fata de varianta MPI, paralelizam bucla interioara folosind
		OpenMP/Pthreads. Sincronizarea (mpi_send mpi_recv) este facuta de un
		singur thread.


===============================================================================


4. Bibliografie:
[1] https://www.programiz.com/dsa/floyd-warshall-algorithm


===============================================================================
