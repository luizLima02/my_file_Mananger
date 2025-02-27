#include "disco.hpp"

enum COMMANDS{
    CREATE,
    DELETE,
    LIST,
    SORT,
    READ,
    CONCAT,
    EXIT_A
};

struct comando
{
    COMMANDS command;
    std::string nome;
    std::string nome2;
    int tamanho;
    int inicio;
    int fim;
};

comando processar_comando(){
    comando com;
    std::string campo_1;
    std::string campo_2 = "";
    std::string campo_3 = "";
    std::string campo_4 = "";
    /*le a linha formatada*/
    std::string linha;
    std::getline(std::cin, linha);
    //passa os valores da linha para as variaveis
    std::istringstream iss(linha);
    iss >> campo_1 >> campo_2 >> campo_3 >> campo_4;

    if(campo_1.compare("criar") == 0){
        com.command = CREATE;
        com.nome = campo_2;
        com.tamanho = std::stoi(campo_3);
        return com;

    }else if(campo_1.compare("sair") == 0){
        com.command = EXIT_A;
        return com;

    }else if(campo_1.compare("apagar") == 0){
        com.command = DELETE;
        com.nome = campo_2;
        return com;

    }else if(campo_1.compare("ordenar") == 0){
        com.command = SORT;
        com.nome = campo_2;
        return com;

    }else if(campo_1.compare("ler") == 0){
        com.command = READ;
        com.nome = campo_2;
        com.inicio = std::stoi(campo_3);
        com.fim = std::stoi(campo_4);
        return com;

    }else if(campo_1.compare("concatenar") == 0){
        com.command = CONCAT;
        com.nome = campo_2;
        com.nome2 = campo_3;
        return com;

    }else{
        com.command = LIST;
        com.fim = 0;
        com.inicio = 0;
        com.tamanho = 0;
        com.nome = "";
        com.nome2 = "";
        return com;
    }

    com.command = LIST;
    com.fim = 0;
    com.inicio = 0;
    com.tamanho = 0;
    com.nome = "";
    com.nome2 = "";
    return com;
}



int main() {

    bool loop = true;
    Disco* hd = new Disco();
    if(hd->iniciado_com_sucesso == false){return 1;}
    
    while(loop){

        std::cout << "digite o comando:\n";
        comando comand = processar_comando();
        

        if(comand.command == LIST){
            hd->Listar_Arquivos();
            hd->Print_space();

        }else if(comand.command == CREATE){
            hd->Cria_Arquivo(comand.nome, comand.tamanho);

        }else if(comand.command == DELETE){
            hd->Apaga_Arquivo(comand.nome);

        }else if(comand.command == READ){
            hd->Ler_Arquivo(comand.nome, comand.inicio, comand.fim);

        }else if(comand.command == SORT){
            hd->Ordenar_Arquivo(comand.nome);

        }else if(comand.command == CONCAT){
            hd->Concatenar_arquivos(comand.nome, comand.nome2);

        }else if(comand.command == EXIT_A){
            loop = false;
        }
        //std::cout << "\n";
        std::cout << "\n------------------------------------------------\n";
    }
    
    hd->Desmontar();
    return 0;
}

