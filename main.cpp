#include <bits/stdc++.h>
#include "yaml-cpp/yaml.h" 

using namespace std;

enum TipoEvento { CHEGADA_EXTERNA, FIM_SERVICO };

struct Evento {
    double tempo;
    TipoEvento tipo;
    string nome_fila;
    int id;
    Evento(double t, TipoEvento ty, const string& nf, int i) : tempo(t), tipo(ty), nome_fila(nf), id(i) {}
};

struct ComparadorEvento {
    bool operator()(Evento const& a, Evento const& b) const {
        if (a.tempo != b.tempo) return a.tempo > b.tempo;
        return a.id > b.id;
    }
};

struct Transicao {
    string destino;
    double probabilidade;
};

struct Fila {
    string nome;
    int servidores_ocupados = 0;
    int total = 0;
    int capacidade;
    int servidores;
    double min_srv, max_srv;
    double min_arr, max_arr;
    
    vector<double> tempo_estado; 
    int perdas = 0;
    double ultimo_tempo = 0.0;
};

const long long MAX_RANDOMS = 100000;
string NOME_ARQUIVO_CONFIG = "model.yml"; 

map<string, Fila> redeDeFilas;
map<string, vector<Transicao>> regrasDeRoteamento;
double primeira_chegada = 0.0;
string FILA_CHEGADA_EXTERNA = "";

mt19937_64 gerador(42);
uniform_real_distribution<double> U(0.0, 1.0);
long long randoms_used = 0;

inline double uniforme(double a, double b) {
    if (randoms_used >= MAX_RANDOMS) return 0.0;
    randoms_used++;
    return a + (b - a) * U(gerador);
}

priority_queue<Evento, vector<Evento>, ComparadorEvento> agenda;
double relogio_sim = 0.0;

void atualizar_tempos(double agora) {
    for (auto& pair : redeDeFilas) {
        Fila& f = pair.second;
        if (f.total < (int)f.tempo_estado.size()) {
            f.tempo_estado[f.total] += (agora - f.ultimo_tempo);
        }
        f.ultimo_tempo = agora;
    }
}

void agendar(double t, TipoEvento tp, const string& nome_fila) {
    static int next_id = 0;
    agenda.emplace(t, tp, nome_fila, next_id++);
}

void processar_chegada_na_fila(const string& nome_fila, bool is_external) {
    Fila& f = redeDeFilas[nome_fila];

    if (f.total < f.capacidade) {
        f.total++;
        
        if (f.servidores_ocupados < f.servidores) {
            f.servidores_ocupados++;
            double s = uniforme(f.min_srv, f.max_srv);
            agendar(relogio_sim + s, FIM_SERVICO, nome_fila);
        }
    } else {
        f.perdas++;
    }

    if (is_external) {
        double ia = uniforme(f.min_arr, f.max_arr);
        agendar(relogio_sim + ia, CHEGADA_EXTERNA, nome_fila);
    }
}

void processar_saida_e_roteamento(const string& nome_fila_origem) {
    Fila& fOrigem = redeDeFilas[nome_fila_origem];

    fOrigem.total--;
    
    if (fOrigem.total >= fOrigem.servidores_ocupados) {
        double s = uniforme(fOrigem.min_srv, fOrigem.max_srv);
        agendar(relogio_sim + s, FIM_SERVICO, nome_fila_origem);
    } else {
        fOrigem.servidores_ocupados--;
    }

    if (regrasDeRoteamento.count(nome_fila_origem)) {
        double u = uniforme(0.0, 1.0);
        double acumulador_prob = 0.0;
        
        for (const auto& transicao : regrasDeRoteamento[nome_fila_origem]) {
            acumulador_prob += transicao.probabilidade;

            if (u < acumulador_prob) {
                string destino = transicao.destino;
                
                if (destino == "EXTERIOR") {
                    break; 
                } else if (redeDeFilas.count(destino)) {
                    processar_chegada_na_fila(destino, false); 
                    break;
                }
            }
        }
    }
}

bool carregar_modelo_dinamico() {
    try {
        YAML::Node config = YAML::LoadFile(NOME_ARQUIVO_CONFIG);

        // 1. Carregar Filas (queues)
        const YAML::Node& queues = config["queues"];
        for (YAML::const_iterator it = queues.begin(); it != queues.end(); ++it) {
            string nomeFila = it->first.as<string>();
            const YAML::Node& dadosFila = it->second;

            Fila f;
            f.nome = nomeFila;
            f.servidores = dadosFila["servers"].as<int>();
            
            f.capacidade = dadosFila["capacity"].as<int>();
            if (f.capacidade == 99999) {
                 f.capacidade = INT_MAX;
            }

            f.min_srv = dadosFila["minService"].as<double>();
            f.max_srv = dadosFila["maxService"].as<double>();
            
            if (dadosFila["minArrival"]) {
                f.min_arr = dadosFila["minArrival"].as<double>();
                f.max_arr = dadosFila["maxArrival"].as<double>();
            }
            
            int k_size = (f.capacidade == INT_MAX ? 50 : f.capacidade + 1);
            f.tempo_estado.resize(k_size, 0.0);
            
            redeDeFilas[nomeFila] = f;
        }
        
        // 2. Carregar Roteamento (network)
        const YAML::Node& network = config["network"];
        for (const auto& transicao : network) {
            string source = transicao["source"].as<string>();
            string destination = transicao["destination"].as<string>();
            double probability = transicao["probability"].as<double>();
            
            regrasDeRoteamento[source].push_back({destination, probability});
        }
        
        // 3. Carregar Chegada Inicial (arrivals)
        const YAML::Node& arrivals = config["arrivals"];
        if (arrivals) {
            for (YAML::const_iterator it = arrivals.begin(); it != arrivals.end(); ++it) {
                FILA_CHEGADA_EXTERNA = it->first.as<string>();
                primeira_chegada = it->second.as<double>();
                break; 
            }
        } else {
            cerr << "ERRO: O nó 'arrivals' não foi encontrado no arquivo YML." << endl;
            return false;
        }

    } catch (const YAML::BadFile& e) {
        cerr << "ERRO: O arquivo de configuracao '" << NOME_ARQUIVO_CONFIG << "' nao foi encontrado. " << e.what() << endl;
        return false;
    } catch (const YAML::Exception& e) {
        cerr << "ERRO de parsing no arquivo YML: " << e.what() << endl;
        return false;
    }
    return true;
}

int main() {
    if (!carregar_modelo_dinamico()) {
        return 1;
    }

    agendar(primeira_chegada, CHEGADA_EXTERNA, FILA_CHEGADA_EXTERNA);

    while (!agenda.empty() && randoms_used < MAX_RANDOMS) {
        Evento ev = agenda.top(); agenda.pop();
        relogio_sim = ev.tempo;

        atualizar_tempos(relogio_sim); 

        if (randoms_used >= MAX_RANDOMS) break;

        switch (ev.tipo) {
            case CHEGADA_EXTERNA: 
                processar_chegada_na_fila(ev.nome_fila, true); 
                break;
            case FIM_SERVICO: 
                processar_saida_e_roteamento(ev.nome_fila); 
                break;
        }
    }

    cout << fixed << setprecision(3);
    cout << "Tempo total de simulacao: " << relogio_sim << " minutos\n";
    cout << "Numeros aleatorios usados: " << randoms_used << "\n\n";

    for (auto& pair : redeDeFilas) {
        Fila& f = pair.second;
        string capacidade_str = (f.capacidade == INT_MAX ? "inf" : to_string(f.capacidade));
        
        cout << "Fila " << f.nome << " (K=" << capacidade_str << ", c=" << f.servidores << "):\n";
        
        double soma = accumulate(f.tempo_estado.begin(), f.tempo_estado.end(), 0.0);
        
        for (int i = 0; i < (int)f.tempo_estado.size(); i++) {
            if (f.tempo_estado[i] > 0)
                cout << "  Estado " << i << ": "
                     << f.tempo_estado[i] << " min, "
                     << (f.tempo_estado[i] / relogio_sim) << "\n";
        }
        cout << "  Perdas: " << f.perdas << "\n\n";
    }
}