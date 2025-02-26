#ifndef DISCO_H
#define DISCO_H

//C
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
//CPP
#include <sstream>
#include <math.h>
#include <string>
#include <iostream>
#include <cstring>
#include <fstream>
#include <new>
#include <vector>
#include <random>
#include <bitset>

using std::streampos; using std::fstream; using std::cerr; using std::vector; using std::istringstream;

/* 
INFO SISTEMA DE ARQUIVOS:
**o arquivo está particionado nessa sequencia**
// VETOR DE POSICOES LIVRES // size: 179.200 bytes
-vetor de inodes livres: 89.600 bytes -> o bit i representa o inode i
-vetor de blocos livres: 89.600 bytes -> o bit i representa o bloco i

// INODE // size: 197.836.800 bytes
-inode > 716.800 inodes * inode_size=276 > 

//BlOCOS // size: 734.003.200 bytes
qnt: 716.800 blocos
bloco: 1024

// PAGINACAO - 26 bytes // size: 141.722.624 bytes
-paginacao > 2.097.152 paginas de 68 bytes > 144.703.488 > sobram 729.088 bytes
--28 bytes
---4 bytes -> quantidade de divisoes - no maximo 6
--24 bytes -> vetor de tamanho 6 com 4 bytes cada,
memoria principal
-524.288 > posicoes de 4 bytes disponiveis > divide BLOCOS em 6, no maximo, para cada posicao ter 1 numero de um bloco




total > 1.073.741.824 bytes > 1GB
*/

#define HUGEPAGE_SIZE (2 * 1024 * 1024) // Tamanho de uma hugepage padrão (2MB)
#define BLOCK_SIZE (1024)
#define QNT_INODES (716800)
#define QNT_BLOCOS (716800)

//Estruturas

struct Inode{
    /*nome do inode*/
    char nome[8];
    /*Diretorio*/
    char diretorio[4];
    /*numero do proximo inode*/
    char next[4];
    /*quantos bytes dos blocos desse inode estão preenchidos: 
    ex: 1 -> bloco[0-3] tem 4 bytes escritos
    */
    char size[4];
    /*vetor de 64 posicoes de 4 bytes de quais bytes formam esse inode
    */
    char blocos[256];

    void set_Nome(const char* name, int size){
        if(size < 7){
            for (int i = 0; i < 8; i++)
            {
                if(i < size){
                    nome[i] = name[i];
                }else{
                    nome[i] = 0;
                }
            }
        }else{
            for (int i = 0; i < 8; i++)
            {
                nome[i] = name[i];
            }
        }
        
    }

    void set_Dir(int dir){
        std::memcpy(diretorio, &dir, sizeof(int));
    }

    void set_Size(int tam){
        std::memcpy(size, &tam, sizeof(int));
    }

    void set_next(int prox){
        std::memcpy(next, &prox, sizeof(int));
    }

    //maximo qnt = 64
    void set_blocos(unsigned int* vals, int qnt, int offset = 0){
        for(int i = 0; i < qnt; i++){
            std::memcpy(&blocos[i * 4], &vals[(offset*64) + i], sizeof(unsigned int));
        }
    }

    Inode(){}

    Inode(bool preenche){
        //nao tem next
        int prox = QNT_INODES;
        std::memcpy(next, &prox, sizeof(int));
        //tamanho é zero
        int tam = 0;
        std::memcpy(size, &tam, sizeof(int));
        int exte = 0;
        std::memcpy(diretorio, &exte, sizeof(int));
        for(int i = 0; i < 64; i++){
            if(i < 2){
                int mena = 0;
                std::memcpy(&nome[i * 4], &mena, sizeof(int));
            }
            int pre = QNT_BLOCOS+1;
            std::memcpy(&blocos[i * 4], &pre, sizeof(int));
        }
    }

}typedef Inode;

//funcoes auxiliares

int contarBitsSetados(unsigned char byte) {
    std::bitset<8> bits(byte);
    return bits.count();
}

bool Nome_vazio(const char* nome){
    bool vazio = true;
    for (int i = 0; i < 8; i++)
    {
        if(nome[i] != 0){ return false; }
    }
    return vazio;
}

bool Compara_nomes(std::string nome1, std::string nome2){
    return (nome1 == nome2);
}

char* Monta_nome(std::string name){
    char* nome = new(std::nothrow)char[8];
    if(name.size() < 7){
        for (int i = 0; i < 8; i++)
        {
            if(i < name.size()){
                nome[i] = name[i];
            }else{
                nome[i] = 0;
            }
            
        }
    }else{
        for (int i = 0; i < 8; i++)
        {
            nome[i] = name[i];
        }
    }
    return nome;
}


unsigned int Gera_aleatorio(){
     // Criar um dispositivo de aleatoriedade
     std::random_device rd;
     // Utilizar o dispositivo para inicializar um gerador de números aleatórios
     std::mt19937 gen(rd());
     // Definir a distribuição de números inteiros não assinados
     std::uniform_int_distribution<unsigned int> dis;
     // Gerar e retornar um número aleatório
     auto numRet = dis(gen);
     std::cout << numRet << "\n";
     return numRet;
}


/*Preenche o vetor (bloco) na (qnt) passada*/
void Gerar_numeros(unsigned int* bloco, unsigned int qnt){
    std::memset(bloco, 0, 256*sizeof(unsigned int));
    for(int i = 0; i < qnt; i++){
        bloco[i] = Gera_aleatorio();
    }
}

//classe principal

class Disco
{
private:
    const char* path = "../disco.ln";
    fstream *arquivo;
    /*Inicio do vetor inodes livres*/
    const streampos start_inodes_livre = 0;
    /*Inicio do vetor blocos livres*/
    const streampos start_blocos_livre = 89600;
    /*Inicio da particao inodes*/
    const streampos start_inodes = 179200;
    const streampos start_blocos = 197926400; //197.926.400
    const streampos start_pages = 931929600; //tamanho de inodes + tamanho de vetores livres + tamanho de part_blocos + (4 -> qnt de partes que blocos foi dividido)
    /*Tamanhos em bytes do vetor livre*/
    const int free_vetor_size = 89600;
    /*Tamanhos em bytes da particao inode*/
    const int inodes_size = 197836800;
    /*Tamanhos em bytes da particao blocos*/
    const int blocos_size = 734003200;
    /*Tamanhos em bytes da particao pages*/
    const int pages_size = 28;
    /*verifica se o disco esta montado*/
    bool montado = false;
    /*Tamanho do disco*/
    long int tamanho_disco;
    /*espaco livre*/
    long int tamanho_livre;
    /*inode disponiveis*/
    int inode_usados;
    
public:
    //espaco livre no disco
    void Print_space();
    //Vetores de posicoes livres
    /*verifica se o inode i está livre, usar o valor desejado -1 -> [1-sizeInodes]*/
    bool Is_inode_free(unsigned int);
    /*verifica se o bloco i está livre, usar o valor desejado -1 -> [1-sizeInodes]*/
    bool Is_bloco_free(unsigned int);
    /*pega o inode i do vetor de livres, usar o valor desejado -1 -> [1-sizeInodes]*/
    bool Book_inode(unsigned int);
    /*pega o bloco i do vetor de livres, usar o valor desejado -1 -> [1-size]*/
    bool Book_bloco(unsigned int);
    //libera a posicao i do vetor de inodes, usar o valor desejado -1 -> [1-size]
    bool Release_inode(unsigned int);
    //libera o a posicao i do vetor de inodes, usar o valor desejado -1 -> [1-size]
    bool Release_bloco(unsigned int);
    //alocacao de inode e blocos
    //escreve o inode para o arquivo, usar o valor desejado -1 -> [1-size]
    bool Write_Block(unsigned int, unsigned int*, unsigned int size);
    //escreve o inode para o arquivo, usar o valor desejado -1 -> [1-size]
    bool Write_Inode(unsigned int, Inode);
    //deleta o inode i, usar o valor desejado -1 -> [1-sizeblocos]
    bool Delete_Inode(unsigned int);
    // le o inode I, usar o valor desejado -1 -> [1-sizeblocos]
    Inode Read_inode(unsigned int);

    Inode Read_inode(std::string);

    //bool File_exists(std::string);
    // le o bloco I e armazena no bloco, usar o valor desejado -1 -> [1-size]
    void Read_block(unsigned int, unsigned int*);

    unsigned int Ask_bloco(){
        for(unsigned int i = 0; i < QNT_BLOCOS; i++){
            if(Is_bloco_free(i)){
                return i;
            }
        }
        return QNT_BLOCOS+1;
    }
    
    unsigned int Ask_inode(){
        for(unsigned int i = 0; i < QNT_INODES; i++){
            if(Is_inode_free(i)){
                return i;
            }
        }
        return QNT_INODES+1;
    }
    //FUNCOES CORE

    //tamanho do arquivo, usar o valor desejado -1 -> [1-size]
    long int Size_Inode(unsigned int);
    long int Size_Inode(std::string);
    //tamanho real do arquivo, usar o valor desejado -1 -> [1-size]
    long int Real_Size_Inode(unsigned int);
    long int Real_Size_Inode(std::string);
    
    //printa o inode i, usar o valor desejado -1 -> [1-size]
    void Printar_Inode(unsigned int);
    //
    void Printar_Bloco(unsigned int);
    void Printar_Bloco(unsigned int, int);
    void Printar_Bloco(unsigned int, int, int);
    //
    int Get_inode_index(std::string);
    //
    int Get_inode_index(std::string, int);
    //pega o indice x
    unsigned int Get_Bloco_Aux(unsigned int, int);
    //escreve no indice x
    void Set_Bloco_Aux(unsigned int, int, unsigned int);
    /*le o byte x do arquivo passado*/
    unsigned int Read_File_index(std::string, unsigned int);

    /*muda o byte x do arquivo passado*/
    void Write_File_index(std::string, unsigned int, unsigned int);

    /*adiciona o byte para o arquivo*/
    void Add_File_index_aux(std::string);
    void Add_File_index(std::string, unsigned int);
    //
    void Change_Block_4byte(unsigned int bloco_num, unsigned int, unsigned int);
    unsigned int Read_Block_4byte(unsigned int bloco_num, unsigned int);
    /*Funcao para verificar se o arquivo existe e abrir*/
    int Montar();
    /*Funcao para verificar se o arquivo estiver aberto, fecha-lo*/
    int Desmontar();

    //arquivos

    void Listar_Arquivos();

    void Cria_Arquivo(std::string , unsigned int);

    void Cria_Escrever_inode(unsigned int, std::string , unsigned int, unsigned int, unsigned int*, unsigned int, unsigned int);

    bool Apaga_Arquivo(std::string);

    void Ler_Arquivo(std::string, unsigned int, unsigned int);

    //Inode ler_inode(int);
    void Printar_path(){
        printf("local do arquivo disco: %s\n", this->path);
    }

    Disco(){
        Montar();
        std::cout << "carregando sistema...\n";
        tamanho_disco = 734003200;
        tamanho_livre = 734003200;
        inode_usados  = 0;
        for(int i = 0; i < free_vetor_size; i+=8){
            std::cout << "\rProgresso: " << i+8 << "/" << free_vetor_size << std::flush;
            //bloco
            arquivo->seekg(this->start_blocos_livre + (streampos)i);
            char byte_bloco_lido;
            arquivo->read(&byte_bloco_lido, sizeof(byte_bloco_lido));
            tamanho_livre -= contarBitsSetados(byte_bloco_lido) * 1024;
            //Inode
            arquivo->seekg(this->start_inodes_livre + (streampos)i);
            char byte_inode_lido;
            arquivo->read(&byte_inode_lido, sizeof(byte_inode_lido));
            inode_usados += contarBitsSetados(byte_inode_lido);
        }
        std::cout << "\nsistema carregado\n";
    }
    ~Disco();
};

////Funcoes auxiliares

//Vetores de posicoes livres

//INODES
bool Disco::Is_inode_free(unsigned int i){
    if(!montado){return false;} if(i >= QNT_INODES){return false;}
    int indiceByte = i / 8;
    int indiceBit = i % 8;
    //vai para o byte a ser lido
    //std::cout << "estou no byte: " << this->start_inodes_livre + (streampos)indiceByte << "\n";
    arquivo->seekg(this->start_inodes_livre + (streampos)indiceByte);
    //le o byte
    char byte_lido;
    arquivo->read(&byte_lido, sizeof(byte_lido));
    //std::cout << "i: " << i << /*" \nbyte lido: " << byte_lido << " indice byte: " << indiceByte << " indice bit: " << indiceBit <<*/ std::endl;
    //le o bit
    byte_lido &= (0x80 >> indiceBit);
    /*if(byte_lido == 0){
        std::cout  << i << " esta livre\n";
    }else{
        std::cout << i << " nao esta livre\n";
    }*/
    return (byte_lido == 0);
}

bool Disco::Book_inode(unsigned int i){
    if(!montado){return false;} if(i >= QNT_INODES){return false;}
    if(Is_inode_free(i) == false){return false;}
    bool sucesso = true;
    int indiceByte = i / 8;
    int indiceBit = i % 8;

    //vai para o byte a ser lido
    //std::cout << "va para o byte: " << this->start_inodes_livre + (streampos)indiceByte << "\n";
    arquivo->seekg(this->start_inodes_livre + (streampos)indiceByte);

    //le o byte
    char byte_lido;
    arquivo->read(&byte_lido, sizeof(byte_lido));

    //escreve para o byte
    char byte_escrever = 0x80;
    byte_escrever = (byte_escrever >> indiceBit);
    byte_escrever |= byte_lido;
    //std::cout << "i: " << i << " \nbyte escrito: " << byte_escrever << " indice byte: " << indiceByte << " indice bit: " << indiceBit << std::endl;
    
    //retorna para a posicao para escrever
    arquivo->seekg(this->start_inodes_livre + (streampos)indiceByte);
    arquivo->write(&byte_escrever, sizeof(byte_escrever));


    return sucesso;
}

bool Disco::Release_inode(unsigned int i){
    if(!montado){return false;} if(i >= QNT_INODES){return false;}
    if(Is_inode_free(i) == true){return false;}
    bool sucesso = true;
    int indiceByte = i / 8;
    int indiceBit = i % 8;

    //vai para o byte a ser lido
    //std::cout << "va para o byte: " << this->start_inodes_livre + (streampos)indiceByte << "\n";
    arquivo->seekg(this->start_inodes_livre + (streampos)indiceByte);

    //le o byte
    char byte_lido;
    arquivo->read(&byte_lido, sizeof(byte_lido));

    //escreve para o byte
    char byte_escrever = 0x80;
    byte_escrever = ~(byte_escrever >> indiceBit);

    byte_escrever &= byte_lido;
    //std::cout << "i: " << i << " \nbyte escrito: " << byte_escrever << " indice byte: " << indiceByte << " indice bit: " << indiceBit << std::endl;
    
    //retorna para a posicao para escrever
    arquivo->seekg(this->start_inodes_livre + (streampos)indiceByte);
    arquivo->write(&byte_escrever, sizeof(byte_escrever));


    return sucesso;
}

//escreve o inode para o arquivo, garanta que i está livre para escrever
bool Disco::Write_Inode(unsigned int i, Inode node){
    //vai ate a posicao I onde o inode sera escrito
    this->arquivo->seekg(this->start_inodes + (streampos)(sizeof(Inode)*i));
    this->arquivo->write(reinterpret_cast<char*>(&node), sizeof(Inode));
    return true;
}

void Disco::Change_Block_4byte(unsigned int bloco_num, unsigned int indice, unsigned int valor){
    this->arquivo->seekg(this->start_blocos + (streampos)(QNT_BLOCOS*bloco_num) + (indice*4));
    this->arquivo->write(reinterpret_cast<char*>(&valor), sizeof(unsigned int));
}

unsigned int Disco::Read_Block_4byte(unsigned int bloco_num, unsigned int indice){
    this->arquivo->seekg(this->start_blocos + (streampos)(QNT_BLOCOS*bloco_num) + (indice*4));
    unsigned int valor;
    this->arquivo->read(reinterpret_cast<char*>(&valor), sizeof(unsigned int));
    return valor;
}

//le os inodes do arquivo
Inode Disco::Read_inode(unsigned int inode_num){
    //vai ate a posicao I onde o inode sera escrito
    this->arquivo->seekg(this->start_inodes + (streampos)(sizeof(Inode)*inode_num));
    //lê o inode
    Inode node;
    this->arquivo->read(reinterpret_cast<char*>(&node), sizeof(Inode));
    return node;
}

//le os inodes do arquivo
Inode Disco::Read_inode(std::string nomeArquivo){
    std::string nomeString(Monta_nome(nomeArquivo), 8);
    for (int i = 0; i < QNT_INODES; i++)
    {
        this->arquivo->seekg(this->start_inodes + (streampos)(sizeof(Inode)*i));
        Inode node;
        this->arquivo->read(reinterpret_cast<char*>(&node), sizeof(Inode));
        std::string name(node.nome, 8);
        if(Compara_nomes(name, nomeString)){return node;}
    }
    
    return Inode(true);
}

//le os inodes do arquivo
int Disco::Get_inode_index(std::string nomeArquivo){
    std::string nomeString(Monta_nome(nomeArquivo), 8);
    for (int i = 0; i < QNT_INODES; i++)
    {
        this->arquivo->seekg(this->start_inodes + (streampos)(sizeof(Inode)*i));
        Inode node;
        this->arquivo->read(reinterpret_cast<char*>(&node), sizeof(Inode));
        std::string name(node.nome, 8);
        if(Compara_nomes(name, nomeString)){return i;}
    }
    return QNT_INODES;
}

long int Disco::Size_Inode(unsigned int inode_num){
    Inode inode_atual = this->Read_inode(inode_num);
    int size = 0;
    for(int i = 0; i < 64; i++){
        unsigned int valor = *(reinterpret_cast<unsigned int*>(inode_atual.blocos + (i*4)));
        if(valor >= QNT_BLOCOS){
            break;
        }else{
            size+=1024;
        }
    }
    int proximo = *(reinterpret_cast<int*>(inode_atual.next));
    if(proximo < QNT_INODES){
        return size + Size_Inode(proximo);
    }else{
        return size;
    }
    return size;
}

long int Disco::Size_Inode(std::string nome){
    Inode inode_atual = this->Read_inode(nome);
    int size = 0;
    for(int i = 0; i < 64; i++){
        unsigned int valor = *(reinterpret_cast<unsigned int*>(inode_atual.blocos + (i*4)));
        if(valor >= QNT_BLOCOS){
            break;
        }else{
            size+=1024;
        }
    }
    int proximo = *(reinterpret_cast<unsigned int*>(inode_atual.next));
    if(proximo < QNT_INODES){
        return size + Size_Inode(proximo);
    }else{
        return size;
    }
    return size;
}

long int Disco::Real_Size_Inode(unsigned int inode_num){
    Inode inode_atual = this->Read_inode(inode_num);
    long int size = 0;
    int usado = *(reinterpret_cast<int*>(inode_atual.size));
    size += usado * 4;
    int proximo = *(reinterpret_cast<int*>(inode_atual.next));
    if(proximo < QNT_INODES){
        return size + Real_Size_Inode(proximo);
    }else{
        return size;
    }
    return size;
}

long int Disco::Real_Size_Inode(std::string nome){
    Inode inode_atual = this->Read_inode(nome);
    long int size = 0;
    int usado = *(reinterpret_cast<int*>(inode_atual.size));
    size += usado * 4;
    int proximo = *(reinterpret_cast<int*>(inode_atual.next));
    if(proximo < QNT_INODES){
        return size + Real_Size_Inode(proximo);
    }else{
        return size;
    }
    return size;
}

void Disco::Printar_Inode(unsigned int i){
    Inode inod = Read_inode(i);
    if(!Nome_vazio(inod.nome)){
        long int diskSize = Size_Inode(i);
        long int realSize = Real_Size_Inode(i);
        std::cout << "Nome: " <<  std::string(inod.nome) << " { " <<  " tamanho em disco(em bytes): " << diskSize << " | " << "tamanho real(em bytes): " << realSize << " }\n";
    }
}

bool Disco::Delete_Inode(unsigned int i){
    //libera o inode
    Release_inode(i);
    //pega o inode
    Inode Rnode = Read_inode(i);
    //apaga o inode
    Inode node = Read_inode(i);
    std::memset(&node, 0, sizeof(Inode));
    //vai ate a posicao I onde o inode sera escrito
    this->arquivo->seekg(this->start_inodes + (streampos)(sizeof(Inode)*i));
    //escreve o inode
    this->arquivo->write(reinterpret_cast<char*>(&node), sizeof(Inode));
    int next = *reinterpret_cast<int*>(Rnode.next);
    //libera os blocos
    for(int i = 0; i < 64; i++){
        int valor = *(reinterpret_cast<int*>(Rnode.blocos + (i*4)));
        if(valor < QNT_BLOCOS+1){
            this->Release_bloco(valor);
        }else{
            break;
        }
    }
    if(next < QNT_INODES){return Delete_Inode(next);}
    return true;
}

void Disco::Cria_Escrever_inode(unsigned int indice, std::string nome, unsigned int tamanho, unsigned int proximo, unsigned int* vetor_blocos, unsigned int qnt_blocos, unsigned int offset){
    Inode node(true);
    node.set_Nome(nome.c_str(), nome.size());
    node.set_Size(tamanho);
    node.set_next(proximo);
    node.set_blocos(vetor_blocos, qnt_blocos, offset);
    this->Write_Inode(indice, node);
    return;
}

//BLOCOS

bool Disco::Is_bloco_free(unsigned int i){
    if(!montado){return false;} if(i >= QNT_INODES){return false;}
    int indiceByte = i / 8;
    int indiceBit = i % 8;
    //vai para o byte a ser lido
    //std::cout << "estou no byte: " << this->start_blocos_livre + (streampos)indiceByte << "\n";
    arquivo->seekg(this->start_blocos_livre + (streampos)indiceByte);
    //le o byte
    char byte_lido;
    arquivo->read(&byte_lido, sizeof(byte_lido));
    //std::cout << "i: " << i << /*" \nbyte lido: " << byte_lido << " indice byte: " << indiceByte << " indice bit: " << indiceBit <<*/ std::endl;
    //le o bit
    byte_lido &= (0x80 >> indiceBit);
    /*if(byte_lido == 0){
        std::cout << i << " esta livre\n";
    }else{
        std::cout << i << " nao esta livre\n";
    }*/
    return (byte_lido == 0);
}

bool Disco::Book_bloco(unsigned int i){
    if(!montado){return false;} if(i >= QNT_INODES){return false;}
    if(Is_bloco_free(i) == false){return false;}
    bool sucesso = true;
    int indiceByte = i / 8;
    int indiceBit = i % 8;

    //vai para o byte a ser lido
    //std::cout << "va para o byte: " << this->start_inodes_livre + (streampos)indiceByte << "\n";
    arquivo->seekg(this->start_blocos_livre + (streampos)indiceByte);

    //le o byte
    char byte_lido;
    arquivo->read(&byte_lido, sizeof(byte_lido));

    //escreve para o byte
    char byte_escrever = 0x80;
    byte_escrever = (byte_escrever >> indiceBit);
    byte_escrever |= byte_lido;
    //std::cout << "i: " << i << " \nbyte escrito: " << byte_escrever << " indice byte: " << indiceByte << " indice bit: " << indiceBit << std::endl;
    
    //retorna para a posicao para escrever
    arquivo->seekg(this->start_blocos_livre + (streampos)indiceByte);
    arquivo->write(&byte_escrever, sizeof(byte_escrever));
    this->inode_usados++;

    return sucesso;
}

bool Disco::Release_bloco(unsigned int i){
    if(!montado){return false;} if(i >= QNT_INODES){return false;}
    if(Is_bloco_free(i) == true){return false;}
    bool sucesso = true;
    int indiceByte = i / 8;
    int indiceBit = i % 8;

    //vai para o byte a ser lido
    //std::cout << "va para o byte: " << this->start_blocos_livre + (streampos)indiceByte << "\n";
    arquivo->seekg(this->start_blocos_livre + (streampos)indiceByte);

    //le o byte
    char byte_lido;
    arquivo->read(&byte_lido, sizeof(byte_lido));

    //escreve para o byte
    char byte_escrever = 0x80;
    byte_escrever = ~(byte_escrever >> indiceBit);

    byte_escrever &= byte_lido;
    //std::cout << "i: " << i << " \nbyte escrito: " << byte_escrever << " indice byte: " << indiceByte << " indice bit: " << indiceBit << std::endl;
    
    //retorna para a posicao para escrever
    arquivo->seekg(this->start_blocos_livre + (streampos)indiceByte);
    arquivo->write(&byte_escrever, sizeof(byte_escrever));
    this->inode_usados--;

    return sucesso;
}

//Alocacao de INODES, size 0 - 256
bool Disco::Write_Block(unsigned int i, unsigned int* bloco, unsigned int size){
    this->arquivo->seekg(this->start_blocos + (streampos)(BLOCK_SIZE*i));
    this->arquivo->write(reinterpret_cast<char*>(bloco), size*sizeof(unsigned int));
    return true;
}

//le os inodes do arquivo
void Disco::Read_block(unsigned int block_num, unsigned int* blocosLidos){
    //vai ate a posicao I onde o inode sera escrito
    int offset = BLOCK_SIZE*block_num;
    this->arquivo->seekg(this->start_blocos + (streampos)(offset));//(streampos)(BLOCK_SIZE*block_num)
    //lê o inode
    this->arquivo->read(reinterpret_cast<char*>(blocosLidos), 256*sizeof(unsigned int));
}

void Disco::Printar_Bloco(unsigned int bloco_num){
    if(bloco_num > QNT_BLOCOS){return;}
    unsigned int* block_read = new(std::nothrow)unsigned int[256];
    this->Read_block(bloco_num, block_read);
    std::cout << "bloco: " << bloco_num;
    for(int j = 0; j < 256; j++){
        if(j%16 == 0){std::cout << "\n";}
        std::cout << block_read[j] << " ";
    }
    delete[] block_read;
}

void Disco::Printar_Bloco(unsigned int bloco_num, int indice){
    if(bloco_num > QNT_BLOCOS){return;}
    unsigned int* block_read = new(std::nothrow)unsigned int[256];
    this->Read_block(bloco_num, block_read);
    std::cout << block_read[indice] << " ";
    delete[] block_read;
}

unsigned int Disco::Get_Bloco_Aux(unsigned int bloco_num, int indice){
    unsigned int* block_read = new(std::nothrow)unsigned int[256];
    this->Read_block(bloco_num, block_read);
    unsigned int retorno = block_read[indice];
    delete[] block_read;
    return retorno;
}

void Disco::Set_Bloco_Aux(unsigned int bloco_num, int indice, unsigned int val){
    unsigned int* block_read = new(std::nothrow)unsigned int[256];
    this->Read_block(bloco_num, block_read);
    block_read[indice] = val;
    this->Write_Block(bloco_num, block_read, 256);
    delete[] block_read;
    return;
}

void Disco::Printar_Bloco(unsigned int bloco_num, int start, int end){
    if(bloco_num > QNT_BLOCOS){return;}
    unsigned int* block_read = new(std::nothrow)unsigned int[256];
    int fim = std::min(end, 256);
    this->Read_block(bloco_num, block_read);
    for (int i = start; i < end; i++)
    {
        std::cout << block_read[i] << " ";
    }
    delete[] block_read;
}

//arquivos
/*le o byte x do arquivo passado*/
unsigned int Disco::Read_File_index(std::string nome, unsigned int indice){
    //define as variaveis
    Inode node = this->Read_inode(nome);
    long int tamanho = this->Real_Size_Inode(nome);
    long int qntBlocos = this->Size_Inode(nome);
    //contem o indice
    if(indice >= tamanho){indice = tamanho-1;}
    else if(indice < 0){indice = 0;}
    //descobre os indices dos blocos
    int blocoIndex = floor((float)indice/256.0); //16.384
    int blocoIndexBackup = blocoIndex;
    //qual inode tem o indice
    int inodeIndex = floor((float)blocoIndex/64.0);
    //vai para o inode inode index
    for (int i = 0; i < inodeIndex; i++)
    {
        int next = *reinterpret_cast<int*>(node.next);
        node = this->Read_inode(next);
        blocoIndexBackup -= 64;
    }
    //le o bloco blocoIndex
    //pega o bloco
    vector<int> blocos;
    for(int i = 0; i < 64; i++){
        int valor = *(reinterpret_cast<int*>(node.blocos + (i*4)));
        if(valor >= QNT_BLOCOS){
            break;
        }else{
            blocos.push_back(valor);
        }
    }
    unsigned int valor = this->Get_Bloco_Aux(blocos[blocoIndexBackup], indice-(256*blocoIndex));
    return valor;
}

/*muda o byte x do arquivo passado*/
void Disco::Write_File_index(std::string nome, unsigned int indice, unsigned int valor){
    //define as variaveis
    Inode node = this->Read_inode(nome);
    long int tamanho = this->Real_Size_Inode(nome);
    long int qntBlocos = this->Size_Inode(nome);
    //contem o indice
    if(indice >= tamanho || indice < 0){return;}
    //descobre os indices dos blocos
    int blocoIndex = floor((float)indice/256.0); //16.384
    int blocoIndexBackup = blocoIndex;
    //qual inode tem o indice
    int inodeIndex = floor((float)blocoIndex/64.0);
    //vai para o inode inode index
    for (int i = 0; i < inodeIndex; i++)
    {
        int next = *reinterpret_cast<int*>(node.next);
        node = this->Read_inode(next);
        blocoIndexBackup -= 64;
    }
    //le o bloco blocoIndex
    //pega o bloco
    vector<int> blocos;
    for(int i = 0; i < 64; i++){
        int valor = *(reinterpret_cast<int*>(node.blocos + (i*4)));
        if(valor >= QNT_BLOCOS){
            break;
        }else{
            blocos.push_back(valor);
        }
    }
    this->Set_Bloco_Aux(blocos[blocoIndexBackup], indice-(256*blocoIndex), valor);
}

/*aumenta o tamanho do inode*/
void Disco::Add_File_index_aux(std::string nome){
    //define as variaveis
    Inode node = this->Read_inode(nome);
    long int tamanho = this->Real_Size_Inode(nome);
    long int qntBlocos = this->Size_Inode(nome);
    int last_pos = tamanho-1;
    //vou para o inode da ultima posicao
    int blocoIndex = floor((float)last_pos/256.0); //16.384
    int blocoIndexBackup = blocoIndex;
    //qual inode tem o indice
    int inodeIndex = floor((float)blocoIndex/64.0);
    int next = QNT_INODES;
    //vou para o ultimo inode
    for (int i = 0; i < inodeIndex; i++)
    {
        next = *reinterpret_cast<int*>(node.next);
        node = this->Read_inode(next);
        blocoIndexBackup -= 64;
    }
    //chego no ultimo inode
    //tamanho do ultimo inode
    unsigned int size_inode = *(reinterpret_cast<unsigned int*>(node.size));
    if(size_inode == (unsigned int)16384){
        //se ele for = (unsigned int)16384 esta cheio
    }else{
        //se nao
        //ele tem bloco livre?
        int livre = qntBlocos - tamanho;
        if(livre > 0){
            //tem bloco livre 
            //-> adiciona ao final do bloco
            //pega o bloco
            vector<int> blocos;
            for(int i = 0; i < 64; i++){
                unsigned int valor = *(reinterpret_cast<unsigned int*>(node.blocos + (i*4)));
                if(valor >= QNT_BLOCOS){
                    break;
                }else{
                    blocos.push_back(valor);
                }
            }
            //-> aumenta o tamanho em 1
            node.set_Size(size_inode+1);
            //-> diminui o espaco livre em 4
            this->tamanho_livre -=4;
            if(next < QNT_INODES){
                this->Write_Inode(next, node);
                std::cout << "escrito em:" << next << "\n";
            }else{
                int indice = this->Get_inode_index(nome);
                this->Write_Inode(indice, node);
                std::cout << "escrito em:" << indice << "\n";
            }
            return; //-> retorna
            
        }else{
            //nao tem bloco livre
        }
    }
    
    
   
        
        
        
        
        
    
    //-> acha o indice i no inode do primeiro bloco livre
    //-> peco um bloco
    //-> adiciono o indice desse bloco na posicao i
    //->escrevo o valor no bloco
    //->escrevo o bloco no arquivo
    //retorna
}

void Disco::Add_File_index(std::string nome, unsigned int valor){
    this->Add_File_index_aux(nome);
    int indice = this->Real_Size_Inode(nome);
    this->Write_File_index(nome, (indice/4)-1, valor);
}

//FUNCOES REQUERIDAS

/*passar o nome do arquivo e o tamanho -> quantos numeros escrever*/
void Disco::Cria_Arquivo(std::string nome, unsigned int size){
    if(size == 0){return;}
    int size_backup = size;
    //quantos blocos são necessarios para escrever o arquivo
    unsigned int qntBlocos = ceil(size/256.0);
    unsigned int qntBlocos_backup = qntBlocos;
    if(qntBlocos*1024 > tamanho_livre){
        std::cout << "sem espaco suficiente\n";
        return;
    }
    //pega os blocos que seram usados 
    //vector<unsigned int> blocos;
    unsigned int* blocos = new(std::nothrow)unsigned int[qntBlocos];
    for (unsigned int i = 0; i < qntBlocos; i++)
    {
        blocos[i] = this->Ask_bloco();if(blocos[i] >= QNT_BLOCOS){ delete[] blocos; return;}
        this->Book_bloco(blocos[i]);
    }
    std::cout << "qnt blocos: " << qntBlocos << "\n";

    //quantos inodes sao precisos para guardar o bloco /////////////////////////////////
    int qntInodes = ceil(qntBlocos/64.0);
    if(qntInodes > (QNT_INODES - inode_usados)){
        std::cout << "sem inodes disponiveis\n";
        return;
    }
    std::cout << "qnt inodes: " << qntInodes << "\n";
    //endereco dos inodes
    //vector<unsigned int> endrInodes;
    unsigned int* endrInodes = new(std::nothrow)unsigned int[qntInodes];
    for (int i = 0; i < qntInodes; i++)
    {
        endrInodes[i] = this->Ask_inode();if(endrInodes[i] >= QNT_INODES){delete[] blocos; delete[] endrInodes; return;}
        this->Book_inode(endrInodes[i]);
        //endrInodes.push_back(inodeA);
    }

    //monta o inode raiz //////////////////////////////////////////////////////////////
    int offset_blocos_data = 0;
    unsigned int nxt = QNT_INODES;
    if(qntInodes > 1){nxt = endrInodes[1];}
    if(qntBlocos <= 64){
        this->Cria_Escrever_inode(endrInodes[0], nome, size, nxt, blocos, qntBlocos, offset_blocos_data);
    }else{
        this->Cria_Escrever_inode(endrInodes[0], nome, (unsigned int)16384, nxt, blocos, (unsigned int)64, offset_blocos_data);
        offset_blocos_data++;
        size -= 16384;
        qntBlocos -= 64;
    }
    ///Monta inodes folhas ////////////////////////////////////////////////////////////
    for (int i = 1; i < qntInodes; i++){
        std::string folhaNome = "";
        if(i+1 < qntInodes){
            nxt = endrInodes[i+1];
        }else{
            nxt = QNT_INODES;
        }

        if(qntBlocos <= 64){ 
            this->Cria_Escrever_inode(endrInodes[i], folhaNome, size, nxt, blocos, qntBlocos, offset_blocos_data);
        }else{
            //raiz.set_Size(16384); raiz.set_blocos(blocos.data(), 64, offset_blocos_data);
            this->Cria_Escrever_inode(endrInodes[i], folhaNome, (unsigned int)16384, nxt, blocos, (unsigned int)64, offset_blocos_data);
            offset_blocos_data++;
            size -= 16384;
            qntBlocos -= 64;
        }
        //escreve inode
    }
    ///////////////////////////////////////////////////////////////////////////////////
    ///// PREENCHE OS BLOCOS COM NUMEROS INTEIROS ALEATORIOS
    int indice_bloco = 0;
    int qnt_escritos = 0;
    unsigned int* bloco_u = new(std::nothrow)unsigned int[256];
    while(size_backup > 0){
        std::cout << "numeros gerados\n";
        Gerar_numeros(bloco_u, size_backup);
        this->Write_Block(blocos[indice_bloco], bloco_u, 256);
        indice_bloco++;
        size_backup -= 256;
    }
    
    delete[] bloco_u;
    delete[] blocos;
    delete[] endrInodes;
    //////////////////////////////////////////////////////////////////////////////////
    tamanho_livre -= (qntBlocos_backup*1024);
    std::cout << "arquivo escrito\n";
    return;
}

bool Disco::Apaga_Arquivo(std::string nomeArquivo){
    std::string nomeString(Monta_nome(nomeArquivo), 8);
    for (int i = 0; i < QNT_INODES; i++)
    {
        this->arquivo->seekg(this->start_inodes + (streampos)(sizeof(Inode)*i));
        Inode node;
        this->arquivo->read(reinterpret_cast<char*>(&node), sizeof(Inode));
        std::string name(node.nome, 8);
        if(Compara_nomes(name, nomeString)){
            int tamanho = this->Size_Inode(i);
            this->tamanho_livre += tamanho;
            this->Delete_Inode(i);
            std::cout << name << " apagado " << tamanho << "Bytes livres\n";
            return true;
        }
    }
    std::cout << "Arquivo nao encontrado\n";
    return false;
}

void Disco::Listar_Arquivos(){
    std::cout << "ARQUIVOS:\n";
    for (int i = 0; i < QNT_INODES; i++)
    {
        if(!this->Is_inode_free(i)){
            Printar_Inode(i);
        }
    }
    
}

void Disco::Ler_Arquivo(std::string nomeArquivo, unsigned int inicio, unsigned int fim){
    Inode node = this->Read_inode(nomeArquivo);
    long int tamanho = this->Real_Size_Inode(nomeArquivo);
    long int qntBlocos = this->Size_Inode(nomeArquivo);
    if(fim > tamanho || fim < 0){return;}
    if(inicio > tamanho || inicio < 0){return;}
    if(inicio == fim){
        int blocoIndex = floor((float)inicio/256.0); //16.384
        int blocoIndexBackup = blocoIndex;
        //qual inode tem
        int inodeIndex = floor((float)blocoIndex/64.0);
        //vai para o inode inode index
        for (int i = 0; i < inodeIndex; i++)
        {
            int next = *reinterpret_cast<int*>(node.next);
            node = this->Read_inode(next);
            blocoIndexBackup -= 64;
        }
        //le o bloco blocoIndex
        //pega o bloco
        vector<int> blocos;
        for(int i = 0; i < 64; i++){
            int valor = *(reinterpret_cast<int*>(node.blocos + (i*4)));
            if(valor >= QNT_BLOCOS){
                break;
            }else{
                blocos.push_back(valor);
            }
        }
        this->Printar_Bloco(blocos[blocoIndexBackup], inicio-(256*blocoIndex));
        std::cout << "\n";
        return;
    }
    int start = std::min(inicio, fim);
    int end = std::max(inicio, fim);
    //acha o bloco de inicio
    int blocoIndex_start = floor((float)start/256.0); //1
    //acha o inode do inicio
    int inodeIndex_start = floor((float)blocoIndex_start/64.0); //0

    //acha o bloco do fim
    int blocoIndex_end = floor((float)end/256.0); //64
    //acha o inode do fim
    int inodeIndex_end = floor((float)blocoIndex_end/64.0); //1
    //vai para o inode de inicio
    int blocoIndex_startBackup = blocoIndex_start;
    int blocoIndex_endBackup = blocoIndex_end;
    //vai para o primeiro inode
    for (int i = 0; i < inodeIndex_start; i++)
    {
            int next = *reinterpret_cast<int*>(node.next);
            node = this->Read_inode(next);
            blocoIndex_startBackup -= 64;
            blocoIndex_endBackup -= 64;
            inodeIndex_end--;
    }
    inodeIndex_start = 0;
    //pega os blocos do inode start
    vector<int> blocos;
    for(int i = 0; i < 64; i++){
        int valor = *(reinterpret_cast<int*>(node.blocos + (i*4)));
        if(valor >= QNT_BLOCOS){
            break;
        }else{
            blocos.push_back(valor);
        }
    }
    //ambos os valores estao no mesmo inode
    if(inodeIndex_start == inodeIndex_end){
        //estao no mesmo bloco
        if(blocoIndex_start == blocoIndex_end){ this->Printar_Bloco(blocos[blocoIndex_startBackup], start-(256*blocoIndex_start), end-(256*blocoIndex_start)); std::cout << "\n"; return;}
        //nao estao no mesmo bloco
        else{
            //printa no blocos[blocoIndex_start] do indiceS até o ultimo
            this->Printar_Bloco(blocos[blocoIndex_startBackup], start-(256*blocoIndex_start), 256);
            //printa todos do meio
            for (int i = blocoIndex_start+1; i < blocoIndex_end; i++)
            {
                this->Printar_Bloco(blocos[i]);
            }
            //printa no blocos[blocoIndex_end] do 0 até o indiceE
            this->Printar_Bloco(blocos[blocoIndex_endBackup], 0, end-(256*blocoIndex_end));
            std::cout << "\n";
            return;
        }
    //nao estao no mesmo inode
    }else{
        //printa todos os blocos do inode atual no bloco[blocoIndex_start] na posicao indiceS até o ultimo bloco
        this->Printar_Bloco(blocos[blocoIndex_startBackup], start-(256*blocoIndex_start), 256);
        for(int i = blocoIndex_startBackup+1; i < 64; i++){
            this->Printar_Bloco(blocos[i]);
        }
        //avanca inodeIndex_end inodes, printando todos os blocos
        for (int i = 0; i < inodeIndex_end; i++)
        {
            int next = *reinterpret_cast<int*>(node.next);
            node = this->Read_inode(next);
            blocoIndex_endBackup -= 64;
            //se o inode atual nao for o inode do ultimo, eu printo todos os blocos
            if(i+1 < inodeIndex_end){
                for(int j = 0; j < 64; j++){
                    int valor = *(reinterpret_cast<int*>(node.blocos + (i*4)));
                    if(valor >= QNT_BLOCOS){
                        break;
                    }else{
                        this->Printar_Bloco(valor);
                    }
                }
            }
        }

        //printa todos os blocos ate blocoIndex_end, e o bloco[blocoIndex_end] da posicao 0 ate a posicao indiceE
        //pega os blocos do "ultimo" inode
        vector<int> blocos_end;
        for(int i = 0; i < 64; i++){
            int valor = *(reinterpret_cast<int*>(node.blocos + (i*4)));
            if(valor >= QNT_BLOCOS){ break;}
            else{ blocos_end.push_back(valor);}
        }
        //printa todos os blocos ate o blocoIndex_end
        for(int i = 0; i < blocoIndex_endBackup; i++){
            this->Printar_Bloco(blocos_end[i]);
        }

        //printa o bloco blocoIndex_end
        this->Printar_Bloco(blocos_end[blocoIndex_endBackup],0, end-(256*blocoIndex_end));
        std::cout << "\n";
        return;
    }
}

void Disco::Print_space(){
    long int space_usado = this->tamanho_disco - this->tamanho_livre;
    std::cout << "DISCO:\n";
    std::cout << "Tamanho total: " << (int)(this->tamanho_disco/1024) << " MB\n"
              << "Tamanho disponivel: " << (int)(this->tamanho_livre/1024) << " MB\n";
}

int Disco::Montar()
{
    this->arquivo = new fstream(this->path, std::ios::in | std::ios::out | std::ios::binary);
    if (!this->arquivo) {
        std::cerr << "Não foi possível abrir o arquivo!" << std::endl;
        return 1;
    }else{
        //printf("Disco Montado\n");
        montado = true;
        //this->arquivo->close();
        return 0;
    }
}

int Disco::Desmontar()
{
    //this->arquivo = new fstream(this->path, std::ios::in | std::ios::out | std::ios::binary);
    if (!this->arquivo) {
        std::cerr << "Não foi possível fechar o arquivo!" << std::endl;
        return 1;
    }else{
        printf("Disco Desmontado\n");
        montado = false;
        this->arquivo->close();
        return 0;
    }
}

Disco::~Disco(){}


#endif