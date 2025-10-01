#include <bits/stdc++.h>
using namespace std;

enum TipoEvento { CHEGADA_EXTERNA, FIM_S1, FIM_S2, FIM_S3 };

struct Evento {
    double tempo;
    TipoEvento tipo;
    int id;
    Evento(double t, TipoEvento ty, int i) : tempo(t), tipo(ty), id(i) {}
};

struct ComparadorEvento {
    bool operator()(Evento const& a, Evento const& b) const {
        if (a.tempo != b.tempo) return a.tempo > b.tempo;
        return a.id > b.id;
    }
};

const double chegada_baixa = 2.0, chegada_alta = 4.0; 
const double srv1_baixo = 1.0, srv1_alto = 2.0;       
const double srv2_baixo = 4.0, srv2_alto = 6.0;      
const double srv3_baixo = 5.0, srv3_alto = 15.0;     

const int c1 = 1, K1 = INT_MAX; // G/G/1 (capacidade infinita)
const int c2 = 2, K2 = 5;       // G/G/2/5
const int c3 = 2, K3 = 10;      // G/G/2/10

const double primeira_chegada = 2.0; 
const long long MAX_RANDOMS = 100000;

mt19937_64 gerador(42);
uniform_real_distribution<double> U(0.0, 1.0);

long long randoms_used = 0;

inline double uniforme(double a, double b) {
    randoms_used++;
    return a + (b - a) * U(gerador);
}

//estrutura das filas
struct Fila {
    int servidores_ocupados = 0;
    int total = 0;
    int capacidade;
    int servidores;
    vector<double> tempo_estado; 
    int perdas = 0;
    double ultimo_tempo = 0.0;
};

Fila fila1{0, 0, K1, c1, vector<double>(50, 0.0), 0, 0.0};
Fila fila2{0, 0, K2, c2, vector<double>(K2+1, 0.0), 0, 0.0};
Fila fila3{0, 0, K3, c3, vector<double>(K3+1, 0.0), 0, 0.0};

priority_queue<Evento, vector<Evento>, ComparadorEvento> agenda;
double relogio_sim = 0.0;

void atualizar_tempos(Fila &f, double agora) {
    if (f.total < (int)f.tempo_estado.size()) {
        f.tempo_estado[f.total] += (agora - f.ultimo_tempo);
    }
    f.ultimo_tempo = agora;
}

void agendar(double t, TipoEvento tp) {
    static int next_id = 0;
    agenda.emplace(t, tp, next_id++);
}

void chegada_externa() {
    // Cliente chega na fila 1
    if (fila1.total < fila1.capacidade) {
        fila1.total++;
        if (fila1.servidores_ocupados < fila1.servidores) {
            fila1.servidores_ocupados++;
            double s = uniforme(srv1_baixo, srv1_alto);
            agendar(relogio_sim + s, FIM_S1);
        }
    }

    double ia = uniforme(chegada_baixa, chegada_alta);
    agendar(relogio_sim + ia, CHEGADA_EXTERNA);
}

void saida_fila(Fila &f, TipoEvento origem) {
    f.total--;
    if (f.total >= f.servidores_ocupados) {
        double s;
        if (origem == FIM_S1) s = uniforme(srv1_baixo, srv1_alto);
        else if (origem == FIM_S2) s = uniforme(srv2_baixo, srv2_alto);
        else s = uniforme(srv3_baixo, srv3_alto);
        agendar(relogio_sim + s, origem);
    } else {
        f.servidores_ocupados--;
    }
}

void roteamento_fila1() {
    double u = uniforme(0.0, 1.0);
    if (u < 0.8) {
        // vai para fila 2
        if (fila2.total < fila2.capacidade) {
            fila2.total++;
            if (fila2.servidores_ocupados < fila2.servidores) {
                fila2.servidores_ocupados++;
                double s = uniforme(srv2_baixo, srv2_alto);
                agendar(relogio_sim + s, FIM_S2);
            }
        } else fila2.perdas++;
    } else {
        // vai para fila 3
        if (fila3.total < fila3.capacidade) {
            fila3.total++;
            if (fila3.servidores_ocupados < fila3.servidores) {
                fila3.servidores_ocupados++;
                double s = uniforme(srv3_baixo, srv3_alto);
                agendar(relogio_sim + s, FIM_S3);
            }
        } else fila3.perdas++;
    }
}

void roteamento_fila2() {
    double u = uniforme(0.0, 1.0);
    if (u < 0.3) {
        // volta para fila 1
        if (fila1.total < fila1.capacidade) {
            fila1.total++;
            if (fila1.servidores_ocupados < fila1.servidores) {
                fila1.servidores_ocupados++;
                double s = uniforme(srv1_baixo, srv1_alto);
                agendar(relogio_sim + s, FIM_S1);
            }
        }
    } else if (u < 0.8) {
        // vai para fila 3
        if (fila3.total < fila3.capacidade) {
            fila3.total++;
            if (fila3.servidores_ocupados < fila3.servidores) {
                fila3.servidores_ocupados++;
                double s = uniforme(srv3_baixo, srv3_alto);
                agendar(relogio_sim + s, FIM_S3);
            }
        } else fila3.perdas++;
    } else {
    }
}

void roteamento_fila3() {
    double u = uniforme(0.0, 1.0);
    if (u < 0.7) {
        // volta para fila 1
        if (fila1.total < fila1.capacidade) {
            fila1.total++;
            if (fila1.servidores_ocupados < fila1.servidores) {
                fila1.servidores_ocupados++;
                double s = uniforme(srv1_baixo, srv1_alto);
                agendar(relogio_sim + s, FIM_S1);
            }
        }
    } else {
    
    }
}
int main() {
    // primeira chegada fixa
    agendar(primeira_chegada, CHEGADA_EXTERNA);

    while (!agenda.empty() && randoms_used < MAX_RANDOMS) {
        Evento ev = agenda.top(); agenda.pop();
        relogio_sim = ev.tempo;

        atualizar_tempos(fila1, relogio_sim);
        atualizar_tempos(fila2, relogio_sim);
        atualizar_tempos(fila3, relogio_sim);

        if (randoms_used >= MAX_RANDOMS) break;

        switch (ev.tipo) {
            case CHEGADA_EXTERNA: chegada_externa(); break;
            case FIM_S1: saida_fila(fila1, FIM_S1); roteamento_fila1(); break;
            case FIM_S2: saida_fila(fila2, FIM_S2); roteamento_fila2(); break;
            case FIM_S3: saida_fila(fila3, FIM_S3); roteamento_fila3(); break;
        }
    }

    cout << fixed << setprecision(3);
    cout << "Tempo total de simulacao: " << relogio_sim << " minutos\n";
    cout << "Numeros aleatorios usados: " << randoms_used << "\n\n";

    auto report = [&](Fila &f, string nome) {
        cout << "Fila " << nome << ":\n";
        double soma = accumulate(f.tempo_estado.begin(), f.tempo_estado.end(), 0.0);
        for (int i = 0; i < (int)f.tempo_estado.size(); i++) {
            if (f.tempo_estado[i] > 0)
                cout << "  Estado " << i << ": "
                     << f.tempo_estado[i] << " min, "
                     << (f.tempo_estado[i] / soma) << "\n";
        }
        cout << "  Perdas: " << f.perdas << "\n\n";
    };

    report(fila1, "1");
    report(fila2, "2");
    report(fila3, "3");
}
