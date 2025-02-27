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

paginacao: 141.722.624
qnt de paginas usadas 4-> bytes
Paginas:total 141.722.620
usado: 141.722.136
pagina: 4 bytes, quantas posicoes estao sendo usadas, maximo e 141.722.620

memoria principal
-524.288 > posicoes de 4 bytes disponiveis > divide BLOCOS em 6, no maximo, para cada posicao ter 1 numero de um bloco




total > 1.073.741.824 bytes > 1GB
*/

#define HUGEPAGE_SIZE (2 * 1024 * 1024) // Tamanho de uma hugepage padrão (2MB)
#define BLOCK_SIZE (1024)
#define QNT_INODES (716800)
#define QNT_BLOCOS (716800)


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

    void set_bloco(unsigned int val, unsigned int pos){
        std::memcpy(&blocos[pos * 4], &val, sizeof(unsigned int));
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
     //std::cout << numRet << "\n";
     return numRet;
}


/*Preenche o vetor (bloco) na (qnt) passada*/
void Gerar_numeros(unsigned int* bloco, unsigned int qnt){
    std::memset(bloco, 0, 256*sizeof(unsigned int));
    for(int i = 0; i < qnt; i++){
        bloco[i] = Gera_aleatorio();
    }
}


void fill_memory(void *ptr, size_t size) {
    // Preencher a memÃ³ria alocada para garantir que seja realmente alocada
    memset(ptr, 0, size);
}

//passar o ponteiro já na posicao certa
void change_memory_index(unsigned int *ptr, unsigned int val){
    memset(ptr, val, sizeof(unsigned int));
}

//ordenacao
// Função auxiliar para trocar dois elementos
void swap(unsigned int& a, unsigned int& b) {
    unsigned int temp = a;
    a = b;
    b = temp;
}

// Função para ajustar o heap
void heapify(unsigned int* arr, int n, int i) {
    int largest = i; // Inicializa o maior como raiz
    int left = 2 * i + 1; // Filho esquerdo
    int right = 2 * i + 2; // Filho direito

    // Se o filho esquerdo é maior que a raiz
    if (left < n && arr[left] > arr[largest]) {
        largest = left;
    }

    // Se o filho direito é maior que o maior até agora
    if (right < n && arr[right] > arr[largest]) {
        largest = right;
    }

    // Se o maior não é a raiz
    if (largest != i) {
        swap(arr[i], arr[largest]);

        // Recursivamente ajusta o sub-heap afetado
        heapify(arr, n, largest);
    }
}

// Função Heap Sort
void heapSort(unsigned int* arr, int n) {
    // Constrói o heap (reorganiza o vetor)
    for (int i = n / 2 - 1; i >= 0; i--) {
        heapify(arr, n, i);
    }

    // Um por um, extrai um elemento do heap
    for (int i = n - 1; i >= 0; i--) {
        // Move a raiz atual para o fim
        swap(arr[0], arr[i]);

        // Chama heapify no heap reduzido
        heapify(arr, i, 0);
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
    /*endereco hugepages*/
    void* memory_addr;
    bool iniciado_com_sucesso = false;
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
    bool Write_Block(unsigned int, unsigned int*, unsigned int);
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

    void Read_block(unsigned int, unsigned int*, unsigned int);

    //paginacao
    unsigned int Read_page_qnt();

    void Read_page(unsigned int, unsigned int*);
    void Read_page(unsigned int, unsigned int*, unsigned int);

    void Set_page_qnt(unsigned int);
    void Set_page(unsigned int, unsigned int*);
    void Set_page(unsigned int, unsigned int*, unsigned int);

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

    //manipulacao hugepages
    void unload_mem(std::string);

    void unload_mem(unsigned int bloco, unsigned int offset, unsigned int size);

    void unload_mem_page(unsigned int page, unsigned int offset, unsigned int size);

    //carrega um bloco para a memoria
    void Load_to_Mem(unsigned int, unsigned int);

    //carrega os k numeros da bloco para a memoria
    void Load_to_Mem(unsigned int, unsigned int, unsigned int);

    //carrega os blocos para a memoria
    void Load_to_Mem(unsigned int*, unsigned int, unsigned int);

    //carrega uma pagina para a memoria
    void Load_Page_to_Mem(unsigned int, unsigned int);

    //carrega os k numeros da pagina para a memoria
    void Load_Page_to_Mem(unsigned int, unsigned int, unsigned int);

    void Load_to_Mem(std::string nome, unsigned int memoffset, unsigned int start, unsigned int end);

    //arquivos

    void Listar_Arquivos();

    void Cria_Arquivo(std::string , unsigned int);

    void Cria_Escrever_inode(unsigned int, std::string , unsigned int, unsigned int, unsigned int*, unsigned int, unsigned int);

    bool Apaga_Arquivo(std::string);

    void Ler_Arquivo(std::string, unsigned int, unsigned int);

    //(nome, blocos, qnt_blocos, tamanho_arquivo)
    void Merge_sort_externo(std::string, unsigned int*, unsigned int, unsigned int);

    void Ordenar_Arquivo(std::string);

    //Inode ler_inode(int);
    void Printar_path(){
        printf("local do arquivo disco: %s\n", this->path);
    }

    Disco(){
        this->Montar();
        std::cout << "carregando sistema...\n";

        // Alocar memória com mmap
        this->memory_addr = mmap(nullptr, HUGEPAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS| MAP_HUGETLB, -1, 0);
        if (this->memory_addr == MAP_FAILED) {
            perror("mmap");
            return;
        }
        fill_memory(this->memory_addr, HUGEPAGE_SIZE);
        std::cout << "Hugepage allocated at: " << this->memory_addr << std::endl;
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
        iniciado_com_sucesso = true;
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

//muda os bytes especificos do bloco passado
void Disco::Change_Block_4byte(unsigned int bloco_num, unsigned int indice, unsigned int valor){
    this->arquivo->seekg(this->start_blocos + (streampos)(QNT_BLOCOS*bloco_num) + (streampos)(indice*4));
    this->arquivo->write(reinterpret_cast<char*>(&valor), sizeof(unsigned int));
}

//le os bytes especificos do bloco passado
unsigned int Disco::Read_Block_4byte(unsigned int bloco_num, unsigned int indice){
    this->arquivo->seekg(this->start_blocos + (streampos)(QNT_BLOCOS*bloco_num) + (streampos)(indice*4));
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

/*retorna o numero de bytes no arquivo*/
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
/*retorna o numero de bytes no arquivo*/
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
/*retorna o numero real de bytes no arquivo*/
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
/*retorna o numero real de bytes no arquivo*/
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
    //vai ate a posicao I onde o bloco esta escrito
    int offset = BLOCK_SIZE*block_num;
    this->arquivo->seekg(this->start_blocos + (streampos)(offset));//(streampos)(BLOCK_SIZE*block_num)
    //lê o inode
    this->arquivo->read(reinterpret_cast<char*>(blocosLidos), 256*sizeof(unsigned int));
}

void Disco::Read_block(unsigned int block_num, unsigned int* blocosLidos, unsigned int numeros){
    //vai ate a posicao I onde o bloco esta escrito
    int offset = BLOCK_SIZE*block_num;
    this->arquivo->seekg(this->start_blocos + (streampos)(offset));//(streampos)(BLOCK_SIZE*block_num)
    //lê o inode
    this->arquivo->read(reinterpret_cast<char*>(blocosLidos), numeros*sizeof(unsigned int));
}

//printar blocos

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

//fim printar blocos

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
    long int tamanho = this->Real_Size_Inode(nome)/4;
    long int qntBlocos = this->Size_Inode(nome)/4;
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
        //cria o inode
        
        unsigned int nxt = QNT_INODES;
        unsigned int bloco = this->Ask_bloco();if(bloco >= QNT_BLOCOS){return;}
        unsigned int inodeEndr = this->Ask_inode();if(inodeEndr >= QNT_INODES){return;}
        vector<unsigned int> blocos;blocos.push_back(bloco);
        //std::cout << "inode Endr: " << inodeEndr << "\n";
        
        this->Cria_Escrever_inode(inodeEndr, std::string(""), 1, nxt, blocos.data(), 1, 0);
        this->tamanho_livre -=4;
        node.set_next(inodeEndr);
        if(next < QNT_INODES){ //nao estou no primeiro inode
            this->Write_Inode(next, node);
            std::cout << "node escrito em:" << next << "\n";
        }else{//estou no primeiro inode
            int indice = this->Get_inode_index(nome);
            this->Write_Inode(indice, node);
            std::cout << "node escrito em:" << indice << "\n";
        }
        return;
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
            unsigned int bloco = this->Ask_bloco();if(bloco >= QNT_BLOCOS){return;}
            //cria o bloco
            this->Book_bloco(bloco);
            //aponta inode para o bloco
            for(int i = 0; i < 64; i++){
                unsigned int valor = *(reinterpret_cast<unsigned int*>(node.blocos + (i*4)));
                if(valor >= QNT_BLOCOS){
                    node.set_bloco(bloco, i); //recebe valor, indice
                    break;
                }
            }
            //-> aumenta o tamanho em 1
            node.set_Size(size_inode+1);
            //-> diminui o espaco livre em 4
            this->tamanho_livre -=4;
            //escreve o inode no disco
            if(next < QNT_INODES){
                this->Write_Inode(next, node);
                std::cout << "escrito em:" << next << "\n";
            }else{
                int indice = this->Get_inode_index(nome);
                this->Write_Inode(indice, node);
                std::cout << "escrito em:" << indice << "\n";
            }
            return;
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
    int indice = this->Real_Size_Inode(nome)/4;
    this->Write_File_index(nome, indice-1, valor);
}

/////// PAGINACAO

unsigned int Disco::Read_page_qnt(){
    //vai ate a posicao I onde o bloco esta escrito
    this->arquivo->seekg(this->start_pages);
    unsigned int val_retorno;
    //lê o inode
    this->arquivo->read(reinterpret_cast<char*>(val_retorno), sizeof(unsigned int));
}

void Disco::Read_page(unsigned int page_num, unsigned int* pagesLidas){
    //vai ate a posicao I onde o bloco esta escrito
    int offset = (BLOCK_SIZE*page_num)+4;
    this->arquivo->seekg(this->start_pages + (streampos)(offset));
    //lê a pagina
    this->arquivo->read(reinterpret_cast<char*>(pagesLidas), 256*sizeof(unsigned int));
}

void Disco::Read_page(unsigned int page_num, unsigned int* pagesLidas, unsigned int qnt){
     //vai ate a posicao I onde o bloco esta escrito
     int offset = BLOCK_SIZE*page_num+4;
     this->arquivo->seekg(this->start_pages + (streampos)(offset));
     //lê a pagina
     this->arquivo->read(reinterpret_cast<char*>(pagesLidas), qnt*sizeof(unsigned int));
}

void Disco::Set_page_qnt(unsigned int qnt){
    this->arquivo->seekg(this->start_pages);
    this->arquivo->write(reinterpret_cast<char*>(qnt), sizeof(unsigned int));
}

void Disco::Set_page(unsigned int page_num, unsigned int* page){
    int offset = (BLOCK_SIZE*page_num)+4;
    this->arquivo->seekg(this->start_pages + (streampos)(offset));
    this->arquivo->write(reinterpret_cast<char*>(page), 256*sizeof(unsigned int));
}

void Disco::Set_page(unsigned int page_num, unsigned int* page, unsigned int size){
    int offset = (BLOCK_SIZE*page_num)+4;
    this->arquivo->seekg(this->start_pages + (streampos)(offset));
    this->arquivo->write(reinterpret_cast<char*>(page), size*sizeof(unsigned int));
}


/////Huge Pages

void Disco::unload_mem(std::string nome){
    Inode node = this->Read_inode(nome);
    unsigned int size = this->Real_Size_Inode(nome);
    unsigned int escritos = size;
    unsigned int offset = 0;
    while(true){
        //carrego os valores no bloco
        for(int i = 0; i < 64; i++){
            unsigned int valor = *(reinterpret_cast<unsigned int*>(node.blocos + (i*4)));
            if(valor >= QNT_BLOCOS){
                break;
            }else{
                if(escritos <= 256){
                    unsigned int* ponteiro = reinterpret_cast<unsigned int*>(this->memory_addr)+(offset*256);
                    Write_Block(valor, ponteiro, escritos);
                    break;
                }else{
                    unsigned int* ponteiro = reinterpret_cast<unsigned int*>(this->memory_addr)+(offset*256);
                    Write_Block(valor, ponteiro, 256);
                    escritos-= 256;
                }
            }
            offset++;
        }
        int proximo = *(reinterpret_cast<unsigned int*>(node.next));
        if(proximo >= QNT_INODES){break;}
        else{node = this->Read_inode(proximo);} //vai para o proximo inode se existir
    }
}

void Disco::unload_mem(unsigned int bloco, unsigned int offset, unsigned int size){
    unsigned int* ponteiro = reinterpret_cast<unsigned int*>(this->memory_addr)+(offset*256);
    Write_Block(bloco, ponteiro, size);
}

//funcoes memoria hugepages
void Disco::Load_to_Mem(unsigned int bloco, unsigned int memoffset){
    unsigned int* ponteiro = reinterpret_cast<unsigned int*>(this->memory_addr)+memoffset;
    this->Read_block(bloco, ponteiro);
}

void Disco::Load_to_Mem(unsigned int bloco, unsigned int memoffset, unsigned int qnt){
    // Ler o bloco do disco
    unsigned int* ponteiro = reinterpret_cast<unsigned int*>(this->memory_addr)+memoffset;
    this->Read_block(bloco, ponteiro, qnt);
}


void Disco::Load_to_Mem(std::string nomeArquivo, unsigned int memoffset, unsigned int inicio, unsigned int fim){
    // Ler o bloco do disco
    //unsigned int* ponteiro = reinterpret_cast<unsigned int*>(this->memory_addr)+memoffset;
    //this->Read_block(bloco, ponteiro, qnt);
    int indice = 0;
    int start = std::min(inicio, fim);
    int end = std::max(inicio, fim);
    int local_offset = 0;
    if(start == end){
        unsigned int* ponteiro = reinterpret_cast<unsigned int*>(this->memory_addr)+memoffset;
        ponteiro[0] = Read_File_index(nomeArquivo, inicio);
        return;
        //this->Read_block(valores, ponteiro, 1);
    }
    unsigned int* ponteiro = reinterpret_cast<unsigned int*>(this->memory_addr)+memoffset;
    for(int i = start; i < end; i++){
        ponteiro[indice] =  Read_File_index(nomeArquivo, i);
        indice++;
    }
    return;
}

//funcoes memoria hugepages
void Disco::Load_Page_to_Mem(unsigned int page, unsigned int memoffset){
    unsigned int* ponteiro = reinterpret_cast<unsigned int*>(this->memory_addr)+memoffset;
    this->Read_page(page, ponteiro);
}

void Disco::Load_Page_to_Mem(unsigned int page, unsigned int memoffset, unsigned int qnt){
    // Ler o bloco do disco
    unsigned int* ponteiro = reinterpret_cast<unsigned int*>(this->memory_addr)+memoffset;
    this->Read_page(page, ponteiro, qnt);
}

void Disco::Load_to_Mem(unsigned int* blocos, unsigned int qnt, unsigned int memoffset){
    if(memoffset >= 524.287){return;}
    unsigned int* ponteiro = reinterpret_cast<unsigned int*>(this->memory_addr)+memoffset;
    for (int i = 0; i < qnt; i++)
    {
        this->Read_block(blocos[i], ponteiro+(i*256));
    }
}

//FUNCOES REQUERIDAS

///Merge sort externo
void Merge_sort_externo(std::string nome, unsigned int* blocos, unsigned int qnt_blocos, unsigned int qnt_numeros){
    unsigned int qnt_part = qnt_numeros/524288;
    unsigned int* particoes = new(std::nothrow)unsigned int[qnt_part];
    for(int i = 0; i < qnt_part; i++){
        particoes[i] = i*524288;
    }
    //spliting fase
    //para cada particao k de numeros
    //carrega os numeros de k para a memoria
    //ordena
    //grava os numeros de volta
}



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
    unsigned int* blocos = new(std::nothrow)unsigned int[qntBlocos];
    for (unsigned int i = 0; i < qntBlocos; i++)
    {
        //std::cout << "\rProgresso alocando blocos: " << i << "/" << qntBlocos << std::flush;
        blocos[i] = this->Ask_bloco();if(blocos[i] >= QNT_BLOCOS){ delete[] blocos; return;}
        this->Book_bloco(blocos[i]);
    }
    std::cout << "qnt blocos: " << qntBlocos << "\n";

    /////////////////////////////////////////////////////quantos inodes sao precisos para guardar o bloco /////////////////////////////////
    int qntInodes = ceil(qntBlocos/64.0);
    if(qntInodes > (QNT_INODES - inode_usados)){
        std::cout << "sem inodes disponiveis\n";
        return;
    }
    std::cout << "qnt inodes: " << qntInodes << "\n";
    //endereco dos inodes
    unsigned int* endrInodes = new(std::nothrow)unsigned int[qntInodes];
    for (int i = 0; i < qntInodes; i++)
    {
        //std::cout << "\rProgresso alocando inodes: " << i << "/" << qntInodes << std::flush;
        endrInodes[i] = this->Ask_inode();if(endrInodes[i] >= QNT_INODES){delete[] blocos; delete[] endrInodes; return;}
        this->Book_inode(endrInodes[i]);
    }

    ////////////////////////////////////////////////////monta o inode raiz //////////////////////////////////////////////////////////////
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
    //////////////////////////////////////////////////////Monta inodes folhas ////////////////////////////////////////////////////////////
    for (int i = 1; i < qntInodes; i++){
        //std::cout << "\rProgresso montando inodes: " << i << "/" << qntInodes << std::flush;
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
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///// PREENCHE OS BLOCOS COM NUMEROS INTEIROS ALEATORIOS
    int indice_bloco = 0;
    int qnt_escritos = 0;
    unsigned int* bloco_u = new(std::nothrow)unsigned int[256];
    while(size_backup > 0){
        //std::cout << "\rProgresso preechendo blocos: " << indice_bloco << "/" << qntBlocos_backup << std::flush;
        //std::cout << "numeros gerados\n";
        if(size_backup <= 256){
            Gerar_numeros(bloco_u, size_backup);
        }else{
            Gerar_numeros(bloco_u, 256);
        }
        this->Write_Block(blocos[indice_bloco], bloco_u, 256);
        
        indice_bloco++;
        size_backup -= 256;
    }
    
    delete[] bloco_u;
    delete[] blocos;
    delete[] endrInodes;
    //////////////////////////////////////////////////////////////////////////////////
    tamanho_livre -= (qntBlocos_backup*1024);
    std::cout << "\narquivo escrito\n";
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

void Disco::Ordenar_Arquivo(std::string name){
    //pega o inode
    //comeca a calcular o tempo
    struct timespec ts; 
    clock_gettime(CLOCK_REALTIME, &ts);
    long long inico_funcao = ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;

    Inode node = this->Read_inode(name);
    unsigned int qnt_numeros = this->Real_Size_Inode(name)/4;
    unsigned int carregados = qnt_numeros;
    if(qnt_numeros > (unsigned int)35430656){std::cout << "não eh possivel ordenar arquivos desse tamanho\n"; return;}
    if(qnt_numeros < (unsigned int)524288){
        unsigned int offset = 0;
        //ordena direto na memoria
        while(true){
            //carrega todos os blocos na memoria
            for(int i = 0; i < 64; i++){
                unsigned int valor = *(reinterpret_cast<unsigned int*>(node.blocos + (i*4)));
                if(valor >= QNT_BLOCOS){
                    break;
                }else{
                    if(carregados <= 256){
                        this->Load_to_Mem(valor, (offset*256), carregados);
                        break;
                    }else{
                        this->Load_to_Mem(valor, (offset*256), 256);
                        carregados-= 256;
                    }
                }
                offset++;
            }
            int proximo = *(reinterpret_cast<unsigned int*>(node.next));
            if(proximo >= QNT_INODES){break;}
            else{node = this->Read_inode(proximo);} //vai para o proximo inode se existir
        }
        heapSort(reinterpret_cast<unsigned int*>(this->memory_addr), qnt_numeros);
        //joga de volta no inode
        this->unload_mem(name);
        
        clock_gettime(CLOCK_REALTIME, &ts);
        long long fim_funcao = ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
        printf("[ordenar]Tempo de execulcao: %.2lld MS\n", fim_funcao-inico_funcao);
        return;
    }else{
        //merge sort externo

    }
    //copia os valores do paginacao para o inode de 256 em 256;
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
        if (munmap(this->memory_addr, HUGEPAGE_SIZE) == -1) {
            perror("munmap");
            return 1;
        }
        std::cout << "memoria liberada\n";
        return 0;
    }
}

Disco::~Disco(){
    if (munmap(this->memory_addr, HUGEPAGE_SIZE) == -1) {
        perror("munmap");
    }
    std::cout << "memoria liberada\n";
}


#endif