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
    void set_blocos(int* vals, int qnt, int offset = 0){
        for(int i = 0; i < qnt; i++){
            std::memcpy(&blocos[(offset*64) + i * 4], &vals[i], sizeof(int));
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
     return dis(gen);
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

    bool File_exists(std::string);
    // le o bloco I e armazena no bloco, usar o valor desejado -1 -> [1-size]
    void Read_block(unsigned int, unsigned int*);

    unsigned int Ask_bloco(){
        for(unsigned int i = 0; i < QNT_BLOCOS; i++){
            if(Is_bloco_free(i)){
                return i;
            }
        }
        return -1;
    }
    
    unsigned int Ask_inode(){
        for(unsigned int i = 0; i < QNT_INODES; i++){
            if(Is_inode_free(i)){
                return i;
            }
        }
        return -1;
    }
    //FUNCOES CORE

    //tamanho do arquivo, usar o valor desejado -1 -> [1-size]
    long int Size_Inode(unsigned int);
    long int Size_Inode(std::string);
    //tamanho real do arquivo, usar o valor desejado -1 -> [1-size]
    long int Real_Size_Inode(unsigned int);
    long int Real_Size_Inode(std::string);
    //espaco livre no disco
    void Print_space();
    //printa o inode i, usar o valor desejado -1 -> [1-size]
    void Printar_Inode(unsigned int);
    //
    void Printar_Bloco(unsigned int);
    void Printar_Bloco(unsigned int, int);
    void Printar_Bloco(unsigned int, int, int);
    /*Funcao para verificar se o arquivo existe e abrir*/
    int Montar();
    /*Funcao para verificar se o arquivo estiver aberto, fecha-lo*/
    int Desmontar();

    void Listar_Arquivos();

    bool Cria_Arquivo(std::string , unsigned int);

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

//escreve o inode para o arquivo, garanta que i está livre para escrever
bool Disco::Write_Inode(unsigned int i, Inode node){
    //vai ate a posicao I onde o inode sera escrito
    this->arquivo->seekg(this->start_inodes + (streampos)(sizeof(Inode)*i));
    this->arquivo->write(reinterpret_cast<char*>(&node), sizeof(Inode));
    return true;
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

long int Disco::Size_Inode(unsigned int inode_num){
    Inode inode_atual = this->Read_inode(inode_num);
    long int size = 0;
    for(int i = 0; i < 64; i++){
        int valor = *(reinterpret_cast<int*>(inode_atual.blocos + (i*4)));
        if(valor >= QNT_BLOCOS){
            break;
        }else{
            size += 1024;
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
    long int size = 0;
    for(int i = 0; i < 64; i++){
        int valor = *(reinterpret_cast<int*>(inode_atual.blocos + (i*4)));
        if(valor >= QNT_BLOCOS){
            break;
        }else{
            size += 1024;
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
        std::cout << "Nome: " << std::string(inod.nome)  << " { " <<  "tamanho em disco(em bytes): " << diskSize << " | " << "tamanho real(em bytes): " << realSize << " }\n";
    }
}

void Disco::Printar_Bloco(unsigned int bloco_num){
    if(bloco_num > QNT_BLOCOS){return;}
    unsigned int* block_read = new(std::nothrow)unsigned int[256];
    this->Read_block(bloco_num, block_read);
    std::cout << "bloco: " << bloco_num;
    for(int j; j < 256; j++){
        if(j%16 == 0){std::cout << "\n";}
        std::cout << block_read[j] << " ";
    }
    std::cout << "\n";
    delete[] block_read;
}

void Disco::Printar_Bloco(unsigned int bloco_num, int indice){
    if(bloco_num > QNT_BLOCOS){return;}
    unsigned int* block_read = new(std::nothrow)unsigned int[256];
    this->Read_block(bloco_num, block_read);
    std::cout << block_read[indice] << " ";
    delete[] block_read;
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
/*
escreve o inode para o arquivo, garanta que i está livre para escrever
garanta que o inode existe
*/
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


//768 - 
//FUNCOES REQUERIDAS

/*passar o nome do arquivo e o tamanho -> quantos numeros escrever*/
bool Disco::Cria_Arquivo(std::string nome, unsigned int size){
    int size_backup = size;
    //quantos blocos são necessarios para escrever o arquivo
    int qntBlocos = ceil(size/256.0);
    if(qntBlocos*1024 > tamanho_livre){
        std::cout << "sem espaco suficiente\n";
        return false;
    }
    //pega os blocos que seram usados 
    vector<int> blocos;
    for (int i = 0; i < qntBlocos; i++)
    {
        int bloco = this->Ask_bloco();if(bloco == -1){return false;}
        this->Book_bloco(bloco);
        blocos.push_back(bloco);
    }
    std::cout << "qnt blocos: " << qntBlocos << "\n";

    //quantos inodes sao precisos para guardar o bloco /////////////////////////////////
    int qntInodes = ceil(qntBlocos/64.0);
    if(qntInodes > (QNT_INODES - inode_usados)){
        std::cout << "sem inodes disponiveis\n";
        return false;
    }
    std::cout << "qnt inodes: " << qntInodes << "\n";
    //endereco dos inodes
    vector<int> endrInodes;
    for (int i = 0; i < qntInodes; i++)
    {
        int inodeA = this->Ask_inode();if(inodeA == -1){return false;}
        this->Book_inode(inodeA);
        endrInodes.push_back(inodeA);
    }

    //monta o inode raiz //////////////////////////////////////////////////////////////
    int offset_blocos_data = 0;
    Inode raiz(true);
    //seta o nome e extensao
    raiz.set_Nome(nome.c_str(), nome.size());
    if(qntInodes > 1){raiz.set_next(endrInodes[1]);}
    if(qntBlocos <= 64){raiz.set_Size(size); raiz.set_blocos(blocos.data(), qntBlocos, offset_blocos_data);}
    else{
        raiz.set_Size(16384); raiz.set_blocos(blocos.data(), 64, offset_blocos_data);
        offset_blocos_data++;
        size -= 16384;
        qntBlocos -= 64;
    }
    //escreve inode
    this->Write_Inode(endrInodes[0], raiz);
    ///Monta inodes folhas ////////////////////////////////////////////////////////////
    for (int i = 1; i < qntInodes; i++){
        //reserva o endrInodes[i]
        Inode folha(true);
        if(i+1 < qntInodes){raiz.set_next(endrInodes[i+1]);}
        if(qntBlocos <= 64){raiz.set_Size(size); raiz.set_blocos(blocos.data(), qntBlocos, offset_blocos_data);}
        else{
            raiz.set_Size(16384); raiz.set_blocos(blocos.data(), 64, offset_blocos_data);
            offset_blocos_data++;
            size -= 16384;
            qntBlocos -= 64;
        }
        //escreve inode
        this->Write_Inode(endrInodes[i], folha);
    }
    ///////////////////////////////////////////////////////////////////////////////////
    ///// PREENCHE OS BLOCOS COM NUMEROS INTEIROS ALEATORIOS
    unsigned int* bloco_u = new(std::nothrow)unsigned int[256];
    bool discarregado = false;
    int indice_bloco = 0;
    int qnt_escritos = 0;
    for(int i = 0; i < size_backup; i++){
        discarregado = false;
        //gera um numero aleatorio e insere em bloco_u
        unsigned int num = Gera_aleatorio();
        bloco_u[qnt_escritos] = num;
        qnt_escritos++;
        if(qnt_escritos >= 256){
            std::cout << "valor-blocos[indice_bloco]: " << blocos[indice_bloco] << "\n";
            discarregado = true;
            this->Write_Block(blocos[indice_bloco], bloco_u, qnt_escritos);
            indice_bloco++;
            qnt_escritos = 0;
        }
    }
    if(discarregado == false){
        this->Write_Block(blocos[indice_bloco], bloco_u, qnt_escritos);
    }
    tamanho_livre -= (qntBlocos*1024);
    std::cout << "arquivo escrito\n";
    delete[] bloco_u;
    return true;
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
            std::cout << name << " apagado " << tamanho << "kb livres\n";
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
    if(fim >= tamanho || fim < 0){return;}
    if(inicio >= tamanho || inicio < 0){return;}
    if(inicio == fim){
        int indice = inicio;
        int blocoIndex = floor((float)inicio/256.0); //16.384
        //qual inode tem
        int inodeIndex = floor((float)blocoIndex/64.0);
        //vai para o inode inode index
        for (int i = 0; i < inodeIndex; i++)
        {
            int next = *reinterpret_cast<int*>(node.next);
            node = this->Read_inode(next);
            blocoIndex -= 64;
            indice -= 16384;
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
        this->Printar_Bloco(blocos[blocoIndex], indice);
        std::cout << "\n";
    }
    int start = std::min(inicio, fim);
    int end = std::max(inicio, fim);
    //acha o bloco de inicio
    //acha o bloco do fim

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