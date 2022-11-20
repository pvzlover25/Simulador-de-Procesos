#include <vector>
#include <queue>
#include <random>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <condition_variable>
using namespace std;
minstd_rand rng;


int N = 20; 		// procesos maximos, rango [a, b]
int M = 3;			// cpus
int a = 1, b = 5;	// [a, b]

vector<vector<queue<int>>> runqueue(2);		// 2 runqueues, rango (0, colas-1)
vector<int> tiempo;
int colas = 10;
int N2 = 10;		// "pid" actual maximo

int activa = 0, expirada = 1, esperando = 0;
int quantum = 1;

vector<vector<mutex>> locks(2);
mutex mE, mG;
condition_variable cvE, cvG;

int defPrioridad(int prioridadActual, int nuevoTiempo){
	int nuevaP = 0;

	if(nuevoTiempo/quantum > 3) nuevaP = min(9, prioridadActual + 1);
	else nuevaP = max(0, prioridadActual - 1);

	return nuevaP;
}


int hebra_E(int h){
	int cola = 0;
	while(true){
		locks[activa][cola].lock();
		if(!runqueue[activa][cola].empty()){
			int proceso = runqueue[activa][cola].front();
			runqueue[activa][cola].pop();
			cout << "CPU " << h << ", Proceso " << proceso << " ejecutandose" << endl;
			locks[activa][cola].unlock();

			int tiempoEjecucion = min(tiempo[proceso], quantum);
			sleep(tiempoEjecucion);
			
			locks[expirada][cola].lock();
			if(tiempo[proceso] > quantum){
				tiempo[proceso] = a + rng()%b;			// ?????????????????????????
				runqueue[expirada][ defPrioridad(cola, tiempo[proceso]) ].push(proceso);	
			}else cout << "Proceso " << proceso << " finalizado" << endl;
			locks[expirada][cola].unlock();

		}else{
			locks[activa][cola].unlock();

			if(cola+1 == colas){
				unique_lock<mutex> m(mE);

				esperando++;
				if(esperando == M) cvG.notify_one();
				cvE.wait(m);
			}

			cola = (cola+1)%colas;
		}

	}

	return 0;
}


int hebra_G(){
	while(true){
		unique_lock<mutex> m(mG);
		cvG.wait(m);

		unique_lock<mutex> m2(mE);
		esperando = 0;
		swap(activa, expirada);

		int procesosActuales = 0;
		for(int i=0; i<colas; i++) procesosActuales += runqueue[activa][i].size();

		int nuevosProc = N - procesosActuales;

		for(int i=0; i<nuevosProc; i++){
			runqueue[activa][colas/2].push(N2+i);
			tiempo.push_back(a + rng()%b);
		}
		N2 += nuevosProc;

		cout << endl << "Cola" << endl;
		cout << "Activa" << " Proceso(Tiempo)" << endl;
		for(int i=0; i<colas; i++){
			int aux = runqueue[activa][i].size();
			if(aux == 0) cout << " " << i << "     -";
			else cout << " " << i << "    ";

			while(aux--){
				int aux2 = runqueue[activa][i].front();
				cout << " " << aux2 << "(" << tiempo[aux2] << ")";
				runqueue[activa][i].push(aux2);
				runqueue[activa][i].pop();
			}
			cout << endl;
		}
		cout << endl;


		cvE.notify_all();
	}

	return 0;
}


int main(){
	rng.seed(time(NULL));
	cout << "N M a b" << endl;
	cin >> N >> M >> a >> b;

	runqueue[0] = vector<queue<int>>(colas);
	runqueue[1] = vector<queue<int>>(colas);
	locks[0] = vector<mutex>(colas);
	locks[1] = vector<mutex>(colas);

	for(int i=0; i<N; i++){
		runqueue[activa][colas/2].push(i);
		tiempo.push_back(a + rng()%b);
	}

	cout << endl << "Cola" << endl;
	cout << "Activa" << " Proceso(Tiempo)" << endl;
	for(int i=0; i<colas; i++){
		int aux = runqueue[activa][i].size();
		if(aux == 0) cout << " " << i << "     -";
		else cout << " " << i << "    ";

		while(aux--){
			int aux2 = runqueue[activa][i].front();
			cout << " " << aux2 << "(" << tiempo[aux2] << ")";
			runqueue[activa][i].push(aux2);
			runqueue[activa][i].pop();
		}
		cout << endl;
	}
	cout << endl;

	vector<thread> t;
	for(int i=0; i<M; i++){
		thread aux(hebra_E, i);
		t.push_back( move(aux) );
	}
	thread aux(hebra_G);
	t.push_back( move(aux) );

	for(int i=0; i<t.size(); i++) t[i].join();

	return 0;
}
